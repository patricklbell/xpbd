#pragma once

#include <sys/time.h>

#include <X11/Xlib.h>

typedef struct OS_GFX_X11State OS_GFX_X11State;
struct OS_GFX_X11State {
    Display* display;
    Atom atom_wm_close;

    b32 scroll_press_this_update;
    b32 is_scroll_pressed;
    s32 scroll_direction;
};

static OS_GFX_X11State os_gfx_x11_state = zero_struct;