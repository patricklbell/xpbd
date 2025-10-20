#pragma once

#include <sys/time.h>

#include <X11/Xlib.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>

typedef struct OS_GFX_X11_State OS_GFX_X11_State;
struct OS_GFX_X11_State {
    Display* display;
    
    Atom atom_wm_close;
};

global OS_GFX_X11_State os_gfx_x11_state = zero_struct;

internal Window    os_gfx_x11_handle_to_window(OS_Handle handle);

internal b32       os_gfx_x11_keysym_to_os_key(KeySym k, OS_Key* key);
internal vec2_f32  os_gfx_x11_transform_mouse(OS_Handle window, int x, int y);
internal b32       os_gfx_x11_button_to_event(OS_Handle window, OS_Event* event, XButtonEvent* xbutton);
internal b32       os_gfx_x11_key_pressed_to_event(OS_Handle window, OS_Event* event, XKeyPressedEvent* xkey);
internal b32       os_gfx_x11_key_released_to_event(OS_Handle window, OS_Event* event, XKeyReleasedEvent* xkey);
internal b32       os_gfx_x11_client_message_to_event(OS_Handle window, OS_Event* event, XClientMessageEvent* xclient);
internal b32       os_gfx_x11_motion_notify_to_event(OS_Handle window, OS_Event* event, XMotionEvent* xmotion);
internal OS_Events os_gfx_x11_window_poll_events(Arena* arena, OS_Handle window);