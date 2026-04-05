/*
 * Saved tag menu - saved targets + add new
 */

#include "../tagtinker_app.h"

enum {
    TargetMenuAddNew = 100,
};

static void target_menu_cb(void* ctx, uint32_t index) {
    EslPwnApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void eslpwn_scene_target_menu_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Target Tag");

    /* Add new target */
    submenu_add_item(app->submenu, "+ Add Tag", TargetMenuAddNew, target_menu_cb, app);

    /* List saved targets */
    for(uint8_t i = 0; i < app->target_count; i++) {
        /* Use index as event id (0..15) */
        submenu_add_item(
            app->submenu,
            app->targets[i].name[0] ? app->targets[i].name : app->targets[i].barcode,
            i,
            target_menu_cb,
            app);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewSubmenu);
}

bool eslpwn_scene_target_menu_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(event.event == TargetMenuAddNew) {
        /* Go to barcode input, then come back */
        scene_manager_set_scene_state(
            app->scene_manager, EslPwnSceneBarcodeInput, EslPwnSceneTargetActions);
        scene_manager_next_scene(app->scene_manager, EslPwnSceneBarcodeInput);
        return true;
    }

    /* Selected a saved target */
    if(event.event < app->target_count) {
        app->selected_target = (int8_t)event.event;
        memcpy(app->barcode, app->targets[event.event].barcode, ESLPWN_BC_LEN + 1);
        memcpy(app->plid, app->targets[event.event].plid, 4);
        app->barcode_valid = true;
        scene_manager_next_scene(app->scene_manager, EslPwnSceneTargetActions);
        return true;
    }

    return false;
}

void eslpwn_scene_target_menu_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    submenu_reset(app->submenu);
}
