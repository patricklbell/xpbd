// core
internal void input_init() {
    Arena* arena = arena_alloc();
    input_state = push_array(arena, INPUT_State, 1);
    input_state->arena = arena;
}

internal void input_update() {
    {DeferResource(Temp temp = temp_begin(input_state->arena), temp_end(temp)){
        OS_Events* events = os_gfx_consume_events(temp.arena, /*wait*/ false);
    
        // reset state
        input_state->is_mouse_moving = false;
        input_state->is_wheel_moving = false;
        input_state->mouse_delta = (vec2_f32){};
        input_state->wheel_delta = (vec2_f32){};
        MemoryZeroArray(input_state->pressed);
    
        // apply effect of each event on input state from oldest to newest
        for EachList_N(n, OS_EventNode, events->last, prev) {
            OS_Event* event = &n->v;
    
            if (event->type == OS_EventType_MouseMove) {
                // compute delta only if we have a valid previous position,
                // this avoids issues such as exiting the window
                // @todo focus events
                if (input_state->is_mouse_position_accurate) {
                    vec2_f32 event_delta = sub_2f32(event->mouse_position, input_state->mouse_position);
                    input_state->mouse_delta = add_2f32(input_state->mouse_delta, event_delta);
                }
    
                input_state->mouse_position = event->mouse_position;
                input_state->is_mouse_position_accurate = true;
                input_state->is_mouse_moving = true;
            } else if (event->type == OS_EventType_Press) {
                if (!input_state->held[event->key])
                    input_state->pressed[event->key] = true;
                input_state->held[event->key] = true;
            } else if (event->type == OS_EventType_Release) {
                input_state->held[event->key] = false;
                input_state->pressed[event->key] = false;
            } else if (event->type == OS_EventType_Wheel) {
                input_state->wheel_delta = add_2f32(input_state->wheel_delta, event->wheel_delta);
                input_state->is_wheel_moving = true;
            }
        }
    }}
}

// queries
internal vec2_f32 input_mouse_position() {
    return input_state->mouse_position;
}
internal b32 input_mouse_delta(vec2_f32* delta) {
    *delta = input_state->mouse_delta;
    return input_state->is_mouse_moving;
}

internal b32 input_wheel_delta(vec2_f32* delta) {
    *delta = input_state->wheel_delta;
    return input_state->is_wheel_moving;
}

internal b32 input_left_mouse_held() {
    return input_is_key_held(OS_Key_LeftMouseButton);
}
internal b32 input_right_mouse_held() {
    return input_is_key_held(OS_Key_RightMouseButton);
}

internal b32 input_left_mouse_pressed() {
    return input_is_key_pressed(OS_Key_LeftMouseButton);
}
internal b32 input_right_mouse_pressed() {
    return input_is_key_pressed(OS_Key_RightMouseButton);
}

internal b32 input_is_key_held(OS_Key key) {
    return input_state->held[key];
}
internal b32 input_is_key_pressed(OS_Key key) {
    return input_state->pressed[key];
}