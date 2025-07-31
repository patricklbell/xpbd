mat3x3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m) {
    vec3_f32 d2 = elmul_3f32(dimensions,dimensions);
    vec3_f32 inv_D = mul_3f32(
        make_3f32(1.f/(d2.y + d2.z), 1.f/(d2.x + d2.z), 1.f/(d2.x + d2.y)),
        12.f/m
    );
    return make_scale_3x3f32(inv_D);
}

f32 phys_tet_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4) {
    vec3_f32 d21 = sub_3f32(v2, v1);
    vec3_f32 d31 = sub_3f32(v3, v1);
    vec3_f32 d41 = sub_3f32(v4, v1);

    return phys_tet_volume_axis(d21, d31, d41);
}

f32 phys_tet_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41) {
    return (1.f/6.f)*dot_3f32(cross_3f32(d21, d31), d41);
}