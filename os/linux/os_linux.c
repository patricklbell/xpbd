f64 os_now_seconds() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return (f64)tval.tv_sec + (f64)tval.tv_usec / Million(1.f);
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
    attributes.event_mask = ExposureMask |
                            KeyPressMask | KeyReleaseMask | 
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
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

static OS_Event* os_window_add_event(Arena* arena, OS_EventList* list, OS_EventType type) {
    OS_EventNode* n = push_array(arena, OS_EventNode, 1);
    n->v.type = type;
    dllist_push_back(list->first, list->last, n);
    list->length++;
    return &n->v;
}

static b32 os_linux_x11_button_to_key(OS_Key* key, unsigned int button) {
    switch (button) {
        case Button1: *key = OS_Key_LeftMouseButton; break;
        case Button3: *key = OS_Key_RightMouseButton; break;
        default: return 0;
    }   

    return 1;
}

static vec2_f32 os_linux_transform_mouse(OS_Handle window, int x, int y) {
    return (vec2_f32){
        .x = (f32)x,
        .y = os_window_size(window).y - (f32)y
    };
}

OS_EventList os_window_poll_events(Arena* arena, OS_Handle window) {
    OS_EventList events = zero_struct;

    XEvent event;
    while (XPending(os_gfx_x11_state.display)) {
        XNextEvent(os_gfx_x11_state.display, &event);

        switch (event.type) {
            case ClientMessage: {
                if (event.xclient.data.l[0] == os_gfx_x11_state.atom_wm_close) {
                    os_window_add_event(arena, &events, OS_EventType_Quit);
                }
                break;
            }
            case ButtonPress:
            case ButtonRelease: {
                OS_Key key;
                if (os_linux_x11_button_to_key(&key, event.xbutton.button)) {
                    OS_Event* e = os_window_add_event(
                        arena, &events,
                        event.type == ButtonPress ? OS_EventType_Press : OS_EventType_Release
                    );
                    e->key = key;
                    e->mouse_position = os_linux_transform_mouse(window, event.xbutton.x, event.xbutton.y);
                    e->time.seconds = (f64)event.xbutton.time / Thousand(1);
                }
                break;
            }
            case MotionNotify: {
                OS_Event* e = os_window_add_event(arena, &events, OS_EventType_MouseMove);
                e->mouse_position = os_linux_transform_mouse(window, event.xmotion.x, event.xmotion.y);
                e->time.seconds = (f64)event.xmotion.time / Thousand(1);
            }
        }
    }

    return events;
}
