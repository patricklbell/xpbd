#pragma once

typedef void(*PHYS_DBG_DrawEdgeBatch)(vec3_f32* points, vec3_f32* colors, u32 count);
typedef void(*PHYS_DBG_DrawPointBatch)(vec3_f32* points, vec4_f32* colors, f32* radii, u32 count);

typedef struct PHYS_DBG_DrawContext PHYS_DBG_DrawContext;
struct PHYS_DBG_DrawContext {
    PHYS_World* w;
    PHYS_DBG_DrawEdgeBatch draw_edge_batch;
    PHYS_DBG_DrawPointBatch draw_point_batch;

    vec3_f32 constraint_colors[PHYS_ConstraintType_COUNT];
    vec3_f32 collider_colors[PHYS_ColliderType_COUNT];
    b32 is_color_set;
    vec3_f32 color;

    b32 draw_forces;
    vec3_f32 min_force_color_hsl;
    vec3_f32 max_force_color_hsl;
    f32 max_force;
};

PHYS_DBG_DrawContext phys_dbg_d_make_context(PHYS_World* w, PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch);

void phys_dbg_d_constraint_distance(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Distance* c);
void phys_dbg_d_constraint_volume(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Volume* c);

void phys_dbg_d_collider_sphere(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Sphere* c);
void phys_dbg_d_collider_plane(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Plane* c);
void phys_dbg_d_collider_triangle(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Triangle* c);
void phys_dbg_d_collider_rect_cuboid(PHYS_DBG_DrawContext* ctx, PHYS_Collider_RectCuboid* c);

void phys_dbg_d_constraint(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c);
void phys_dbg_d_collider(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c);
void phys_dbg_d_body(PHYS_DBG_DrawContext* ctx, PHYS_Body* c);

void phys_dbg_d_constraints(PHYS_DBG_DrawContext* ctx, PHYS_ConstraintType* blacklist, int blacklist_count);
void phys_dbg_d_colliders(PHYS_DBG_DrawContext* ctx, PHYS_ColliderType* blacklist, int blacklist_count);
void phys_dbg_d_bodies(PHYS_DBG_DrawContext* ctx);

void phys_dbg_d_world(PHYS_DBG_DrawContext* ctx);

// helpers
static void phys_dbg_use_color(PHYS_DBG_DrawContext* ctx, vec3_f32 color);