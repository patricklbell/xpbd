#pragma once

#include <emscripten.h>
#include <emscripten/html5.h>

typedef struct OS_GFX_WASM_State OS_GFX_WASM_State;
struct OS_GFX_WASM_State {
    vec2_f32 window_size;
    vec2_f32 window_position;
    f32 window_pixel_ratio;

    Arena* events_arenas[2];
    int active_events_arena;
    OS_Events queued_events, submitted_events;
};

global OS_GFX_WASM_State os_gfx_wasm_state;

#define OS_GFX_WASM_MAGIC_HANDLE 42189

internal void     os_gfx_wasm_add_event(OS_Event os_event);
internal vec2_f32 os_gfx_wasm_transform_screen_xy(int x, int y);
internal EM_BOOL  os_gfx_wasm_scroll_callback(int eventType, const EmscriptenWheelEvent *wheelEvent, void* userData);
internal EM_BOOL  os_gfx_wasm_mouse_down_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData);
internal EM_BOOL  os_gfx_wasm_mouse_move_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData);
internal EM_BOOL  os_gfx_wasm_mouse_up_callback(int eventType, const EmscriptenMouseEvent *mouseEvent, void* userData);