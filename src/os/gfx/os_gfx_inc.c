#include "os_gfx_core.c"

#if OS_LINUX
    #include "x11/os_gfx_x11.c"
#elif OS_WEB
    #include "wasm/os_gfx_wasm.c"
#else
    #error OS not supported.
#endif