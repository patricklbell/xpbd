#pragma once

#include <windowsx.h>
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

typedef struct OS_GFX_Win32_Window OS_GFX_Win32_Window;
struct OS_GFX_Win32_Window {
    OS_GFX_Win32_Window* next;
    OS_GFX_Win32_Window* prev;
    HWND hwnd;
    HDC dc;
    vec2_f32 size;
};

typedef struct OS_GFX_Win32_Windows OS_GFX_Win32_Windows;
struct OS_GFX_Win32_Windows {
    OS_GFX_Win32_Window* first;
    OS_GFX_Win32_Window* last;
};

typedef struct OS_GFX_Win32_State OS_GFX_Win32_State;
struct OS_GFX_Win32_State {
    Arena* arena;

    HINSTANCE hInstance;
    OS_GFX_Win32_Windows windows;

    Arena* events_arena;
    OS_Events events;

    OS_LoopFunction callback;
    void* data;
    b32 quit;
};

global OS_GFX_Win32_State* os_gfx_win32_state = NULL;

LRESULT CALLBACK os_gfx_win32_wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// helpers
internal OS_Handle            os_gfx_win32_gfx_window_to_handle(OS_GFX_Win32_Window* window);
internal OS_GFX_Win32_Window* os_gfx_win32_handle_to_window(OS_Handle handle);
internal OS_GFX_Win32_Window* os_gfx_win32_hwnd_to_gfx_window(HWND hwnd);

internal b32       os_win32_vk_to_os_key(u32 vk, OS_Key* key);
internal OS_Event* os_gfx_win32_add_event(OS_EventType type);