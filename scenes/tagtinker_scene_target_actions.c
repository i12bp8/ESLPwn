/*
 * Target Actions — what to do with a selected ESL
 */

#include "../tagtinker_app.h"
#include <dialogs/dialogs.h>

static void target_actions_cb(void* ctx, uint32_t index) {
    TagTinkerApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

static void show_target_details(TagTinkerApp* app, const TagTinkerTarget* target) {
    if(!target) return;

    char body[160];
    if(target->profile.known && target->profile.width && target->profile.height) {
        snprintf(
            body,
            sizeof(body),
            "Type: %u\nKind: %s\nSize: %ux%u\nColor: %s",
            target->profile.type_code,
            tagtinker_profile_kind_label(target->profile.kind),
            target->profile.width,
            target->profile.height,
            tagtinker_profile_color_label(target->profile.color));
    } else if(target->profile.known) {
        snprintf(
            body,
            sizeof(body),
            "Type: %u\nKind: %s\nColor: %s",
            target->profile.type_code,
            tagtinker_profile_kind_label(target->profile.kind),
            tagtinker_profile_color_label(target->profile.color));
    } else {
        snprintf(body, sizeof(body), "Type: %u\nProfile: Unknown", target->profile.type_code);
    }

    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Tag Details", 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, body, 64, 18, AlignCenter, AlignTop);
    dialog_message_set_buttons(message, "OK", NULL, NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

void tagtinker_scene_target_actions_on_enter(void* ctx) {
    TagTinkerApp* app = ctx;
    TagTinkerTarget* target = (app->selected_target >= 0) ? &app->targets[app->selected_target] : NULL;
    bool allow_graphics = tagtinker_target_supports_graphics(target);

    submenu_reset(app->submenu);

    char header[24];
    snprintf(header, sizeof(header), "Tag: %.8s...", app->barcode);
    submenu_set_header(app->submenu, header);
    submenu_add_item(app->submenu, "Tag Details", TagTinkerTargetDetails, target_actions_cb, app);

    if(allow_graphics) {
        submenu_add_item(app->submenu, "Show Text Preset", TagTinkerTargetPushText, target_actions_cb, app);
        submenu_add_item(app->submenu, "Show Custom Image", TagTinkerTargetPushImage, target_actions_cb, app);
    }

    submenu_add_item(app->submenu, "LED Response Check",  TagTinkerTargetPingFlash,     target_actions_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, TagTinkerViewSubmenu);
}

bool tagtinker_scene_target_actions_on_event(void* ctx, SceneManagerEvent event) {
    TagTinkerApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    switch(event.event) {
    case TagTinkerTargetDetails:
        show_target_details(app, &app->targets[app->selected_target]);
        return true;
    case TagTinkerTargetPushText:
        if(!tagtinker_target_supports_graphics(&app->targets[app->selected_target])) return true;
        scene_manager_next_scene(app->scene_manager, TagTinkerScenePresetList);
        return true;
    case TagTinkerTargetPushImage:
        if(!tagtinker_target_supports_graphics(&app->targets[app->selected_target])) return true;
        scene_manager_next_scene(app->scene_manager, TagTinkerSceneImageUpload);
        return true;
    case TagTinkerTargetPingFlash:
        {
            TagTinkerTarget* target = &app->targets[app->selected_target];
            
            /* The blink command actually requires two frames:
             * 1. A wake ping (lots of repeats)
             * 2. The green LED command payload (0x06 0xC9 0x00 0x00 0x00 0x00)
             */
            app->frame_seq_count = 2;
            app->frame_sequence = malloc(sizeof(uint8_t*) * 2);
            app->frame_lengths  = malloc(sizeof(size_t) * 2);
            app->frame_repeats  = malloc(sizeof(uint16_t) * 2);
            
            /* 1. Wake ping */
            app->frame_sequence[0] = malloc(TAGTINKER_MAX_FRAME_SIZE);
            app->frame_lengths[0] = tagtinker_make_ping_frame(app->frame_sequence[0], target->plid);
            app->frame_repeats[0] = 500;
            
            /* 2. LED command */
            app->frame_sequence[1] = malloc(TAGTINKER_MAX_FRAME_SIZE);
            const uint8_t blink_payload[6] = {0x06, 0xC9, 0x00, 0x00, 0x00, 0x00};
            app->frame_lengths[1] = tagtinker_make_addressed_frame(
                app->frame_sequence[1], target->plid, blink_payload, 6);
            app->frame_repeats[1] = 100;
            
            /* Put the first frame in the preview buffer */
            memcpy(app->frame_buf, app->frame_sequence[0], app->frame_lengths[0]);
            app->frame_len = app->frame_lengths[0];
            
            app->tx_spam = false;
            scene_manager_next_scene(app->scene_manager, TagTinkerSceneTransmit);
        }
        return true;
    }
    return false;
}

void tagtinker_scene_target_actions_on_exit(void* ctx) {
    TagTinkerApp* app = ctx;
    submenu_reset(app->submenu);
}
