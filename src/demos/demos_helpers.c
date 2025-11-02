// camera
internal void demos_camera_update_instrinsics(DEMOS_Camera* cam) {
    cam->projection = make_perspective_4x4f32(cam->fov, cam->aspect_ratio, cam->near_z, cam->far_z);
    cam->inv_projection = inv_4x4f32(cam->projection);
}
internal void demos_camera_update_extrinsics(DEMOS_Camera* cam) {
    cam->view = make_look_at_4x4f32(cam->eye, cam->target, make_up_3f32());
    cam->inv_view = inv_4x4f32(cam->view);
}
internal DEMOS_Camera demos_make_camera(vec2_f32 window_size, vec3_f32 eye, vec3_f32 target) {
    DEMOS_Camera cam = {
        .fov=DegreesToRad(45),
        .aspect_ratio=window_size.x / window_size.y,
        .near_z=0.1f,
        .far_z=100.f,
        .eye=eye,
        .target=target,
    };
    demos_camera_update_instrinsics(&cam);
    demos_camera_update_extrinsics(&cam);

    return cam;
}
internal void demos_camera_move(DEMOS_Camera* cam, vec3_f32 eye) {
    cam->eye = eye;
    demos_camera_update_extrinsics(cam);
}
internal void demos_camera_pan(DEMOS_Camera* cam, vec3_f32 pan) {
    cam->eye = add_3f32(cam->eye, pan);
    cam->target = add_3f32(cam->target, pan);
    demos_camera_update_extrinsics(cam);
}
internal void demos_camera_resize_window(DEMOS_Camera* cam, vec2_f32 window_size) {
    cam->aspect_ratio = window_size.x / window_size.y;
    demos_camera_update_instrinsics(cam);
}

// controls
internal void demos_controls_camera_orbit(OS_Handle window, f64 dt, DEMOS_Camera* camera) {
    vec2_f32 delta_px;
    if (input_mouse_delta(&delta_px) && (input_left_mouse_held() || input_right_mouse_held())) {
        vec2_f32 window_size = os_gfx_window_size(window);
        vec2_f32 delta = make_2f32(delta_px.x/window_size.x, delta_px.y/window_size.y);
     
        vec3_f32 d = sub_3f32(camera->eye, camera->target);
        f32 d_length = length_3f32(d);
        vec3_f32 f = mul_3f32(d, 1.f/d_length);
        vec3_f32 s = normalize_3f32(cross_3f32(f, make_up_3f32()));
        vec3_f32 u = cross_3f32(s, f);
    
        if (input_left_mouse_held()) {
            // stop rotation at poles
            f32 polarity = f.y;
            const f32 north_pole = 0.95f, south_pole = -0.95f;
            if (polarity > north_pole && -delta.y > 0) {
                delta.y = 0;
            }
            if (polarity < south_pole && -delta.y < 0) {
                delta.y = 0;
            }
        
            // compute rotation
            vec4_f32 xrot = make_angle_axis_quat(delta.x*-2.f*PI_F32, u);
            vec4_f32 yrot = make_angle_axis_quat(delta.y*PI_F32, s);
            d = rot_quat(d, xrot);
            d = rot_quat(d, yrot);
            
            demos_camera_move(camera, add_3f32(d, camera->target));
        } else if (input_right_mouse_held()) {
            vec3_f32 xpan = mul_3f32(s, d_length*delta.x);
            vec3_f32 ypan = mul_3f32(u, d_length*delta.y);
            vec3_f32 pan = add_3f32(xpan, ypan);

            demos_camera_pan(camera, pan);
        }
    }

    vec2_f32 scroll_delta;
    if (input_wheel_delta(&scroll_delta)) {
        vec3_f32 d = sub_3f32(camera->eye, camera->target);
        f32 d_length = length_3f32(d);
        f32 d_zoom = Clamp(
            d_length - DEMOS_CONTROLS_ORBIT_ZOOM_MULT*scroll_delta.y*(f32)dt*(1.f + exp_f32(DEMOS_CONTROLS_ORBIT_ZOOM_RATE*d_length)),
            EPSILON_F32,
            DEMOS_CONTROLS_ORBIT_ZOOM_MAX
        );
        d = mul_3f32(d, d_zoom / d_length);

        demos_camera_move(camera, add_3f32(d, camera->target));
    }
}

