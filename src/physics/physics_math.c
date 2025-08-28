vec3_f32 phys_scale_rotate_translate(vec3_f32 x, vec3_f32 scale, vec4_f32 rotation, vec3_f32 translation) {
    return add_3f32(
        rot_quat(elmul_3f32(x, scale), rotation),
        translation
    );
}

vec3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m) {
    vec3_f32 d2 = elmul_3f32(dimensions,dimensions);
    vec3_f32 inv_D = mul_3f32(
        make_3f32(1.f/(d2.y + d2.z), 1.f/(d2.x + d2.z), 1.f/(d2.x + d2.y)),
        12.f/m
    );
    return inv_D;
}

f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4) {
    vec3_f32 d21 = sub_3f32(v2, v1);
    vec3_f32 d31 = sub_3f32(v3, v1);
    vec3_f32 d41 = sub_3f32(v4, v1);

    return phys_tetrahedron_volume_axis(d21, d31, d41);
}

f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41) {
    return (1.f/6.f)*dot_3f32(cross_3f32(d21, d31), d41);
}

b32 phys_contact_points_spheres(
    vec3_f32 in_p1, vec3_f32 in_p2, f32 in_r1, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
) {
    f32 r = in_r1 + in_r2;
    vec3_f32 diff = sub_3f32(in_p2, in_p1);
    f32 d2 = dot_3f32(diff, diff);
    if (d2 >= r*r)
        return false;
    
    f32 d = sqrt_f32(d2);
    vec3_f32 n = mul_3f32(diff, 1.f/d);

    *out_d = r - d;
    *out_n = n;
    *out_r1 = mul_3f32(n,+in_r1);
    *out_r2 = mul_3f32(n,-in_r2);

    return true;
}

b32 phys_contact_points_plane_sphere(
    vec3_f32 in_p1, vec3_f32 in_p2, vec3_f32 in_n, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
) {
    f32 r = in_r2;
    f32 h = dot_3f32(sub_3f32(in_p2, in_p1), in_n);
    if (h >= r)
        return false;
    
    *out_d = r - h;
    *out_n = in_n;
    *out_r1 = sub_3f32(add_3f32(in_p2, mul_3f32(in_n, -h)), in_p1);
    *out_r2 = mul_3f32(in_n, -r);

    return true;
}