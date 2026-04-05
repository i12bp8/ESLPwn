/*
 * About — credits (state=0) or Android teaser (state=1)
 */

#include "../tagtinker_app.h"

typedef struct {
    uint32_t mode;
    uint32_t tick;
} AboutViewModel;

static void about_draw_cb(Canvas* canvas, void* _model) {
    AboutViewModel* model = _model;

    canvas_set_font(canvas, FontPrimary);
    if(model->mode == 1) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "In Progress");
    } else {
        canvas_draw_str_aligned(
            canvas, 64, 10, AlignCenter, AlignTop, ESLPWN_DISPLAY_NAME " v" ESLPWN_VERSION);
        
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignTop, "Ported by I12BP8");
        canvas_draw_str_aligned(canvas, 64, 34, AlignCenter, AlignTop, "Research by furrtek");
        canvas_draw_str_aligned(canvas, 64, 44, AlignCenter, AlignTop, "Owned ESLs only");
        canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignTop, "No store use");
    }
}

void eslpwn_scene_about_on_enter(void* ctx) {
    EslPwnApp* app = ctx;
    uint32_t mode = scene_manager_get_scene_state(app->scene_manager, EslPwnSceneAbout);

    if(!app->about_view_allocated) {
        view_allocate_model(app->about_view, ViewModelTypeLockFree, sizeof(AboutViewModel));
        view_set_context(app->about_view, app);
        view_set_draw_callback(app->about_view, about_draw_cb);
        app->about_view_allocated = true;
    }

    AboutViewModel* model = view_get_model(app->about_view);
    model->mode = mode;
    model->tick = 0;
    view_commit_model(app->about_view, true);

    view_dispatcher_switch_to_view(app->view_dispatcher, EslPwnViewAbout);
}

bool eslpwn_scene_about_on_event(void* ctx, SceneManagerEvent event) {
    EslPwnApp* app = ctx;
    if(event.type == SceneManagerEventTypeTick) {
        AboutViewModel* model = view_get_model(app->about_view);
        model->tick++;
        view_commit_model(app->about_view, true);
        return true;
    }
    return false;
}

void eslpwn_scene_about_on_exit(void* ctx) {
    UNUSED(ctx);
}
