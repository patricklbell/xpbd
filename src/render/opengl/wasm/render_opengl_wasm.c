void r_ogl_os_init() {
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.alpha = true;
    attributes.depth = true;
    attributes.stencil = true;
    attributes.premultipliedAlpha = false;
    attributes.preserveDrawingBuffer = false;
    attributes.antialias = false;
    attributes.enableExtensionsByDefault = true;
    attributes.explicitSwapControl = false;
    attributes.majorVersion = 2;
    attributes.minorVersion = 0;
    // Passing a nullptr target chooses the DOM canvas element specified by
    // Module.canvas from JS.
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE handle = emscripten_webgl_create_context("#canvas", &attributes);
    Assert(handle != 0);

    EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(handle);
    Assert(res == EMSCRIPTEN_RESULT_SUCCESS);
}

void r_ogl_os_cleanup() {}

void r_ogl_os_window_swap(OS_Handle window, R_Handle rwindow) {
    Assert(window.v64 == OS_GFX_WASM_MAGIC_HANDLE);
    Assert(rwindow.v64 == OS_GFX_WASM_MAGIC_HANDLE);
}

R_Handle r_os_equip_window(OS_Handle window) {
    Assert(window.v64 == OS_GFX_WASM_MAGIC_HANDLE);
    return (R_Handle){ .v64 = OS_GFX_WASM_MAGIC_HANDLE };
}

void r_os_unequip_window(OS_Handle window, R_Handle rwindow) {
    Assert(window.v64 == OS_GFX_WASM_MAGIC_HANDLE);
    Assert(rwindow.v64 == OS_GFX_WASM_MAGIC_HANDLE);
}

void r_os_select_window(OS_Handle window, R_Handle rwindow) {
    Assert(window.v64 == OS_GFX_WASM_MAGIC_HANDLE);
    Assert(rwindow.v64 == OS_GFX_WASM_MAGIC_HANDLE);
}