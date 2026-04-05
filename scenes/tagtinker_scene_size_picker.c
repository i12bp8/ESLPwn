/*
 * Size Picker — custom W/H (5px steps), mode, page, color, transmit
 */

#include "../tagtinker_app.h"
#include "../views/tagtinker_font.h"
#include "../protocol/tagtinker_proto.h"
#include <storage/storage.h>

/* Limits */
#define MIN_W  48
#define MAX_W  200
#define STEP_W 8
#define MIN_H  50
#define MAX_H  90
#define STEP_H 5

#define W_COUNT ((MAX_W - MIN_W) / STEP_W + 1)
#define H_COUNT ((MAX_H - MIN_H) / STEP_H + 1)

/* Setting indices */
enum {
    SettingWidth,
    SettingHeight,
    SettingPage,
    SettingColor,
    SettingMode,
    SettingSave,
    SettingTransmit,
};

/* ── Callbacks ── */

static void width_changed(VariableItem* item) {
    EslPwnApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->esl_width = MIN_W + idx * STEP_W;

    char buf[8];
    snprintf(buf, sizeof(buf), "%u", app->esl_width);
    variable_item_set_current_value_text(item, buf);
}

static void height_changed(VariableItem* item) {
    EslPwnApp* app = variable_item_get_context(item);
    uint8_t idx = variable_item_get_current_value_index(item);
    app->esl_height = MIN_H + idx * STEP_H;

    char buf[8];
    snprintf(buf, sizeof(buf), "%u", app->esl_height);
    variable_item_set_current_value_text(item, buf);
}

static void page_changed(VariableItem* item) {
    EslPwnApp* app = variable_item_get_context(item);
    app->img_page = variable_item_get_current_value_index(item);

    char buf[4];
    snprintf(buf, sizeof(buf), "%u", app->img_page);
    variable_item_set_current_value_text(item, buf);
}

static void color_changed(VariableItem* item) {
    EslPwnApp* app = variable_item_get_context(item);
    app->invert_text = (variable_item_get_current_value_index(item) == 1);

    variable_item_set_current_value_text(
        item, app->invert_text ? "W on B" : "B on W");
}

static void mode_changed(VariableItem* item) {
    EslPwnApp* app = variable_item_get_context(item);
    app->color_clear = (variable_item_get_current_value_index(item) == 1);

    variable_item_set_current_value_text(
        item, app->color_clear ? "BW+Clr" : "BW Fast");
}

static void save_presets_to_sd(EslPwnApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, APP_DATA_PATH(""));

    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, APP_DATA_PATH("presets.txt"),
           FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        for(uint8_t i = 0; i < app->preset_count; i++) {
            char line[80];
            int len = snprintf(line, sizeof(line), "%u|%u|%u|%d|%d|%s\n",
                app->presets[i].width, app->presets[i].height,
                app->presets[i].page,
                app->presets[i].invert ? 1 : 0,
                app->presets[i].color_clear ? 1 : 0,
                app->presets[i].text);
            storage_file_write(file, line, (uint16_t)len);
        }
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void setting_cb(void* ctx, uint32_t index) {
    EslPwnApp* app = ctx;

    if(index == SettingSave) {
        /* Save current settings as a preset */
        if(app->preset_count < ESLPWN_MAX_PRESETS) {
            uint8_t idx = app->preset_count++;
            app->presets[idx].width = app->esl_width;
            app->presets[idx].height = app->esl_height;
            app->presets[idx].page = app->img_page;
            app->presets[idx].invert = app->invert_text;
            app->presets[idx].color_clear = app->color_clear;
            strncpy(app->presets[idx].text, app->text_input_buf,
                ESLPWN_PRESET_TEXT_LEN - 1);
            app->presets[idx].text[ESLPWN_PRESET_TEXT_LEN - 1] = '\0';

            save_presets_to_sd(app);
            notification_message(app->notifications, &sequence_success);
            FURI_LOG_I(ESLPWN_TAG, "Preset saved: %ux%u", app->esl_width, app->esl_height);
        }
        return;
    }

    if(index != SettingTransmit) return;

    FURI_LOG_I(ESLPWN_TAG, "TX: %ux%u pg=%u inv=%d clr=%d",
        app->esl_width, app->esl_height, app->img_page,
        app->invert_text, app->color_clear);

    size_t num_pixels = (size_t)app->esl_width * app->esl_height;
    uint8_t* pixels = malloc(num_pixels);
    if(!pixels) return;

    if(app->invert_text) {
        render_text_ex(pixels, app->esl_width, app->esl_height, app->text_input_buf, 0, 1);
    } else {
        render_text(pixels, app->esl_width, app->esl_height, app->text_input_buf);
    }

    EslPwnTarget* target = &app->targets[app->selected_target];

    eslpwn_build_image_sequence(
        app, target->plid, pixels,
        app->esl_width, app->esl_height, app->img_page,
        0, 0, 250);

    free(pixels);
    app->tx_spam = false;
    scene_manager_next_scene(app->scene_manager, EslPwnSceneTransmit);
}

