static EGLSurface r_ogl_egl_handle_to_surface(R_Handle handle) {
    return (EGLSurface)handle.v64;
}

static EGLNativeDisplayType r_ogl_egl_native_display() {
    return (EGLNativeDisplayType) os_gfx_handle().v64;
}

static EGLNativeWindowType r_ogl_egl_native_window(OS_Handle window) {
    return (EGLNativeWindowType) window.v64;
}

// @todo graceful
void r_ogl_os_init() {
    int version = gladLoaderLoadEGL(NULL);

    // connect to display and initialize
    r_ogl_egl_state.display = eglGetDisplay(r_ogl_egl_native_display());
    AssertAlways(r_ogl_egl_state.display != EGL_NO_DISPLAY);
    AssertAlways(eglInitialize(r_ogl_egl_state.display, NULL, NULL));

    // @note reload egl after initializing, not sure why this is needed
    AssertAlways(gladLoaderLoadEGL(r_ogl_egl_state.display));

    // choose openGL as our API
    AssertAlways(eglBindAPI(EGL_OPENGL_API));
    
    // choose the first matching config
    EGLint attr[] = {
        EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
        EGL_CONFORMANT,        EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

        EGL_RED_SIZE,      8,
        EGL_GREEN_SIZE,    8,
        EGL_BLUE_SIZE,     8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE,  8,
        
        EGL_NONE
    };
    EGLint num_config;
    AssertAlways(eglChooseConfig(r_ogl_egl_state.display, attr, &r_ogl_egl_state.config, 1, &num_config));
    AssertAlways(num_config == 1);

    EGLint ctxattr[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
        EGL_NONE
    };
    r_ogl_egl_state.context = eglCreateContext(r_ogl_egl_state.display, r_ogl_egl_state.config, EGL_NO_CONTEXT, ctxattr);
    AssertAlways(r_ogl_egl_state.context != EGL_NO_CONTEXT);

    // activate context without surface
    eglMakeCurrent(r_ogl_egl_state.display, 0, 0, r_ogl_egl_state.context);

    gladLoadGL((GLADloadfunc)r_ogl_os_load_procedure_address);
}

void r_ogl_os_cleanup() {
    eglDestroyContext(r_ogl_egl_state.display, r_ogl_egl_state.context);
    r_ogl_egl_state.context = NULL;

    for EachIndex(i, R_OGL_EGL_MAX_SURFACES) {
        eglDestroySurface(r_ogl_egl_state.display, r_ogl_egl_state.surfaces[i]);
        r_ogl_egl_state.surfaces[i] = NULL;
    }

    eglTerminate(r_ogl_egl_state.display);
    r_ogl_egl_state.display = NULL;

    gladLoaderUnloadEGL();
}

void r_ogl_os_window_swap(OS_Handle window, R_Handle rwindow) {
    eglSwapBuffers(r_ogl_egl_state.display, r_ogl_egl_handle_to_surface(rwindow));
}

void* r_ogl_os_load_procedure_address(char* name) {
    return (void*)eglGetProcAddress(name);
}

R_Handle r_os_equip_window(OS_Handle window) {
    EGLint surfattr[] = {
        EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
        EGL_NONE,
    };
    EGLSurface surface = eglCreateWindowSurface(r_ogl_egl_state.display, r_ogl_egl_state.config, r_ogl_egl_native_window(window), surfattr);
    Assert(surface != EGL_NO_SURFACE);

    for EachIndex(i, R_OGL_EGL_MAX_SURFACES) {
        if (r_ogl_egl_state.surfaces[i] == NULL) {
            r_ogl_egl_state.surfaces[i] = surface;
            break;
        }
    }
    r_ogl_egl_state.num_surfaces++;
    Assert(r_ogl_egl_state.num_surfaces <= R_OGL_EGL_MAX_SURFACES);

    StaticAssert(sizeof(u64) >= sizeof(EGLSurface), r_ogl_render_handle_large_enough);
    return (R_Handle){ .v64 = (u64)surface };
}

void r_os_unequip_window(OS_Handle window, R_Handle rwindow) {
    EGLSurface surface = r_ogl_egl_handle_to_surface(rwindow);
    for EachIndex(i, R_OGL_EGL_MAX_SURFACES) {
        if (r_ogl_egl_state.surfaces[i] == surface) {
            r_ogl_egl_state.surfaces[i] = NULL;
            break;
        }
    }
    r_ogl_egl_state.num_surfaces--;
    eglDestroySurface(r_ogl_egl_state.display, surface);
}

void r_os_select_window(OS_Handle window, R_Handle rwindow) {
    EGLSurface surface = r_ogl_egl_handle_to_surface(rwindow);
    eglMakeCurrent(r_ogl_egl_state.display, surface, surface, r_ogl_egl_state.context);
}