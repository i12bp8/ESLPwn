/*
 * Barcode Input — custom numlock digit selector
 * Auto-prefixes 'N', user enters 16 digits with arrow keys.
 */

#include "../tagtinker_app.h"

static void numlock_done(void* ctx, const char* barcode) {
    EslPwnApp* app = ctx;
    /* Copy result to app barcode buffer */
    strncpy(app->barcode, barcode, ESLPWN_BC_LEN);
    app->barcode[ESLPWN_BC_LEN] = '\0';
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void eslpwn_scene_barcode_input_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    numlock_input_reset(app->numlock);
    numlock_input_set_callback(app->numlock, numlock_done, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewNumlock);
}

bool eslpwn_scene_barcode_input_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    app->barcode_valid = eslpwn_barcode_to_plid(app->barcode, app->plid);

    if(!app->barcode_valid) {
        FURI_LOG_W(ESLPWN_TAG, "Invalid barcode: %s", app->barcode);
        popup_reset(app->popup);
        popup_set_header(app->popup, "Invalid Barcode", 64, 20, AlignCenter, AlignCenter);
        popup_set_text(app->popup,
            "Format: N + 4 + 14 digits",
            64, 40, AlignCenter, AlignCenter);
        popup_set_timeout(app->popup, 2000);
        popup_enable_timeout(app->popup);
        popup_set_callback(app->popup, NULL);
        view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewPopup);
        return true;
    }

    FURI_LOG_I(ESLPWN_TAG, "Barcode: %s -> PLID %02X%02X%02X%02X",
        app->barcode, app->plid[3], app->plid[2], app->plid[1], app->plid[0]);

    /* Auto-save target */
    bool exists = false;
    for(uint8_t i = 0; i < app->target_count; i++) {
        if(strcmp(app->targets[i].barcode, app->barcode) == 0) {
            exists = true;
            app->selected_target = i;
            break;
        }
    }
    if(!exists && app->target_count < ESLPWN_MAX_TARGETS) {
        EslPwnTarget* t = &app->targets[app->target_count];
        memcpy(t->barcode, app->barcode, ESLPWN_BC_LEN + 1);
        memcpy(t->plid, app->plid, 4);
        char suffix[7];
        memcpy(suffix, app->barcode + ESLPWN_BC_LEN - 6, 6);
        suffix[6] = '\0';
        snprintf(t->name, ESLPWN_TARGET_NAME_LEN, "Tag ...%s", suffix);
        app->selected_target = app->target_count;
        app->target_count++;
    }

    uint32_t target_scene = scene_manager_get_scene_state(
        app->scene_manager, EslPwnSceneBarcodeInput);
    scene_manager_next_scene(app->scene_manager, target_scene);
    return true;
}

void eslpwn_scene_barcode_input_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    numlock_input_reset(app->numlock);
}
