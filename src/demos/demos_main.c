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
#include "emcontrols/emcontrols.c"

#include "demos_helpers.c"

internal void window_event_loop(void* data);

int main() {
    ThreadCtx main_ctx;
    thread_equip(&main_ctx);

    Arena* arena = arena_alloc();
    DEMOS_CommonState* cs = push_array(arena, DEMOS_CommonState, 1);
    cs->arena = arena;

    os_gfx_init();
    r_init();
    input_init();
    emcontrols_init(cs->arena);
    phys_dbg_d_init(dbgdraw_edge_batch, dbgdraw_point_batch);
    
    // setup window
    {
        cs->window = os_gfx_window_open(); Assert(!os_is_handle_zero(cs->window));
        cs->rwindow = r_os_equip_window(cs->window);
        os_gfx_window_show(cs->window);
    }
    
    // controls
    {
        emcontrols_add((EMCONTROLS_Control){
            .type = EMCONTROLS_ControlType_Button,
            .label = ntstr8_lit("reset"),
            .data = cs,
            .on_press = &demos_on_reset_button,
        });
    }
    
    // main loop
    {
        b32 demos_err = demos_init_hook(cs); Assert(!demos_err);
        demos_world_start_wrapper(cs);
        os_gfx_start_loop(window_event_loop, cs);
        demos_world_end_wrapper(cs);
    }
    
    os_gfx_window_close(cs->window);
    
    demos_cleanup_hook(cs);
    os_gfx_disconnect_from_rendering(); r_cleanup();
    os_gfx_cleanup();
}

internal void window_event_loop(void* data) {ZoneScoped;
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;    
    f64 ntime = os_now_seconds();
    f64 dt = ntime - cs->time;
    f32 pdt = 1.f/60.f;
    cs->time = ntime;

    input_update();

    // behaviour
    if (cs->is_paused || !demos_controls_phys_drag(cs->w, cs->window, &cs->camera, /*compliance*/ 0.001f)) {
        demos_controls_camera_orbit(cs->window, dt, &cs->camera);
    }
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

    // frame
    DeferCall(r_window_begin_frame(cs->window, cs->rwindow), r_window_end_frame(cs->window, cs->rwindow)) {
        DeferCall(d_begin_pipeline(), d_submit_pipeline(cs->window, cs->rwindow)) {
            demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ false, /*back_face*/ false);
            if (!cs->dont_show_frame)
                demos_frame_hook_wrapper(cs);
                
            if (!cs->is_paused || single_step) {
                dbgdraw_clear();
                phys_world_step(cs->w, pdt);
            }
            if (cs->show_debug) {
                demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ true, /*back_face*/ false);
                dbgdraw_draw();
            }
        }
    }
}

// wrappers
internal void demos_world_start_wrapper(DEMOS_CommonState* cs) {
    cs->w = phys_make_world((PHYS_WorldSettings){});

    demos_world_start_hook(cs->w);
    
    cs->time = os_now_seconds();
}
internal void demos_world_end_wrapper(DEMOS_CommonState* cs) {
    demos_world_end_hook(cs->w);
    phys_world_cleanup(cs->w);
}
internal force_inline void demos_frame_hook_wrapper(DEMOS_CommonState* cs) {
    demos_frame_hook(cs);
}

// emcontrol callbacks
no_inline internal void demos_on_reset_button(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;
    cs->should_reset = true;
}