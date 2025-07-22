#pragma once

#include <emscripten.h>

typedef struct OS_GFX_WASMState OS_GFX_WASMState;
struct OS_GFX_WASMState {
    Arena* events_arenas[2];
    int active_events_arenas;
    OS_Events queued_events;
    OS_Events* submit_events;

    OS_LoopFunction callback;
};

thread_static OS_GFX_WASMState os_gfx_wasm_state;

#define OS_GFX_WASM_MAGIC_HANDLE 42189