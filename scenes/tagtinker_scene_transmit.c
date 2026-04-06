/**
 * Transmit Scene
 * Displays cool animations while transmitting in a background thread.
 */

#include "../tagtinker_app.h"
#include <furi_hal.h>

typedef struct {
    TagTinkerApp* app;
    uint32_t tick;
    bool completed;
    bool ok;
} TxViewModel;

static int32_t tx_thread_callback(void* context) {
    TagTinkerApp* app = context;
    bool ok = true;

    tagtinker_ir_init();

    do {
        if(app->frame_seq_count > 0) {
            for(size_t i = 0; i < app->frame_seq_count; i++) {
                if(!app->tx_active) { ok = false; break; }
                if(i > 0) furi_delay_ms(20);
                ok = tagtinker_ir_transmit(
                    app->frame_sequence[i], app->frame_lengths[i],
                    app->frame_repeats[i], 10);
                if(!ok) break;
            }
        } else {
            ok = tagtinker_ir_transmit(app->frame_buf, app->frame_len, app->repeats, 10);
        }

        if(app->tx_spam && app->tx_active) {
            furi_delay_ms(50);
        }
    } while(app->tx_spam && app->tx_active);

    app->tx_active = false;
    /* Use event payload to securely pass result instead of querying running thread! */
    view_dispatcher_send_custom_event(app->view_dispatcher, 101 + (ok ? 0 : 1)); 
    return 0;
}

static void transmit_draw_cb(Canvas* canvas, void* _model) {
    TxViewModel* model = _model;
    TagTinkerApp* app = model->app;
    uint32_t t = model->tick;

    /* Left: Top-Down Detailed Flipper Zero Vector Model */
    int f_x = -15; // Protruding from left screen edge
    int f_y = 10;
    int f_w = 40;
    int f_h = 36;
    /* Main casing */
    canvas_draw_rframe(canvas, f_x, f_y, f_w, f_h, 3);
    /* Outer screen outline */
    canvas_draw_rframe(canvas, f_x + 10, f_y + 4, 20, 16, 2);
    /* Screen inner solid */
    canvas_draw_box(canvas, f_x + 12, f_y + 6, 16, 12);
    /* Flipper Screen highlight glow */
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_dot(canvas, f_x + 14, f_y + 8); 
    canvas_draw_dot(canvas, f_x + 15, f_y + 8);
    canvas_set_color(canvas, ColorBlack);
    /* D-Pad Matrix */
    canvas_draw_circle(canvas, f_x + 18, f_y + 27, 5); // Outer
    canvas_draw_circle(canvas, f_x + 18, f_y + 27, 2); // Inner button
    /* GPIO / Case Vents */
    for(int i=0; i<4; i++) {
        canvas_draw_dot(canvas, f_x + 32, f_y + 8 + i*3);
        canvas_draw_dot(canvas, f_x + 34, f_y + 8 + i*3);
    }
    /* IR Blaster window on the right tip */
    int ir_x = f_x + f_w; // 25
    int ir_y = f_y + 10;
    canvas_draw_box(canvas, ir_x, ir_y, 4, 16);

    int center_y = f_y + 18; /* perfectly aligns with tag center */

    if (app->tx_active) {
        /* Aggressive Pulsing Data-Stream Chevrons with bits */
        uint8_t wave_phase = (t * 2) % 15;
        for(int w=0; w<2; w++) {
            int r = wave_phase + w*18; 
            if(r > 0 && r < 20) {
                int wave_x = ir_x + 4 + r;
                int wave_h = r/2 + 3; 
                /* 2px thick chevron pointing right */
                canvas_draw_line(canvas, wave_x,   center_y, wave_x - wave_h/2, center_y - wave_h);
                canvas_draw_line(canvas, wave_x,   center_y, wave_x - wave_h/2, center_y + wave_h);
                canvas_draw_line(canvas, wave_x+1, center_y, wave_x+1 - wave_h/2, center_y - wave_h);
                canvas_draw_line(canvas, wave_x+1, center_y, wave_x+1 - wave_h/2, center_y + wave_h);
                /* Data stream particles */
                if((t + w) % 2 == 0) {
                    canvas_draw_dot(canvas, wave_x + 3, center_y - wave_h/2);
                    canvas_draw_dot(canvas, wave_x - 3, center_y + wave_h/2 + 2);
                }
            }
        }
    }

    /* Right: Detailed ESL tag model */
    int tag_x = 52;
    int tag_y = 8;
    int tag_w = 75;
    int tag_h = 40;
    /* Outer casing bounding rect */
    canvas_draw_rframe(canvas, tag_x, tag_y, tag_w, tag_h, 2);
    /* Internal 3D bezel shadow */
    canvas_draw_rframe(canvas, tag_x+1, tag_y+1, tag_w-2, tag_h-2, 1);
    
    /* True 32-bit encoded barcode representation on the rim */
    int bc_x = tag_x + 3;
    int bc_w = 8;
    int bc_y = tag_y + 4;
    uint8_t bar_pattern[] = {0b10110100, 0b11010010, 0b10011011, 0b01101010};
    for(int i=0; i<32; i++) {
        if( (bar_pattern[i/8] >> (7-(i%8))) & 1 ) {
            canvas_draw_line(canvas, bc_x, bc_y + i, bc_x + bc_w - 1, bc_y + i);
        }
    }

    /* Screen display hardware border */
    int scr_x = tag_x + 14;
    int scr_y = tag_y + 3;
    int scr_w = tag_w - 17;
    int scr_h = tag_h - 6;
    canvas_draw_frame(canvas, scr_x - 1, scr_y - 1, scr_w + 2, scr_h + 2);

    /* E-Paper Sweep Logic */
    int cycle = t % 40; /* 2 second loop at 20fps */
    
    if (app->tx_active) {
        bool show_result = (cycle >= 20);
        
        if(!show_result) {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, scr_x + 2, scr_y + 11, "LAB NOTE");
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, scr_x + 5, scr_y + 26, "STUDY");
        } else {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(
                canvas, scr_x + scr_w / 2, scr_y + scr_h / 2, AlignCenter, AlignCenter, "FLIPPED ;)");
        }

        /* Glitchy Matrix Wipe Transition */
        if(cycle >= 15 && cycle < 20) {
            int wipe_h = ((cycle - 15) * scr_h) / 5;
            canvas_draw_box(canvas, scr_x, scr_y, scr_w, wipe_h);
            /* Algorithmic leading edge burn/glitch */
            for(int xx = 0; xx < scr_w; xx += 2) {
                canvas_draw_dot(canvas, scr_x + xx, scr_y + wipe_h + ((xx+t)%4));
            }
        } else if (cycle >= 20 && cycle < 25) {
            int wipe_h = ((cycle - 20) * scr_h) / 5;
            canvas_draw_box(canvas, scr_x, scr_y + wipe_h, scr_w, scr_h - wipe_h);
            /* Algorithmic trailing edge burn/glitch */
            for(int xx = 0; xx < scr_w; xx += 2) {
                canvas_draw_dot(canvas, scr_x + xx, scr_y + wipe_h - 1 - ((xx+t)%4));
            }
        }
    } else {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(
            canvas, scr_x + scr_w / 2, scr_y + scr_h / 2, AlignCenter, AlignCenter, "FLIPPED ;)");
    }

    /* Submenu action hint decoupled from canvas models */
    canvas_set_font(canvas, FontSecondary);
    if(app->tx_spam) {
        canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignTop, "[<-] Stop Repeat");
    } else {
        canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignTop, app->tx_active ? "[<-] Cancel" : "[<-] Back");
    }
}

