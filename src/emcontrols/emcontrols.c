// 
// C API
// 
void emcontrols_init(Arena* arena) {
    if (arena == NULL) {
        arena = arena_alloc();
    }

    Assert(emcontrols_ctx == NULL);
    emcontrols_ctx = push_array(arena, EMCONTROLS_ThreadCtx, 1);

    emcontrols_ctx->arena = arena;
}
void emcontrols_clear() {
    emcontrols_ctx->count = 0;
    emcontrols_notify_update();
}

int emcontrols_add(EMCONTROLS_Control button) {
    Assert(emcontrols_ctx->count < EMCONTROLS_MAX_CONTROLS);

    int id = emcontrols_ctx->count;
    emcontrols_ctx->controls[id] = button;
    emcontrols_ctx->count++;
    emcontrols_notify_update();

    return id;
}
void emcontrols_update(int id, EMCONTROLS_Control button) {
    Assert(id >= 0 && id < emcontrols_ctx->count);
    emcontrols_ctx->controls[id] = button;
    emcontrols_notify_update();
}

// 
// binding code
//
static void (*emcontrols_js_update_callback)(void) = NULL;
EMSCRIPTEN_KEEPALIVE
void emcontrols_set_update_callback(void (*callback)(void)) {
    emcontrols_js_update_callback = callback;
}
static void emcontrols_notify_update(void) {
    if (emcontrols_js_update_callback) {
        (*emcontrols_js_update_callback)();
    }
}

// general
EMSCRIPTEN_KEEPALIVE
int emcontrols_get_control_count() {
    return emcontrols_ctx ? emcontrols_ctx->count : 0;
}
EMSCRIPTEN_KEEPALIVE
const char* emcontrols_get_label(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return NULL;
    return emcontrols_ctx->controls[id].label.cstr;
}

// button
EMSCRIPTEN_KEEPALIVE
int emcontrols_is_button(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return 0;
    return emcontrols_ctx->controls[id].type == EMCONTROLS_ControlType_Button;
}
EMSCRIPTEN_KEEPALIVE
void emcontrols_trigger_button_press(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return;
    
    EMCONTROLS_Control* button = &emcontrols_ctx->controls[id];
    if (button->on_press) {
        button->on_press(button->data);
    }
}

// slider
EMSCRIPTEN_KEEPALIVE
int emcontrols_is_slider(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return 0;
    return emcontrols_ctx->controls[id].type == EMCONTROLS_ControlType_Slider;
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_value(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return 0.f;
    return emcontrols_ctx->controls[id].slider_value;
}
EMSCRIPTEN_KEEPALIVE
void emcontrols_set_slider_value(int id, float value) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return;
    EMCONTROLS_Control* slider = &emcontrols_ctx->controls[id];
    slider->slider_value = value;
    if (slider->on_slider)
        slider->on_slider(slider->slider_value, slider->data);
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_max(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return 0.f;
    return emcontrols_ctx->controls[id].slider_max;
}
EMSCRIPTEN_KEEPALIVE
float emcontrols_get_slider_min(int id) {
    if (!emcontrols_ctx || id >= emcontrols_ctx->count) return 0.f;
    return emcontrols_ctx->controls[id].slider_min;
}