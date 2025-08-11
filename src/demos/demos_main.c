#include "common/common_inc.c"
#include "os/os_inc.c"
#include "hashgrid/hashgrid.c"
#include "physics/physics_inc.c"
#include "render/render_inc.c"
#include "draw/draw.c"
#include "mesh/mesh.c"
#include "input/input.c"
#include "vtk/vtk.c"
#include "dbgdraw/dbgdraw.c"
#include "geo/geo.c"
#if OS_WEB
    #include "emcontrols/emcontrols.c"
#endif

#include "demos_helpers.c"

static void window_event_loop(void* data);
static void reset_demo_callback(void* data);

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

    #if OS_WEB
        emcontrols_init(cs->arena);
        emcontrols_add((EMCONTROLS_Control){
            .type = EMCONTROLS_ControlType_Button,
            .label = ntstr8_lit("reset"),
            .on_press = &reset_demo_callback,
            .data = cs,
        });
    #endif

    // demo hooks section
    if (!demos_persistent_init_hook(cs)) {
        demos_state_init_hook();
        os_gfx_start_window_event_loop(cs->window, window_event_loop, cs, &cs->events);
        demos_state_cleanup_hook();
        demos_persistent_cleanup_hook(cs);
    }

    os_gfx_close_window(cs->window);

    os_gfx_disconnect_from_rendering();
    r_cleanup();

    os_gfx_cleanup();
}

static void window_event_loop(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;

    if (cs->should_reset || input_is_key_pressed(OS_Key_r)) {
        demos_state_cleanup_hook();
        demos_state_init_hook();
        cs->should_reset = false;
    }

    demos_frame_hook(cs);
}

static void reset_demo_callback(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;
    cs->should_reset = true;
}