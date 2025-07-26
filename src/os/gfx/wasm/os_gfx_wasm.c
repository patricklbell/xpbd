#include <emscripten.h>

// helpers
static void os_gfx_wasm_add_os_event(OS_Event os_event, void *user_data) {
    OS_GFX_WASMState* s = (OS_GFX_WASMState*)user_data;

    Arena* arena = s->events_arenas[s->active_events_arena];
    os_gfx_window_add_event(arena, &s->queued_events, os_event);
}

static vec2_f32 os_gfx_wasm_transform_screen_xy(int x, int y) {
    return (vec2_f32){
        .x = (f32)(x - os_gfx_wasm_state.window_position.x),
        .y = (f32)(os_gfx_wasm_state.window_size.y - (y - os_gfx_wasm_state.window_position.y)),
    };
}

// callbacks
EMSCRIPTEN_KEEPALIVE void os_gfx_wasm_resize_callback(int x, int y, int width, int height) {
    os_gfx_wasm_state.window_position = make_2f32((f32)x,(f32)y);
    os_gfx_wasm_state.window_size = make_2f32((f32)width,(f32)height);
    printf("%f %f\n", os_gfx_wasm_state.window_size.x, os_gfx_wasm_state.window_size.y);
}

static EM_BOOL os_gfx_wasm_scroll_callback(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData) {
    OS_Event os_event = {
        .type = OS_EventType_Wheel,
        // @note assumes measured in pixels (deltaMode = DOM_DELTA_PIXEL)
        // https://developer.mozilla.org/en-US/docs/Web/API/WheelEvent/deltaMode
        .wheel_delta = make_2f32((f32)wheelEvent->deltaX, -(f32)wheelEvent->deltaY),
    };

    os_gfx_wasm_add_os_event(os_event, userData);
    return EM_TRUE;
}

static EM_BOOL os_gfx_wasm_mouse_down_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
    OS_Event os_event = {
        .type = OS_EventType_Press,
    };

    // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
    switch (mouseEvent->button) {
        case 0: os_event.key = OS_Key_LeftMouseButton; break;
        case 2: os_event.key = OS_Key_RightMouseButton; break;
        default: return EM_FALSE;
    }
    os_event.mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->screenX, mouseEvent->screenY);

    os_gfx_wasm_add_os_event(os_event, userData);
    return EM_TRUE;
}
static EM_BOOL os_gfx_wasm_mouse_move_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
    OS_Event os_event = {
        .type = OS_EventType_MouseMove,
        .mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->screenX, mouseEvent->screenY),
    };

    os_gfx_wasm_add_os_event(os_event, userData);
    return EM_TRUE;
}
static EM_BOOL os_gfx_wasm_mouse_up_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData) {
    OS_Event os_event = {
        .type = OS_EventType_Release,
    };

    // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
    switch (mouseEvent->button) {
        case 0: os_event.key = OS_Key_LeftMouseButton; break;
        case 2: os_event.key = OS_Key_RightMouseButton; break;
        default: return EM_FALSE;
    }
    os_event.mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->screenX, mouseEvent->screenY);

    os_gfx_wasm_add_os_event(os_event, userData);
    return EM_TRUE;
}

void os_gfx_init() {
    os_gfx_wasm_state = (OS_GFX_WASMState) {
        .events_arenas = { arena_alloc(), arena_alloc() },
        .active_events_arena = 0,
        .submit_events = NULL,
    };
    
    NTString8 selector = ntstr8_lit_init("#canvas");
    emscripten_set_wheel_callback       (selector.cstr, &os_gfx_wasm_state, /* useCapture */ true, os_gfx_wasm_scroll_callback);
    emscripten_set_mousedown_callback   (selector.cstr, &os_gfx_wasm_state, /* useCapture */ true, os_gfx_wasm_mouse_down_callback);
    emscripten_set_mousemove_callback   (selector.cstr, &os_gfx_wasm_state, /* useCapture */ true, os_gfx_wasm_mouse_move_callback);
    emscripten_set_mouseup_callback     (selector.cstr, &os_gfx_wasm_state, /* useCapture */ true, os_gfx_wasm_mouse_up_callback);
}

void os_gfx_disconnect_from_rendering() {}

void os_gfx_cleanup() {}

static OS_Handle os_gfx_make_magic_handle() {
    OS_Handle handle = zero_struct;
    handle.v64[0] = OS_GFX_WASM_MAGIC_HANDLE;
    return handle;
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
    Assert(window.v64[0] == OS_GFX_WASM_MAGIC_HANDLE);
}

vec2_f32 os_gfx_window_size(OS_Handle window) {
    Assert(window.v64[0] == OS_GFX_WASM_MAGIC_HANDLE);

    return os_gfx_wasm_state.window_size;
}

// @todo update api since this doesn't actually allocate events on arena
// in consideration of what most platforms do (i.e. callbacks vs polling)
OS_Events os_gfx_window_poll_events(Arena* arena, OS_Handle window) {
    OS_Events events = os_gfx_wasm_state.queued_events;

    // ping pong between an active arena (the one we are allocating)
    // and a deactivated arena (user is reading from)
    os_gfx_wasm_state.queued_events = (OS_Events){};
    os_gfx_wasm_state.active_events_arena = (os_gfx_wasm_state.active_events_arena + 1)%2;
    arena_clear(os_gfx_wasm_state.events_arenas[os_gfx_wasm_state.active_events_arena]);

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