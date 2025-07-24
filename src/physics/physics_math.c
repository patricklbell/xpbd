mat3x3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m) {
    vec3_f32 d2 = elmul_3f32(dimensions,dimensions);
    vec3_f32 inv_D = mul_3f32(
        make_3f32(1.f/(d2.y + d2.z), 1.f/(d2.x + d2.z), 1.f/(d2.x + d2.y)),
        12.f/m
    );
    return make_scale_3x3f32(inv_D);
}