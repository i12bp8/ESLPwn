/**
 * TagTinker IR transmitter - implementation (v2)
 *
 * Drives the Flipper Zero's built-in IR LEDs at ~1.255 MHz carrier
 * for the ESL pulse-position modulation (PPM) protocol used here.
 *
 * v2 changes from v1:
 * - Removed TIM2 ISR approach (TIM2 conflicts with Flipper's IR RX subsystem!)
 * - Uses DWT cycle counter for precise timing instead (zero timer conflicts)
 * - Properly handles TIM1 bus state (was crashing if already enabled)
 * - Non-blocking repeat loop with cancellation support
 *
 * Architecture:
 *   TIM1 Channel 3N: 1.255 MHz PWM carrier on built-in IR LED (PB9)
 *   DWT->CYCCNT:    Cycle-accurate timing for PPM symbol encoding
 *
 * CRITICAL: The built-in IR LED is on PB9 = TIM1_CH3N (complementary output).
 * NOT CH3! The carrier is gated by toggling OC3M bits in TIM1->CCMR2
 * (which control both CH3 and CH3N). The firmware uses PWM2 mode.
 * For CH3N (complementary), Force Inactive = LED off, PWM2 = carrier on.
 */

#include "tagtinker_ir.h"

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bus.h>
#include <furi_hal_resources.h>
#include <furi_hal_gpio.h>
#include <furi_hal_cortex.h>

#include <stm32wbxx_ll_tim.h>

/* ─── Carrier configuration ───
 * Target: 1.25 MHz carrier for ESL signaling
 * Flipper: 64 MHz system clock, TIM1 PSC=0
 * ARR = 51-1 = 50 → 64MHz/51 = 1,254,901 Hz (+4.9 kHz off, within ±10kHz)
 */
#define CARRIER_TIM       TIM1
#define CARRIER_ARR       (51 - 1)
#define CARRIER_CCR       25  /* ~50% duty cycle */

/* ─── PP4 timing in CPU cycles (64 MHz) ───
 *
 * Protocol reference: ESL Blaster FW03 ir.c, furrtek.org ESL page
 * Base period t = 1/32768 Hz = 30.518 µs
 *
 * Burst duration: ~40 µs = 50 carrier cycles at 1.25 MHz
 *   ESL Blaster: 4 ticks × 10.08µs = 40.32 µs
 *   Our value: 40 µs × 64 = 2560 cycles
 *
 * Symbol gap durations (index = 2-bit symbol value):
 *   From ir.c pp4_steps ordering:
 *   pp4_steps[0] = 5 ticks → sym 00: 6 × 10.08 = 60.48 µs
 *   pp4_steps[1] = 23 ticks → sym 01: 24 × 10.08 = 241.92 µs
 *   pp4_steps[2] = 11 ticks → sym 10: 12 × 10.08 = 120.96 µs
 *   pp4_steps[3] = 17 ticks → sym 11: 18 × 10.08 = 181.44 µs
 *
 * Converting to 64 MHz cycles:
 */
#define PP4_BURST_CYCLES  2581  /* 40.33 µs */
static const uint32_t pp4_gap_cycles[4] = {
    3871,   /* Symbol 00: 60.48 µs */
    15483,  /* Symbol 01: 241.92 µs */
    7741,   /* Symbol 10: 120.96 µs */
    11612,  /* Symbol 11: 181.44 µs */
};

/* ─── PP16 timing in CPU cycles (64 MHz) ───
 * Derived from esl_blaster ir.c: TIM16 steps of 4us.
 * Gap values (27us to 107us) mapped to 16 symbols.
 * Burst: 21us * 64 = 1344 cycles.
 */
#define PP16_BURST_CYCLES 1344
static const uint32_t pp16_gap_cycles[16] = {
    1728, // 0000: 27µs
    3264, // 0001: 51µs
    2240, // 0010: 35µs
    2752, // 0011: 43µs
    9408, // 0100: 147µs
    7872, // 0101: 123µs
    8896, // 0110: 139µs
    8384, // 0111: 131µs
    5312, // 1000: 83µs
    3776, // 1001: 59µs
    4800, // 1010: 75µs
    4288, // 1011: 67µs
    5824, // 1100: 91µs
    7360, // 1101: 115µs
    6336, // 1110: 99µs
    6848  // 1111: 107µs
};

/* ─── Module state ─── */
static bool ir_initialized = false;
static volatile bool ir_stop_requested = false;

/* ─── Carrier control (TIM1 OC3M register manipulation) ─── */

