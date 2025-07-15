#pragma once

typedef struct D_ThreadCtx D_ThreadCtx;
struct D_ThreadCtx
{
    Arena *arena;
    R_PassList passes;
};

thread_static D_ThreadCtx *d_thread_ctx = 0;

void d_begin_frame();
void d_end_frame(OS_Handle window, R_Handle rwindow);

R_PassParams_3D* d_begin_3d_pass(rect_f32 viewport, mat4x4_f32 view, mat4x4_f32 projection);

R_Mesh3DInst* d_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, mat4x4_f32 transform);