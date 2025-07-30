#pragma once

typedef struct D_ThreadCtx D_ThreadCtx;
struct D_ThreadCtx
{
    Arena *arena;
    R_PassList passes;
};

thread_static D_ThreadCtx *d_thread_ctx = NULL;

void d_begin_pipeline();
void d_submit_pipeline(OS_Handle window, R_Handle rwindow);

R_PassParams_3D* d_begin_3d_pass(rect_f32 viewport, mat4x4_f32 view, mat4x4_f32 projection);

R_Mesh3DInstance* d_mesh(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, mat4x4_f32 transform, vec3_f32 color);