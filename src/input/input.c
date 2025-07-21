// core
void input_init() {
    {
        Arena* arena = arena_alloc();
        input_state = push_array(arena, INPUT_State, 1);
        input_state->arena = arena;
    }
}

void input_update(OS_Handle window) {
    Temp scratch = scratch_begin(NULL, 0);

    // reset state
    input_state->has_quit = 0;
    input_state->has_delta = 0;

    // update with events
    f64 latest_mouse_move = -1;
    vec2_f32 new_mouse_position;
    OS_EventList events = os_window_poll_events(scratch.arena, window);

    for EachList(n, OS_EventNode, events.first) {
        OS_Event* event = &n->v;

        if (event->type == OS_EventType_Quit) {
            input_state->has_quit = 1;
        } else if (event->type == OS_EventType_MouseMove && event->time.seconds > latest_mouse_move) {
            latest_mouse_move = event->time.seconds;
            new_mouse_position = event->mouse_position;
        } else if (event->type == OS_EventType_Press) {
            input_state->held[event->key] = 1;

            if (event->key = OS_Key_WheelY) {
                input_state->scroll_delta.y = event->scroll_direction;
            }
        } else if (event->type == OS_EventType_Release) {
            input_state->held[event->key] = 0;

            if (event->key = OS_Key_WheelY) {
                input_state->scroll_delta.y = 0;
            }
        }
    }
    if (latest_mouse_move > 0) {
        if (input_state->has_active_mouse) {
            input_state->delta = sub_2f32(new_mouse_position, input_state->position);
            input_state->has_delta = 1;
        }
        input_state->has_active_mouse = 1;
        input_state->position = new_mouse_position;
    }

    scratch_end(scratch);
}

// queries
b32 input_has_quit() {
    return input_state->has_quit;
}

b32 input_mouse_delta(vec2_f32* delta) {
    if (input_state->has_delta) {
        *delta = input_state->delta;
    }
    return input_state->has_delta;
}

b32 input_scroll_delta(vec2_f32* delta) {
    *delta = input_state->scroll_delta;
    return input_state->held[OS_Key_WheelY];
}

b32 input_left_mouse_held() {
    return input_state->held[OS_Key_LeftMouseButton];
}