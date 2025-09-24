PHYS_World* phys_make_world(PHYS_WorldSettings settings) {
    Arena* arena = arena_alloc();
    PHYS_World* w = push_array(arena, PHYS_World, 1);

    *w = (PHYS_World){
        .arena = arena,
        .step_arena = arena_alloc_ps(MB(1)),
        .substep_arena = arena_alloc(),
        .substeps = (!settings.substeps) ? 16 : settings.substeps,
        .little_g = (!settings.little_g) ? -10 : settings.little_g,
        .min_r = settings.min_collision_distance,
        .restitution_calculation = settings.restitution_calculation,
        .static_friction_calculation = settings.static_friction_calculation,
        .dynamic_friction_calculation = settings.dynamic_friction_calculation,
        .hashgrid_cell_r = 0.03,
        .hashgrid_obj_r = 0.01,
        .colliders = (PHYS_ColliderMap){
            .slots = push_array(arena, PHYS_ColliderList, PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT),
            .slots_count = PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT,
            .free_chain = NULL
        },
        .constraints = (PHYS_ConstraintMap){
            .slots = push_array(arena, PHYS_ConstraintList, PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT),
            .slots_count = PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT,
            .free_chain = NULL
        },
        .dependent_constraints = (PHYS_ConstraintMap){
            .slots = push_array(arena, PHYS_ConstraintList, PHYS_DEPENDENT_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT),
            .slots_count = PHYS_DEPENDENT_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT,
            .free_chain = NULL
        },
        .bodies = (PHYS_BodyDynamicArray){
            .v = (PHYS_Body*)os_allocate(PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY*sizeof(PHYS_Body)),
            .length = 0,
            .capacity = PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY,
        }
    };

    // @todo
    if (settings.hashgrid_cell_size) {
        w->hashgrid_cell_r = settings.hashgrid_cell_size;
    }
    if (settings.hashgrid_object_size) {
        w->hashgrid_obj_r = settings.hashgrid_object_size;
    }
    
    return w;
}

void phys_world_cleanup(PHYS_World* w) {
    os_deallocate(w->bodies.v);
    arena_release(w->arena);
    arena_release(w->step_arena);
    arena_release(w->substep_arena);
}

static void phys_world_add_collision_record(PHYS_World* w, PHYS_CollisionSubstepRecord info) {
    PHYS_CollisionSubstepRecordNode* n = push_array(w->substep_arena, PHYS_CollisionSubstepRecordNode, 1);
    n->v = info;
    stack_push(w->substep_collision_records, n);
}

static void phys_bodies_adjust_allocation(PHYS_World* w) {
    PHYS_Body* new_v = (PHYS_Body*)os_allocate(w->bodies.capacity*sizeof(PHYS_Body));
    memcpy(new_v, w->bodies.v, w->bodies.length*sizeof(PHYS_Body));
    os_deallocate(w->bodies.v);
    w->bodies.v = new_v;
}
PHYS_body_id phys_world_add_body(PHYS_World* w, PHYS_Body b) {
    PHYS_body_id new_id = w->bodies.length;
    w->bodies.length++;

    // @todo reserve large vaddress space and commit pages as needed
    if (w->bodies.length > w->bodies.capacity) {
        w->bodies.capacity *= PHYS_BODY_DYNAMIC_ARRAY_GROWTH;
        phys_bodies_adjust_allocation(w);
    }

    // initialise rotation if not set
    if (dot_4f32(b.rotation, b.rotation) < EPSILON_F32) {
        b.rotation = make_identity_quat();
    }
    
    w->bodies.v[new_id] = b;
    return new_id;
}
void phys_world_remove_body(PHYS_World* w, PHYS_body_id dp) {
    w->bodies.length--;

    if (w->bodies.length < w->bodies.capacity / PHYS_BODY_DYNAMIC_ARRAY_GROWTH) {
        w->bodies.capacity /= PHYS_BODY_DYNAMIC_ARRAY_GROWTH;
        phys_bodies_adjust_allocation(w);
    }
}
PHYS_Body* phys_world_resolve_body(PHYS_World* w, PHYS_body_id dp) {
    Assert(dp >= 0 && dp < w->bodies.length);
    return &w->bodies.v[dp];
}

