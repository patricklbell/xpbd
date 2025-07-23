static Window os_gfx_handle_to_window(OS_Handle handle) {
    return (Window)handle.v64;
}

void os_gfx_init() {
    os_gfx_x11_state.display = XOpenDisplay(NULL);
}

void os_gfx_disconnect_from_rendering() {
    // see https://www.xfree86.org/4.7.0/DRI11.html, when dynamically loading opengl
    // XCloseDisplay calls callbacks in opengl meaning it can't be unloaded at this point,
    // this messes up the setup -> teardown order so we need to close the display before
    // cleaning up rendering
    XCloseDisplay(os_gfx_x11_state.display);
    os_gfx_x11_state.display = NULL;
}

void os_gfx_cleanup() {
    if (os_gfx_x11_state.display != NULL) {
        XCloseDisplay(os_gfx_x11_state.display);
    }
}

OS_Handle os_gfx_handle() {
    StaticAssert(sizeof(u64) >= sizeof(os_gfx_x11_state.display), os_handle_large_enough);
    return (OS_Handle) { .v64 = (u64)os_gfx_x11_state.display };
}

OS_Handle os_gfx_open_window() {
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

    // tell X11 to send a close event
    os_gfx_x11_state.atom_wm_close = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &os_gfx_x11_state.atom_wm_close, 1);

    // show window on display
    XMapWindow(display, window);

    StaticAssert(sizeof(u64) >= sizeof(Window), x11_window_handle_size_check);
    return (OS_Handle) { .v64 = (u64)window };
}

void os_gfx_close_window(OS_Handle window) {
    XDestroyWindow(os_gfx_x11_state.display, os_gfx_handle_to_window(window));
}

vec2_f32 os_gfx_window_size(OS_Handle window) {
    XWindowAttributes gwa;
    XGetWindowAttributes(os_gfx_x11_state.display, os_gfx_handle_to_window(window), &gwa);
    return (vec2_f32){.x = (f32)gwa.width, .y = (f32)gwa.height};
}

static vec2_f32 os_gfx_x11_transform_mouse(OS_Handle window, int x, int y) {
    return (vec2_f32){
        .x = (f32)x,
        .y = os_gfx_window_size(window).y - (f32)y
    };
}

static b32 os_gfx_x11_button_to_event(OS_Handle window, OS_Event* event, XButtonEvent* xbutton) {
    s32 wheel_x = 0, wheel_y = 0;
    switch (xbutton->button) {
        case Button1: event->key = OS_Key_LeftMouseButton; break;
        case Button3: event->key = OS_Key_RightMouseButton; break;
        case Button4: wheel_y = +1; break;
        case Button5: wheel_y = -1; break;
        // @todo why are these missing?
        // case Button6: wheel_x = -1; break;
        // case Button7: wheel_x = +1; break;
        default:      return 0;
    }

    // x11 generates a press & release event for every frame for scrolling,
    // convert this into wheel events with a delta @todo acceleration
    if (wheel_x || wheel_y) {
        // ignore release
        if (xbutton->type != ButtonPress) {
            return 0;
        }

        event->type         = OS_EventType_Wheel;
        event->wheel_delta  = mul_2f32(make_2f32((f32)wheel_x, (f32)wheel_y), OS_GFX_X11_WHEEL_UNIT_TO_PX);
        return 1;
    }

    event->mouse_position = os_gfx_x11_transform_mouse(window, xbutton->x, xbutton->y);

    switch (xbutton->type)
    {
        case ButtonPress: event->type = OS_EventType_Press; break;
        case ButtonRelease: event->type = OS_EventType_Release; break;
        default: InvalidPath;
    }

    return 1;
}

static b32 os_gfx_x11_client_message_to_event(OS_Handle window, OS_Event* event, XClientMessageEvent* xclient) {
    if (xclient->data.l[0] == os_gfx_x11_state.atom_wm_close) {
        event->type = OS_EventType_Quit;
        return 1;
    }

    return 0;
}

static b32 os_gfx_x11_motion_notify_to_event(OS_Handle window, OS_Event* event, XMotionEvent* xmotion) {
    event->type = OS_EventType_MouseMove;
    event->mouse_position = os_gfx_x11_transform_mouse(window, xmotion->x, xmotion->y);

    return 1;

}

OS_Events os_gfx_window_poll_events(Arena* arena, OS_Handle window) {
    OS_Events events = zero_struct;

    XEvent xevent;
    while (XPending(os_gfx_x11_state.display)) {
        XNextEvent(os_gfx_x11_state.display, &xevent);
        
        // build os event from xevent
        b32 handled = 0;
        OS_Event os_event = zero_struct;
        switch (xevent.type) {
            case ButtonPress:
            case ButtonRelease: {
                handled |= os_gfx_x11_button_to_event(window, &os_event, &xevent.xbutton);
                break;
            }
            case MotionNotify: {
                handled |= os_gfx_x11_motion_notify_to_event(window, &os_event, &xevent.xmotion);
                break;
            }
            case ClientMessage: {
                handled |= os_gfx_x11_client_message_to_event(window, &os_event, &xevent.xclient);

                if (os_event.type == OS_EventType_Quit) {
                    events.quit = 1;
                }
                break;
            }
        }

        // add our event if the xevent was handled
        if (handled) {
            os_gfx_window_add_event(arena, &events, os_event);
        }
    }

    return events;
}

void os_gfx_start_window_event_loop(OS_Handle window, OS_LoopFunction callback, void* data, OS_Events* events) {
    Arena* events_arena = arena_alloc();

    while (!events->quit) {
        arena_clear(events_arena);
        *events = os_gfx_window_poll_events(events_arena, window);
        
        (*callback)(data);
    }
}
