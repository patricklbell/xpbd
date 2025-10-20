#pragma once

typedef union OS_Handle OS_Handle;
union OS_Handle {
    u64 v64[2];
    u32 v32[4];
};

// helpers
internal OS_Handle   os_zero_handle();
internal b8          os_is_handle_zero(OS_Handle handle);

// memory management
internal void* os_allocate(u64 size);
internal void  os_deallocate(void* ptr);

// files
internal OS_Handle os_open_readonly_file(NTString8 path);
internal void      os_close_file(OS_Handle file);
internal void      os_set_file_offset(OS_Handle file, u64 offset);
internal b8        os_is_eof(OS_Handle file);
internal NTString8 os_read_line_ml(OS_Handle file, Arena* arena, u64 max_line_length);
internal void      os_read_line_to_buffer_ml(OS_Handle file, NTString8* str, u64 buffer_size);

#define OS_DEFAULT_MAX_LINE_LENGTH 256
#define os_read_line(file, arena) os_read_line_ml(file, arena, OS_DEFAULT_MAX_LINE_LENGTH)
#define os_read_line_to_buffer(file, str) os_read_line_to_buffer_ml(file, str, OS_DEFAULT_MAX_LINE_LENGTH)

// time
internal f64 os_now_seconds();