// collider api
PHYS_collider_id phys_world_add_collider(PHYS_World* w, PHYS_Collider c) {
    PHYS_ColliderNode* new_node;
    if (w->colliders.free_chain != NULL) {
        new_node = w->colliders.free_chain;
        stack_pop(w->colliders.free_chain);
    } else {
        new_node = push_array(w->arena, PHYS_ColliderNode, 1);
        new_node->id.idx = w->colliders.max_idx++;
        new_node->id.version = 1;
    }
    Assert(new_node != NULL);
    
    PHYS_ColliderList* list = &w->colliders.slots[new_node->id.idx % w->colliders.slots_count];
    dllist_push_back(list->first, list->last, new_node);
    w->colliders.length++;
    
    new_node->v = c;
    return new_node->id;
}
static PHYS_ColliderNode* phys_world_resolve_collider_node(PHYS_World* w, u32 idx) {
    u32 slot = idx % w->colliders.slots_count;
    for EachList(n, PHYS_ColliderNode, w->colliders.slots[slot].first) {
        if (n->id.idx == idx) {
            return n;
        }
    }
    return NULL;
}
void phys_world_remove_collider(PHYS_World* w, PHYS_collider_id id) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, id.idx);
    Assert(n != NULL && n->id.version == id.version);

    // remove from slot's list
    u32 slot = id.idx % w->colliders.slots_count;
    PHYS_ColliderList* list = &w->colliders.slots[slot];
    dllist_remove(list->first, list->last, n);

    // invalidate old ids by increasing version
    n->id.version++;
    // add node to free chain for reuse
    stack_push(w->colliders.free_chain, n);

    w->colliders.length--;
}
PHYS_Collider* phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id id) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, id.idx);
    if (n != NULL && n->id.version != id.version) {
        return NULL;
    }
    return &n->v;
}

// helpers for constraint maps
static PHYS_constraint_id phys_constraint_map_add_value(PHYS_ConstraintMap* map, Arena* arena, PHYS_ConstraintNodeValue v) {
    PHYS_ConstraintNode* new_node;
    if (map->free_chain != NULL) {
        new_node = map->free_chain;
        stack_pop(map->free_chain);
    } else {
        new_node = push_array(arena, PHYS_ConstraintNode, 1);
        new_node->id.idx = map->max_idx++;
        new_node->id.version = 1; // @note makes sure valid id is non-zero
    }
    Assert(new_node != NULL);

    PHYS_ConstraintList* list = &map->slots[new_node->id.idx % map->slots_count];
    dllist_push_back(list->first, list->last, new_node);

    new_node->v = v;
    return new_node->id;
}
static PHYS_ConstraintNode* phys_constraint_map_get_node(PHYS_ConstraintMap* map, u32 idx) {
    u32 slot = idx % map->slots_count;
    for EachList(n, PHYS_ConstraintNode, map->slots[slot].first) {
        if (n->id.idx == idx) {
            return n;
        }
    }
    return NULL;
}
static PHYS_ConstraintNodeValue* phys_constraint_map_get_value(PHYS_ConstraintMap* map, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_constraint_map_get_node(map, id.idx);
    if (n == NULL || n->id.version != id.version) {
        return NULL;
    }
    return &n->v;
}
static void phys_constraint_map_delete(PHYS_ConstraintMap* map, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_constraint_map_get_node(map, id.idx);
    Assert(n != NULL && n->id.version == id.version);
    
    // remove from slot's list
    u32 slot = id.idx % map->slots_count;
    PHYS_ConstraintList* list = &map->slots[slot];
    dllist_remove(list->first, list->last, n);

    // invalidate old id by increasing version
    n->id.version++;
    // add node to free chain for reuse
    stack_push(map->free_chain, n);

    map->length--;
}

// constraint
PHYS_constraint_id phys_world_add_constraint(PHYS_World* w, PHYS_Constraint c) {
    return phys_constraint_map_add_value(&w->constraints, w->arena, (PHYS_ConstraintNodeValue){.constraint=c});
}
void phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id id) {
    phys_constraint_map_delete(&w->constraints, id);
}
PHYS_Constraint* phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id id) {
    PHYS_ConstraintNodeValue* v = phys_constraint_map_get_value(&w->constraints, id);
    return (v == NULL) ? NULL : &v->constraint;
}

// dependent constraint
PHYS_constraint_id phys_world_add_dependent_constraint(PHYS_World* w, PHYS_DependentConstraint c) {
    return phys_constraint_map_add_value(&w->dependent_constraints, w->arena, (PHYS_ConstraintNodeValue){.dependent_constraint=c});
}
void phys_world_remove_dependent_constraint(PHYS_World* w, PHYS_constraint_id id) {
    phys_constraint_map_delete(&w->dependent_constraints, id);
}
PHYS_DependentConstraint* phys_world_resolve_dependent_constraint(PHYS_World* w, PHYS_constraint_id id) {
    PHYS_ConstraintNodeValue* v = phys_constraint_map_get_value(&w->dependent_constraints, id);
    return (v == NULL) ? NULL : &v->dependent_constraint;
}

// asserts
b32 phys_world_valid_radius(PHYS_World* w, f32 d) {
    if (w->min_r) {
        return d >= w->min_r;
    }
    return true;
}

// layers
b32 phys_collider_layers_overlap(PHYS_ColliderLayer l1, PHYS_ColliderLayer l2) {
    if (l1 == PHYS_ColliderLayer_NoSelf && l2 == PHYS_ColliderLayer_NoSelf)
        return false;
    return true;
}