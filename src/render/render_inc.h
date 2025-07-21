#pragma once

#define R_BACKEND_OPENGL 0

#if !defined(R_BACKEND)
    #define R_BACKEND R_BACKEND_OPENGL
#endif

#include "render_core.h"

#if R_BACKEND == R_BACKEND_OPENGL
    #include "opengl/render_opengl.h"
#endif