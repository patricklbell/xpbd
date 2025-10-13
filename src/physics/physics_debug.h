#pragma once

#ifndef PHYS_DBG_D_STEP
    #define PHYS_DBG_D_STEP 0
#endif

typedef void(*PHYS_DBG_DrawEdgeBatch)(vec3_f32* points, vec3_f32* colors, u32 count);
typedef void(*PHYS_DBG_DrawPointBatch)(vec3_f32* points, vec3_f32* colors, vec2_f32* radii, u32 count);

typedef enum PHYS_DBG_DrawColorMode {
    PHYS_DBG_DrawColorMode_Manual,
    PHYS_DBG_DrawColorMode_Type,
    // PHYS_DBG_DrawColorMode_Force,
    PHYS_DBG_DrawColorMode_Unique,
} PHYS_DBG_DrawColorMode;

typedef struct PHYS_DBG_ThreadCtx PHYS_DBG_ThreadCtx;
struct PHYS_DBG_ThreadCtx {
    Arena* arena;
    PHYS_DBG_DrawEdgeBatch draw_edge_batch;
    PHYS_DBG_DrawPointBatch draw_point_batch;

    f32 default_point_radius;
    f32 default_normal_length;
    f32 body_radius;
    f32 attachment_radius;

    PHYS_DBG_DrawColorMode color_mode;
    vec3_f32 constraint_colors[PHYS_ConstraintType_COUNT];
    vec3_f32 collider_colors[PHYS_ColliderType_COUNT];
    vec3_f32 body_color;
    b32 is_color_set;
    vec3_f32 color;

    b32 do_colliders;
    b32 do_bodies;
    b32 do_constraints;
    b32 do_collider_normals;
    b32 do_contact_points;
    b32 do_contact_manifold;
    b32 do_limit_angle;
};

internal thread_static PHYS_DBG_ThreadCtx* phys_dbg_d_ctx = NULL;

shared_function PHYS_DBG_ThreadCtx* phys_dbg_d_init(PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch);
shared_function PHYS_DBG_ThreadCtx* phys_dbg_d_get_ctx();

internal vec3_f32 phys_dbg_d_get_unique_color();
internal vec3_f32 phys_dbg_d_get_constraint_color(PHYS_World* w, PHYS_Constraint* c);
internal vec3_f32 phys_dbg_d_get_collider_color(PHYS_World* w, PHYS_Collider* c);
internal vec3_f32 phys_dbg_d_get_body_color(PHYS_World* w, PHYS_Body* b);

internal void phys_dbg_d_constraint_distance(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_constraint_advanced_distance(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_constraint_volume(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_constraint_hinge(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_constraint_swing(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_constraint_twist(PHYS_World* w, PHYS_Constraint* c);

internal void phys_dbg_d_collider_sphere(PHYS_World* w, PHYS_Collider_Sphere* c);
internal void phys_dbg_d_collider_polytope(PHYS_World* w, PHYS_Collider_Polytope* c);

internal void phys_dbg_d_constraint(PHYS_World* w, PHYS_Constraint* c);
internal void phys_dbg_d_collider(PHYS_World* w, PHYS_Collider* c);
internal void phys_dbg_d_body(PHYS_World* w, PHYS_Body* b);

internal void phys_dbg_d_constraints(PHYS_World* w, PHYS_ConstraintType* blacklist, int blacklist_count);
internal void phys_dbg_d_colliders(PHYS_World* w, PHYS_ColliderType* blacklist, int blacklist_count);
internal void phys_dbg_d_bodies(PHYS_World* w);

// helpers
internal void phys_dbg_d_angle(vec3_f32 origin, vec3_f32 n1, vec3_f32 n2, vec3_f32 n1_color, vec3_f32 n2_color, vec3_f32 angle_color, f32 n_length, f32 angle_length);
internal void phys_dbg_d_sector(vec3_f32 origin, vec3_f32 normal, vec3_f32 axis, f32 angle, vec3_f32 color, f32 radius);

#define PHYS_DBG_D_DRAW_POINT(p,c,r)        {vec3_f32 x = p; vec3_f32 y = c; vec2_f32 z = r; phys_dbg_d_ctx->draw_point_batch(&x,&y,&z,1);}
#define PHYS_DBG_D_DRAW_DPOINT(p,c)         PHYS_DBG_D_DRAW_POINT(p,c,make_2f32(phys_dbg_d_ctx->default_point_radius,phys_dbg_d_ctx->default_point_radius))
#define PHYS_DBG_D_DRAW_EDGE(s,e,c)         {vec3_f32 x[2] = {s,e}; vec3_f32 y[2] = {c,c}; phys_dbg_d_ctx->draw_edge_batch(x,y,2);}
#define PHYS_DBG_D_DRAW_NORMAL(o,n,c)       PHYS_DBG_D_DRAW_EDGE(o,add_3f32(o,mul_3f32(n,phys_dbg_d_ctx->default_normal_length)),c)
#define PHYS_DBG_D_DRAW_DANGLE(o,n1,n2,c)   phys_dbg_d_angle(o,n1,n2,c,c,c,phys_dbg_d_ctx->default_normal_length,phys_dbg_d_ctx->default_normal_length/2.f)