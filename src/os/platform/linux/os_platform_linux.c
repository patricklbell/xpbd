// files
static FILE* os_handle_to_FILE(OS_Handle file) {
    return (FILE*)file.v64[0];
}

OS_Handle os_open_readonly_file(NTString8 path) {
    OS_Handle handle = zero_struct;
    handle.v64[0] = (u64)fopen(path.cstr, "r");
    return handle;
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
    result.data = push_array(arena, u8, max_line_length + 1);
    fgets(result.cstr, max_line_length + 1, os_handle_to_FILE(file));
    result.length = strlen(result.cstr);
    return result;
}