// helpers
vec3_f32 phys_rotate_translate(vec3_f32 x, vec4_f32 rotation, vec3_f32 translation) {
    return add_3f32(
        rot_quat(x, rotation),
        translation
    );
}
vec3_f32 phys_inv_rotate_translate(vec3_f32 x, vec4_f32 rotation, vec3_f32 translation) {
    return rot_quat(
        sub_3f32(x, translation),
        inv_quat(rotation)
    );
}
vec3_f32 phys_scale_rotate_translate(vec3_f32 x, vec3_f32 scale, vec4_f32 rotation, vec3_f32 translation) {
    return phys_rotate_translate(elmul_3f32(x, scale), rotation, translation);
}
vec3_f32 phys_polygon_normal_ccw(GEO_Polygon* f) {
    Assert(f->topology >= GEO_Topology_Triangle);
    return cross_3f32(sub_3f32(f->data[1], f->data[0]), sub_3f32(f->data[2], f->data[0]));
}

// moment of interia
vec3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m) {
    vec3_f32 d2 = elmul_3f32(dimensions,dimensions);
    vec3_f32 inv_D = mul_3f32(
        make_3f32(1.f/(d2.y + d2.z), 1.f/(d2.x + d2.z), 1.f/(d2.x + d2.y)),
        12.f/m
    );
    return inv_D;
}
vec3_f32 phys_inv_moment_spehere(f32 r, f32 m) {
    return mul_3f32(make_3f32(1.f,1.f,1.f), (5.f/2.f) / (m*r*r));
}

// volumes and areas
f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4) {
    vec3_f32 d21 = sub_3f32(v2, v1);
    vec3_f32 d31 = sub_3f32(v3, v1);
    vec3_f32 d41 = sub_3f32(v4, v1);

    return phys_tetrahedron_volume_axis(d21, d31, d41);
}

f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41) {
    return (1.f/6.f)*dot_3f32(cross_3f32(d21, d31), d41);
}

// separating axis theorem
PHYS_SATCollisionForm phys_SAT_check_collision_axis(
    vec3_f32 in_axis1, f32 in_min1, f32 in_max1, f32 in_min2, f32 in_max2,
    f32* in_out_d, vec3_f32* out_n
) {
    // Overlap Test
    //         +-------------+
    //   +-----|-----+   2   |
    //   |  1  |     |       |
    //   |     +-----|-------+
    //   +-----------+
    //   A ------C---B ----- D
    //
    // IF A < C AND B > C (Overlap in order object 1 -> object 2)
    // IF C < A AND D > A (Overlap in order object 2 -> object 1)
    f32 A = in_min1;
    f32 B = in_max1;
    f32 C = in_min2;
    f32 D = in_max2;

    if (A <= C && B >= C) {
        f32 d = Min(B - C, D - A);
        if (d < *in_out_d) {
            *in_out_d = d;
            *out_n = mul_3f32(in_axis1, (B - C < D - A) ? +1.f : -1.f);
            return (B - C < D - A) ? PHYS_SATCollisionForm_MaxMin : PHYS_SATCollisionForm_MinMax;
        }

        return PHYS_SATCollisionForm_NotCloser;
    }
    if (C <= A && D >= A) {
        f32 d = Min(D - A, B - C);
        if (d < *in_out_d) {
            *in_out_d = d;
            *out_n = mul_3f32(in_axis1, (D - A < B - C) ? -1.f : +1.f);
            return (D - A < B - C) ? PHYS_SATCollisionForm_MinMax : PHYS_SATCollisionForm_MaxMin;
        }

        return PHYS_SATCollisionForm_NotCloser;
    }

    return PHYS_SATCollisionForm_None;
}
void phys_SAT_polytope_min_max(
    vec3_f32 in_axis, vec3_f32* in_points, u32 in_points_count,
    f32* out_min, f32* out_max
) {
    for EachIndex(i, in_points_count) {
        f32 proj = dot_3f32(in_axis, in_points[i]);

        *out_min = Min(*out_min, proj);
        *out_max = Max(*out_max, proj);
    }
}
void phys_SAT_polytope_min_max_with_aligned_face(
    vec3_f32 in_axis, vec3_f32* in_points, u32* in_indices, u32 in_indices_count, vec3_f32* in_normals,  GEO_Topology in_topology,
    f32* out_min, f32* out_max, u32* out_min_face, u32* out_max_face
) {
    *out_min = MAX_F32;
    *out_max = -MAX_F32;
    // @note alignments of face containing min and max, not the min and max alignments
    f32 min_face_alignment = -MAX_F32, max_face_alignment = -MAX_F32;
    for (u32 facei = 0; facei < in_indices_count/in_topology; facei++) {
        b32 face_contains_min = false, face_contains_max = false;
        
        f32 face_min = *out_min, face_max = *out_max;
        for (u32 indexi = facei*in_topology; indexi < (facei+1)*in_topology; indexi++) {
            f32 proj = dot_3f32(in_axis, in_points[in_indices[indexi]]);

            if (proj <= face_min) {
                face_min = proj;
                face_contains_min = true;
            }
            if (proj >= face_max) {
                face_max = proj;
                face_contains_max = true;
            }
        }
        if (!face_contains_min && !face_contains_max)
            continue;

        f32 face_alignment = abs_f32(dot_3f32(in_normals[facei], in_axis));

        // replace the face if we have a better vertex or better alignment
        if (face_contains_min && (face_min < (*out_min) || face_alignment > min_face_alignment)) {
            *out_min_face = facei*in_topology;
            min_face_alignment = face_alignment;
        }
        if (face_contains_max && (face_max > (*out_max) || face_alignment > max_face_alignment)) {
            *out_max_face = facei*in_topology;
            max_face_alignment = face_alignment;
        }

        *out_min = face_min;
        *out_max = face_max;
    }
}
void phys_SAT_sphere_min_max(
    vec3_f32 in_axis, f32 in_radius,
    f32* out_min, f32* out_max
) {
    *out_min = -in_radius;
    *out_max = +in_radius;
}