static inline void carrier_on(void) {
    /* OC3M = PWM Mode 2 (111) — matching Flipper firmware.
     * For CH3N (complementary): PWM2 inverts → CH3N gets active-low PWM = carrier burst.
     * This matches INFRARED_TX_CCMR_HIGH in furi_hal_infrared.c */
    uint32_t ccmr2 = CARRIER_TIM->CCMR2;
    ccmr2 &= ~(TIM_CCMR2_OC3M);
    ccmr2 |= (TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_0); /* PWM mode 2 */
    CARRIER_TIM->CCMR2 = ccmr2;
}

static inline void carrier_off(void) {
    /* OC3M = Force Inactive (100) — CH3N goes idle = LED off.
     * This matches INFRARED_TX_CCMR_LOW in furi_hal_infrared.c */
    uint32_t ccmr2 = CARRIER_TIM->CCMR2;
    ccmr2 &= ~(TIM_CCMR2_OC3M);
    ccmr2 |= TIM_CCMR2_OC3M_2;
    CARRIER_TIM->CCMR2 = ccmr2;
}

/* ─── Cycle-accurate delay using DWT ─── */

static inline void delay_cycles(uint32_t cycles) {
    uint32_t start = DWT->CYCCNT;
    while((DWT->CYCCNT - start) < cycles) {
        /* Busy wait — handles wraparound via unsigned subtraction */
    }
}

/* ─── Send a single PP4-encoded frame ───
 *
 * PPM on-air sequence for N symbols:
 *   [burst][gap₀][burst][gap₁]...[burst][gapₙ₋₁][burst]
 * = N+1 bursts with N gaps, each gap encoding one 2-bit symbol.
 *
 * Data is sent LSB first, 2 bits at a time per byte (matching ir.c).
 */
static void send_frame_pp4(const uint8_t* data, size_t len) {
    for(size_t byte_idx = 0; byte_idx < len; byte_idx++) {
        uint8_t current_byte = data[byte_idx];

        for(int sym = 0; sym < 4; sym++) {
            uint8_t symbol = current_byte & 0x03;
            current_byte >>= 2;

            /* Burst (carrier ON) */
            carrier_on();
            delay_cycles(PP4_BURST_CYCLES);

            /* Gap (carrier OFF) — duration encodes the symbol */
            carrier_off();
            delay_cycles(pp4_gap_cycles[symbol]);
        }
    }

    /* Final burst (N+1th burst, required by PPM protocol) */
    carrier_on();
    delay_cycles(PP4_BURST_CYCLES);
    carrier_off();
}

/* ─── Send a single PP16-encoded frame ───
 * Encodes 4 bits per symbol, resulting in half the pulses of PP4.
 */
static void send_frame_pp16(const uint8_t* data, size_t len) {
    for(size_t byte_idx = 0; byte_idx < len; byte_idx++) {
        uint8_t current_byte = data[byte_idx];

        for(int sym = 0; sym < 2; sym++) {
            uint8_t symbol = current_byte & 0x0F;
            current_byte >>= 4;

            /* Burst (carrier ON) */
            carrier_on();
            delay_cycles(PP16_BURST_CYCLES);

            /* Gap (carrier OFF) */
            carrier_off();
            delay_cycles(pp16_gap_cycles[symbol]);
        }
    }

    /* Final burst */
    carrier_on();
    delay_cycles(PP16_BURST_CYCLES);
    carrier_off();
}

/* ─── Public API ─── */

void tagtinker_ir_init(void) {
    if(ir_initialized) return;

    /* Safely claim TIM1: must handle case where it's already enabled
     * by the firmware's IR subsystem. Bus enable asserts if already on! */
    if(furi_hal_bus_is_enabled(FuriHalBusTIM1)) {
        furi_hal_bus_disable(FuriHalBusTIM1);
    }
    furi_hal_bus_enable(FuriHalBusTIM1);

    /* Configure GPIO for IR TX (built-in IR LEDs) */
    furi_hal_gpio_init_ex(
        &gpio_infrared_tx,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn1TIM1);

    /* Configure TIM1 as 1.255 MHz carrier */
    LL_TIM_SetPrescaler(CARRIER_TIM, 0);
    LL_TIM_SetAutoReload(CARRIER_TIM, CARRIER_ARR);
    LL_TIM_SetCounter(CARRIER_TIM, 0);

    /* Channel 3 config — controls both CH3 and CH3N outputs.
     * The IR LED is on CH3N (PB9), so we must enable CC3NE.
     * PWM2 mode with preload, matching the firmware. */
    LL_TIM_OC_SetMode(CARRIER_TIM, LL_TIM_CHANNEL_CH3, LL_TIM_OCMODE_PWM2);
    LL_TIM_OC_SetCompareCH3(CARRIER_TIM, CARRIER_CCR);
    LL_TIM_OC_EnablePreload(CARRIER_TIM, LL_TIM_CHANNEL_CH3);

    /* CRITICAL: Enable CH3N (complementary output), NOT CH3!
     * IR LED is on PB9 = TIM1_CH3N. Without this, no IR output at all. */
    LL_TIM_CC_EnableChannel(CARRIER_TIM, LL_TIM_CHANNEL_CH3N);

    /* Main output enable (required for TIM1 advanced timer) */
    LL_TIM_EnableAllOutputs(CARRIER_TIM);

    /* Start timer but force carrier OFF initially */
    carrier_off();
    LL_TIM_EnableCounter(CARRIER_TIM);
    LL_TIM_GenerateEvent_UPDATE(CARRIER_TIM);

    ir_stop_requested = false;
    ir_initialized = true;

    FURI_LOG_I("TagTinker", "IR TX initialized: carrier %.3f MHz",
        (double)(64000000.0f / (CARRIER_ARR + 1) / 1000000.0f));
}

