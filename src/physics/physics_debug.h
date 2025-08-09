#pragma once

typedef void(*PHYS_DBG_DrawEdgeBatch)(vec3_f32* points, vec3_f32* colors, u32 count);
typedef void(*PHYS_DBG_DrawPointBatch)(vec3_f32* points, vec4_f32* colors, f32* radii, u32 count);

typedef enum PHYS_DBG_DrawColorMode {
    PHYS_DBG_DrawColorMode_Manual,
    PHYS_DBG_DrawColorMode_Type,
    PHYS_DBG_DrawColorMode_Force,
    PHYS_DBG_DrawColorMode_Unique,
} PHYS_DBG_DrawColorMode;

typedef struct PHYS_DBG_DrawContext PHYS_DBG_DrawContext;
struct PHYS_DBG_DrawContext {
    PHYS_World* w;
    PHYS_DBG_DrawEdgeBatch draw_edge_batch;
    PHYS_DBG_DrawPointBatch draw_point_batch;

    f32 body_radius;
    f32 max_force;

    PHYS_DBG_DrawColorMode color_mode;
    vec3_f32 constraint_colors[PHYS_ConstraintType_COUNT];
    vec3_f32 collider_colors[PHYS_ColliderType_COUNT];
    vec3_f32 body_color;
    b32 is_color_set;
    vec3_f32 color;
    vec3_f32 min_force_color_hsl;
    vec3_f32 max_force_color_hsl;
};

PHYS_DBG_DrawContext phys_dbg_d_make_context(PHYS_World* w, PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch);

static vec3_f32 phys_dbg_d_get_constraint_color(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c);
static vec3_f32 phys_dbg_d_get_collider_color(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);
static vec3_f32 phys_dbg_d_get_body_color(PHYS_DBG_DrawContext* ctx, PHYS_Body* b);

void phys_dbg_d_constraint_distance(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c);
void phys_dbg_d_constraint_volume(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c);

void phys_dbg_d_collider_sphere(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);
void phys_dbg_d_collider_plane(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);
void phys_dbg_d_collider_rect_cuboid(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);

void phys_dbg_d_constraint(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c);
void phys_dbg_d_collider(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);
void phys_dbg_d_body(PHYS_DBG_DrawContext* ctx, PHYS_Body* b);

void phys_dbg_d_constraints(PHYS_DBG_DrawContext* ctx, PHYS_ConstraintType* blacklist, int blacklist_count);
void phys_dbg_d_colliders(PHYS_DBG_DrawContext* ctx, PHYS_ColliderType* blacklist, int blacklist_count);
void phys_dbg_d_bodies(PHYS_DBG_DrawContext* ctx);

void phys_dbg_d_world(PHYS_DBG_DrawContext* ctx);