internal void emcontrols_notify_update(void);

// 
// C API
// 
internal void emcontrols_init(Arena* arena) {
    if (arena == NULL) {
        arena = arena_alloc();
    }

    Assert(emcontrols_wasm_ctx == NULL);
    emcontrols_wasm_ctx = push_array(arena, EMCONTROLS_WASM_ThreadCtx, 1);

    emcontrols_wasm_ctx->arena = arena;
}
internal void emcontrols_clear() {
    emcontrols_wasm_ctx->count = 0;
    emcontrols_notify_update();
}

internal int emcontrols_add(EMCONTROLS_Control button) {
    Assert(emcontrols_wasm_ctx->count < EMCONTROLS_MAX_CONTROLS);

    int id = emcontrols_wasm_ctx->count;
    emcontrols_wasm_ctx->controls[id] = button;
    emcontrols_wasm_ctx->count++;
    emcontrols_notify_update();

    return id;
}
internal void emcontrols_update(int id, EMCONTROLS_Control button) {
    Assert(id >= 0 && id < emcontrols_wasm_ctx->count);
    emcontrols_wasm_ctx->controls[id] = button;
    emcontrols_notify_update();
}

// 
// binding code
//
global void (*emcontrols_js_update_callback)(void) = NULL;

internal void emcontrols_notify_update(void) {
    if (emcontrols_js_update_callback) {
        (*emcontrols_js_update_callback)();
    }
}
EMSCRIPTEN_KEEPALIVE
void emcontrols_set_update_callback(void (*callback)(void)) {
    emcontrols_js_update_callback = callback;
}

// general
EMSCRIPTEN_KEEPALIVE
int emcontrols_get_control_count() {
    return emcontrols_wasm_ctx ? emcontrols_wasm_ctx->count : 0;
}
EMSCRIPTEN_KEEPALIVE
const char* emcontrols_get_label(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return NULL;
    return emcontrols_wasm_ctx->controls[id].label.cstr;
}

// button
EMSCRIPTEN_KEEPALIVE
int emcontrols_is_button(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return 0;
    return emcontrols_wasm_ctx->controls[id].type == EMCONTROLS_ControlType_Button;
}
EMSCRIPTEN_KEEPALIVE
void emcontrols_trigger_button_press(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return;
    
    EMCONTROLS_Control* button = &emcontrols_wasm_ctx->controls[id];
    if (button->on_press) {
        button->on_press(button->data);
    }
}

// slider
EMSCRIPTEN_KEEPALIVE
int emcontrols_is_slider(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return 0;
    return emcontrols_wasm_ctx->controls[id].type == EMCONTROLS_ControlType_Slider;
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_value(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return 0.f;
    return emcontrols_wasm_ctx->controls[id].slider_value;
}
EMSCRIPTEN_KEEPALIVE
void emcontrols_set_slider_value(int id, float value) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return;
    EMCONTROLS_Control* slider = &emcontrols_wasm_ctx->controls[id];
    slider->slider_value = value;
    if (slider->on_slider)
        slider->on_slider(slider->slider_value, slider->data);
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_max(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return 0.f;
    return emcontrols_wasm_ctx->controls[id].slider_max;
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_min(int id) {
    if (!emcontrols_wasm_ctx || id >= emcontrols_wasm_ctx->count) return 0.f;
    return emcontrols_wasm_ctx->controls[id].slider_min;
}