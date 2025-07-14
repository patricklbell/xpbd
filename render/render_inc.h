#pragma once

#define R_BACKEND_STUB   0
#define R_BACKEND_OPENGL 1

#define R_BACKEND R_BACKEND_OPENGL

#include "render_core.h"

#if R_BACKEND == R_BACKEND_OPENGL
    #include "opengl/render_opengl.h"
#endif