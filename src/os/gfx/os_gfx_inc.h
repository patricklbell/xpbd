#pragma once

#include "os_gfx_core.h"

#if OS_LINUX
    #include "x11/os_gfx_x11.h"
#elif OS_WEB
    #include "wasm/os_gfx_wasm.h"
#elif OS_WINDOWS
    #include "win32/os_gfx_win32.h"
#else
    #error OS not supported.
#endif