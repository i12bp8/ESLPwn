/*
 * Main Menu
 */

#include "../tagtinker_app.h"

enum {
    MainMenuBroadcast,
    MainMenuTargetESL,
    MainMenuAndroid,
    MainMenuAbout,
};

static void main_menu_cb(void* ctx, uint32_t index) {
    EslPwnApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void eslpwn_scene_main_menu_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, ESLPWN_DISPLAY_NAME " v" ESLPWN_VERSION);

    submenu_add_item(app->submenu, "Broadcast Tag", MainMenuBroadcast, main_menu_cb, app);
    submenu_add_item(app->submenu, "Target Tag",    MainMenuTargetESL, main_menu_cb, app);
    submenu_add_item(app->submenu, "Android App",   MainMenuAndroid,   main_menu_cb, app);
    submenu_add_item(app->submenu, "About",         MainMenuAbout,     main_menu_cb, app);

    submenu_set_selected_item(
        app->submenu,
        scene_manager_get_scene_state(app->scene_manager, EslPwnSceneMainMenu));

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewSubmenu);
}

bool eslpwn_scene_main_menu_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    scene_manager_set_scene_state(app->scene_manager, EslPwnSceneMainMenu, event.event);

    switch(event.event) {
    case MainMenuBroadcast:
        scene_manager_next_scene(app->scene_manager, EslPwnSceneBroadcastMenu);
        return true;
    case MainMenuTargetESL:
        scene_manager_next_scene(app->scene_manager, EslPwnSceneTargetMenu);
        return true;
    case MainMenuAndroid:
        /* state=1 tells About scene to show Android teaser */
        scene_manager_set_scene_state(app->scene_manager, EslPwnSceneAbout, 1);
        scene_manager_next_scene(app->scene_manager, EslPwnSceneAbout);
        return true;
    case MainMenuAbout:
        scene_manager_set_scene_state(app->scene_manager, EslPwnSceneAbout, 0);
        scene_manager_next_scene(app->scene_manager, EslPwnSceneAbout);
        return true;
    }
    return false;
}

void eslpwn_scene_main_menu_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    submenu_reset(app->submenu);
}
