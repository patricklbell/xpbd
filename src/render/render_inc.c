#include "render_core.c"

#if R_BACKEND == R_BACKEND_OPENGL
    #include "opengl/render_opengl_inc.c"
#elif R_BACKEND == R_BACKEND_D3D11
    #include "d3d11/render_d3d11_inc.c"
#endif