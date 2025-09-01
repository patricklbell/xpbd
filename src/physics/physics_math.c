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

b32 phys_contact_points_triangle_sphere(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
) {
    f32 r = in_r2;

    vec3_f32 d11 = sub_3f32(in_p2, in_p1[0]);
    f32 d_n = dot_3f32(d11, in_n1);
    if (d_n >= r)
        return false;

    // @note assumes ccw
    vec3_f32 u = sub_3f32(in_p1[1], in_p1[0]);
    f32 d_u = dot_3f32(d11, u);
    if (d_u < 0.f || d_u > 1.f)
        return false;
    
    vec3_f32 v = sub_3f32(in_p1[2], in_p1[0]);
    f32 d_v = dot_3f32(d11, v);
    if (d_v < 0.f || d_v > 1.f)
        return false;
    
    *out_d = length_3f32(d11);
    *out_n = in_n1;
    *out_r1 = sub_3f32(d11, mul_3f32(in_n1, -d_n));
    *out_r2 = mul_3f32(in_n1, -r);

    return true;
}

// @todo efficient implementation
b32 phys_contact_points_triangles(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2[3], vec3_f32 in_n2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
) {
    mat3x3_f32 m;
    m.c1 = cross_3f32(in_n1, in_n2);
    if (dot_3f32(m.c1,m.c1) == 0.f) // parallel
        return false;
    m.c2 = cross_3f32(m.c1, in_n1);
    m.c3 = cross_3f32(m.c1, in_n2);

    // L: f(l) = p1 + (t + l) m.c1 + s m.c2
    vec3_f32 d11 = sub_3f32(in_p1[0], in_p2[0]);
    vec3_f32 tsv = mul_3x3f32(inv_3x3f32(m), d11);
    
    // project line onto uv space of triangle 1
    vec3_f32 o = add_3f32(mul_3f32(m.c1, tsv.x), mul_3f32(m.c2, tsv.y));

    vec3_f32 u = sub_3f32(in_p1[1], in_p1[0]);
    vec3_f32 v = sub_3f32(in_p1[2], in_p1[0]);
    f32 o_u = dot_3f32(o, u);
    f32 o_v = dot_3f32(o, v);
    f32 d_u = dot_3f32(m.c1, u);
    f32 d_v = dot_3f32(m.c1, v);

    int i = 0;
    vec2_f32 uv_coords[4];

    // (u,0), (u,1)
    if (d_u != 0.f) {   
        f32 v_0_t = (0.f - o_u) / d_u;
        f32 v_1_t = (1.f - o_u) / d_u;

        f32 u_v_0 = o_v + v_0_t*d_v;
        f32 u_v_1 = o_v + v_1_t*d_v;

        if (u_v_0 >= 0.f && u_v_0 <= 1.f)
            uv_coords[i++] = make_2f32(u_v_0, 0.f);
        if (u_v_0 >= 0.f && u_v_0 <= 1.f)
            uv_coords[i++] = make_2f32(u_v_1, 1.f);
    }
    // (0,v), (1,v)
    if (d_v != 0.f) {   
        f32 u_0_t = (0.f - o_v) / d_v;
        f32 u_1_t = (1.f - o_v) / d_v;

        f32 v_u_0 = o_u + u_0_t*d_u;
        f32 v_u_1 = o_u + u_1_t*d_u;

        if (v_u_0 >= 0.f && v_u_0 <= 1.f)
            uv_coords[i++] = make_2f32(0.f, v_u_0);
        if (v_u_1 >= 0.f && v_u_1 <= 1.f)
            uv_coords[i++] = make_2f32(1.f, v_u_1);
    }

    if (i < 2)
        return false;
    Assert(i == 2);

    vec3_f32 i1 = add_3f32(mul_3f32(u, uv_coords[0].x), mul_3f32(v, uv_coords[0].y));
    vec3_f32 i2 = add_3f32(mul_3f32(u, uv_coords[1].x), mul_3f32(v, uv_coords[1].y));

    // define intersection at midpoint of segment for each triangle
    vec3_f32 m_1 = mul_3f32(add_3f32(i1, i2), 0.5f);
    vec3_f32 m_2 = add_3f32(m_1, d11);

    f32 d1 = Min(Min(
        length2_3f32(sub_3f32(m_1, in_p1[0])),
        length2_3f32(sub_3f32(m_1, in_p1[1]))),
        length2_3f32(sub_3f32(m_1, in_p1[2]))
    );
    f32 d2 = Min(Min(
        length2_3f32(sub_3f32(m_2, in_p2[0])),
        length2_3f32(sub_3f32(m_2, in_p2[1]))),
        length2_3f32(sub_3f32(m_2, in_p2[2]))
    );

    // use normal of triangle with greatest penetration
    if (d1 > d2) {
        *out_d = sqrt_f32(d1);
        *out_n = in_n1;
    } else {
        *out_d = sqrt_f32(d2);
        *out_n = in_n2;
    }
    *out_r1 = m_1;
    *out_r2 = m_2;

    return true;
}