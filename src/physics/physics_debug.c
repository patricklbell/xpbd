PHYS_DBG_DrawContext phys_dbg_d_default_context(PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch);

void phys_dbg_d_constraint_distance(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Distance* c) {
    vec3_f32 edges[] = {
        phys_world_resolve_body(ctx->w, c->b1)->position,
        phys_world_resolve_body(ctx->w, c->b2)->position,
    };
    vec3_f32 colors[] = {
        ctx->color,
        ctx->color,
    };

    ctx->draw_edge_batch(edges, colors, 1);
}
void phys_dbg_d_constraint_volume(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Volume* c) {
    static const int num_points = ArrayLength(c->p);
    vec3_f32 edges[num_points*2], colors[num_points*2];
    for EachIndex(i, num_points) {
        edges [2*i+0] = phys_world_resolve_body(ctx->w, c->p[(i+0)%num_points])->position;
        edges [2*i+1] = phys_world_resolve_body(ctx->w, c->p[(i+1)%num_points])->position;
        colors[2*i+0] = ctx->color;
        colors[2*i+1] = ctx->color;
    }

    ctx->draw_edge_batch(edges, colors, 1);
}

void phys_dbg_d_collider_sphere(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Sphere* c) {
    return; // @todo
}
void phys_dbg_d_collider_plane(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Plane* c) {
    return; // @todo
}
void phys_dbg_d_collider_triangle(PHYS_DBG_DrawContext* ctx, PHYS_Collider_Triangle* c) {
    return; // @todo
}
void phys_dbg_d_collider_rect_cuboid(PHYS_DBG_DrawContext* ctx, PHYS_Collider_RectCuboid* c) {
    return; // @todo
}

void phys_dbg_d_constraint(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c) {
    phys_dbg_use_color(ctx, ctx->constraint_colors[c->type]);
    switch (c->type) {
        case PHYS_ConstraintType_Distance: {
            phys_dbg_d_constraint_distance(ctx, &c->distance);
        }break;
        case PHYS_ConstraintType_Volume: {
            phys_dbg_d_constraint_volume(ctx, &c->volume);
        }break;
    }
}
void phys_dbg_d_collider(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    phys_dbg_use_color(ctx, ctx->collider_colors[c->type]);
    switch (c->type) {
        case PHYS_ColliderType_Sphere: {
            phys_dbg_d_collider_sphere(ctx, &c->sphere);
        }break;
        case PHYS_ColliderType_Plane: {
            phys_dbg_d_collider_plane(ctx, &c->plane);
        }break;
        case PHYS_ColliderType_Triangle: {
            phys_dbg_d_collider_triangle(ctx, &c->triangle);
        }break;
        case PHYS_ColliderType_RectCuboid: {
            phys_dbg_d_collider_rect_cuboid(ctx, &c->rect_cuboid);
        }break;
    }
}

void phys_dbg_d_constraints(PHYS_DBG_DrawContext* ctx) {
    for EachIndex(slot, ctx->w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, ctx->w->constraints.slots[slot]) {
            phys_dbg_d_constraint(ctx, &constraint_n->v);
        }
    }
}
void phys_dbg_d_colliders(PHYS_DBG_DrawContext* ctx) {
    for EachIndex(slot, ctx->w->colliders.slots_count) {
        for EachList(collider_n, PHYS_ColliderNode, ctx->w->colliders.slots[slot]) {
            phys_dbg_d_collider(ctx, &collider_n->v);
        }
    }
}

// helpers
static void phys_dbg_use_color(PHYS_DBG_DrawContext* ctx, vec3_f32 color) {
    if (!ctx->is_color_set) {
        ctx->color = color;
    }
    ctx->is_color_set = 0;
}