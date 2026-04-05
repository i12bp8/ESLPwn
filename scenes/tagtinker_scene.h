/*
 * ESLPwn — Scene Definitions
 */

#pragma once

#include <gui/scene_manager.h>

typedef enum {
    EslPwnSceneMainMenu,
    EslPwnSceneBroadcastMenu,
    EslPwnSceneBroadcast,
    EslPwnSceneTargetMenu,
    EslPwnSceneTargetActions,
    EslPwnSceneBarcodeInput,
    EslPwnSceneTextInput,
    EslPwnScenePresetList,
    EslPwnSceneSizePicker,
    EslPwnSceneImageUpload,
    EslPwnSceneTransmit,
    EslPwnSceneAbout,
    EslPwnSceneCount,
} EslPwnScene;

/* Scene handler declarations */
void eslpwn_scene_main_menu_on_enter(void* ctx);
bool eslpwn_scene_main_menu_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_main_menu_on_exit(void* ctx);

void eslpwn_scene_broadcast_menu_on_enter(void* ctx);
bool eslpwn_scene_broadcast_menu_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_broadcast_menu_on_exit(void* ctx);

void eslpwn_scene_broadcast_on_enter(void* ctx);
bool eslpwn_scene_broadcast_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_broadcast_on_exit(void* ctx);

void eslpwn_scene_target_menu_on_enter(void* ctx);
bool eslpwn_scene_target_menu_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_target_menu_on_exit(void* ctx);

void eslpwn_scene_target_actions_on_enter(void* ctx);
bool eslpwn_scene_target_actions_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_target_actions_on_exit(void* ctx);

void eslpwn_scene_barcode_input_on_enter(void* ctx);
bool eslpwn_scene_barcode_input_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_barcode_input_on_exit(void* ctx);

void eslpwn_scene_text_input_on_enter(void* ctx);
bool eslpwn_scene_text_input_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_text_input_on_exit(void* ctx);

void eslpwn_scene_size_picker_on_enter(void* ctx);
bool eslpwn_scene_size_picker_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_size_picker_on_exit(void* ctx);

void eslpwn_scene_preset_list_on_enter(void* ctx);
bool eslpwn_scene_preset_list_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_preset_list_on_exit(void* ctx);

void eslpwn_scene_image_upload_on_enter(void* ctx);
bool eslpwn_scene_image_upload_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_image_upload_on_exit(void* ctx);

void eslpwn_scene_transmit_on_enter(void* ctx);
bool eslpwn_scene_transmit_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_transmit_on_exit(void* ctx);

void eslpwn_scene_about_on_enter(void* ctx);
bool eslpwn_scene_about_on_event(void* ctx, SceneManagerEvent event);
void eslpwn_scene_about_on_exit(void* ctx);
