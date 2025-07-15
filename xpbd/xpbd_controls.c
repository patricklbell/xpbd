void xpbd_controls_orbit_camera(OS_Handle window, vec3_f32* eye, vec3_f32* target) {
    vec2_f32 delta_px;
    if (!input_mouse_delta(&delta_px) || !input_left_mouse_held()) {
        return;
    }

    vec2_f32 window_size = os_window_size(window);
    vec2_f32 delta_a = make_2f32(-2*PI * delta_px.x/window_size.x, -PI * delta_px.y/window_size.y);
 
    vec3_f32 d = sub_3f32(*eye, *target);
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
    vec4_f32 xrot = make_quat(delta_a.x, u);
    vec4_f32 yrot = make_quat(delta_a.y, s);
    d = rot_quat(d, xrot);
    d = rot_quat(d, yrot);
    
    *eye = add_3f32(d, *target);
}