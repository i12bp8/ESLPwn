/*
 * ESLPwn — Number Lock Barcode Input
 *
 * Clean centered digit entry: N + 16 digits in groups of 4.
 * UP/DOWN cycle, LEFT/RIGHT move, OK confirm, BACK cancel.
 */

#include "numlock_input.h"
#include <gui/elements.h>
#include <furi.h>
#include <string.h>

#define NUM_DIGITS  16
#define CHAR_W      6
#define GROUP_SIZE  4
#define GROUP_GAP   5
#define PREFIX_W    10

typedef struct {
    uint8_t digits[NUM_DIGITS];
    uint8_t cursor;
} NumlockModel;

static uint8_t digit_x(uint8_t i) {
    uint8_t groups = i / GROUP_SIZE;
    /* Spread across horizontal: Prefix(8) + 16 chars(6) + 3 gaps(5) = 119. Left margin = 4. */
    return 4 + 8 + i * CHAR_W + groups * GROUP_GAP;
}

static void numlock_draw(Canvas* canvas, void* model_v) {
    NumlockModel* m = model_v;
    canvas_clear(canvas);

    /* 1. Header Bar — Inverted Tech Banner */
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_box(canvas, 0, 0, 128, 12);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 6, AlignCenter, AlignCenter, "TARGET ACQUISITION");
    canvas_set_color(canvas, ColorBlack);

    /* 2. Main Input Frame — Heavy industrial border */
    int frame_y = 19;
    int frame_h = 24;
    canvas_draw_rframe(canvas, 1, frame_y, 126, frame_h, 2);
    /* Inner shadow/border detail */
    canvas_draw_line(canvas, 2, frame_y+1, 125, frame_y+1);

    const uint8_t baseline = 36;
    canvas_set_font(canvas, FontPrimary);
    
    /* Fixed 'N' Prefix */
    canvas_draw_str(canvas, 4, baseline, "N");

    /* Iterating Digits */
    for(uint8_t i = 0; i < NUM_DIGITS; i++) {
        uint8_t x = digit_x(i);
        char ch[2] = {'0' + m->digits[i], '\0'};

        if(i == m->cursor) {
            /* Selected digit bounding block */
            canvas_draw_box(canvas, x - 1, frame_y + 3, CHAR_W + 2, frame_h - 6);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_str(canvas, x, baseline, ch);
            canvas_set_color(canvas, ColorBlack);
            
            /* Sharp Navigator Arrows pointing at selection */
            uint8_t cx = x + CHAR_W / 2 - 1; /* Center of char */
            
            /* Up pointer */
            canvas_draw_line(canvas, cx, frame_y - 4, cx - 2, frame_y - 2);
            canvas_draw_line(canvas, cx, frame_y - 4, cx + 2, frame_y - 2);
            canvas_draw_line(canvas, cx, frame_y - 4, cx, frame_y - 1); 
            
            /* Down pointer */
            canvas_draw_line(canvas, cx, frame_y + frame_h + 3, cx - 2, frame_y + frame_h + 1);
            canvas_draw_line(canvas, cx, frame_y + frame_h + 3, cx + 2, frame_y + frame_h + 1);
            canvas_draw_line(canvas, cx, frame_y + frame_h + 3, cx, frame_y + frame_h); 
            
        } else {
            /* Standard unselected digit */
            canvas_draw_str(canvas, x, baseline, ch);
        }
    }

    /* 3. Footer UI with Clean Button Callouts */
    canvas_set_font(canvas, FontSecondary);
    
    /* D-Pad Hint Icons */
    canvas_draw_str(canvas, 2, 59, "<\x12\x13> Sel"); 
    canvas_draw_str(canvas, 45, 59, "^\x18\x19v Set");

    /* Thick OK Button */
    canvas_draw_rbox(canvas, 92, 48, 34, 14, 2);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 109, 55, AlignCenter, AlignCenter, "Hold OK");
    canvas_set_color(canvas, ColorBlack);
}

static bool numlock_input(InputEvent* input, void* ctx) {
    NumlockInput* numlock = ctx;
    if(input->type != InputTypePress && input->type != InputTypeRepeat) return false;

    bool consumed = false;

    with_view_model(
        numlock->view,
        NumlockModel * m,
        {
            switch(input->key) {
            case InputKeyUp:
                m->digits[m->cursor] = (m->digits[m->cursor] + 1) % 10;
                consumed = true;
                break;
            case InputKeyDown:
                m->digits[m->cursor] = (m->digits[m->cursor] + 9) % 10;
                consumed = true;
                break;
            case InputKeyLeft:
                if(m->cursor > 0) m->cursor--;
                consumed = true;
                break;
            case InputKeyRight:
                if(m->cursor < NUM_DIGITS - 1) m->cursor++;
                consumed = true;
                break;
            case InputKeyOk: {
                char barcode[18];
                barcode[0] = 'N';
                for(uint8_t i = 0; i < NUM_DIGITS; i++)
                    barcode[1 + i] = '0' + m->digits[i];
                barcode[17] = '\0';
                if(numlock->callback)
                    numlock->callback(numlock->callback_ctx, barcode);
                consumed = true;
                break;
            }
            default:
                break;
            }
        },
        consumed);

    return consumed;
}

NumlockInput* numlock_input_alloc(void) {
    NumlockInput* numlock = malloc(sizeof(NumlockInput));
    numlock->view = view_alloc();
    view_allocate_model(numlock->view, ViewModelTypeLocking, sizeof(NumlockModel));
    view_set_draw_callback(numlock->view, numlock_draw);
    view_set_input_callback(numlock->view, numlock_input);
    view_set_context(numlock->view, numlock);
    numlock->callback = NULL;
    numlock->callback_ctx = NULL;

    with_view_model(
        numlock->view,
        NumlockModel * m,
        {
            memset(m->digits, 0, NUM_DIGITS);
            m->digits[0] = 4;
            m->cursor = 1;
        },
        true);

    return numlock;
}

void numlock_input_free(NumlockInput* numlock) {
    view_free(numlock->view);
    free(numlock);
}

View* numlock_input_get_view(NumlockInput* numlock) {
    return numlock->view;
}

void numlock_input_set_callback(NumlockInput* numlock, NumlockCallback cb, void* ctx) {
    numlock->callback = cb;
    numlock->callback_ctx = ctx;
}

void numlock_input_reset(NumlockInput* numlock) {
    with_view_model(
        numlock->view,
        NumlockModel * m,
        {
            memset(m->digits, 0, NUM_DIGITS);
            m->digits[0] = 4;
            m->cursor = 1;
        },
        true);
}
