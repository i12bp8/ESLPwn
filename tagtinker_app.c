/*
 * ESLPwn - ESL Flipper Zero application
 *
 * Transmit infrared commands to supported ESL displays
 * using the built-in IR LED at 1.255 MHz carrier.
 *
 * App by I12BP8 - github.com/i12bp8
 * Research by furrtek - github.com/furrtek
 *
 * SPDX-License-Identifier: MIT
 */

#include "tagtinker_app.h"

static bool navigation_cb(void* ctx) {
    EslPwnApp* app = ctx;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void tick_cb(void* ctx) {
    EslPwnApp* app = ctx;
    scene_manager_handle_tick_event(app->scene_manager);
}

static bool custom_event_cb(void* ctx, uint32_t event) {
    EslPwnApp* app = ctx;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

extern const SceneManagerHandlers eslpwn_scene_handlers;

static EslPwnApp* app_alloc(void) {
    EslPwnApp* app = malloc(sizeof(EslPwnApp));
    memset(app, 0, sizeof(EslPwnApp));

    /* Defaults */
    app->page = 0;
    app->duration = 15;
    app->repeats = 200;
    app->draw_width = 48;
    app->draw_height = 48;
    app->img_page = 1;
    app->esl_width = 200;
    app->esl_height = 80;
    app->color_clear = false;
    app->invert_text = false;
    strcpy(app->text_input_buf, "TagTinker");
    
    app->target_count = 1;
    app->selected_target = 0;
    EslPwnTarget* t = &app->targets[0];
    strcpy(t->barcode, "N4611470825513274");
    eslpwn_barcode_to_plid(t->barcode, t->plid);
    snprintf(t->name, ESLPWN_TARGET_NAME_LEN, "Tag ...13274");

    /* Scene manager */
    app->scene_manager = scene_manager_alloc(&eslpwn_scene_handlers, app);

    /* View dispatcher */
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, navigation_cb);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, tick_cb, 50);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_cb);

    /* GUI */
    app->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    /* Notifications */
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    /* Views */
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewSubmenu, submenu_get_view(app->submenu));

    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewVarItemList,
        variable_item_list_get_view(app->var_item_list));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewTextInput, text_input_get_view(app->text_input));

    app->popup = popup_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewPopup, popup_get_view(app->popup));

    app->widget = widget_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewWidget, widget_get_view(app->widget));

    app->numlock = numlock_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewNumlock, numlock_input_get_view(app->numlock));

    app->transmit_view = view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewTransmit, app->transmit_view);

    app->about_view = view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, EslPwnViewAbout, app->about_view);

    app->dialogs = furi_record_open(RECORD_DIALOGS);

    /* Allocate Thread and Timer for animations and TX */
    app->tx_thread = furi_thread_alloc();
    furi_thread_set_name(app->tx_thread, "ESLPwnTx");
    furi_thread_set_stack_size(app->tx_thread, 2048);
    furi_thread_set_priority(app->tx_thread, FuriThreadPriorityHighest);
    furi_thread_set_context(app->tx_thread, app);

    return app;
}

static void app_free(EslPwnApp* app) {
    eslpwn_ir_deinit();

    if(app->frame_sequence) {
        for(size_t i = 0; i < app->frame_seq_count; i++)
            free(app->frame_sequence[i]);
        free(app->frame_sequence);
        free(app->frame_lengths);
        free(app->frame_repeats);
    }

    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewVarItemList);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewPopup);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewNumlock);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewTransmit);
    view_dispatcher_remove_view(app->view_dispatcher, EslPwnViewAbout);

    submenu_free(app->submenu);
    variable_item_list_free(app->var_item_list);
    text_input_free(app->text_input);
    popup_free(app->popup);
    widget_free(app->widget);
    numlock_input_free(app->numlock);

    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_DIALOGS);

    view_free(app->transmit_view);
    view_free(app->about_view);
    furi_thread_free(app->tx_thread);

    free(app);
}

int32_t eslpwn_app_main(void* p) {
    UNUSED(p);
    EslPwnApp* app = app_alloc();
    scene_manager_next_scene(app->scene_manager, EslPwnSceneMainMenu);
    view_dispatcher_run(app->view_dispatcher);
    app_free(app);
    return 0;
}
