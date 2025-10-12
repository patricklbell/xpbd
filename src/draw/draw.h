#pragma once

typedef struct D_ThreadCtx D_ThreadCtx;
struct D_ThreadCtx
{
    Arena *arena;
    R_PassList passes;
};

thread_static D_ThreadCtx *d_thread_ctx = NULL;

internal void d_begin_pipeline();
internal void d_submit_pipeline(OS_Handle window, R_Handle rwindow);

internal R_PassParams_3D* d_make_3d_pass(rect_f32 viewport, mat4x4_f32 view, mat4x4_f32 projection, b32 debug, b32 back_face);

internal void* d_3d(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, R_Mesh3DMaterial material, void* instance, u64 instance_size, b32 debug, b32 back_face);

// wrappers
internal R_Mesh3DInstance*    d_lambertian_mesh_with_face(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, mat4x4_f32 transform, vec3_f32 color, b32 back_face);
internal R_PBRMesh3DInstance* d_pbr_mesh_with_face(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, mat4x4_f32 transform, vec3_f32 albedo, f32 roughness, vec3_f32 specular, b32 back_face);
#define                       d_lambertian_mesh(vertices, flags, indices, topology, transform, color)                   d_lambertian_mesh_with_face(vertices, flags, indices, topology, transform, color, 0)
#define                       d_lambertian_mesh_bf(vertices, flags, indices, topology, transform, color)                d_lambertian_mesh_with_face(vertices, flags, indices, topology, transform, color, 1)
#define                       d_pbr_mesh(vertices, flags, indices, topology, transform, albedo, roughness, specular)    d_pbr_mesh_with_face(vertices, flags, indices, topology, transform, albedo, roughness, specular, 0)
#define                       d_pbr_mesh_bf(vertices, flags, indices, topology, transform, albedo, roughness, specular) d_pbr_mesh_with_face(vertices, flags, indices, topology, transform, albedo, roughness, specular, 1)

internal void d_debug(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology);
internal void d_splat(R_Handle vertices, R_VertexFlag flags);