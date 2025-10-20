#pragma once

#include <sys/time.h>

#include <X11/Xlib.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>


typedef struct OS_GFX_X11_Window OS_GFX_X11_Window;
struct OS_GFX_X11_Window {
    OS_GFX_X11_Window* next;
    OS_GFX_X11_Window* prev;
    Window wndw;
    vec2_f32 size;
};

typedef struct OS_GFX_X11_Windows OS_GFX_X11_Windows;
struct OS_GFX_X11_Windows {
    OS_GFX_X11_Window* first;
    OS_GFX_X11_Window* last;
};

typedef struct OS_GFX_X11_State OS_GFX_X11_State;
struct OS_GFX_X11_State {
    Arena* arena;

    Display* display;
    Atom atom_wm_close;
    OS_GFX_X11_Windows windows;

    b32 quit;
    OS_Events events;
};

global OS_GFX_X11_State* os_gfx_x11_state = NULL;

internal OS_Handle          os_gfx_x11_gfx_window_to_handle(OS_GFX_X11_Window* window);
internal OS_GFX_X11_Window* os_gfx_x11_handle_to_gfx_window(OS_Handle handle);
internal OS_GFX_X11_Window* os_gfx_x11_window_to_gfx_window(Window wndw);

// helpers
internal b32 os_gfx_x11_keysym_to_os_key(KeySym k, OS_Key* key);
internal b32 os_gfx_x11_button_to_event(OS_Event* event, XButtonEvent* xbutton);
internal b32 os_gfx_x11_key_pressed_to_event(OS_Event* event, XKeyPressedEvent* xkey);
internal b32 os_gfx_x11_key_released_to_event(OS_Event* event, XKeyReleasedEvent* xkey);
internal b32 os_gfx_x11_motion_notify_to_event(OS_Event* event, XMotionEvent* xmotion);
internal b32 os_gfx_x11_configure_notify_to_event(OS_Event* event, XConfigureEvent* xconfigure);
internal b32 os_gfx_x11_client_message_to_event(OS_Event* event, XClientMessageEvent* xclient);

internal vec2_f32  os_gfx_x11_transform_mouse(OS_GFX_X11_Window* window, int x, int y);