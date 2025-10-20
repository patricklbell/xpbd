// 
// native
// 
// files
internal force_inline FILE* os_handle_to_FILE(OS_Handle file) {
    return (FILE*)file.v64[0];
}

// 
// hooks
// 
// files
internal OS_Handle os_open_readonly_file(NTString8 path) {
    OS_Handle handle = zero_struct;
    handle.v64[0] = (u64)fopen(path.cstr, "r");
    return handle;
}

internal void os_close_file(OS_Handle file) {
    fclose(os_handle_to_FILE(file));
}

internal void os_set_file_offset(OS_Handle file, u64 offset) {
    fseek(os_handle_to_FILE(file), offset, SEEK_SET);
}

internal b8 os_is_eof(OS_Handle file) {
    return feof(os_handle_to_FILE(file));
}

internal NTString8 os_read_line_ml(OS_Handle file, Arena* arena, u64 max_line_length) {
    NTString8 result;
    result.cstr = push_array(arena, char, max_line_length + 1);
    os_read_line_to_buffer_ml(file, &result, max_line_length+1);
    return result;
}

internal void os_read_line_to_buffer_ml(OS_Handle file, NTString8* str, u64 buffer_size) {
    fgets(str->cstr, buffer_size, os_handle_to_FILE(file));
    str->length = strlen(str->cstr);
}

// time
internal f64 os_now_seconds() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return (f64)tval.tv_sec + (f64)tval.tv_usec / Million(1.f);
}

// memory management
internal void* os_allocate(u64 size) {
    void* ptr = malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

internal void os_deallocate(void* ptr) {
    TracyFree(ptr);
    free(ptr);
}