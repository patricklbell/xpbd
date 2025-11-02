// 
// native
// 
LRESULT CALLBACK os_gfx_win32_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    
    OS_GFX_Win32_Window* window = os_gfx_win32_hwnd_to_gfx_window(hwnd);

    switch (uMsg) {
        case WM_DESTROY:{
            PostQuitMessage(0);
        }break;
        case WM_PAINT:{
            PAINTSTRUCT ps = {0};
            BeginPaint(hwnd, &ps);

            os_gfx_win32_state->callback(os_gfx_win32_state->data);

            EndPaint(hwnd, &ps);
        }break;
        
        case WM_SIZE:{
            OS_Event* event = os_gfx_win32_add_event(OS_EventType_Resize);
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            window->size = (vec2_f32){.x = (f32)width, .y = (f32)height};
            event->window_size = window->size;
        }break;

        case WM_MOUSEMOVE:{
            OS_Event* event = os_gfx_win32_add_event(OS_EventType_MouseMove);
            event->mouse_position.x = (f32)GET_X_LPARAM(lParam);
            event->mouse_position.y = (f32)GET_Y_LPARAM(lParam);
        }break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:{
            OS_Event* event = os_gfx_win32_add_event(OS_EventType_Press);
            switch (uMsg) {
                case WM_LBUTTONDOWN: event->key = OS_Key_LeftMouseButton; break;
                case WM_MBUTTONDOWN: event->key = OS_Key_MiddleMouseButton; break;
                case WM_RBUTTONDOWN: event->key = OS_Key_RightMouseButton; break;
            }
            event->mouse_position.x = (f32)GET_X_LPARAM(lParam);
            event->mouse_position.y = (f32)GET_Y_LPARAM(lParam);
        }break;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:{
            OS_Event* event = os_gfx_win32_add_event(OS_EventType_Release);
            switch (uMsg) {
                case WM_LBUTTONUP: event->key = OS_Key_LeftMouseButton; break;
                case WM_MBUTTONUP: event->key = OS_Key_MiddleMouseButton; break;
                case WM_RBUTTONUP: event->key = OS_Key_RightMouseButton; break;
            }
            event->mouse_position.x = (f32)GET_X_LPARAM(lParam);
            event->mouse_position.y = (f32)GET_Y_LPARAM(lParam);
        }break;

        case WM_MOUSEWHEEL:{
            OS_Event* event = os_gfx_win32_add_event(OS_EventType_Wheel);
            event->mouse_position.x = (f32)GET_X_LPARAM(lParam);
            event->mouse_position.y = (f32)GET_Y_LPARAM(lParam);
            event->wheel_delta.x = 0.0f;
            event->wheel_delta.y = (f32)(GET_WHEEL_DELTA_WPARAM(wParam)) / (f32)WHEEL_DELTA;
        }break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:{
            OS_Key key;
            if (os_win32_vk_to_os_key((u32)wParam, &key)) {
                OS_Event* event = os_gfx_win32_add_event(OS_EventType_Press);
                event->key = key;
            }
        }break;

        case WM_KEYUP:
        case WM_SYSKEYUP:{
            OS_Key key;
            if (os_win32_vk_to_os_key((u32)wParam, &key)) {
                OS_Event* event = os_gfx_win32_add_event(OS_EventType_Release);
                event->key = key;
            }
        }break;

        default:{
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }break;
    }

    return result;
}

internal OS_Handle os_gfx_win32_gfx_window_to_handle(OS_GFX_Win32_Window* window) {
    OS_Handle handle = os_zero_handle();
    handle.v64[0] = (u64)window;
    return handle;
}

internal OS_GFX_Win32_Window* os_gfx_win32_handle_to_window(OS_Handle handle) {
    return (OS_GFX_Win32_Window*)handle.v64[0];
}

internal OS_GFX_Win32_Window* os_gfx_win32_hwnd_to_gfx_window(HWND hwnd) {
    for EachList(n, OS_GFX_Win32_Window, os_gfx_win32_state->windows.first) {
        if (n->hwnd == hwnd)
            return n;
    }
    return NULL;
}

