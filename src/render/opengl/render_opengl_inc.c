#include "render_opengl_core.c"

// platform specific backend
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WAYLAND
    #include "egl/render_opengl_egl.c"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #include "wasm/render_opengl_wasm.c"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WINDOWS
    #include "wgl/render_opengl_wgl.c"
#else
    // @todo XWINDOWS -> glx
    // @todo LINUX -> detect
    #error Unsupported windowing system.
#endif