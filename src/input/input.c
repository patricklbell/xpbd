// core
void input_init() {
    {
        Arena* arena = arena_alloc();
        input_state = push_array(arena, INPUT_State, 1);
        input_state->arena = arena;
    }
}

void input_update(OS_Events* events) {
    Temp scratch = scratch_begin(NULL, 0);

    // reset state
    input_state->is_mouse_moving = 0;
    input_state->is_wheel_moving = 0;
    input_state->mouse_delta = (vec2_f32){};
    input_state->wheel_delta = (vec2_f32){};

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
            input_state->is_mouse_position_accurate = 1;
            input_state->is_mouse_moving = 1;
        } else if (event->type == OS_EventType_Press) {
            input_state->held[event->key] = 1;
        } else if (event->type == OS_EventType_Release) {
            input_state->held[event->key] = 0;
        } else if (event->type == OS_EventType_Wheel) {
            input_state->wheel_delta = add_2f32(input_state->wheel_delta, event->wheel_delta);
            input_state->is_wheel_moving = 1;
        }
    }
    
    scratch_end(scratch);
}

// queries
b32 input_mouse_delta(vec2_f32* delta) {
    *delta = input_state->mouse_delta;
    return input_state->is_mouse_moving;
}

b32 input_wheel_delta(vec2_f32* delta) {
    *delta = input_state->wheel_delta;
    return input_state->is_wheel_moving;
}

b32 input_left_mouse_held() {
    return input_state->held[OS_Key_LeftMouseButton];
}

b32 input_right_mouse_held() {
    return input_state->held[OS_Key_RightMouseButton];
}