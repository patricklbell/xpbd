f64 os_now_seconds() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return (f64)tval.tv_sec + (f64)tval.tv_usec / Million(1);
}

void os_gfx_init() {
    os_gfx_x11_state.display = XOpenDisplay(NULL);
}

void os_gfx_cleanup() {
    XCloseDisplay(os_gfx_x11_state.display);
    os_gfx_x11_state = zero_struct;
}

OS_Handle os_gfx_handle() {
    StaticAssert(sizeof(u64) >= sizeof(os_gfx_x11_state.display), os_handle_large_enough);
    return (OS_Handle) { .v64 = (u64)os_gfx_x11_state.display };
}

static Window os_handle_to_window(OS_Handle handle) {
    return (Window)handle.v64;
}

OS_Handle os_open_window() {
    Display* display = os_gfx_x11_state.display;
    Assert(display != NULL);

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Visual *visual = DefaultVisual(display, screen);

    Colormap colormap = XCreateColormap(display, root, visual, AllocNone);

    XSetWindowAttributes attributes;
    attributes.colormap = colormap;
    attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask;
    unsigned long valuemask = CWColormap | CWEventMask; // which attributes we attach

    int x = 0, y = 0;
    unsigned int w = 800, h = 400, border_width = 0;
    int depth = DefaultDepth(display, screen);

    Window window = XCreateWindow(
        display, root,
        x, y, w, h,
        border_width,
        depth,
        InputOutput,
        visual,
        valuemask, &attributes
    );
    XFreeColormap(display, colormap);
    
    if (!window) {
        return os_zero_handle();
    }

    // Cause X11 to send a 
    os_gfx_x11_state.atom_wm_close = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &os_gfx_x11_state.atom_wm_close, 1);

    // show window on display
    XMapWindow(display, window);

    StaticAssert(sizeof(u64) >= sizeof(Window), x11_window_handle_size_check);
    return (OS_Handle) { .v64 = (u64)window };
}

void os_close_window(OS_Handle window) {
    XDestroyWindow(os_gfx_x11_state.display, os_handle_to_window(window));
}

vec2_f32 os_window_size(OS_Handle window) {
    XWindowAttributes gwa;
    XGetWindowAttributes(os_gfx_x11_state.display, os_handle_to_window(window), &gwa);
    return (vec2_f32){.x = (f32)gwa.width, .y = (f32)gwa.height};
}

b32 os_window_has_close_event(OS_Handle window) {
    XEvent event;
    while (XPending(os_gfx_x11_state.display)) {
        XNextEvent(os_gfx_x11_state.display, &event);

        if (event.type == ClientMessage && event.xclient.data.l[0] == os_gfx_x11_state.atom_wm_close) {
            return 1;
        }
    }

    return 0;
}

