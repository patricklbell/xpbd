// camera
void demos_camera_controls_orbit(OS_Handle window, f32 dt, DEMOS_Camera* camera) {
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
            const f32 north_pole = 0.95, south_pole = -0.95;
            if (polarity > north_pole && -delta.y > 0) {
                delta.y = 0;
            }
            if (polarity < south_pole && -delta.y < 0) {
                delta.y = 0;
            }
        
            // compute rotation
            vec4_f32 xrot = make_angle_axis_quat(delta.x*-2.f*PI, u);
            vec4_f32 yrot = make_angle_axis_quat(delta.y*-1.f*PI, s);
            d = rot_quat(d, xrot);
            d = rot_quat(d, yrot);
            
            camera->eye = add_3f32(d, camera->target);
        } else if (input_right_mouse_held()) {
            vec3_f32 xpan = mul_3f32(s, d_length*delta.x);
            vec3_f32 ypan = mul_3f32(u,-d_length*delta.y);
            vec3_f32 pan = add_3f32(xpan, ypan);

            camera->eye = add_3f32(camera->eye, pan);
            camera->target = add_3f32(camera->target, pan);
        }
    }

    vec2_f32 scroll_delta;
    if (input_wheel_delta(&scroll_delta)) {
        vec3_f32 d = sub_3f32(camera->eye, camera->target);
        f32 d_length = length_3f32(d);
        f32 d_zoom = Clamp(
            d_length - DEMOS_CONTOLS_ORBIT_ZOOM_MULT*scroll_delta.y*dt*(1.f + exp_f32(DEMOS_CONTOLS_ORBIT_ZOOM_RATE*d_length)),
            EPSILON_F32,
            DEMOS_CONTOLS_ORBIT_ZOOM_MAX
        );
        d = mul_3f32(d, d_zoom / d_length);
        camera->eye = add_3f32(d, camera->target);
    }
}

// rendering
R_PassParams_3D* demos_d_begin_3d_pass_camera(OS_Handle window, DEMOS_Camera* camera, b32 debug) {
    vec2_f32 window_size = os_gfx_window_size(window);
    rect_f32 viewport = make_rect_f32((vec2_f32){}, window_size);
    mat4x4_f32 projection = make_perspective_4x4f32(DegreesToRad(45), window_size.x / window_size.y, 0.1, 100.f);
    mat4x4_f32 view = make_look_at_4x4f32(camera->eye, camera->target, make_up_3f32());
    return d_make_3d_pass(viewport, view, projection, debug);   
}