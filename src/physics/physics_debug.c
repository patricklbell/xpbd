PHYS_DBG_ThreadCtx* phys_dbg_d_init(PHYS_DBG_DrawEdgeBatch draw_edge_batch, PHYS_DBG_DrawPointBatch draw_point_batch) {
    Assert(phys_dbg_d_ctx == NULL);

    Arena* arena = arena_alloc();

    phys_dbg_d_ctx = push_array(arena, PHYS_DBG_ThreadCtx, 1);
    *phys_dbg_d_ctx = (PHYS_DBG_ThreadCtx){
        .arena = arena,
        .draw_edge_batch = draw_edge_batch,
        .draw_point_batch = draw_point_batch,
        .default_point_radius = 0.05,
        .default_normal_length = 0.1,
        .body_radius = 0.05,
        .attachment_radius = 0.01,
        .max_force = 10.f,
        .color_mode = PHYS_DBG_DrawColorMode_Type,
        .min_force_color_hsl = make_3f32(240.f/360.f,1,1),
        .max_force_color_hsl = make_3f32(000.f/360.f,1,1),
    };

    vec3_f32 hsl_color = make_3f32(0,1,1);
    int hue_length = ArrayLength(phys_dbg_d_ctx->collider_colors) + ArrayLength(phys_dbg_d_ctx->constraint_colors) + 2;
    int hue_i = 0;
    for EachElement(i, phys_dbg_d_ctx->collider_colors) {
        hue_i++;
        hsl_color.x = hue_i / (f32)hue_length;
        phys_dbg_d_ctx->collider_colors[i] = hsl_to_rgb(hsl_color);
    }
    for EachElement(i, phys_dbg_d_ctx->constraint_colors) {
        hue_i++;
        hsl_color.x = hue_i / (f32)hue_length;
        phys_dbg_d_ctx->constraint_colors[i] = hsl_to_rgb(hsl_color);
    }
    
    hue_i++;
    hsl_color.x = hue_i / (f32)hue_length;
    phys_dbg_d_ctx->body_color = hsl_to_rgb(hsl_color);

    return phys_dbg_d_ctx;
}

static vec3_f32 phys_dbg_d_get_unique_color() {
    return hsl_to_rgb(make_3f32(rand_f32(),1,1));
}

static vec3_f32 phys_dbg_d_get_constraint_color(PHYS_World* w, PHYS_Constraint* c) {
    switch (phys_dbg_d_ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return phys_dbg_d_ctx->constraint_colors[c->type];
        case PHYS_DBG_DrawColorMode_Force: return hsl_to_rgb(
            lerp_3f32(
                phys_dbg_d_ctx->min_force_color_hsl, phys_dbg_d_ctx->max_force_color_hsl,
                smoothstep_f32(0.f, phys_dbg_d_ctx->max_force, c->force)
            )
        );
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)c); return phys_dbg_d_get_unique_color();
        default: return phys_dbg_d_ctx->color;
    }
}
static vec3_f32 phys_dbg_d_get_collider_color(PHYS_World* w, PHYS_Collider* c) {
    switch (phys_dbg_d_ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return phys_dbg_d_ctx->collider_colors[c->base.type];
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)c); return phys_dbg_d_get_unique_color();
        default: return phys_dbg_d_ctx->color;
    }
}
static vec3_f32 phys_dbg_d_get_body_color(PHYS_World* w, PHYS_Body* b) {
    switch (phys_dbg_d_ctx->color_mode) {
        case PHYS_DBG_DrawColorMode_Type: return phys_dbg_d_ctx->body_color;
        case PHYS_DBG_DrawColorMode_Unique: srand((u64)b); return phys_dbg_d_get_unique_color();
        default: return phys_dbg_d_ctx->color;
    }
}

