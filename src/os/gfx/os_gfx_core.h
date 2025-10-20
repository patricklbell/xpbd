#pragma once

// events
typedef enum OS_EventType {
    OS_EventType_Press,
    OS_EventType_Release,
    OS_EventType_MouseMove,
    OS_EventType_Wheel, // @todo acceleration/velocity
    OS_EventType_Quit,
    OS_EventType_Resize,
    OS_EventType_COUNT,
} OS_EventType;

typedef enum OS_Key {
    // mouse buttons
    OS_Key_LeftMouseButton,
    OS_Key_MiddleMouseButton,
    OS_Key_RightMouseButton,

    // alphabet
    OS_Key_a, OS_Key_b, OS_Key_c, OS_Key_d, OS_Key_e, OS_Key_f,
    OS_Key_g, OS_Key_h, OS_Key_i, OS_Key_j, OS_Key_k, OS_Key_l,
    OS_Key_m, OS_Key_n, OS_Key_o, OS_Key_p, OS_Key_q, OS_Key_r,
    OS_Key_s, OS_Key_t, OS_Key_u, OS_Key_v, OS_Key_w, OS_Key_x,
    OS_Key_y, OS_Key_z,
    
    // numbers
    OS_Key_0, OS_Key_1, OS_Key_2, OS_Key_3, OS_Key_4,
    OS_Key_5, OS_Key_6, OS_Key_7, OS_Key_8, OS_Key_9,
    
    // function
    OS_Key_F1, OS_Key_F2, OS_Key_F3, OS_Key_F4, OS_Key_F5,
    OS_Key_F6, OS_Key_F7, OS_Key_F8, OS_Key_F9, OS_Key_F10,
    OS_Key_F11, OS_Key_F12,
    
    // modifiers
    OS_Key_Shift, OS_Key_Control, OS_Key_Alt, OS_Key_Super,
    
    // navigation
    OS_Key_Up, OS_Key_Down, OS_Key_Left, OS_Key_Right,
    OS_Key_Home, OS_Key_End, OS_Key_PageUp, OS_Key_PageDown,
    
    // special
    OS_Key_Space, OS_Key_Enter, OS_Key_Escape, OS_Key_Backspace,
    OS_Key_Tab, OS_Key_Delete, OS_Key_Insert,
    
    // punctuation
    OS_Key_Comma, OS_Key_Period, OS_Key_Semicolon, OS_Key_Apostrophe,
    OS_Key_Backtick, OS_Key_Minus, OS_Key_Equal, OS_Key_Slash,
    OS_Key_Backslash, OS_Key_LeftBracket, OS_Key_RightBracket,

    OS_Key_COUNT,
} OS_Key;

typedef struct OS_Event OS_Event;
struct OS_Event { 
    OS_EventType type;
    OS_Key key;
    // OS_Handle window; @todo
    union {
        vec2_f32 mouse_position;
        vec2_f32 window_size;
    };
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
};

// graphics and windowing api
internal void        os_gfx_init();
internal void        os_gfx_disconnect_from_rendering();
internal void        os_gfx_cleanup();
internal OS_Handle   os_gfx_handle();

internal OS_Handle   os_gfx_window_open();
internal void        os_gfx_window_close(OS_Handle window);
internal vec2_f32    os_gfx_window_size(OS_Handle window);
internal void        os_gfx_window_show(OS_Handle window);

typedef void (*OS_LoopFunction)(void* data);
internal void os_gfx_start_loop(OS_LoopFunction callback, void* data);

internal OS_Events* os_gfx_consume_events(Arena* arena, b32 wait);

// helpers
internal OS_Event* os_gfx_add_event(Arena* arena, OS_Events* list, OS_Event event);
