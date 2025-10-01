#pragma once

// helpers
internal vec3_f32 phys_rotate_translate(vec3_f32 x, vec4_f32 rotation, vec3_f32 translation);
internal vec3_f32 phys_inv_rotate_translate(vec3_f32 x, vec4_f32 rotation, vec3_f32 translation);
internal vec3_f32 phys_scale_rotate_translate(vec3_f32 x, vec3_f32 scale, vec4_f32 rotation, vec3_f32 translation);
internal vec3_f32 phys_polygon_normal_ccw(GEO_Polygon* f);

// moment of interia
internal vec3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m);
internal vec3_f32 phys_inv_moment_spehere(f32 r, f32 m);

// volumes and areas
internal f32 phys_triangle_area(vec3_f32 x1, vec3_f32 x2, vec3_f32 x3);
internal f32 phys_triangle_area_axis(vec3_f32 d21, vec3_f32 d31);

internal f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4);
internal f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41);

// separating axis theorem, @note checks in_out_d to maximise depth
typedef enum PHYS_SATCollisionForm {
    PHYS_SATCollisionForm_None = 0,
    PHYS_SATCollisionForm_NotCloser,
    PHYS_SATCollisionForm_MaxMin,
    PHYS_SATCollisionForm_MinMax,
} PHYS_SATCollisionForm;

internal PHYS_SATCollisionForm phys_SAT_check_collision_axis(
    vec3_f32 in_axis1,
    f32 in_min1, f32 in_max1, f32 in_min2, f32 in_max2,
    f32* in_out_d, vec3_f32* out_n
);
internal void phys_SAT_polytope_min_max(
    vec3_f32 in_axis, vec3_f32* in_points, u32 in_points_count,
    f32* out_min, f32* out_max
);
internal void phys_SAT_polytope_min_max_with_aligned_face(
    vec3_f32 in_axis, vec3_f32* in_points, u32* in_indices, u32 in_indices_count, vec3_f32* in_normals,  GEO_Topology in_topology,
    f32* out_min, f32* out_max, u32* out_min_face, u32* out_max_face
);
internal void phys_SAT_sphere_min_max(
    vec3_f32 in_axis, f32 in_radius,
    f32* out_min, f32* out_max
);

// contact points
internal b32 phys_contact_point_spheres(
    vec3_f32 in_p1, vec3_f32 in_p2, f32 in_r1, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
internal b32 phys_contact_point_plane_sphere(
    vec3_f32 in_p1, vec3_f32 in_p2, vec3_f32 in_n, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
// @note out_d is relative to first triangle point
internal b32 phys_contact_point_triangle_sphere(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
internal b32 phys_contact_point_triangles(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2[3], vec3_f32 in_n2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);

// raycast
internal b32 phys_raycast_sphere(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_sphere_center, f32 in_sphere_radius,
    f32* out_contact
);
internal b32 phys_raycast_plane(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_plane_origin, vec3_f32 in_plane_normal,
    f32* out_contact
);
internal b32 phys_raycast_triangle(
    vec3_f32 in_origin, vec3_f32 in_direction,
    vec3_f32 in_tri_a, vec3_f32 in_tri_b, vec3_f32 in_tri_c, // @note ccw
    f32* out_contact
);