void phys_dbg_d_constraint_distance(PHYS_World* w, PHYS_Constraint* c) {
    PHYS_Body* b1 = phys_world_resolve_body(w, c->distance.b1);
    PHYS_Body* b2 = phys_world_resolve_body(w, c->distance.b2);

    vec3_f32 edges[] = { b1->position, b2->position };
    if (c->distance.is_offset) {
        vec3_f32 r1 = rot_quat(c->distance.offset1, b1->rotation);
        vec3_f32 r2 = rot_quat(c->distance.offset2, b2->rotation);
        edges[0] = add_3f32(edges[0], r1);
        edges[1] = add_3f32(edges[1], r2);
    }

    vec3_f32 color = phys_dbg_d_get_constraint_color(w, c);
    vec3_f32 colors[] = { color, color };

    phys_dbg_d_ctx->draw_edge_batch(edges, colors, ArrayLength(edges));

    vec2_f32 radii[] = {
        make_2f32(phys_dbg_d_ctx->attachment_radius, phys_dbg_d_ctx->attachment_radius),
        make_2f32(phys_dbg_d_ctx->attachment_radius, phys_dbg_d_ctx->attachment_radius),
    };
    phys_dbg_d_ctx->draw_point_batch(edges, colors, radii, ArrayLength(edges));
}
void phys_dbg_d_constraint_volume(PHYS_World* w, PHYS_Constraint* c) {
    static const int points_count = ArrayLength(c->volume.p)*(ArrayLength(c->volume.p)-1); // 2*(n choose 2)
    vec3_f32 points[points_count], colors[points_count];

    vec3_f32 color = phys_dbg_d_get_constraint_color(w, c);
    
    int offset = 0;
    for (int i = 0; i < ArrayLength(c->volume.p); i++) {
        for (int j = i+1; j < ArrayLength(c->volume.p); j++) {
            points[offset] = phys_world_resolve_body(w, c->volume.p[i])->position;
            colors[offset] = color;
            offset++;

            points[offset] = phys_world_resolve_body(w, c->volume.p[j])->position;
            colors[offset] = color;
            offset++;
        }
    }

    phys_dbg_d_ctx->draw_edge_batch(points, colors, points_count);
}
void phys_dbg_d_constraint_hinge(PHYS_World* w, PHYS_Constraint* c) {
    return; // @todo
}

void phys_dbg_d_collider_sphere(PHYS_World* w, PHYS_Collider_Sphere* c) {
    static u32 parallels = 3, points_per_parallel = 16, dims_shown = 3, dims = 3;

    f32 r = c->base.r;
    f32 gap = r/(parallels+1);
    DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        u32 i = 0;
        u32 total_points = 2*(dims_shown*(1 + 2*parallels))*points_per_parallel;
        vec3_f32* points = push_array(scratch.arena, vec3_f32, total_points);
        vec3_f32* colors = push_array(scratch.arena, vec3_f32, total_points);

        for EachIndex(paralleli, 1+parallels) {
            f32 gapi = paralleli*gap;
            f32 ri = sqrt_f32(r*r - gapi*gapi);
            for EachIndex(pointi, points_per_parallel) {
                f32 t1 = 2.f*PI*(f32)pointi/points_per_parallel;
                f32 x1 = ri*cos_f32(t1);
                f32 y1 = ri*sin_f32(t1);

                f32 t2 = 2.f*PI*(f32)(pointi+1)/points_per_parallel;
                f32 x2 = ri*cos_f32(t2);
                f32 y2 = ri*sin_f32(t2);

                for EachIndex(dim, dims_shown) {
                    // +parallel
                    points[i].v[dim] = +gapi; points[i].v[(dim+1)%dims] = x1; points[i].v[(dim+2)%dims] = y1;
                    i++;
                    points[i].v[dim] = +gapi; points[i].v[(dim+1)%dims] = x2; points[i].v[(dim+2)%dims] = y2;
                    i++;
                    if (paralleli == 0) continue;

                    // -parallel
                    points[i].v[dim] = -gapi; points[i].v[(dim+1)%dims] = x1; points[i].v[(dim+2)%dims] = y1;
                    i++;
                    points[i].v[dim] = -gapi; points[i].v[(dim+1)%dims] = x2; points[i].v[(dim+2)%dims] = y2;
                    i++;
                }
            }
        }
        Assert(i == total_points);

        PHYS_Body* b = phys_world_resolve_body(w, c->base.p);
        vec3_f32 color = phys_dbg_d_get_collider_color(w, (PHYS_Collider*)c);
        for EachIndex(i, total_points) {
            points[i] = phys_rotate_translate(points[i], b->rotation, b->position);
            colors[i] = color;
        }

        phys_dbg_d_ctx->draw_edge_batch(points, colors, total_points);
    }
}
void phys_dbg_d_collider_polytope(PHYS_World* w, PHYS_Collider_Polytope* c) {
    DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        vec3_f32 color = phys_dbg_d_get_collider_color(w, (PHYS_Collider*)c);
        PHYS_Body* b = phys_world_resolve_body(w, c->base.p);

        u32 point_count = 2*c->indices_count;
        u32 point_offset = 0;
        vec3_f32* points = push_array(scratch.arena, vec3_f32, point_count);
        vec3_f32* colors = push_array(scratch.arena, vec3_f32, point_count);
        for GEO_EachEdge_Ring_Open(u, v, u32, c->indices, c->indices_count, c->topology) {
            colors[point_offset] = color;
            points[point_offset++] = phys_rotate_translate(c->points[u], b->rotation, b->position);
            colors[point_offset] = color;
            points[point_offset++] = phys_rotate_translate(c->points[v], b->rotation, b->position);
        } GEO_EachEdge_Ring_Close;

        if (phys_dbg_d_ctx->do_collider_normals) {
            for EachIndex(ni, c->normals_count) {
                // vec3_f32 centroid = {0};
                // for EachIndex(pi, c->topology) {
                //     centroid = add_3f32(centroid, phys_rotate_translate(c->points[c->indices[ni*c->topology + pi]], b->rotation, b->position));
                // }
                // centroid = mul_3f32(centroid, 1.f/c->topology);
                // PHYS_DBG_D_DRAW_NORMAL(centroid, c->normals[ni], make_3f32(1,0,0));

                // computed normal from face to check winding order
                GEO_Polygon f = {.topology=c->topology};
                vec3_f32 centroid = {0};
                for EachIndex(pi, c->topology) {
                    vec3_f32 v = c->points[c->indices[ni*c->topology + pi]];

                    centroid = add_3f32(centroid, phys_rotate_translate(v, b->rotation, b->position));
                    f.data[pi] = rot_quat(v, b->rotation);
                }
                centroid = mul_3f32(centroid, 1.f/c->topology);
                PHYS_DBG_D_DRAW_NORMAL(centroid, normalize_3f32(phys_polygon_normal_ccw(&f)), make_3f32(1,0,0));
            }
        }
    
        phys_dbg_d_ctx->draw_edge_batch(points, colors, point_count);
    }
}

