/*
 * TagTinker — Text Input Scene
 *
 * Prompts user for a text string, then goes to Size Picker.
 */

#include "../tagtinker_app.h"

static void text_input_done_cb(void* ctx) {
    TagTinkerApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, 0);
}

void tagtinker_scene_text_input_on_enter(void* ctx) {
    TagTinkerApp* app = ctx;
    bool clear = scene_manager_get_scene_state(app->scene_manager, TagTinkerSceneTextInput) == 0;
    
    if(clear) {
        memset(app->text_input_buf, 0, sizeof(app->text_input_buf));
        scene_manager_set_scene_state(app->scene_manager, TagTinkerSceneTextInput, 1);
    }

    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Text to display:");
    text_input_set_result_callback(
        app->text_input,
        text_input_done_cb,
        app,
        app->text_input_buf,
        sizeof(app->text_input_buf),
        clear);

    view_dispatcher_switch_to_view(app->view_dispatcher, TagTinkerViewTextInput);
}

bool tagtinker_scene_text_input_on_event(void* ctx, SceneManagerEvent event) {
    TagTinkerApp* app = ctx;
    if(event.type != SceneManagerEventTypeCustom) return false;

    if(strlen(app->text_input_buf) == 0) {
        scene_manager_search_and_switch_to_previous_scene(
            app->scene_manager, TagTinkerSceneTargetActions);
        return true;
    }

    /* Configure settings (Add Preset flow) */
    scene_manager_next_scene(app->scene_manager, TagTinkerSceneSizePicker);
    return true;
}

void tagtinker_scene_text_input_on_exit(void* ctx) {
    TagTinkerApp* app = ctx;
    text_input_reset(app->text_input);
}
