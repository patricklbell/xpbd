#include "common/common_inc.c"
#include "os/os_inc.c"
#include "physics/physics_inc.c"
#include "render/render_inc.c"
#include "draw/draw.c"
#include "mesh/mesh.c"
#include "input/input.c"
#include "demos_controls.c"

void window_event_loop(void* data);

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

    // demo hooks section
    if (!demos_init_hook(cs)) {
        os_gfx_start_window_event_loop(cs->window, window_event_loop, cs, &cs->events);
        demos_shutdown_hook(cs);
    }

    os_gfx_close_window(cs->window);

    os_gfx_disconnect_from_rendering();
    r_cleanup();

    os_gfx_cleanup();
}

void window_event_loop(void* data) {
    DEMOS_CommonState* cs = (DEMOS_CommonState*)data;

    input_update(&cs->events);
    demos_frame_hook(cs);
}