internal b32 os_win32_vk_to_os_key(u32 vk, OS_Key* key) {
    if (vk >= 'A' && vk <= 'Z') {
        *key = IntToEnum(OS_Key, (int)OS_Key_a + (vk - 'A'));
        return true;
    }
    if (vk >= '0' && vk <= '9') {
        *key = IntToEnum(OS_Key, (int)OS_Key_0 + (vk - '0'));
        return true;
    }

    switch (vk) {
        case VK_LEFT:              *key = OS_Key_Left; return true;
        case VK_RIGHT:             *key = OS_Key_Right; return true;
        case VK_UP:                *key = OS_Key_Up; return true;
        case VK_DOWN:              *key = OS_Key_Down; return true;
        case VK_SPACE:             *key = OS_Key_Space; return true;
        case VK_RETURN:            *key = OS_Key_Enter; return true;
        case VK_ESCAPE:            *key = OS_Key_Escape; return true;
        case VK_BACK:              *key = OS_Key_Backspace; return true;
        case VK_TAB:               *key = OS_Key_Tab; return true;
        case VK_DELETE:            *key = OS_Key_Delete; return true;
        case VK_INSERT:            *key = OS_Key_Insert; return true;
        case VK_SHIFT:             *key = OS_Key_Shift; return true;
        case VK_CONTROL:           *key = OS_Key_Control; return true;
        case VK_MENU:              *key = OS_Key_Alt; return true;
        case VK_LWIN:
        case VK_RWIN:              *key = OS_Key_Super; return true;
        case VK_HOME:              *key = OS_Key_Home; return true;
        case VK_END:               *key = OS_Key_End; return true;
        case VK_PRIOR:             *key = OS_Key_PageUp; return true;
        case VK_NEXT:              *key = OS_Key_PageDown; return true;
        case VK_F1:                *key = OS_Key_F1; return true;
        case VK_F2:                *key = OS_Key_F2; return true;
        case VK_F3:                *key = OS_Key_F3; return true;
        case VK_F4:                *key = OS_Key_F4; return true;
        case VK_F5:                *key = OS_Key_F5; return true;
        case VK_F6:                *key = OS_Key_F6; return true;
        case VK_F7:                *key = OS_Key_F7; return true;
        case VK_F8:                *key = OS_Key_F8; return true;
        case VK_F9:                *key = OS_Key_F9; return true;
        case VK_F10:               *key = OS_Key_F10; return true;
        case VK_F11:               *key = OS_Key_F11; return true;
        case VK_F12:               *key = OS_Key_F12; return true;
        case VK_OEM_COMMA:         *key = OS_Key_Comma; return true;
        case VK_OEM_PERIOD:        *key = OS_Key_Period; return true;
        case VK_OEM_1:             *key = OS_Key_Semicolon; return true;
        case VK_OEM_7:             *key = OS_Key_Apostrophe; return true;
        case VK_OEM_3:             *key = OS_Key_Backtick; return true;
        case VK_OEM_MINUS:         *key = OS_Key_Minus; return true;
        case VK_OEM_PLUS:          *key = OS_Key_Equal; return true;
        case VK_OEM_2:             *key = OS_Key_Slash; return true;
        case VK_OEM_5:             *key = OS_Key_Backslash; return true;
        case VK_OEM_4:             *key = OS_Key_LeftBracket; return true;
        case VK_OEM_6:             *key = OS_Key_RightBracket; return true;
    }

    return false;
}

internal OS_Event* os_gfx_win32_add_event(OS_EventType type) {
    return os_gfx_add_event(os_gfx_win32_state->events_arena, &os_gfx_win32_state->events, (OS_Event){.type=type});
}

