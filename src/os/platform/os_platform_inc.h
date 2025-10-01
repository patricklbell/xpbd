#pragma once

#include "os_platform_core.h"

#if OS_LINUX
    #include "linux+wasm/os_platform_linux+wasm.h"
    #include "linux/os_platform_linux.h"
#elif OS_WEB
    #include "linux+wasm/os_platform_linux+wasm.h"
    #include "wasm/os_platform_wasm.h"
#else
    #error OS not supported.
#endif