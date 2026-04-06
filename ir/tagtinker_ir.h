/**
 * TagTinker IR transmitter - header
 *
 * Low-level IR transmitter for the ESL protocol used by this project.
 * Drives the Flipper Zero's built-in IR LEDs at 1.255 MHz carrier
 * by directly programming TIM1 registers, bypassing furi_hal_infrared.
 *
 * Uses DWT cycle counter for symbol timing (no timer conflicts).
 * PP4 mode (2-bit symbols) for this POC.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Initialize the IR transmitter.
 * Claims TIM1 for carrier, configures GPIO for IR output.
 */
void tagtinker_ir_init(void);

/**
 * Deinitialize the IR transmitter.
 * Releases TIM1, restores GPIO state.
 */
void tagtinker_ir_deinit(void);

/**
 * Transmit a frame using PP4 modulation.
 * Blocking call — returns when all repeats complete or cancelled.
 *
 * @param data      frame bytes
 * @param len       byte count
 * @param repeats   times to repeat (0 = send once)
 * @param delay     inter-repeat delay (×500µs, 10 = 5ms like ESL Blaster)
 * @return          true if completed, false if cancelled/error
 */
bool tagtinker_ir_transmit(const uint8_t* data, size_t len, uint16_t repeats, uint8_t delay);

/**
 * Check if transmitting.
 */
bool tagtinker_ir_is_busy(void);

/**
 * Stop any ongoing transmission.
 */
void tagtinker_ir_stop(void);
