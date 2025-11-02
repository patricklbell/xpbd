// 
// native
// 
internal void os_gfx_wasm_add_event(OS_Event os_event) {
    Arena* arena = os_gfx_wasm_state.events_arenas[os_gfx_wasm_state.active_events_arena];
    os_gfx_add_event(arena, &os_gfx_wasm_state.queued_events, os_event);
}

internal vec2_f32 os_gfx_wasm_transform_screen_xy(int x, int y) {
    return (vec2_f32){
        .x = os_gfx_wasm_state.window_pixel_ratio*x - os_gfx_wasm_state.window_position.x,
        .y = os_gfx_wasm_state.window_pixel_ratio*y - os_gfx_wasm_state.window_position.y,
    };
}

// @note this is managed on JS size where it is attached to a resize observer
C_LINKAGE EMSCRIPTEN_KEEPALIVE
void os_gfx_wasm_resize_callback(float x, float y, float width, float height, float pixel_ratio) {
    os_gfx_wasm_state.window_position = make_2f32((f32)x,(f32)y);
    os_gfx_wasm_state.window_size = make_2f32((f32)width,(f32)height);
    os_gfx_wasm_state.window_pixel_ratio = (f32)pixel_ratio;

    os_gfx_wasm_add_event((OS_Event){
        .type = OS_EventType_Resize,
        .window_size = os_gfx_wasm_state.window_size,
    });
}

internal EM_BOOL os_gfx_wasm_scroll_callback(int eventType, const EmscriptenWheelEvent *wheelEvent, void* userData) {
    os_gfx_wasm_add_event((OS_Event){
        .type = OS_EventType_Wheel,
        // @note assumes measured in pixels (deltaMode = DOM_DELTA_PIXEL)
        // https://developer.mozilla.org/en-US/docs/Web/API/WheelEvent/deltaMode
        .wheel_delta = make_2f32(sgnnum_f32(wheelEvent->deltaX), -sgnnum_f32(wheelEvent->deltaY)),
    });
    return EM_TRUE;
}

internal EM_BOOL os_gfx_wasm_mouse_down_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData) {
    OS_Event os_event = {
        .type = OS_EventType_Press,
    };

    // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
    switch (mouseEvent->button) {
        case 0: os_event.key = OS_Key_LeftMouseButton; break;
        case 2: os_event.key = OS_Key_RightMouseButton; break;
        default: return EM_FALSE;
    }
    os_event.mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->clientX, mouseEvent->clientY);

    os_gfx_wasm_add_event(os_event);
    return EM_TRUE;
}
internal EM_BOOL os_gfx_wasm_mouse_move_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData) {
    os_gfx_wasm_add_event((OS_Event){
        .type = OS_EventType_MouseMove,
        .mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->clientX, mouseEvent->clientY),
    });
    return EM_TRUE;
}
internal EM_BOOL os_gfx_wasm_mouse_up_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData) {
    OS_Event os_event = {
        .type = OS_EventType_Release,
    };

    // https://developer.mozilla.org/en-US/docs/Web/API/MouseEvent/button
    switch (mouseEvent->button) {
        case 0: os_event.key = OS_Key_LeftMouseButton; break;
        case 2: os_event.key = OS_Key_RightMouseButton; break;
        default: return EM_FALSE;
    }
    os_event.mouse_position = os_gfx_wasm_transform_screen_xy(mouseEvent->clientX, mouseEvent->clientY);

    os_gfx_wasm_add_event(os_event);
    return EM_TRUE;
}

// 
// hooks
// 
internal void os_gfx_init() {
    os_gfx_wasm_state = (OS_GFX_WASM_State) {
        .events_arenas = { arena_alloc(), arena_alloc() },
        .active_events_arena = 0,
    };
    
    NTString8 selector = ntstr8_lit_init("#canvas");
    emscripten_set_wheel_callback       (selector.cstr, NULL, /* useCapture */ true, os_gfx_wasm_scroll_callback);
    emscripten_set_mousedown_callback   (selector.cstr, NULL, /* useCapture */ true, os_gfx_wasm_mouse_down_callback);
    emscripten_set_mousemove_callback   (selector.cstr, NULL, /* useCapture */ true, os_gfx_wasm_mouse_move_callback);
    emscripten_set_mouseup_callback     (selector.cstr, NULL, /* useCapture */ true, os_gfx_wasm_mouse_up_callback);
}

internal void os_gfx_disconnect_from_rendering() {}

internal void os_gfx_cleanup() {}

internal OS_Handle os_gfx_make_magic_handle() {
    OS_Handle handle = zero_struct;
    handle.v64[0] = OS_GFX_WASM_MAGIC_HANDLE;
    return handle;
}

internal OS_Handle os_gfx_handle() {
    return os_gfx_make_magic_handle();
}

internal OS_Events* os_gfx_consume_events(Arena* arena, b32 wait) {
    // ping pong between an active arena (the one we are allocating)
    // and a deactivated arena (user is reading from)
    os_gfx_wasm_state.active_events_arena = (os_gfx_wasm_state.active_events_arena + 1)%2;
    arena_clear(os_gfx_wasm_state.events_arenas[os_gfx_wasm_state.active_events_arena]);
    
    os_gfx_wasm_state.submitted_events = os_gfx_wasm_state.queued_events;
    MemoryZeroStruct(&os_gfx_wasm_state.queued_events);
    return &os_gfx_wasm_state.submitted_events;
}

internal OS_Handle os_gfx_window_open() {
    #if BUILD_DEBUG
        static int counter = 0;
        // multiple windows doesn't make sense in wasm
        Assert(counter == 0);
        counter++;
    #endif
    return os_gfx_make_magic_handle();
}

internal void os_gfx_window_close(OS_Handle window) {
    Assert(window.v64[0] == OS_GFX_WASM_MAGIC_HANDLE);
}

internal vec2_f32 os_gfx_window_size(OS_Handle window) {
    Assert(window.v64[0] == OS_GFX_WASM_MAGIC_HANDLE);

    return os_gfx_wasm_state.window_size;
}

internal void os_gfx_window_show(OS_Handle os) {}

internal void os_gfx_start_loop(OS_LoopFunction callback, void* data) {
    emscripten_set_main_loop_arg(callback, data, 0, true);
}