// 
// native
//
// files
internal force_inline HANDLE os_handle_to_HANDLE(OS_Handle file) {
    return (HANDLE)file.v64[0];
}

// 
// hooks
// 
// files
internal OS_Handle os_open_readonly_file(NTString8 path) {
    DWORD access_flags = GENERIC_READ;
    DWORD share_mode = 0;
    DWORD creation_disposition = OPEN_EXISTING;
    DWORD flags_and_attributes = FILE_ATTRIBUTE_NORMAL;

    HANDLE h_file = CreateFileA((LPCSTR)path.cstr, access_flags, share_mode, /*lpSecurityAttributes*/ NULL, creation_disposition, flags_and_attributes, /*hTemplateFile*/ NULL);

    OS_Handle handle = zero_struct;
    if(h_file != INVALID_HANDLE_VALUE) {
        handle.v64[0] = (u64)h_file;
    } else {
        DWORD err = GetLastError();
        (void)err;
    }
    return handle;
}

internal void os_close_file(OS_Handle file) {
    BOOL result = CloseHandle(os_handle_to_HANDLE(file));
    (void)result;
}

internal void os_set_file_offset(OS_Handle file, u64 offset) {
    HANDLE h_file = os_handle_to_HANDLE(file);
    LARGE_INTEGER li_offset;
    li_offset.QuadPart = (LONGLONG)offset;
    SetFilePointerEx(h_file, li_offset, NULL, FILE_BEGIN);
}

internal b8 os_is_eof(OS_Handle file) {
    HANDLE h_file = os_handle_to_HANDLE(file);
    LARGE_INTEGER file_size;
    GetFileSizeEx(h_file, &file_size);
    
    LARGE_INTEGER position;
    LARGE_INTEGER li_zero = zero_struct;
    SetFilePointerEx(h_file, li_zero, &position, FILE_CURRENT);
    
    return position.QuadPart >= file_size.QuadPart;
}

internal NTString8 os_read_line_ml(OS_Handle file, Arena* arena, u64 max_line_length) {
    NTString8 result;
    result.cstr = push_array(arena, char, max_line_length + 1);
    os_read_line_to_buffer_ml(file, &result, max_line_length+1);
    return result;
}

internal void os_read_line_to_buffer_ml(OS_Handle file, NTString8* str, u64 buffer_size) {
    HANDLE h_file = os_handle_to_HANDLE(file);
    
    DWORD bytes_read = 0;
    char ch;
    str->length = 0;
    while (str->length < buffer_size) {
        BOOL success = ReadFile(h_file, &ch, 1, &bytes_read, NULL);
        if (!success || bytes_read == 0) break;
        
        if (ch == '\r') continue;
        if (ch == '\n') break;
        
        str->cstr[str->length] = ch;
        str->length++;
    }
    str->cstr[str->length] = 0;
}

// time
internal f64 os_now_seconds() {
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency); // @todo
    QueryPerformanceCounter(&counter);
    return (f64)counter.QuadPart / (f64)frequency.QuadPart;
}

// memory management
internal void* os_allocate(u64 size) {
    LPVOID ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    TracyAlloc((void*)ptr, size);
    return (void*)ptr;
}

internal void os_deallocate(void* ptr) {
    TracyFree(ptr);
    VirtualFree((LPVOID)ptr, /*dwSize*/ 0, /*dwFreeType*/ MEM_RELEASE);
}