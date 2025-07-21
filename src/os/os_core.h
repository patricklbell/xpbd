#pragma once

typedef union OS_Handle OS_Handle;
union OS_Handle {
    u64 v64;
};

// helpers
OS_Handle   os_zero_handle();
b8          os_is_handle_zero(OS_Handle handle);

// memory management
void* os_allocate(u64 size);
void  os_deallocate(void* ptr);

// files
OS_Handle   os_open_readonly_file(NTString8 path);
void        os_close_file(OS_Handle file);
void        os_set_file_offset(OS_Handle file, u64 offset);
b8          os_is_eof(OS_Handle file);
NTString8   os_read_line_ml(Arena* arena, OS_Handle file, u64 max_line_length);

#define DEFAULT_MAX_LINE_LENGTH 256
#define os_read_line(arena, file) os_read_line_ml(arena, file, DEFAULT_MAX_LINE_LENGTH)

// graphics and windowing api
void        os_gfx_init();
void        os_gfx_cleanup();
OS_Handle   os_gfx_handle();

OS_Handle   os_open_window();
void        os_close_window(OS_Handle window);
vec2_f32    os_window_size(OS_Handle window);

// events
typedef enum OS_EventType {
    OS_EventType_Press,
    OS_EventType_Release,
    OS_EventType_MouseMove,
    OS_EventType_Quit,
    OS_EventType_COUNT,
} OS_EventType;

typedef enum OS_Key {
    OS_Key_LeftMouseButton,
    OS_Key_RightMouseButton,
    OS_Key_WheelY,
    OS_Key_COUNT,
} OS_Key;

typedef struct OS_Event OS_Event;
struct OS_Event {
    OS_EventType type;
    OS_Key key;
    struct {
        f64 seconds;
    } time;
    vec2_f32 mouse_position;
    s32 scroll_direction;
};

typedef struct OS_EventNode OS_EventNode;
struct OS_EventNode {
    OS_EventNode* next;
    OS_EventNode* prev;
    OS_Event v;
};

typedef struct OS_EventList OS_EventList;
struct OS_EventList {
    OS_EventNode* first;
    OS_EventNode* last;
    u64 length;
};

OS_EventList os_window_poll_events(Arena* arena, OS_Handle window);

// time
f64 os_now_seconds();