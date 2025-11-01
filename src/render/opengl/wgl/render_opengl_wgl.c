// 
// native
// 
internal void r_ogl_wgl_set_pixel_format(HDC dc, int* pf) {
    PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd) };
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    *pf = ChoosePixelFormat(dc, &pfd); AssertAlways(*pf != 0);
    int describe = DescribePixelFormat(dc, *pf, sizeof(pfd), &pfd); AssertAlways(describe != 0);
    BOOL set_pf = SetPixelFormat(dc, *pf, &pfd);  AssertAlways(set_pf == TRUE);
}

internal void r_ogl_wgl_choose_pixel_format_arb(HDC dc, int* pf) {
    int attr[] = {
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_SUPPORT_OPENGL_ARB, 1,
        WGL_DOUBLE_BUFFER_ARB,  1,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        
        WGL_SAMPLES_ARB,        2,
        WGL_COLOR_BITS_ARB,    32,
        WGL_DEPTH_BITS_ARB,    24,
        WGL_STENCIL_BITS_ARB,   8,
        0
    };
    UINT num_formats = 0;
    wglChoosePixelFormatARB(dc, attr, 0, 1, pf, &num_formats);
}

// 
// hooks
// 
// based on glad wgl example
// https://github.com/Dav1dde/glad/blob/glad2/example/c/wgl.c
internal void r_ogl_os_init() {
    HWND dummy_hwnd = 0;
    {
        WNDCLASSEXA wndclass = { sizeof(wndclass) };
        wndclass.lpfnWndProc = DefWindowProcA;
        wndclass.hInstance = GetModuleHandle(0);
        wndclass.lpszClassName = "bootstrap-window";
        ATOM wndatom = RegisterClassExA(&wndclass); AssertAlways(wndatom != 0);
        dummy_hwnd = CreateWindowExA(
            0,
            MAKEINTATOM(wndatom),
            "",
            0, 
            
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            
            NULL, NULL,
            wndclass.hInstance,
            NULL
        );

        if (dummy_hwnd == NULL) {
            DWORD err = GetLastError();
            (void)err;
            AssertAlways(false);
        }
    }

    HDC dc = GetDC(dummy_hwnd);
    AssertAlways(dc != NULL); // @todo

    // set pixel format for device context
    int pf = 1;
    r_ogl_wgl_set_pixel_format(dc, &pf);

    // create and enable a temporary (helper) opengl context
    HGLRC dummy_ctx = wglCreateContext(dc); AssertAlways(dummy_ctx != NULL);
    wglMakeCurrent(dc, dummy_ctx);
    
    // load wgl extensions
    gladLoaderLoadWGL(dc);

    r_ogl_wgl_choose_pixel_format_arb(dc, &pf);
    
    int ctxattr[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, R_OGL_OPENGL_MAJOR_VERSION,
        WGL_CONTEXT_MINOR_VERSION_ARB, R_OGL_OPENGL_MINOR_VERSION,
        WGL_CONTEXT_FLAGS_ARB,
        WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        #if BUILD_DEBUG
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        #endif
        0
    };
    HGLRC opengl_ctx = wglCreateContextAttribsARB(dc, dummy_ctx, ctxattr); AssertAlways(opengl_ctx != NULL);
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy_ctx);
    wglMakeCurrent(dc, opengl_ctx);
    // wglSwapIntervalEXT(1);
    
    // @todo this needs to look at both opengl32 (for builtins) and wgl, manually loading avoids this
    int glad_ld = gladLoaderLoadGL(); AssertAlways(glad_ld != 0);

    r_ogl_wgl_state.dc = dc;
    r_ogl_wgl_state.hglrc = opengl_ctx;
    r_ogl_wgl_state.dummy_hwnd = dummy_hwnd;
}
internal void r_ogl_os_cleanup() {
    wglDeleteContext(r_ogl_wgl_state.hglrc);
    ReleaseDC(r_ogl_wgl_state.dummy_hwnd, r_ogl_wgl_state.dc);
    DestroyWindow(r_ogl_wgl_state.dummy_hwnd);
    MemoryZeroStruct(&r_ogl_wgl_state);
}
internal void r_ogl_os_window_swap(OS_Handle os, R_Handle r) {
    OS_GFX_Win32_Window* window = os_gfx_win32_handle_to_window(os);
    AssertAlways(window != NULL);
    SwapBuffers(window->dc);
}

// 
// OS hooks
// 
r_hook R_Handle r_os_equip_window(OS_Handle os) {
    OS_GFX_Win32_Window* window = os_gfx_win32_handle_to_window(os);
    HWND hwnd = window->hwnd;
    HDC dc = window->dc;

    wglMakeCurrent(dc, r_ogl_wgl_state.hglrc);

    int pf = 1;
    r_ogl_wgl_set_pixel_format(dc, &pf);
    r_ogl_wgl_choose_pixel_format_arb(dc, &pf);

    // make opengl convert linear colors to srgb for default framebuffer
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);

    R_Handle result = {0};
    return result;
}

r_hook void r_os_unequip_window(OS_Handle os, R_Handle r) {
}

r_hook void r_os_select_window(OS_Handle os, R_Handle r) {
    OS_GFX_Win32_Window* window = os_gfx_win32_handle_to_window(os);
    AssertAlways(window != NULL);
    wglMakeCurrent(window->dc, r_ogl_wgl_state.hglrc);
    // @todo multi-window
    glDrawBuffer(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  
}