// 
// hooks
// 
internal void os_gfx_init() {
    Assert(os_gfx_win32_state == NULL);
    Arena* arena = arena_alloc();
    os_gfx_win32_state = push_array(arena, OS_GFX_Win32_State, 1);
    
    os_gfx_win32_state->arena = arena;
    os_gfx_win32_state->hInstance = GetModuleHandle(0);
    os_gfx_win32_state->events_arena = arena; // @note should be overwritten when consuming events

    {
        WNDCLASSEXA wndclass = {sizeof(WNDCLASSEXA)};
        wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wndclass.lpfnWndProc = os_gfx_win32_wnd_proc;
        wndclass.hInstance = os_gfx_win32_state->hInstance;
        wndclass.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wndclass.hIcon = LoadIcon(os_gfx_win32_state->hInstance, MAKEINTRESOURCE(1));
        wndclass.style = CS_VREDRAW|CS_HREDRAW;
        wndclass.lpszClassName = "graphical-window";
        ATOM wndatom = RegisterClassExA(&wndclass); AssertAlways(wndatom != 0);
    }
}
internal void os_gfx_cleanup() {
    for EachList(n, OS_GFX_Win32_Window, os_gfx_win32_state->windows.first) {
        DestroyWindow(n->hwnd);
        os_deallocate(n);
    }
    arena_release(os_gfx_win32_state->arena);
    os_gfx_win32_state = NULL;
}

internal OS_Events* os_gfx_consume_events(Arena* arena, b32 wait) {
    os_gfx_win32_state->events_arena = arena;
    MemoryZeroStruct(&os_gfx_win32_state->events);

    MSG msg = zero_struct;
    if (!wait || GetMessage(&msg, NULL, 0, 0) != 0) {
        b32 already_waited = wait;

        while (already_waited || PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            // @todo error handling
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
            if (msg.message == WM_QUIT) {
                os_gfx_win32_add_event(OS_EventType_Quit);
                os_gfx_win32_state->quit = true;
            }

            already_waited = false;
        }
    }

    return &os_gfx_win32_state->events;
}

internal void os_gfx_disconnect_from_rendering() {
    // @todo
}

internal OS_Handle os_gfx_handle() {
    Assert(os_gfx_win32_state != NULL);
    OS_Handle handle;
    handle.v64[0] = (u64)os_gfx_win32_state->hInstance;
    return handle;
}

internal OS_Handle os_gfx_window_open() {
    HWND hwnd = CreateWindowExA(
        WS_EX_APPWINDOW,
        "graphical-window",
        "xpbd", // @todo
        WS_OVERLAPPEDWINDOW,

        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL, NULL,
        os_gfx_win32_state->hInstance,
        NULL
    );
    if (hwnd == NULL) {
        #if BUILD_DEBUG
            DWORD err = GetLastError();
            (void)err;
        #endif
        return os_zero_handle();
    }

    OS_GFX_Win32_Window* window = (OS_GFX_Win32_Window*)os_allocate(sizeof(OS_GFX_Win32_Window));
    dllist_push_back(os_gfx_win32_state->windows.first, os_gfx_win32_state->windows.last, window);
    window->hwnd = hwnd;
    window->dc = GetDC(hwnd);
    Assert(window->dc != NULL); // @todo
    return os_gfx_win32_gfx_window_to_handle(window);
}

internal void os_gfx_window_close(OS_Handle handle) {
    OS_GFX_Win32_Window* window = os_gfx_win32_handle_to_window(handle);

    DestroyWindow(window->hwnd);
    dllist_remove(os_gfx_win32_state->windows.first, os_gfx_win32_state->windows.last, window);
    os_deallocate(window);
}
internal vec2_f32 os_gfx_window_size(OS_Handle handle) {
    OS_GFX_Win32_Window *window = os_gfx_win32_handle_to_window(handle);
    return window->size;
}

internal void os_gfx_window_show(OS_Handle os) {
    OS_GFX_Win32_Window *window = os_gfx_win32_handle_to_window(os);
    ShowWindow(window->hwnd, SW_SHOW);

    RECT rect = {0};
    GetClientRect(window->hwnd, &rect);
    window->size = (vec2_f32){.x = (f32)rect.right - (f32)rect.left, .y = (f32)rect.bottom - (f32)rect.top};
}

internal void os_gfx_start_loop(OS_LoopFunction callback, void* data) {
    os_gfx_win32_state->callback = callback;
    os_gfx_win32_state->data = data;
    while (!os_gfx_win32_state->quit) {
        callback(data);
    }
}
