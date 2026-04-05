/*
 * ESLPwn — App State
 */

#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/popup.h>
#include <gui/modules/widget.h>
#include <dialogs/dialogs.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>

#include "views/numlock_input.h"

#include "scenes/tagtinker_scene.h"
#include "protocol/tagtinker_proto.h"
#include "ir/tagtinker_ir.h"

#define ESLPWN_TAG          "TagTinker"
#define ESLPWN_DISPLAY_NAME "TagTinker"
#define ESLPWN_VERSION      "1.1"
#define ESLPWN_BC_LEN   17
#define ESLPWN_HEX_LEN  64
#define ESLPWN_MAX_TARGETS 16
#define ESLPWN_TARGET_NAME_LEN 16
#define ESLPWN_MAX_PRESETS 6
#define ESLPWN_PRESET_TEXT_LEN 32

/* Views */
typedef enum {
    EslPwnViewSubmenu,
    EslPwnViewVarItemList,
    EslPwnViewTextInput,
    EslPwnViewPopup,
    EslPwnViewWidget,
    EslPwnViewNumlock,
    EslPwnViewTargetActions,
    EslPwnViewTransmit,
    EslPwnViewAbout,
} EslPwnView;

/* Saved ESL target */
typedef struct {
    char name[ESLPWN_TARGET_NAME_LEN + 1];
    char barcode[ESLPWN_BC_LEN + 1];
    uint8_t plid[4];
} EslPwnTarget;

struct EslPwnApp {
    /* GUI */
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    DialogsApp* dialogs;

    /* Views */
    Submenu* submenu;
    VariableItemList* var_item_list;
    TextInput* text_input;
    Popup* popup;
    Widget* widget;
    NumlockInput* numlock;
    View* target_actions_view;
    View* transmit_view;
    View* about_view;
    bool transmit_view_allocated;
    bool about_view_allocated;

    /* TX state */
    bool tx_active;
    FuriThread* tx_thread;

    /* Broadcast settings */
    uint8_t broadcast_type;
    uint8_t page;
    uint16_t duration;
    uint16_t repeats;
    bool forever;
    bool tx_spam;

    /* Current target */
    char barcode[ESLPWN_BC_LEN + 1];
    uint8_t plid[4];
    bool barcode_valid;
    int8_t selected_target; /* -1 = none */

    /* Saved targets */
    EslPwnTarget targets[ESLPWN_MAX_TARGETS];
    uint8_t target_count;

    /* Text to push */
    char text_input_buf[64];

    /* ESL display size for current target */
    uint16_t esl_width;
    uint16_t esl_height;

    /* Frame buffer */
    uint8_t frame_buf[ESLPWN_MAX_FRAME_SIZE];
    size_t frame_len;

    /* Multi-frame sequence */
    uint8_t** frame_sequence;
    size_t* frame_lengths;
    uint16_t* frame_repeats;
    size_t frame_seq_count;
    bool use_hardcoded_image;
    bool invert_text;
    bool color_clear;

    /* Saved presets */
    struct {
        uint16_t width;
        uint16_t height;
        uint8_t  page;
        bool     invert;
        bool     color_clear;
        char     text[ESLPWN_PRESET_TEXT_LEN];
    } presets[ESLPWN_MAX_PRESETS];
    uint8_t preset_count;

    /* Image settings */
    uint8_t img_page;
    uint16_t draw_width;
    uint16_t draw_height;

    /* Indicates which mode triggered raw cmd (0=broadcast, 1=targeted) */
    uint8_t raw_mode;
};

/* Main menu items */
typedef enum {
    EslPwnMenuBroadcast,
    EslPwnMenuTargetESL,
    EslPwnMenuAbout,
} EslPwnMainMenuItem;

/* Broadcast menu items */
typedef enum {
    EslPwnBroadcastFlipPage,
    EslPwnBroadcastDebugScreen,
} EslPwnBroadcastMenuItem;

/* Target action items */
typedef enum {
    EslPwnTargetPushText,
    EslPwnTargetPushImage,
    EslPwnTargetPushTestImage,
    EslPwnTargetPingFlash,
} EslPwnTargetActionItem;
