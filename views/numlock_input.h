/*
 * TagTinker - number lock barcode input view
 *
 * Custom view for entering a 17-character ESL barcode.
 * Format: Letter + 16 digits.
 *
 * Controls:
 *   UP/DOWN   - cycle digit 0-9
 *   LEFT/RIGHT - move cursor position
 *   OK        - confirm barcode
 *   BACK      - cancel
 */

#pragma once

#include <gui/view.h>

typedef void (*NumlockCallback)(void* ctx, const char* barcode);

typedef struct {
    View* view;
    NumlockCallback callback;
    void* callback_ctx;
} NumlockInput;

NumlockInput* numlock_input_alloc(void);
void numlock_input_free(NumlockInput* numlock);
View* numlock_input_get_view(NumlockInput* numlock);
void numlock_input_set_callback(NumlockInput* numlock, NumlockCallback cb, void* ctx);
void numlock_input_reset(NumlockInput* numlock);
