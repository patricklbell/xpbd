// 
// native
// 
internal OS_Handle os_gfx_x11_gfx_window_to_handle(OS_GFX_X11_Window* window) {
    OS_Handle handle = os_zero_handle();
    handle.v64[0] = (u64)window;
    return handle;
}

internal OS_GFX_X11_Window* os_gfx_x11_handle_to_gfx_window(OS_Handle handle) {
    return (OS_GFX_X11_Window*)handle.v64[0];
}

internal OS_GFX_X11_Window* os_gfx_x11_window_to_gfx_window(Window wndw) {
  for EachList(n, OS_GFX_X11_Window, os_gfx_x11_state->windows.first) {
        if (n->wndw == wndw)
            return n;
    }
    return NULL;
}

internal vec2_f32 os_gfx_x11_transform_mouse(OS_GFX_X11_Window* window, int x, int y) {
    return (vec2_f32){
        .x = (f32)x,
        .y = (f32)y
    };
}

internal b32 os_gfx_x11_button_to_event(OS_Event* event, XButtonEvent* xbutton) {
    s32 wheel_x = 0, wheel_y = 0;
    switch (xbutton->button) {
        case Button1: event->key = OS_Key_LeftMouseButton; break;
        case Button2: event->key = OS_Key_MiddleMouseButton; break;
        case Button3: event->key = OS_Key_RightMouseButton; break;
        case Button4: wheel_y = +1; break;
        case Button5: wheel_y = -1; break;
        // @todo why are these missing?
        // case Button6: wheel_x = -1; break;
        // case Button7: wheel_x = +1; break;
        default:      return false;
    }

    // x11 generates a press & release event for every frame for scrolling,
    // convert this into wheel events with a delta @todo acceleration
    if (wheel_x || wheel_y) {
        // ignore release
        if (xbutton->type != ButtonPress) {
            return false;
        }

        event->type         = OS_EventType_Wheel;
        event->wheel_delta  = (vec2_f32){.x = (f32)wheel_x, .y = (f32)wheel_y };
        return true;
    }

    OS_GFX_X11_Window* window = os_gfx_x11_window_to_gfx_window(xbutton->window);
    event->mouse_position = os_gfx_x11_transform_mouse(window, xbutton->x, xbutton->y);

    switch (xbutton->type)
    {
        case ButtonPress: event->type = OS_EventType_Press; break;
        case ButtonRelease: event->type = OS_EventType_Release; break;
        default: InvalidPath;
    }

    return true;
}

internal b32 os_gfx_x11_key_pressed_to_event(OS_Event* event, XKeyPressedEvent* xkey) {
    event->type = OS_EventType_Press;
    
    KeySym keysym = XLookupKeysym(xkey, 0);
    return os_gfx_x11_keysym_to_os_key(keysym, &event->key);
}

internal b32 os_gfx_x11_key_released_to_event(OS_Event* event, XKeyReleasedEvent* xkey) {
    event->type = OS_EventType_Release;
    
    KeySym keysym = XLookupKeysym(xkey, 0);
    return os_gfx_x11_keysym_to_os_key(keysym, &event->key);
}

internal b32 os_gfx_x11_client_message_to_event(OS_Event* event, XClientMessageEvent* xclient) {
    if (xclient->data.l[0] == os_gfx_x11_state->atom_wm_close) {
        event->type = OS_EventType_Quit;
        return true;
    }

    return false;
}

internal b32 os_gfx_x11_motion_notify_to_event(OS_Event* event, XMotionEvent* xmotion) {
    event->type = OS_EventType_MouseMove;

    OS_GFX_X11_Window* window = os_gfx_x11_window_to_gfx_window(xmotion->window);
    event->mouse_position = os_gfx_x11_transform_mouse(window, xmotion->x, xmotion->y);

    return true;
}

internal b32 os_gfx_x11_configure_notify_to_event(OS_Event* event, XConfigureEvent* xconfigure) {
    event->type = OS_EventType_Resize;
    
    OS_GFX_X11_Window* window = os_gfx_x11_window_to_gfx_window(xconfigure->window);
    event->window_size = (vec2_f32){.x = (f32)xconfigure->width, .y = (f32)xconfigure->height};
    if (event->window_size.x != window->size.x || event->window_size.y != window->size.y) {
        window->size = event->window_size;
        return true;
    }
    return false;
}

