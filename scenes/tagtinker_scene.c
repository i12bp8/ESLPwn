/*
 * Scene handler table
 */

#include "tagtinker_scene.h"

void(*const eslpwn_scene_on_enter_handlers[])(void*) = {
    eslpwn_scene_main_menu_on_enter,
    eslpwn_scene_broadcast_menu_on_enter,
    eslpwn_scene_broadcast_on_enter,
    eslpwn_scene_target_menu_on_enter,
    eslpwn_scene_target_actions_on_enter,
    eslpwn_scene_barcode_input_on_enter,
    eslpwn_scene_text_input_on_enter,
    eslpwn_scene_preset_list_on_enter,
    eslpwn_scene_size_picker_on_enter,
    eslpwn_scene_image_upload_on_enter,
    eslpwn_scene_transmit_on_enter,
    eslpwn_scene_about_on_enter,
};

bool(*const eslpwn_scene_on_event_handlers[])(void*, SceneManagerEvent) = {
    eslpwn_scene_main_menu_on_event,
    eslpwn_scene_broadcast_menu_on_event,
    eslpwn_scene_broadcast_on_event,
    eslpwn_scene_target_menu_on_event,
    eslpwn_scene_target_actions_on_event,
    eslpwn_scene_barcode_input_on_event,
    eslpwn_scene_text_input_on_event,
    eslpwn_scene_preset_list_on_event,
    eslpwn_scene_size_picker_on_event,
    eslpwn_scene_image_upload_on_event,
    eslpwn_scene_transmit_on_event,
    eslpwn_scene_about_on_event,
};

void(*const eslpwn_scene_on_exit_handlers[])(void*) = {
    eslpwn_scene_main_menu_on_exit,
    eslpwn_scene_broadcast_menu_on_exit,
    eslpwn_scene_broadcast_on_exit,
    eslpwn_scene_target_menu_on_exit,
    eslpwn_scene_target_actions_on_exit,
    eslpwn_scene_barcode_input_on_exit,
    eslpwn_scene_text_input_on_exit,
    eslpwn_scene_preset_list_on_exit,
    eslpwn_scene_size_picker_on_exit,
    eslpwn_scene_image_upload_on_exit,
    eslpwn_scene_transmit_on_exit,
    eslpwn_scene_about_on_exit,
};

const SceneManagerHandlers eslpwn_scene_handlers = {
    .on_enter_handlers = eslpwn_scene_on_enter_handlers,
    .on_event_handlers = eslpwn_scene_on_event_handlers,
    .on_exit_handlers = eslpwn_scene_on_exit_handlers,
    .scene_num = EslPwnSceneCount,
};
