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
                        uint8_t* img_data = malloc(w * h);
                        memset(img_data, 0, w * h);

                        uint32_t row_stride = ((bmp_w + 31) / 32) * 4;
                        uint8_t* row_buf = malloc(row_stride);

                        storage_file_seek(file, data_offset, true);

                        for(int32_t y = 0; y < bmp_h; y++) {
                            /* target_y is the logical row index 0..h-1 */
                            int32_t target_y = top_down ? y : (bmp_h - 1 - y);
                            storage_file_read(file, row_buf, row_stride);
                            
                            if(target_y >= 0 && target_y < (int32_t)h) {
                                for(int32_t x = 0; x < bmp_w && x < (int32_t)w; x++) {
                                    uint8_t byte = row_buf[x / 8];
                                    uint8_t bit = (byte >> (7 - (x % 8))) & 1;
                                    img_data[target_y * w + x] = (bit == 0) ? 0 : 1; 
                                }
                            }
                        }
                        free(row_buf);

                        /* Force color-clear and use BMP's own dimensions */
                        bool prev_clr = app->color_clear;
                        app->color_clear = true; 
                        tagtinker_build_image_sequence(app, target->plid, img_data, (uint16_t)w, (uint16_t)h, 0, 0, 0, 250);
                        app->color_clear = prev_clr;

                        memcpy(app->frame_buf, app->frame_sequence[0], app->frame_lengths[0]);
                        app->frame_len = app->frame_lengths[0];
                        free(img_data);
                        
                        app->tx_spam = false;
                        scene_manager_next_scene(app->scene_manager, TagTinkerSceneTransmit);
                    } else {
                        show_error_dialog(app, "Must be 1-bit BMP");
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
