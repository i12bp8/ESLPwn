#include "../tagtinker_app.h"
#include <dialogs/dialogs.h>
#include <storage/storage.h>
#include <gui/canvas.h>

static void show_error_dialog(TagTinkerApp* app, const char* text) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Load Error", 64, 0, AlignCenter, AlignTop);
    dialog_message_set_text(message, text, 64, 26, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, "OK", NULL, NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

void tagtinker_scene_image_upload_on_enter(void* ctx) {
    TagTinkerApp* app = ctx;
    TagTinkerTarget* target = &app->targets[app->selected_target];

    FuriString* file_path = furi_string_alloc();
    furi_string_set(file_path, "/ext/apps_data/tagtinker");
    
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, furi_string_get_cstr(file_path));

    DialogsFileBrowserOptions browser_options;
    memset(&browser_options, 0, sizeof(browser_options));
    dialog_file_browser_set_basic_options(&browser_options, ".bmp", NULL);
    browser_options.hide_dot_files = true;

    if(dialog_file_browser_show(app->dialogs, file_path, file_path, &browser_options)) {
        File* file = storage_file_alloc(storage);
        if(storage_file_open(file, furi_string_get_cstr(file_path), FSAM_READ, FSOM_OPEN_EXISTING)) {
            uint8_t header[54];
            if(storage_file_read(file, header, sizeof(header)) == sizeof(header)) {
                if(header[0] == 'B' && header[1] == 'M') {
                    uint32_t data_offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);
                    int32_t bmp_w = header[18] | (header[19] << 8) | (header[20] << 16) | (header[21] << 24);
                    int32_t bmp_h = header[22] | (header[23] << 8) | (header[24] << 16) | (header[25] << 24);
                    uint16_t bpp = header[28] | (header[29] << 8);

                    bool top_down = false;
                    if(bmp_h < 0) {
                        top_down = true;
                        bmp_h = -bmp_h;
                    }

                    if(bpp == 1) {
                        size_t w = (size_t)bmp_w;
                        size_t h = (size_t)bmp_h;
                        UNUSED(data_offset);
                        UNUSED(top_down);
                        tagtinker_prepare_bmp_tx(
                            app,
                            target->plid,
                            furi_string_get_cstr(file_path),
                            (uint16_t)w,
                            (uint16_t)h,
                            app->img_page);
                        scene_manager_next_scene(app->scene_manager, TagTinkerSceneImageOptions);
                    } else if(bpp == 24 || bpp == 32) {
                        size_t w = (size_t)bmp_w;
                        size_t h = (size_t)bmp_h;
                        UNUSED(data_offset);
                        UNUSED(top_down);
                        tagtinker_prepare_bmp_tx(
                            app,
                            target->plid,
                            furi_string_get_cstr(file_path),
                            (uint16_t)w,
                            (uint16_t)h,
                            app->img_page);
                        scene_manager_next_scene(app->scene_manager, TagTinkerSceneImageOptions);
                    } else {
                        show_error_dialog(app, "Use 1/24/32-bit BMP");
                    }
                } else {
                    show_error_dialog(app, "Invalid BMP magic");
                }
            } else {
                show_error_dialog(app, "File too small");
            }
            storage_file_close(file);
        } else {
            show_error_dialog(app, "Could not open file");
        }
        storage_file_free(file);
    } else {
        /* User cancelled browser */
        scene_manager_previous_scene(app->scene_manager);
    }

    furi_record_close(RECORD_STORAGE);
    furi_string_free(file_path);
}

bool tagtinker_scene_image_upload_on_event(void* ctx, SceneManagerEvent event) {
    UNUSED(ctx);
    UNUSED(event);
    return false;
}

void tagtinker_scene_image_upload_on_exit(void* ctx) {
    UNUSED(ctx);
}
