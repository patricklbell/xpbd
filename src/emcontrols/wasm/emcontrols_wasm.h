#pragma once

typedef struct EMCONTROLS_WASM_ThreadCtx EMCONTROLS_WASM_ThreadCtx;
struct EMCONTROLS_WASM_ThreadCtx {
    Arena* arena;

    EMCONTROLS_Control controls[EMCONTROLS_MAX_CONTROLS];
    int count;
};

global EMCONTROLS_WASM_ThreadCtx* emcontrols_wasm_ctx = NULL;

internal void emcontrols_update(int id, EMCONTROLS_Control button);