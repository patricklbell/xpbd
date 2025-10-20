#pragma once

#define R_BACKEND_OPENGL 1
#define R_BACKEND_D3D11 2

#if !defined(R_BACKEND) && OS_LINUX
    #define R_BACKEND R_BACKEND_OPENGL
#elif !defined(R_BACKEND) && OS_WINDOWS
    // @todo d3d11
    #define R_BACKEND R_BACKEND_OPENGL
#endif

#include "render_core.h"

#if R_BACKEND == R_BACKEND_OPENGL
    #include "opengl/render_opengl_inc.h"
#elif R_BACKEND == R_BACKEND_D3D11
    #error D3D11 backend is not implemented
    #include "d3d11/render_d3d11_inc.h"
#endif