internal b32 os_gfx_x11_keysym_to_os_key(KeySym k, OS_Key* key) {
    if (k >= XK_a && k <= XK_z) {
        *key = (OS_Key)(OS_Key_a + (k - XK_a));
        return true;
    } else if (k >= XK_A && k <= XK_Z) {
        *key = (OS_Key)(OS_Key_a + (k - XK_A));
        return true;
    }

    if (k >= XK_0 && k <= XK_9) {
        *key = (OS_Key)(OS_Key_0 + (k - XK_0));
        return true;
    }

    if (k >= XK_F1 && k <= XK_F12) {
        *key = (OS_Key)(OS_Key_F1 + (k - XK_F1));
        return true;
    }

    switch (k) {
        case XK_Shift_L:
        case XK_Shift_R:      *key = OS_Key_Shift; return true;
        case XK_Control_L:
        case XK_Control_R:    *key = OS_Key_Control; return true;
        case XK_Alt_L:
        case XK_Alt_R:
        case XK_Meta_L:
        case XK_Meta_R:       *key = OS_Key_Alt; return true;
        case XK_Super_L:
        case XK_Super_R:
        case XK_Hyper_L:
        case XK_Hyper_R:      *key = OS_Key_Super; return true;
    
        case XK_Up:           *key = OS_Key_Up; return true;
        case XK_Down:         *key = OS_Key_Down; return true;
        case XK_Left:         *key = OS_Key_Left; return true;
        case XK_Right:        *key = OS_Key_Right; return true;
        case XK_Home:         *key = OS_Key_Home; return true;
        case XK_End:          *key = OS_Key_End; return true;
        case XK_Page_Up:      *key = OS_Key_PageUp; return true;
        case XK_Page_Down:    *key = OS_Key_PageDown; return true;
    
        case XK_space:        *key = OS_Key_Space; return true;
        case XK_Return:       *key = OS_Key_Enter; return true;
        case XK_Escape:       *key = OS_Key_Escape; return true;
        case XK_BackSpace:    *key = OS_Key_Backspace; return true;
        case XK_Tab:          *key = OS_Key_Tab; return true;
        case XK_Delete:       *key = OS_Key_Delete; return true;
        case XK_Insert:       *key = OS_Key_Insert; return true;
    
        case XK_comma:        *key = OS_Key_Comma; return true;
        case XK_period:       *key = OS_Key_Period; return true;
        case XK_semicolon:    *key = OS_Key_Semicolon; return true;
        case XK_apostrophe:   *key = OS_Key_Apostrophe; return true;
        case XK_grave:        *key = OS_Key_Backtick; return true;
        case XK_minus:        *key = OS_Key_Minus; return true;
        case XK_equal:        *key = OS_Key_Equal; return true;
        case XK_slash:        *key = OS_Key_Slash; return true;
        case XK_backslash:    *key = OS_Key_Backslash; return true;
        case XK_bracketleft:  *key = OS_Key_LeftBracket; return true;
        case XK_bracketright: *key = OS_Key_RightBracket; return true;
    }

    return false;
}

internal OS_Events* os_gfx_consume_events(Arena* arena, b32 wait) {
    MemoryZeroStruct(&os_gfx_x11_state->events);

    XEvent xevent;
    b32 can_block = wait;
    while (can_block || XPending(os_gfx_x11_state->display)) {
        XNextEvent(os_gfx_x11_state->display, &xevent);
        can_block = false;

        // build os event from xevent
        b32 handled = false;
        OS_Event os_event = zero_struct;
        switch (xevent.type) {
            case ButtonPress:
            case ButtonRelease: {
                handled |= os_gfx_x11_button_to_event(&os_event, &xevent.xbutton);
            }break;
            case KeyPress:{
                handled |= os_gfx_x11_key_pressed_to_event(&os_event, &xevent.xkey);
            }break;
            case KeyRelease:{
                handled |= os_gfx_x11_key_released_to_event(&os_event, &xevent.xkey);
            }break;
            case MotionNotify: {
                handled |= os_gfx_x11_motion_notify_to_event(&os_event, &xevent.xmotion);
            }break;
            case ConfigureNotify: {
                handled |= os_gfx_x11_configure_notify_to_event(&os_event, &xevent.xconfigure);
            }break;
            case ClientMessage: {
                handled |= os_gfx_x11_client_message_to_event(&os_event, &xevent.xclient);

                if (os_event.type == OS_EventType_Quit) {
                    os_gfx_x11_state->quit = true;
                }
            }break;
        }

        // add our event if the xevent was handled
        if (handled) {
            os_gfx_add_event(arena, &os_gfx_x11_state->events, os_event);
        }
    }

    return &os_gfx_x11_state->events;
}

