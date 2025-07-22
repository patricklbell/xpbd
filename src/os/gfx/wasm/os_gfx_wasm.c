void os_gfx_init() {
    os_gfx_wasm_state = (OS_GFX_WASMState) {
        .events_arenas = { arena_alloc(), arena_alloc() },
        .active_events_arenas = 0,
        .submit_events = NULL,
    };
    
    // @todo attach event listeners
}

void os_gfx_cleanup() {}

static OS_Handle os_gfx_make_magic_handle() {
    return (OS_Handle) { .v64 = OS_GFX_WASM_MAGIC_HANDLE };
}

OS_Handle os_gfx_handle() {
    return os_gfx_make_magic_handle();
}

OS_Handle os_gfx_open_window() {
    #if BUILD_DEBUG
        static int counter = 0;
        // multiple windows doesn't make sense in wasm
        Assert(counter == 0);
    #endif
    return os_gfx_make_magic_handle();
}

void os_gfx_close_window(OS_Handle window) {
    Assert(window.v64 == OS_GFX_WASM_MAGIC_HANDLE);
}

vec2_f32 os_gfx_window_size(OS_Handle window) {
    // @todo
    return (vec2_f32){.x = (f32)300, .y = (f32)150};
}

// @todo update api since this doesn't actually allocate events on arena
// in consideration of what most platforms do (i.e. callbacks vs polling)
OS_Events os_gfx_window_poll_events(Arena* arena, OS_Handle window) {
    OS_Events events = os_gfx_wasm_state.queued_events;

    // ping pong between an active arena (the one we are allocating)
    // and a deactivated arena (user is reading from)
    os_gfx_wasm_state.queued_events = (OS_Events){};
    os_gfx_wasm_state.active_events_arenas = (os_gfx_wasm_state.active_events_arenas + 1)%2;
    arena_clear(os_gfx_wasm_state.events_arenas[os_gfx_wasm_state.active_events_arenas]);

    return events;
}

static void os_gfx_window_event_loop_callback_wrapper(void* data) {
    *os_gfx_wasm_state.submit_events = os_gfx_window_poll_events(NULL, os_gfx_make_magic_handle());

    (*os_gfx_wasm_state.callback)(data);
}

void os_gfx_start_window_event_loop(OS_Handle window, OS_LoopFunction callback, void* data, OS_Events* events) {
    os_gfx_wasm_state.callback = callback;
    os_gfx_wasm_state.submit_events = events;
    *os_gfx_wasm_state.submit_events = os_gfx_wasm_state.queued_events;

    emscripten_set_main_loop_arg(os_gfx_window_event_loop_callback_wrapper, data, 0, true);
}