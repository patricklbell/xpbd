PHYS_DBG_DrawContext phys_dbg_d_make_context(PHYS_World* w, PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch) {
    PHYS_DBG_DrawContext ctx = {
        .w = w,
        .draw_edge_batch = draw_edge_batch,
        .draw_point_batch = draw_point_batch,
        .body_radius = 0.05,
        .max_force = 10.f,
        .min_force_color_hsl = make_3f32(240.f/360.f,1,1),
        .max_force_color_hsl = make_3f32(000.f/360.f,1,1),
        .body_color = make_3f32(0,1,0),
    };

    vec3_f32 hsl_color = make_3f32(0,1,1);
    int hue_length = ArrayLength(ctx.collider_colors) + ArrayLength(ctx.constraint_colors) + 1;
    int hue_i = 0;
    for EachElement(i, ctx.collider_colors) {
        hue_i++;
        hsl_color.x = hue_i / (f32)hue_length;
        ctx.collider_colors[i] = hsl_to_rgb(hsl_color);
    }
    for EachElement(i, ctx.constraint_colors) {
        hue_i++;
        hsl_color.x = hue_i / (f32)hue_length;
        ctx.constraint_colors[i] = hsl_to_rgb(hsl_color);
    }

    return ctx;
}

static vec3_f32 phys_dbg_d_get_unique_color() {
    return hsl_to_rgb(make_3f32(rand_f32(),1,1));
}

static vec3_f32 phys_dbg_d_get_constraint_color(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c) {
    switch (ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return ctx->constraint_colors[c->type];
        case PHYS_DBG_DrawColorMode_Force: return hsl_to_rgb(
            lerp_3f32(
                ctx->min_force_color_hsl, ctx->max_force_color_hsl,
                smoothstep_f32(0.f, ctx->max_force, c->force)
            )
        );
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)c); return phys_dbg_d_get_unique_color();
        default: return ctx->color;
    }
}
static vec3_f32 phys_dbg_d_get_collider_color(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    switch (ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return ctx->collider_colors[c->type];
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)c); return phys_dbg_d_get_unique_color();
        default: return ctx->color;
    }
}
static vec3_f32 phys_dbg_d_get_body_color(PHYS_DBG_DrawContext* ctx, PHYS_Body* b) {
    switch (ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return ctx->body_color;
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)b); return phys_dbg_d_get_unique_color();
        default: return ctx->color;
    }
}

void phys_dbg_d_constraint_distance(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c) {
    PHYS_Body* b1 = phys_world_resolve_body(ctx->w, c->distance.b1);
    PHYS_Body* b2 = phys_world_resolve_body(ctx->w, c->distance.b2);

    vec3_f32 edges[] = { b1->position, b2->position };
    if (c->distance.is_offset) {
        vec3_f32 r1 = rot_quat(c->distance.offset1, b1->rotation);
        vec3_f32 r2 = rot_quat(c->distance.offset2, b2->rotation);
        edges[0] = add_3f32(edges[0], r1);
        edges[1] = add_3f32(edges[1], r2);
    }

    vec3_f32 color = phys_dbg_d_get_constraint_color(ctx, c);
    vec3_f32 colors[] = { color, color };

    ctx->draw_edge_batch(edges, colors, 2);
}
void phys_dbg_d_constraint_volume(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c) {
    static const int points_count = ArrayLength(c->volume.p)*(ArrayLength(c->volume.p)-1); // 2*(n choose 2)
    vec3_f32 points[points_count], colors[points_count];

    vec3_f32 color = phys_dbg_d_get_constraint_color(ctx, c);
    
    int offset = 0;
    for (int i = 0; i < ArrayLength(c->volume.p); i++) {
        for (int j = i+1; j < ArrayLength(c->volume.p); j++) {
            points[offset] = phys_world_resolve_body(ctx->w, c->volume.p[i])->position;
            colors[offset] = color;
            offset++;

            points[offset] = phys_world_resolve_body(ctx->w, c->volume.p[j])->position;
            colors[offset] = color;
            offset++;
        }
    }

    ctx->draw_edge_batch(points, colors, points_count);
}

