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
    b32 pressed[OS_Key_COUNT];
};
global INPUT_State* input_state = NULL;

// core
internal void input_init();
internal void input_update(OS_Events* events);

// queries
internal vec2_f32 input_mouse_position();
internal b32 input_mouse_delta(vec2_f32* delta);
internal b32 input_wheel_delta(vec2_f32* delta);
internal b32 input_left_mouse_held();
internal b32 input_right_mouse_held();
internal b32 input_left_mouse_pressed();
internal b32 input_right_mouse_pressed();

internal b32 input_is_key_held(OS_Key key);
internal b32 input_is_key_pressed(OS_Key key);