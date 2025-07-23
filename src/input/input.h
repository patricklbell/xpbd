#pragma once

typedef struct INPUT_State INPUT_State;
struct INPUT_State {
    Arena* arena;
    
    b32 is_mouse_position_accurate;
    b32 is_mouse_moving;
    vec2_f32 mouse_position;
    vec2_f32 mouse_delta;

    b32 is_wheel_moving;
    vec2_f32 wheel_delta;

    b32 held[OS_Key_COUNT];
};
INPUT_State* input_state = NULL;

// core
void    input_init();
void    input_update(OS_Events* events);

// queries
b32 input_mouse_delta(vec2_f32* delta);
b32 input_wheel_delta(vec2_f32* delta);
b32 input_left_mouse_held();
b32 input_right_mouse_held();