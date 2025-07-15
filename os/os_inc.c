#include "os_core.c"

#if OS_LINUX
    #include "linux/os_linux.c"
#else
    #error OS not supported.
#endif