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

// helpers
OS_Handle os_zero_handle() {
    OS_Handle handle = zero_struct;
    return handle;
}

b8 os_is_handle_zero(OS_Handle handle) {
    return handle.v64[0] == 0 && handle.v64[1] == 0;
}

// memory management
void* os_allocate(u64 size) {
    return malloc(size);
}

void os_deallocate(void* ptr) {
    free(ptr);
}