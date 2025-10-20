#pragma once

// opengl definitions
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #include <GLES3/gl32.h>
#else
    #define GLAD_GL_IMPLEMENTATION
    #include "third_party/glad/gl.h"
#endif

// opengl-es compatibility
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #define R_OGL_USES_ES 1
#else
    #define R_OGL_USES_ES 0
#endif

#include "render_opengl_core.h"

// platform specific backend
#if OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WAYLAND
    #include "egl/render_opengl_egl.h"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WASM
    #include "wasm/render_opengl_wasm.h"
#elif OS_WINDOWING_SYSTEM == OS_WINDOWING_SYSTEM_WINDOWS
    #include "wgl/render_opengl_wgl.h"
#else
    // @todo XWINDOWS -> glx
    // @todo LINUX -> detect
    #error Unsupported windowing system.
#endif