void tagtinker_scene_transmit_on_enter(void* context) {
    TagTinkerApp* app = context;

    if(!app->transmit_view_allocated) {
        view_allocate_model(app->transmit_view, ViewModelTypeLockFree, sizeof(TxViewModel));
        view_set_context(app->transmit_view, app);
        view_set_draw_callback(app->transmit_view, transmit_draw_cb);
        app->transmit_view_allocated = true;
    }

    TxViewModel* model = view_get_model(app->transmit_view);
    model->app = app;
    model->tick = 0;
    model->completed = false;
    model->ok = true;
    view_commit_model(app->transmit_view, true);

    view_dispatcher_switch_to_view(app->view_dispatcher, TagTinkerViewTransmit);

    app->tx_active = true;
    furi_thread_set_callback(app->tx_thread, tx_thread_callback);
    furi_thread_start(app->tx_thread);
}

bool tagtinker_scene_transmit_on_event(void* context, SceneManagerEvent event) {
    TagTinkerApp* app = context;

    if(event.type == SceneManagerEventTypeBack) {
        if(app->tx_active) {
            app->tx_active = false;
            tagtinker_ir_stop();
            return true;
        } else {
            if(!scene_manager_search_and_switch_to_previous_scene(app->scene_manager, TagTinkerSceneTargetActions)) {
                if(!scene_manager_search_and_switch_to_previous_scene(app->scene_manager, TagTinkerSceneBroadcast)) {
                    if(!scene_manager_search_and_switch_to_previous_scene(app->scene_manager, TagTinkerSceneBroadcastMenu)) {
                        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, TagTinkerSceneMainMenu);
                    }
                }
            }
            return true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        TxViewModel* model = view_get_model(app->transmit_view);
        model->tick++;
        view_commit_model(app->transmit_view, true);
        return true;
    } else if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == 101 || event.event == 102) { /* Thread Done */
            app->tx_active = false;
            tagtinker_ir_deinit();
            TxViewModel* model = view_get_model(app->transmit_view);
            model->completed = true;
            model->ok = (event.event == 101);
            view_commit_model(app->transmit_view, true);
            notification_message(app->notifications, &sequence_success);
        }
        return true;
    }
    return false;
}

void tagtinker_scene_transmit_on_exit(void* context) {
    TagTinkerApp* app = context;
    
    app->tx_active = false;
    tagtinker_ir_stop();
    furi_thread_join(app->tx_thread);
    tagtinker_ir_deinit();

    if(app->frame_sequence) {
        for(size_t i = 0; i < app->frame_seq_count; i++)
            free(app->frame_sequence[i]);
        free(app->frame_sequence);
        free(app->frame_lengths);
        free(app->frame_repeats);
        app->frame_sequence = NULL;
        app->frame_lengths = NULL;
        app->frame_repeats = NULL;
        app->frame_seq_count = 0;
    }
}