/* ── Scene handlers ── */

void eslpwn_scene_size_picker_on_enter(void* ctx) {
    EslPwnApp* app = ctx;

    variable_item_list_reset(app->var_item_list);

    /* Clamp to valid grid positions */
    if(app->esl_width < MIN_W) app->esl_width = MIN_W;
    if(app->esl_width > MAX_W) app->esl_width = MAX_W;
    if(app->esl_height < MIN_H) app->esl_height = MIN_H;
    if(app->esl_height > MAX_H) app->esl_height = MAX_H;

    /* Snap to nearest step */
    uint8_t w_idx = (app->esl_width - MIN_W + STEP_W / 2) / STEP_W;
    if(w_idx >= W_COUNT) w_idx = W_COUNT - 1;
    app->esl_width = MIN_W + w_idx * STEP_W;

    uint8_t h_idx = (app->esl_height - MIN_H + STEP_H / 2) / STEP_H;
    if(h_idx >= H_COUNT) h_idx = H_COUNT - 1;
    app->esl_height = MIN_H + h_idx * STEP_H;

    /* Width */
    VariableItem* item_w = variable_item_list_add(
        app->var_item_list, "Width", W_COUNT, width_changed, app);
    variable_item_set_current_value_index(item_w, w_idx);
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", app->esl_width);
        variable_item_set_current_value_text(item_w, buf);
    }

    /* Height */
    VariableItem* item_h = variable_item_list_add(
        app->var_item_list, "Height", H_COUNT, height_changed, app);
    variable_item_set_current_value_index(item_h, h_idx);
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", app->esl_height);
        variable_item_set_current_value_text(item_h, buf);
    }

    /* Page */
    VariableItem* item_pg = variable_item_list_add(
        app->var_item_list, "Page", 8, page_changed, app);
    variable_item_set_current_value_index(item_pg, app->img_page);
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%u", app->img_page);
        variable_item_set_current_value_text(item_pg, buf);
    }

    /* Color (text polarity) */
    VariableItem* item_col = variable_item_list_add(
        app->var_item_list, "Color", 2, color_changed, app);
    variable_item_set_current_value_index(item_col, app->invert_text ? 1 : 0);
    variable_item_set_current_value_text(
        item_col, app->invert_text ? "W on B" : "B on W");

    /* Mode (BW Fast vs BW+Color clear) */
    VariableItem* item_mode = variable_item_list_add(
        app->var_item_list, "Mode", 2, mode_changed, app);
    variable_item_set_current_value_index(item_mode, app->color_clear ? 1 : 0);
    variable_item_set_current_value_text(
        item_mode, app->color_clear ? "BW+Clr" : "BW Fast");

    /* Save preset button */
    variable_item_list_add(app->var_item_list, "[*] Save Preset", 0, NULL, app);

    /* Transmit button */
    variable_item_list_add(app->var_item_list, ">> Send to Tag <<", 0, NULL, app);

    variable_item_list_set_enter_callback(app->var_item_list, setting_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewVarItemList);
}

bool eslpwn_scene_size_picker_on_event(void* ctx, SceneManagerEvent event) {
    UNUSED(ctx);
    UNUSED(event);
    return false;
}

void eslpwn_scene_size_picker_on_exit(void* ctx) {
    EslPwnApp* app = ctx;
    variable_item_list_reset(app->var_item_list);
}
