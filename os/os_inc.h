#pragma once

#include "os_core.h"

#if OS_LINUX
    #include "linux/os_linux.h"
#else
    #error OS not supported.
#endif