void phys_dbg_d_collider_sphere(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    return; // @todo
}
void phys_dbg_d_collider_plane(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    return; // @todo
}
void phys_dbg_d_collider_rect_cuboid(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    return; // @todo
}

void phys_dbg_d_constraint(PHYS_DBG_DrawContext* ctx, PHYS_Constraint* c) {
    switch (c->type) {
        case PHYS_ConstraintType_Distance: {
            phys_dbg_d_constraint_distance(ctx, c);
        }break;
        case PHYS_ConstraintType_Volume: {
            phys_dbg_d_constraint_volume(ctx, c);
        }break;
    }
}
void phys_dbg_d_collider(PHYS_DBG_DrawContext* ctx, PHYS_Collider* c) {
    switch (c->type) {
        case PHYS_ColliderType_Sphere: {
            phys_dbg_d_collider_sphere(ctx, c);
        }break;
        case PHYS_ColliderType_Plane: {
            phys_dbg_d_collider_plane(ctx, c);
        }break;
        case PHYS_ColliderType_RectCuboid: {
            phys_dbg_d_collider_rect_cuboid(ctx, c);
        }break;
    }
}
void phys_dbg_d_body(PHYS_DBG_DrawContext* ctx, PHYS_Body* b) {
    vec3_f32 offsets[] = {{1, 1, 1},{-1,-1,1},{-1,1,-1},{1,-1,-1}};
    f32 offset_scale = ctx->body_radius;

    static const int points_count = ArrayLength(offsets)*(ArrayLength(offsets)-1); // 2*(n choose 2)
    vec3_f32 points[points_count], colors[points_count];

    vec3_f32 color = phys_dbg_d_get_body_color(ctx, b);

    int offset = 0;
    for (int i = 0; i < ArrayLength(offsets); i++) {
        for (int j = i+1; j < ArrayLength(offsets); j++) {
            points[offset] = add_3f32(b->position, mul_3f32(offsets[i], offset_scale));
            colors[offset] = color;
            offset++;

            points[offset] = add_3f32(b->position, mul_3f32(offsets[j], offset_scale));
            colors[offset] = color;
            offset++;
        }
    }

    ctx->draw_edge_batch(points, colors, points_count);
}

static b32 phys_dbg_d_is_blacklisted(int value, int* blacklist, int blacklist_count) {
    b32 blacklisted = false;
    for EachIndex(blacklist_i, blacklist_count) {
        if (blacklist[blacklist_i] == value) {
            return true;
        }
    }

    return false;
}

void phys_dbg_d_constraints(PHYS_DBG_DrawContext* ctx, PHYS_ConstraintType* blacklist, int blacklist_count) {
    for EachIndex(slot, ctx->w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, ctx->w->constraints.slots[slot]) {
            PHYS_ConstraintType type = constraint_n->v.type;
            if (!phys_dbg_d_is_blacklisted(type, (int*)blacklist, blacklist_count)) {
                phys_dbg_d_constraint(ctx, &constraint_n->v);
            }
        }
    }
}
void phys_dbg_d_colliders(PHYS_DBG_DrawContext* ctx, PHYS_ColliderType* blacklist, int blacklist_count) {
    for EachIndex(slot, ctx->w->colliders.slots_count) {
        for EachList(collider_n, PHYS_ColliderNode, ctx->w->colliders.slots[slot]) {
            PHYS_ColliderType type = collider_n->v.type;
            if (!phys_dbg_d_is_blacklisted(type, (int*)blacklist, blacklist_count)) {
                phys_dbg_d_collider(ctx, &collider_n->v);
            }
        }
    }
}
void phys_dbg_d_bodies(PHYS_DBG_DrawContext* ctx) {
    for EachIndex(i, ctx->w->bodies.length) {
        PHYS_Body* b = &ctx->w->bodies.v[i];
        phys_dbg_d_body(ctx, b);
    }
}

void phys_dbg_d_world(PHYS_DBG_DrawContext* ctx) {
    phys_dbg_d_colliders(ctx, NULL, 0);
    phys_dbg_d_constraints(ctx, NULL, 0);
    phys_dbg_d_bodies(ctx);
}