internal vec3_f32 demos_controls_get_mouse_ray(DEMOS_Camera* camera, OS_Handle window) {
    vec2_f32 window_size = os_gfx_window_size(window);
    vec2_f32 mouse_pos = input_mouse_position();
    
    mat4x4_f32 test1 = matmul_4x4f32(camera->projection, camera->inv_projection);
    mat4x4_f32 test2 = matmul_4x4f32(camera->view, camera->inv_view);

    vec2_f32 ndc = make_2f32(
        2.0f*mouse_pos.x/window_size.x - 1.0f,
        1.0f - 2.0f*mouse_pos.y/window_size.y
    );
    
    vec4_f32 mouse_clip = make_4f32(ndc.x, ndc.y, 0.5f, 1.0f);
    vec4_f32 mouse_view = mul_4x4f32(camera->inv_projection, mouse_clip);
    mouse_view = mul_4f32(mouse_view, 1.f/mouse_view.w);
    vec4_f32 mouse_world = mul_4x4f32(camera->inv_view, mouse_view);
    mouse_world = mul_4f32(mouse_world, 1.f/mouse_world.w);
    
    return normalize_3f32(sub_3f32(mouse_world.xyz, camera->eye));
}

internal b32 demos_controls_phys_drag(PHYS_World* w, OS_Handle window, DEMOS_Camera* camera, f32 compliance) {
    static DEMOS_PhysDragState* state = NULL;
    if (state == NULL) {
        Arena* arena = arena_alloc();
        state = push_array(arena, DEMOS_PhysDragState, 1);
        state->arena = arena;
    }

    // initial
    if (!state->drag_active) {
        if (input_left_mouse_pressed()) {
            vec2_f32 window_size = os_gfx_window_size(window);

            vec3_f32 o = camera->eye;
            vec3_f32 d = demos_controls_get_mouse_ray(camera, window);

            {DeferResource(Temp scratch = scratch_begin_a(state->arena), scratch_end(scratch)){
                PHYS_HitList hit_list = phys_make_hit_list(scratch.arena);
                phys_world_raycast(w, o, d, PHYS_ColliderLayer_All_No1, &hit_list);
                PHYS_HitListNode* closest_node = phys_hit_list_closest(&hit_list);

                if (closest_node != NULL) {
                    vec3_f32 hit_world = add_3f32(o, mul_3f32(d, closest_node->data.contact));
                    PHYS_Collider* hit_collider = phys_world_resolve_collider_unchecked(w, closest_node->id);
                    PHYS_Body* hit_body = phys_world_resolve_body_unchecked(w, hit_collider->base.p);

                    state->drag_active = true;
                    state->drag_plane_normal = d;
                    state->pin = phys_world_add_fixed_point(w, hit_world);
                    state->drag = phys_world_add_constraint(w, (PHYS_Constraint){
                        .type=PHYS_ConstraintType_AdvancedDistance,
                        .compliance=compliance,
                        .advanced_distance={
                            .body1=hit_collider->base.p,
                            .body2=state->pin,
                            .offset1=sub_3f32(hit_world, hit_body->position),
                        },
                    });
                }
            }}
        }
        return state->drag_active;
    }

    // release
    if (!input_left_mouse_held()) {
        phys_world_remove_constraint(w, state->drag);
        phys_world_remove_body(w, state->pin);
        state->drag_active = false;
        return state->drag_active;
    }

    // move
    vec2_f32 delta_px;
    if (input_mouse_delta(&delta_px)) {
        vec3_f32 o = camera->eye;
        vec3_f32 d = demos_controls_get_mouse_ray(camera, window);

        PHYS_Body* pin = phys_world_resolve_body_unchecked(w, state->pin);

        f32 t;
        if (phys_raycast_plane(o, d, pin->position, state->drag_plane_normal, &t)) {
            pin->position = add_3f32(o, mul_3f32(d, t));
        }
    }

    return state->drag_active;
}

// rendering
internal R_PassParams_3D* demos_d_begin_3d_pass_camera(OS_Handle window, DEMOS_Camera* camera, b32 debug, b32 back_face) {
    vec2_f32 window_size = os_gfx_window_size(window);
    vec2_f32 tl = zero_struct;
    rect_f32 viewport = make_rect_f32(tl, window_size);
    demos_camera_resize_window(camera, window_size); // @todo event

    return d_make_3d_pass(viewport, camera->view, camera->projection, /*debug*/ debug, /*back_face*/ back_face);
}