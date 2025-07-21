#pragma once

typedef struct INPUT_State INPUT_State;
struct INPUT_State {
    Arena* arena;
    
    b32 has_quit;
    
    b32 has_active_mouse;
    b32 has_delta;
    vec2_f32 position;
    vec2_f32 delta;

    vec2_f32 scroll_delta;

    b32 held[OS_Key_COUNT];
};
INPUT_State* input_state = NULL;

// core
void    input_init();
void    input_update(OS_Handle window);

// queries
b32 input_has_quit();
b32 input_mouse_delta(vec2_f32* delta);
b32 input_scroll_delta(vec2_f32* delta);
b32 input_left_mouse_held();