// camera
void demos_camera_controls_orbit(OS_Handle window, f32 dt, DEMOS_Camera* camera) {
    vec2_f32 delta_px;
    if (input_mouse_delta(&delta_px) && input_left_mouse_held()) {
        vec2_f32 window_size = os_gfx_window_size(window);
        vec2_f32 delta_a = make_2f32(-2*PI * delta_px.x/window_size.x, -PI * delta_px.y/window_size.y);
     
        vec3_f32 d = sub_3f32(camera->eye, camera->target);
        vec3_f32 f = normalize_3f32(d);
        vec3_f32 s = normalize_3f32(cross_3f32(f, make_up_3f32()));
        vec3_f32 u = cross_3f32(s, f);
    
        // stop rotation at poles
        f32 polarity = f.y;
        const f32 north_pole = 0.9, south_pole = -0.9;
        if (polarity > north_pole && delta_a.y > 0) {
            delta_a.y = 0;
        }
        if (polarity < south_pole && delta_a.y < 0) {
            delta_a.y = 0;
        }
    
        // compute rotation
        vec4_f32 xrot = make_axis_angle_quat(delta_a.x, u);
        vec4_f32 yrot = make_axis_angle_quat(delta_a.y, s);
        d = rot_quat(d, xrot);
        d = rot_quat(d, yrot);
        
        camera->eye = add_3f32(d, camera->target);
    }

    vec2_f32 scroll_delta;
    if (input_wheel_delta(&scroll_delta)) {
        vec3_f32 d = sub_3f32(camera->eye, camera->target);
        f32 d_length = length_3f32(d);
        f32 d_zoom = d_length - scroll_delta.y*dt*DEMOS_CONTOLS_ORBIT_ZOOM;
        d = mul_3f32(d, d_zoom / d_length);
        camera->eye = add_3f32(d, camera->target);
    }
}

// rendering
R_PassParams_3D* d_begin_3d_pass_camera(OS_Handle window, DEMOS_Camera* camera) {
    vec2_f32 window_size = os_gfx_window_size(window);
    rect_f32 viewport = make_rect_f32((vec2_f32){}, window_size);
    mat4x4_f32 projection = make_perspective_4x4f32(DegreesToRad(45), window_size.x / window_size.y, 0.1, 100.f);
    mat4x4_f32 view = make_look_at_4x4f32(camera->eye, camera->target, make_up_3f32());
    return d_begin_3d_pass(viewport, view, projection);   
}