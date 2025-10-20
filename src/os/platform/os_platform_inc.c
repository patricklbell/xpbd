#include "os_platform_core.c"

#if OS_LINUX || OS_WEB
    // @todo native web api allowing async loads
    #include "linux+wasm/os_platform_linux+wasm.c"
#elif OS_WINDOWS
    #include "win32/os_platform_win32.c"
#else
    #error OS not supported.
#endif  