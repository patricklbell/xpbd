#include "os_gfx_core.c"

#if OS_LINUX
    #include "x11/os_gfx_x11.c"
#elif OS_WEB
    #include "wasm/os_gfx_wasm.c"
#elif OS_WINDOWS
    #include "win32/os_gfx_win32.c"
#else
    #error OS not supported.
#endif