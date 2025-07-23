#if OS_LINUX || OS_WEB
    #include "linux+wasm/os_platform_linux+wasm.c"
#else
    #error OS not supported.
#endif

// helpers
OS_Handle os_zero_handle() {
    return (OS_Handle) { .v64 = 0 };
}

b8 os_is_handle_zero(OS_Handle handle) {
    return handle.v64 == 0;
}

// memory management
void* os_allocate(u64 size) {
    return malloc(size);
}

void os_deallocate(void* ptr) {
    free(ptr);
}

// files
static FILE* os_handle_to_FILE(OS_Handle file) {
    return (FILE*)file.v64;
}

OS_Handle os_open_readonly_file(NTString8 path) {
    return (OS_Handle) { .v64 = (u64)fopen((char*)path.data, "r") };
}

void os_close_file(OS_Handle file) {
    fclose(os_handle_to_FILE(file));
}

void os_set_file_offset(OS_Handle file, u64 offset) {
    fseek(os_handle_to_FILE(file), offset, SEEK_SET);
}

b8 os_is_eof(OS_Handle file) {
    return feof(os_handle_to_FILE(file));
}

NTString8 os_read_line_ml(Arena* arena, OS_Handle file, u64 max_line_length) {
    NTString8 result;
    result.data = push_array(arena, u8, max_line_length);
    fgets(result.cstr, max_line_length, os_handle_to_FILE(file));
    result.length = strlen(result.cstr);
    return result;
}