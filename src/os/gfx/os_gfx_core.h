#pragma once

// graphics and windowing api
void        os_gfx_init();
void        os_gfx_disconnect_from_rendering();
void        os_gfx_cleanup();
OS_Handle   os_gfx_handle();

OS_Handle   os_gfx_open_window();
void        os_gfx_close_window(OS_Handle window);
vec2_f32    os_gfx_window_size(OS_Handle window);

// events
typedef enum OS_EventType {
    OS_EventType_Press,
    OS_EventType_Release,
    OS_EventType_MouseMove,
    OS_EventType_Wheel,
    OS_EventType_Quit,
    OS_EventType_COUNT,
} OS_EventType;

typedef enum OS_Key {
    OS_Key_LeftMouseButton,
    OS_Key_RightMouseButton,
    OS_Key_COUNT,
} OS_Key;

typedef struct OS_Event OS_Event;
struct OS_Event {
    OS_EventType type;
    OS_Key key;
    vec2_f32 mouse_position;
    vec2_f32 wheel_delta;
};

typedef struct OS_EventNode OS_EventNode;
struct OS_EventNode {
    OS_EventNode* next;
    OS_EventNode* prev;
    OS_Event v;
};

typedef struct OS_Events OS_Events;
struct OS_Events {
    OS_EventNode* first;
    OS_EventNode* last;
    u64 length;

    b32 quit;
};

// helpers
static void os_gfx_window_add_event(Arena* arena, OS_Events* list, OS_Event event);
static OS_Events os_gfx_window_poll_events(Arena* arena, OS_Handle window);

typedef void (*OS_LoopFunction)(void* data);
void os_gfx_start_window_event_loop(OS_Handle window, OS_LoopFunction callback, void* data, OS_Events* events);
