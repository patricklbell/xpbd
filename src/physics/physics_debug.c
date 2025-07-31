// Helper function to convert HSL to RGB (all values in [0.0, 1.0])
static vec3_f32 hsl_to_rgb(vec3_f32 hsl) {
    f32 h = hsl.h;
    f32 s = hsl.s;
    f32 l = hsl.l;

    // Clamp HSL values to [0.0, 1.0]
    h = (h < 0.0f) ? 0.0f : (h > 1.0f) ? 1.0f : h;
    s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s;
    l = (l < 0.0f) ? 0.0f : (l > 1.0f) ? 1.0f : l;

    // If saturation is near zero, return grayscale
    if (s < 1e-6f) {
        return make_3f32(l, l, l);
    }

    // Convert hue to [0.0, 6.0)
    f32 h6 = h * 6.0f;
    if (h6 >= 6.0f) h6 = 0.0f;  // Wrap around (hue is circular)

    // Calculate intermediate values
    int sector = (int)h6;
    f32 frac = h6 - sector;
    f32 p = l * (1.0f - s);
    f32 q = l * (1.0f - s * frac);
    f32 t = l * (1.0f - s * (1.0f - frac));

    // Compute RGB based on the sector
    switch (sector) {
        case 0:  return make_3f32(l, t, p);
        case 1:  return make_3f32(q, l, p);
        case 2:  return make_3f32(p, l, t);
        case 3:  return make_3f32(p, q, l);
        case 4:  return make_3f32(t, p, l);
        case 5:  return make_3f32(l, p, q);
        default: Assert(0);
    }
}

PHYS_DBG_DrawContext phys_dbg_d_make_context(PHYS_World* w, PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch) {
    PHYS_DBG_DrawContext ctx = {
        .w = w,
        .draw_edge_batch = draw_edge_batch,
        .draw_point_batch = draw_point_batch,
        .max_force = 100.,
    };

    vec3_f32 hsl_color = make_3f32(0, 1.0, 0.5);
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

static vec3_f32 phys_dbg_d_get_constraint_color(PHYS_DBG_DrawContext* ctx, f32 force) {
    if (ctx->draw_forces) {
        if (force > ctx->max_force) {
            ctx->max_force = force;
        } else {
            ctx->max_force *= 0.99f;
        }
        return hsl_to_rgb(lerp_3f32(ctx->min_force_color_hsl, ctx->max_force_color_hsl, smoothstep_f32(0.f, ctx->max_force, force)));
    }

    return ctx->color;
}

void phys_dbg_d_constraint_distance(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Distance* c) {
    PHYS_Body* b1 = phys_world_resolve_body(ctx->w, c->b1);
    PHYS_Body* b2 = phys_world_resolve_body(ctx->w, c->b2);

    vec3_f32 edges[] = { b1->position, b2->position };
    if (c->is_offset) {
        vec3_f32 r1 = rot_quat(c->offset1, b1->rotation);
        vec3_f32 r2 = rot_quat(c->offset2, b2->rotation);
        edges[0] = add_3f32(edges[0], r1);
        edges[1] = add_3f32(edges[1], r2);
    }

    vec3_f32 color = phys_dbg_d_get_constraint_color(ctx, c->force);
    vec3_f32 colors[] = { color, color };

    ctx->draw_edge_batch(edges, colors, 2);
}
void phys_dbg_d_constraint_volume(PHYS_DBG_DrawContext* ctx, PHYS_Constraint_Volume* c) {
    static const int points_count = ArrayLength(c->p)*(ArrayLength(c->p)-1); // 2*(n choose 2)
    vec3_f32 points[points_count], colors[points_count];

    vec3_f32 color = phys_dbg_d_get_constraint_color(ctx, c->force);
    
    int offset = 0;
    for (int i = 0; i < ArrayLength(c->p); i++) {
        for (int j = i+1; j < ArrayLength(c->p); j++) {
            points[offset] = phys_world_resolve_body(ctx->w, c->p[i])->position;
            colors[offset] = color;
            offset++;

            points[offset] = phys_world_resolve_body(ctx->w, c->p[j])->position;
            colors[offset] = color;
            offset++;
        }
    }

    ctx->draw_edge_batch(points, colors, points_count);
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
void phys_dbg_d_body(PHYS_DBG_DrawContext* ctx, PHYS_Body* c) {
    vec3_f32 offsets[] = {{1, 1, 1},{-1,-1,1},{-1,1,-1},{1,-1,-1}};
    f32 offset_scale = 0.05;

    static const int points_count = ArrayLength(offsets)*(ArrayLength(offsets)-1); // 2*(n choose 2)
    vec3_f32 points[points_count], colors[points_count];

    int offset = 0;
    for (int i = 0; i < ArrayLength(offsets); i++) {
        for (int j = i+1; j < ArrayLength(offsets); j++) {
            points[offset] = add_3f32(c->position, mul_3f32(offsets[i], offset_scale));
            colors[offset] = ctx->color;
            offset++;

            points[offset] = add_3f32(c->position, mul_3f32(offsets[j], offset_scale));
            colors[offset] = ctx->color;
            offset++;
        }
    }

    ctx->draw_edge_batch(points, colors, points_count);
}

static b32 phys_dbg_d_is_blacklisted(int value, int* blacklist, int blacklist_count) {
    b32 blacklisted = 0;
    for EachIndex(blacklist_i, blacklist_count) {
        if (blacklist[blacklist_i] == value) {
            return 1;
        }
    }

    return 0;
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

// helpers
static void phys_dbg_use_color(PHYS_DBG_DrawContext* ctx, vec3_f32 color) {
    if (!ctx->is_color_set) {
        ctx->color = color;
    }
    ctx->is_color_set = 0;
}