#pragma once

#define GLAD_WGL_IMPLEMENTATION
#include "third_party/glad/wgl.h"
#pragma comment(lib, "opengl32")

typedef struct R_OGL_WGL_State R_OGL_WGL_State;
struct R_OGL_WGL_State {
    HDC dc;
    HGLRC hglrc;
    HWND dummy_hwnd;
};
global R_OGL_WGL_State r_ogl_wgl_state = zero_struct;

internal void* r_ogl_wgl_procedure_address(char* name);

internal void r_ogl_wgl_set_pixel_format(HDC dc, int* pf);
internal void r_ogl_wgl_choose_pixel_format_arb(HDC dc, int* pf);