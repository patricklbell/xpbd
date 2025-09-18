#pragma once

// helpers
vec3_f32 phys_rotate_translate(vec3_f32 x, vec4_f32 rotation, vec3_f32 translation);
vec3_f32 phys_scale_rotate_translate(vec3_f32 x, vec3_f32 scale, vec4_f32 rotation, vec3_f32 translation);

// moment of interia
vec3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m);

// volumes and areas
f32 phys_triangle_area(vec3_f32 x1, vec3_f32 x2, vec3_f32 x3);
f32 phys_triangle_area_axis(vec3_f32 d21, vec3_f32 d31);

f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4);
f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41);

// separating axis theorem, @note checks in_out_d to maximise depth
b32 phys_SAT_check_collision_axis(
    vec3_f32 in_axis1, f32 in_min1, f32 in_max1, f32 in_min2, f32 in_max2,
    f32* in_out_d, vec3_f32* out_n
);
void phys_SAT_polytope_min_max(
    vec3_f32 in_axis, vec3_f32* in_points, u32 in_points_count,
    f32* out_min, f32* out_max
);
void phys_SAT_sphere_min_max(
    vec3_f32 in_axis, f32 in_radius,
    f32* out_min, f32* out_max
);

// contact points
b32 phys_contact_point_spheres(
    vec3_f32 in_p1, vec3_f32 in_p2, f32 in_r1, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
b32 phys_contact_point_plane_sphere(
    vec3_f32 in_p1, vec3_f32 in_p2, vec3_f32 in_n, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
// @note out_d is relative to first triangle point
b32 phys_contact_point_triangle_sphere(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
b32 phys_contact_point_triangles(
    vec3_f32 in_p1[3], vec3_f32 in_n1, vec3_f32 in_p2[3], vec3_f32 in_n2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);