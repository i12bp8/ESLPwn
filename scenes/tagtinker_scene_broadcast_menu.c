/*
 * Broadcast menu - page select / diagnostic page
 */

#include "../tagtinker_app.h"

static void broadcast_menu_cb(void* ctx, uint32_t index) {
    EslPwnApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void eslpwn_scene_broadcast_menu_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Broadcast Tag");

    submenu_add_item(app->submenu, "Change Page",      EslPwnBroadcastFlipPage,    broadcast_menu_cb, app);
    submenu_add_item(app->submenu, "Show Debug Page",  EslPwnBroadcastDebugScreen, broadcast_menu_cb, app);

    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, EslPwnSceneBroadcastMenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewSubmenu);
}

bool eslpwn_scene_broadcast_menu_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    scene_manager_set_scene_state(app->scene_manager, EslPwnSceneBroadcastMenu, event.event);

    switch(event.event) {
    case EslPwnBroadcastFlipPage:
        app->broadcast_type = EslPwnBroadcastFlipPage;
        scene_manager_next_scene(app->scene_manager, EslPwnSceneBroadcast);
        return true;
    case EslPwnBroadcastDebugScreen:
        app->broadcast_type = EslPwnBroadcastDebugScreen;
        scene_manager_next_scene(app->scene_manager, EslPwnSceneBroadcast);
        return true;
    }
    return false;
}

void eslpwn_scene_broadcast_menu_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    submenu_reset(app->submenu);
}