// contact points
b32 phys_contact_point_spheres(
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

b32 phys_contact_point_plane_sphere(
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

b32 phys_contact_point_triangle_sphere(
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
b32 phys_contact_point_triangles(
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

// raycast
b32 phys_raycast_sphere(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_sphere_center, f32 in_sphere_radius,
    f32* out_contact
) {
    // R: x = o + t*d
    // S: |x - s_o|^2 <= s_r^2
    // 
    // (o + t*d - s_o)(o + t*d - s_o)^T <= s_r^2
    // t^2 |d|^2 + 2*t*d . (o - s_o) + |o - s_o|^2 - s_r^2 <= 0
    // real sln when D >= 0
    vec3_f32 diff = sub_3f32(in_origin, in_sphere_center);

    f32 a = length2_3f32(in_direction);
    f32 b = 2*dot_3f32(in_direction, diff);
    f32 c = length2_3f32(diff) - in_sphere_radius*in_sphere_radius;

    f32 D = b*b - 4*a*c;
    if (D <= 0)
        return false;

    f32 cnst = -b/(2.f*a);
    f32 pm = sqrt_f32(D)/(2.f*a);

    // behind origin
    if (cnst + pm < 0 && cnst - pm < 0)
        return false; 
    
    // closest contact
    *out_contact = (cnst - pm < 0) ? (cnst + pm) : (cnst - pm);
    return true;
}
b32 phys_raycast_plane(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_plane_origin, vec3_f32 in_plane_normal,
    f32* out_contact
) {
    // R: x = o + t*d
    // P: (x - p_o) . p_n = 0
    // 
    // t*(d.p_n) + (o - p_o).p_n = 0
    // t = (p_o - o) . p_n / (d.p_n)

    f32 d_dot_pn = dot_3f32(in_direction, in_plane_normal);
    // parallel
    if (d_dot_pn == 0.f)
        return false;

    *out_contact = dot_3f32(sub_3f32(in_plane_origin, in_origin), in_plane_normal)/d_dot_pn;
    return true;
}
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
b32 phys_raycast_triangle(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_tri_a, vec3_f32 in_tri_b, vec3_f32 in_tri_c,
    f32* out_contact
) {
    vec3_f32 edge1 = sub_3f32(in_tri_b, in_tri_a);
    vec3_f32 edge2 = sub_3f32(in_tri_c, in_tri_a);
    vec3_f32 ray_cross_e2 = cross_3f32(in_direction, edge2);
    f32 det = dot_3f32(edge1, ray_cross_e2);

    if (det == 0.f)
        return false;

    f32 inv_det = 1.f / det;
    vec3_f32 s = sub_3f32(in_origin, in_tri_a);
    f32 u = inv_det * dot_3f32(s, ray_cross_e2);

    if ((u < 0 && abs_f32(u) > EPSILON_F32) || (u > 1 && abs_f32(u-1) > EPSILON_F32))
        return false;

    vec3_f32 s_cross_e1 = cross_3f32(s, edge1);
    f32 v = inv_det * dot_3f32(in_direction, s_cross_e1);

    if ((v < 0 && abs_f32(v) > EPSILON_F32) || (u + v > 1 && abs_f32(u + v - 1) > EPSILON_F32))
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    f32 t = inv_det * dot_3f32(edge2, s_cross_e1);

    if (t > EPSILON_F32) // ray intersection
    {
        *out_contact = t;
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}