// 
// hooks
// 
internal void os_gfx_init() {
    Assert(os_gfx_x11_state == NULL);
    Arena* arena = arena_alloc();
    os_gfx_x11_state = push_array(arena, OS_GFX_X11_State, 1);
    os_gfx_x11_state->arena = arena;
    os_gfx_x11_state->display = XOpenDisplay(NULL);
}

internal void os_gfx_disconnect_from_rendering() {
    // see https://www.xfree86.org/4.7.0/DRI11.html, when dynamically loading opengl
    // XCloseDisplay calls callbacks in opengl meaning it can't be unloaded at this point,
    // this messes up the setup -> teardown order so we need to close the display before
    // cleaning up rendering
    XCloseDisplay(os_gfx_x11_state->display);
    os_gfx_x11_state->display = NULL;
}

internal void os_gfx_cleanup() {
    if (os_gfx_x11_state->display != NULL) {
        XCloseDisplay(os_gfx_x11_state->display);
    }
    for EachList(n, OS_GFX_X11_Window, os_gfx_x11_state->windows.first) {
        os_deallocate(n);
    }
    arena_release(os_gfx_x11_state->arena);
    os_gfx_x11_state = NULL;
}

internal OS_Handle os_gfx_handle() {
    StaticAssert(sizeof(u64) >= sizeof(os_gfx_x11_state->display), os_handle_large_enough);
    OS_Handle handle = zero_struct;
    handle.v64[0] = (u64)os_gfx_x11_state->display;
    return handle;
}

internal OS_Handle os_gfx_window_open() {
    Display* display = os_gfx_x11_state->display;
    Assert(display != NULL);

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Visual *visual = DefaultVisual(display, screen);

    Colormap colormap = XCreateColormap(display, root, visual, AllocNone);

    XSetWindowAttributes attributes;
    attributes.colormap = colormap;
    attributes.event_mask = ExposureMask |
                            KeyPressMask | KeyReleaseMask | 
                            ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                            StructureNotifyMask;
    unsigned long valuemask = CWColormap | CWEventMask; // which attributes we attach

    // @todo
    int x = 0, y = 0;
    unsigned int w = 800, h = 400, border_width = 0;
    int depth = DefaultDepth(display, screen);

    Window wndw = XCreateWindow(
        display, root,
        x, y, w, h,
        border_width,
        depth,
        InputOutput,
        visual,
        valuemask, &attributes
    );
    XFreeColormap(display, colormap);
    
    if (!wndw) {
        return os_zero_handle();
    }
    
    // tell X11 to send a close event
    os_gfx_x11_state->atom_wm_close = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, wndw, &os_gfx_x11_state->atom_wm_close, 1);

    OS_GFX_X11_Window* window = (OS_GFX_X11_Window*)os_allocate(sizeof(OS_GFX_X11_Window));
    dllist_push_back(os_gfx_x11_state->windows.first, os_gfx_x11_state->windows.last, window);
    window->wndw = wndw;
    return os_gfx_x11_gfx_window_to_handle(window);
}

internal void os_gfx_window_close(OS_Handle os) {
    OS_GFX_X11_Window* window = os_gfx_x11_handle_to_gfx_window(os);
    Assert(window != NULL);

    XDestroyWindow(os_gfx_x11_state->display, window->wndw);
    dllist_remove(os_gfx_x11_state->windows.first, os_gfx_x11_state->windows.last, window);
    os_deallocate(window);
}

internal vec2_f32 os_gfx_window_size(OS_Handle os) {
    OS_GFX_X11_Window* window = os_gfx_x11_handle_to_gfx_window(os);
    Assert(window != NULL);

    return window->size;
}

internal void os_gfx_window_show(OS_Handle os) {
    OS_GFX_X11_Window *window = os_gfx_x11_handle_to_gfx_window(os);
    XMapWindow(os_gfx_x11_state->display, window->wndw);

    XWindowAttributes gwa;
    XGetWindowAttributes(os_gfx_x11_state->display, window->wndw, &gwa);
    window->size = (vec2_f32){.x = (f32)gwa.width, .y = (f32)gwa.height};
}

internal void os_gfx_start_loop(OS_LoopFunction callback, void* data) {
    while (!os_gfx_x11_state->quit) {
        callback(data);
    }
}

