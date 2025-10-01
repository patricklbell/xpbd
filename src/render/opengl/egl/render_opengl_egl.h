#pragma once

#define GLAD_EGL_IMPLEMENTATION
#include "third_party/glad/egl.h"

// @todo list
#define R_OGL_EGL_MAX_SURFACES 2

typedef struct R_OGL_OS_State R_OGL_OS_State;
struct R_OGL_OS_State {
    EGLDisplay display;
    EGLContext context;
    EGLConfig config;

    EGLSurface surfaces[R_OGL_EGL_MAX_SURFACES];
    u64 active_surfaces_count;
};

global R_OGL_OS_State r_ogl_egl_state = zero_struct;

internal EGLSurface           r_ogl_egl_handle_to_surface(R_Handle handle);
internal EGLNativeDisplayType r_ogl_egl_native_display();
internal EGLNativeWindowType  r_ogl_egl_native_window(OS_Handle window);
internal void*                r_ogl_egl_procedure_address(char* name);
internal EGLSurface           r_ogl_egl_handle_to_surface(R_Handle handle);
internal EGLNativeDisplayType r_ogl_egl_native_display();
internal EGLNativeWindowType  r_ogl_egl_native_window(OS_Handle window);