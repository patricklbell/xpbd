#pragma once

mat3x3_f32 phys_inv_moment_rect_cuboid(vec3_f32 dimensions, f32 m);

f32 phys_triangle_area(vec3_f32 x1, vec3_f32 x2, vec3_f32 x3);
f32 phys_triangle_area_axis(vec3_f32 d21, vec3_f32 d31);

f32 phys_tetrahedron_volume(vec3_f32 v1, vec3_f32 v2, vec3_f32 v3, vec3_f32 v4);
f32 phys_tetrahedron_volume_axis(vec3_f32 d21, vec3_f32 d31, vec3_f32 d41);

u32* phys_find_triangles_neighbors(Arena* arena, u32* indices, u32 indices_count);
