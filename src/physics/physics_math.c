mat3x3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m) {
    vec3_f32 d2 = elmul_3f32(dimensions,dimensions);
    vec3_f32 D = mul_3f32(make_3f32(d2.y + d2.z, d2.x + d2.z, d2.x + d2.y), m*(1.f/12.f));
    // @todo simplify
    return inv_3x3f32(make_scale_3x3f32(D));
}