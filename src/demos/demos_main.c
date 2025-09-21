#include "common/common_inc.c"
#include "os/os_inc.c"
#include "hashgrid/hashgrid.c"
#include "geo/geo.c"
#include "physics/physics_inc.c"
#include "render/render_inc.c"
#include "draw/draw.c"
#include "mesh/mesh.c"
#include "input/input.c"
#include "vtk/vtk.c"
#include "dbgdraw/dbgdraw.c"
#if OS_WEB
    #include "emcontrols/emcontrols.c"
#endif

#include "demos_helpers.c"

static void window_event_loop(void* data);

int main() {
    ThreadCtx main_ctx;
    thread_equip(&main_ctx);
    Arena* main_arena = arena_alloc();

    DEMOS_CommonState* cs = push_array(main_arena, DEMOS_CommonState, 1);
    cs->arena = main_arena;

    // initialize windowing api
    os_gfx_init();

    // open a window
    cs->window = os_gfx_open_window();
    if (os_is_handle_zero(cs->window)) {
        os_gfx_cleanup();
        return 1;
    }

    // initialize rendering api
    r_init();
    
    // equip window for rendering
    cs->rwindow = r_os_equip_window(cs->window);

    input_init();
    phys_dbg_d_init(dbgdraw_edge_batch, dbgdraw_point_batch);

    #if OS_WEB
        emcontrols_init(cs->arena);
        emcontrols_add((EMCONTROLS_Control){
            .type = EMCONTROLS_ControlType_Button,
            .label = ntstr8_lit("reset"),
            .data = cs,
            .on_press = &on_demo_button,
        });
        emcontrols_add((EMCONTROLS_Control){
            .type = EMCONTROLS_ControlType_Slider,
            .label = ntstr8_lit("gravity"),
            .data = cs,
            .on_slider = &on_slider_gravity,
            .slider_value = -10,
            .slider_min = -20,
            .slider_max = +20,
        });
    #endif

    // demo hooks section
    if (!demos_init_hook(cs)) {
        demos_world_start_wrapper(cs);
        os_gfx_start_window_event_loop(cs->window, window_event_loop, cs, &cs->events);
        demos_world_end_wrapper(cs);
        demos_cleanup_hook(cs);
    }

    os_gfx_close_window(cs->window);

    os_gfx_disconnect_from_rendering();
    r_cleanup();

    os_gfx_cleanup();
}

static void window_event_loop(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;
    f64 ntime = os_now_seconds();
    f64 dt = ntime - cs->time;
    f64 pdt = 1.f/60.f;
    cs->time = ntime;

    input_update(&cs->events);
    demos_camera_controls_orbit(cs->window, dt, &cs->camera);
    if (cs->should_reset || input_is_key_pressed(OS_Key_r)) {
        demos_world_end_wrapper(cs);
        demos_world_start_wrapper(cs);
        cs->should_reset = false;
    }
    if (input_is_key_pressed(OS_Key_Space)) {
        cs->is_paused = !cs->is_paused;
    }
    b32 single_step = false;
    if (cs->is_paused && input_is_key_pressed(OS_Key_Period)) {
        single_step = true;
    }
    if (cs->is_paused && input_is_key_pressed(OS_Key_Comma)) {
        single_step = true;
        pdt *= -1.f;
    }
    if (input_is_key_pressed(OS_Key_f)) {
        cs->dont_show_frame = !cs->dont_show_frame;
    }
    if (input_is_key_pressed(OS_Key_d)) {
        cs->show_debug = !cs->show_debug;
    }

    DeferCall(r_window_begin_frame(cs->window, cs->rwindow), r_window_end_frame(cs->window, cs->rwindow)) {
        DeferCall(d_begin_pipeline(), d_submit_pipeline(cs->window, cs->rwindow)) {
            demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ false);
            if (!cs->dont_show_frame)
                demos_frame_hook(cs);
                
            if (!cs->is_paused || single_step) {
                dbgdraw_clear();
                phys_world_step(cs->w, pdt);
            }
            if (cs->show_debug) {
                demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ true);
                dbgdraw_draw();
            }
        }
    }
}

// wrappers
void demos_world_start_wrapper(DEMOS_CommonState* cs) {
    cs->w = phys_make_world((PHYS_WorldSettings){});

    demos_world_start_hook(cs->w);
    
    cs->time = os_now_seconds();
}
void demos_world_end_wrapper(DEMOS_CommonState* cs) {
    demos_world_end_hook(cs->w);
    phys_world_cleanup(cs->w);
}

// emcontrol callbacks
static void on_demo_button(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;
    cs->should_reset = true;
}

static void on_slider_gravity(f32 value, void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;
    if (cs->w != NULL) {
        cs->w->little_g = value;
    }
}