#pragma once

vec3_f32 phys_scale_rotate_translate(vec3_f32 x, vec3_f32 scale, vec4_f32 rotation, vec3_f32 translation);

vec3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m);

// @todo
f32 phys_triangle_area(vec3_f32 x1, vec3_f32 x2, vec3_f32 x3);
f32 phys_triangle_area_axis(vec3_f32 d21, vec3_f32 d31);

f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4);
f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41);

// @note should return normal at 1st object's contact point
b32 phys_contact_points_spheres(
    vec3_f32 in_p1, vec3_f32 in_p2, f32 in_r1, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);
b32 phys_contact_points_plane_sphere(
    vec3_f32 in_p1, vec3_f32 in_p2, vec3_f32 in_n, f32 in_r2,
    f32* out_d, vec3_f32* out_r1, vec3_f32* out_r2, vec3_f32* out_n
);