void tagtinker_ir_deinit(void) {
    if(!ir_initialized) return;

    tagtinker_ir_stop();

    /* Force carrier off and disable TIM1 */
    carrier_off();
    LL_TIM_DisableAllOutputs(CARRIER_TIM);
    LL_TIM_CC_DisableChannel(CARRIER_TIM, LL_TIM_CHANNEL_CH3N);
    LL_TIM_DisableCounter(CARRIER_TIM);

    /* Reset TIM1 bus so firmware can reclaim it for normal IR */
    if(furi_hal_bus_is_enabled(FuriHalBusTIM1)) {
        furi_hal_bus_disable(FuriHalBusTIM1);
    }

    /* Restore GPIO to safe state */
    furi_hal_gpio_init(&gpio_infrared_tx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    ir_initialized = false;
    FURI_LOG_I("TagTinker", "IR TX deinitialized");
}

bool tagtinker_ir_transmit(const uint8_t* data, size_t len, uint16_t repeats_raw, uint8_t delay) {
    if(!ir_initialized) return false;
    if(len == 0 || len > 255) return false;

    ir_stop_requested = false;
    
    // MSB of repeats indicates PP16 protocol!
    bool is_pp16 = (repeats_raw & 0x8000) != 0;
    uint32_t repeats = repeats_raw & 0x7FFF;

    FURI_LOG_I("TagTinker", "TX start: %zu bytes, %lu repeats (PP%d), %u delay",
        len, repeats, is_pp16 ? 16 : 4, delay);

    /* Transmit frame with repeats.
     * Between repeats we yield briefly so FreeRTOS stays happy
     * and the user can cancel via tagtinker_ir_stop(). */
    for(uint32_t rep = 0; rep <= repeats; rep++) {
        if(ir_stop_requested) {
            FURI_LOG_I("TagTinker", "TX cancelled at repeat %lu", rep);
            carrier_off();
            return false;
        }

        /* Send one frame (interrupts stay enabled — DWT handles timing) */
        if(is_pp16) {
            send_frame_pp16(data, len);
        } else {
            send_frame_pp4(data, len);
        }

        /* Delay between repeats: delay × 500µs (matching ESL Blaster).
         * ESL Blaster: TickCounter = RepeatDelay * 50 ticks, each ~10µs = 500µs per unit */
        if(rep < repeats) {
            uint32_t delay_us = (uint32_t)delay * 500;
            if(delay_us > 0) {
                uint32_t delay_ms_yield = delay_us / 1000;
                uint32_t delay_us_busy = delay_us % 1000;
                
                /* Yield to FreeRTOS for the bulk of the delay to allow the GUI to animate! */
                if(delay_ms_yield > 0) {
                    furi_delay_ms(delay_ms_yield);
                }
                
                /* Busy loop only for the sub-millisecond remainder */
                if(delay_us_busy > 0) {
                    delay_cycles(delay_us_busy * 64); /* 64 cycles per µs */
                }
            }
        }

        /* Fallback yield to FreeRTOS every 10 repeats if the delay parameter was 0 or 1 */
        if(((uint32_t)delay * 500) < 1000 && (rep % 10) == 9) {
            furi_delay_ms(1);
        }
    }

    FURI_LOG_I("TagTinker", "TX complete");
    return true;
}

bool tagtinker_ir_is_busy(void) {
    return false; /* Transmit is blocking in this version */
}

void tagtinker_ir_stop(void) {
    ir_stop_requested = true;
    carrier_off();
}
