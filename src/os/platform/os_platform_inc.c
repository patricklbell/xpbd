#include "os_platform_core.c"

#if OS_LINUX
    #include "linux+wasm/os_platform_linux+wasm.c"
    #include "linux/os_platform_linux.c"
#elif OS_WEB
    #include "linux+wasm/os_platform_linux+wasm.c"
    // @todo, loading files async is unreliable, embed for now
    // #include "wasm/os_platform_wasm.c"
    #include "linux/os_platform_linux.c"
#else
    #error OS not supported.
#endif