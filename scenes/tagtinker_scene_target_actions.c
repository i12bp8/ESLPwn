/*
 * Target Actions — what to do with a selected ESL
 */

#include "../tagtinker_app.h"

static void target_actions_cb(void* ctx, uint32_t index) {
    EslPwnApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void eslpwn_scene_target_actions_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    submenu_reset(app->submenu);

    char header[24];
    snprintf(header, sizeof(header), "Tag: %.8s...", app->barcode);
    submenu_set_header(app->submenu, header);
    submenu_add_item(app->submenu, "Show Text Preset",    EslPwnTargetPushText,      target_actions_cb, app);
    submenu_add_item(app->submenu, "Show Custom Image",   EslPwnTargetPushImage,     target_actions_cb, app);
    submenu_add_item(app->submenu, "Show Test Pattern",   EslPwnTargetPushTestImage, target_actions_cb, app);
    submenu_add_item(app->submenu, "LED Response Check",  EslPwnTargetPingFlash,     target_actions_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewSubmenu);
}

bool eslpwn_scene_target_actions_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case EslPwnTargetPushText:
        scene_manager_next_scene(app->scene_manager, EslPwnScenePresetList);
        return true;
    case EslPwnTargetPushImage:
        app->use_hardcoded_image = false;
        scene_manager_next_scene(app->scene_manager, EslPwnSceneImageUpload);
        return true;
    case EslPwnTargetPushTestImage:
        app->use_hardcoded_image = true;
        scene_manager_next_scene(app->scene_manager, EslPwnSceneImageUpload);
        return true;
    case EslPwnTargetPingFlash:
        {
            EslPwnTarget* target = &app->targets[app->selected_target];
            
            /* The blink command actually requires two frames:
             * 1. A wake ping (lots of repeats)
             * 2. The green LED command payload (0x06 0xC9 0x00 0x00 0x00 0x00)
             */
            app->frame_seq_count = 2;
            app->frame_sequence = malloc(sizeof(uint8_t*) * 2);
            app->frame_lengths  = malloc(sizeof(size_t) * 2);
            app->frame_repeats  = malloc(sizeof(uint16_t) * 2);
            
            /* 1. Wake ping */
            app->frame_sequence[0] = malloc(ESLPWN_MAX_FRAME_SIZE);
            app->frame_lengths[0] = eslpwn_make_ping_frame(app->frame_sequence[0], target->plid);
            app->frame_repeats[0] = 500;
            
            /* 2. LED command */
            app->frame_sequence[1] = malloc(ESLPWN_MAX_FRAME_SIZE);
            const uint8_t blink_payload[6] = {0x06, 0xC9, 0x00, 0x00, 0x00, 0x00};
            app->frame_lengths[1] = eslpwn_make_addressed_frame(
                app->frame_sequence[1], target->plid, blink_payload, 6);
            app->frame_repeats[1] = 100;
            
            /* Put the first frame in the preview buffer */
            memcpy(app->frame_buf, app->frame_sequence[0], app->frame_lengths[0]);
            app->frame_len = app->frame_lengths[0];
            
            app->tx_spam = false;
            scene_manager_next_scene(app->scene_manager, EslPwnSceneTransmit);
        }
        return true;
    }
    return false;
}

void eslpwn_scene_target_actions_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    submenu_reset(app->submenu);
}
