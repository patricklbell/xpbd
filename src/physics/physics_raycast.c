shared_function PHYS_HitList phys_make_hit_list(Arena* arena) {
    return (PHYS_HitList){
        .arena = arena,
        .hits = 0,
        .length = 0,
    };
}
shared_function void phys_hit_list_add(PHYS_HitList* hl, PHYS_collider_id id, PHYS_HitListData data) {
    PHYS_HitListNode* n = push_array(hl->arena, PHYS_HitListNode, 1);
    n->id = id;
    n->data = data;
    stack_push(hl->hits, n);
    hl->length++;
}
shared_function PHYS_HitListNode* phys_hit_list_closest(PHYS_HitList* hl) {
    f32 min_t = MAX_F32;
    PHYS_HitListNode* min_n = NULL;
    for EachList(n, PHYS_HitListNode, hl->hits) {
        if (n->data.contact < min_t) {
            min_t = n->data.contact;
            min_n = n;
        }
    }
    return min_n;
}

shared_function void phys_raycast_collider(PHYS_World* w, PHYS_Collider* c, PHYS_collider_id id, vec3_f32 origin, vec3_f32 direction, PHYS_HitList* out_hits) {
    PHYS_Body* body = phys_world_resolve_body(w, c->base.p);

    PHYS_HitListData data;
    switch (c->base.type) {
        case PHYS_ColliderType_Sphere:{
            f32 t;
            if (phys_raycast_sphere(origin, direction, body->position, c->base.r, &data.contact))
                phys_hit_list_add(out_hits, id, data);
        }break;
        case PHYS_ColliderType_Polytope:{
            for (u32 indicei = 0; indicei < c->polytope.indices_count; indicei+=c->polytope.topology) {
                vec3_f32 v1 = c->polytope.points[c->polytope.indices[indicei]];
                v1 = phys_rotate_translate(v1, body->rotation, body->position);

                for (u32 v3_indicei = indicei + 2; v3_indicei < indicei + c->polytope.topology; v3_indicei++) {
                    vec3_f32 v2 = c->polytope.points[c->polytope.indices[v3_indicei-1]];
                    vec3_f32 v3 = c->polytope.points[c->polytope.indices[v3_indicei]];

                    v2 = phys_rotate_translate(v2, body->rotation, body->position);
                    v3 = phys_rotate_translate(v3, body->rotation, body->position);
                    if (phys_raycast_triangle(origin, direction, v1, v2, v3, &data.contact)) {
                        data.polytope_indice_idx = indicei;
                        phys_hit_list_add(out_hits, id, data);
                    }
                }
            }
        }break;
    }
}
shared_function void phys_world_raycast(PHYS_World* w, vec3_f32 origin, vec3_f32 direction, PHYS_ColliderLayer layer, PHYS_HitList* out_hits) {
    for EachIndex(slot, w->colliders.slots_count) {
        for EachList(collider_n, PHYS_ColliderNode, w->colliders.slots[slot].first) {
            if (!phys_collider_layers_overlap(collider_n->v.base.layer, layer))
                continue;
            phys_raycast_collider(w, &collider_n->v, collider_n->id, origin, direction, out_hits);
        }
    }
}