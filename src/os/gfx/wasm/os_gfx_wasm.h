#pragma once

typedef struct OS_GFX_WASMState OS_GFX_WASMState;
struct OS_GFX_WASMState {
    vec2_f32 window_size;
    vec2_f32 window_position;

    Arena* events_arenas[2];
    int active_events_arena;
    OS_Events queued_events;
    OS_Events* submit_events;

    OS_LoopFunction callback;
};

thread_static OS_GFX_WASMState os_gfx_wasm_state;

#define OS_GFX_WASM_MAGIC_HANDLE 42189