void phys_dbg_d_constraint(PHYS_World* w, PHYS_Constraint* c) {
    switch (c->type) {
        case PHYS_ConstraintType_Distance: {
            phys_dbg_d_constraint_distance(w, c);
        }break;
        case PHYS_ConstraintType_Volume: {
            phys_dbg_d_constraint_volume(w, c);
        }break;
        case PHYS_ConstraintType_Hinge: {
            phys_dbg_d_constraint_hinge(w, c);
        }break;
    }
}
void phys_dbg_d_collider(PHYS_World* w, PHYS_Collider* c) {
    switch (c->base.type) {
        case PHYS_ColliderType_Sphere: {
            phys_dbg_d_collider_sphere(w, &c->sphere);
        }break;
        case PHYS_ColliderType_Polytope: {
            phys_dbg_d_collider_polytope(w, &c->polytope);
        }break;
    }
}
void phys_dbg_d_body(PHYS_World* w, PHYS_Body* b) {
    // draw a tetrahedron at body position
    // vec3_f32 offsets[] = {{1, 1, 1},{-1,-1,1},{-1,1,-1},{1,-1,-1}};
    // f32 offset_scale = phys_dbg_d_ctx->body_radius;

    // static const int points_count = ArrayLength(offsets)*(ArrayLength(offsets)-1); // 2*(n choose 2)
    // vec3_f32 points[points_count], colors[points_count];

    // vec3_f32 color = phys_dbg_d_get_body_color(w, b);

    // int offset = 0;
    // for (int i = 0; i < ArrayLength(offsets); i++) {
    //     for (int j = i+1; j < ArrayLength(offsets); j++) {
    //         points[offset] = add_3f32(b->position, mul_3f32(offsets[i], offset_scale));
    //         colors[offset] = color;
    //         offset++;

    //         points[offset] = add_3f32(b->position, mul_3f32(offsets[j], offset_scale));
    //         colors[offset] = color;
    //         offset++;
    //     }
    // }

    PHYS_DBG_D_DRAW_POINT(b->position, phys_dbg_d_get_body_color(w, b), make_2f32(phys_dbg_d_ctx->body_radius, phys_dbg_d_ctx->body_radius));
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

void phys_dbg_d_constraints(PHYS_World* w, PHYS_ConstraintType* blacklist, int blacklist_count) {
    for EachIndex(slot, w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            PHYS_ConstraintType type = constraint_n->v.type;
            if (!phys_dbg_d_is_blacklisted(type, (int*)blacklist, blacklist_count)) {
                phys_dbg_d_constraint(w, &constraint_n->v);
            }
        }
    }
}
void phys_dbg_d_colliders(PHYS_World* w, PHYS_ColliderType* blacklist, int blacklist_count) {
    for EachIndex(slot, w->colliders.slots_count) {
        for EachList(collider_n, PHYS_ColliderNode, w->colliders.slots[slot]) {
            PHYS_ColliderType type = collider_n->v.base.type;
            if (!phys_dbg_d_is_blacklisted(type, (int*)blacklist, blacklist_count)) {
                phys_dbg_d_collider(w, &collider_n->v);
            }
        }
    }
}
void phys_dbg_d_bodies(PHYS_World* w) {
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];
        phys_dbg_d_body(w, b);
    }
}