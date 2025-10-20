#pragma once

#include "os_platform_core.h"

#if OS_LINUX || OS_WEB
    #include "linux+wasm/os_platform_linux+wasm.h"
#elif OS_WINDOWS
    #include "win32/os_platform_win32.h"
#else
    #error OS not supported.
#endif