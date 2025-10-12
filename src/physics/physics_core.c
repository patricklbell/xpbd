shared_function PHYS_World* phys_make_world(PHYS_WorldSettings settings) {
    Arena* arena = arena_alloc();
    PHYS_World* w = push_array(arena, PHYS_World, 1);

    // @todo optimize settings and decide appropriate api
    *w = (PHYS_World){
        .arena = arena,
        .step_arena = arena_alloc_ps(MB(1)),
        .substep_arena = arena_alloc(),
        .prebuilt_arena = arena_alloc(),

        .substeps = (!settings.substeps) ? 16 : settings.substeps,
        .little_g = (!settings.little_g) ? -10 : settings.little_g,

        .restitution_calculation = settings.restitution_calculation,
        .static_friction_calculation = settings.static_friction_calculation,
        .dynamic_friction_calculation = settings.dynamic_friction_calculation,

        .min_r = settings.min_collision_distance,
        .min_v_mult = 1.0f,
        .hashgrid_cell_size = 0.01,
        .hashgrid_obj_size = 0.01,
    };
    static u32 pa_growth = 2, pa_initial_capacity = 16;
    phys_pooled_array_alloc(&w->bodies, sizeof(PHYS_Body), pa_growth, pa_initial_capacity);
    phys_pooled_array_alloc(&w->colliders, sizeof(PHYS_Collider), pa_growth, pa_initial_capacity);
    phys_pooled_array_alloc(&w->constraints, sizeof(PHYS_Constraint), pa_growth, pa_initial_capacity);
    phys_pooled_array_alloc(&w->dependent_constraints, sizeof(PHYS_DependentConstraint), pa_growth, pa_initial_capacity);

    if (settings.hashgrid_cell_size) {
        w->hashgrid_cell_size = settings.hashgrid_cell_size;
    }
    if (settings.hashgrid_object_size) {
        w->hashgrid_obj_size = settings.hashgrid_object_size;
    }
    
    return w;
}

shared_function void phys_world_cleanup(PHYS_World* w) {
    phys_pooled_array_release(&w->dependent_constraints);
    phys_pooled_array_release(&w->constraints);
    phys_pooled_array_release(&w->colliders);
    phys_pooled_array_release(&w->bodies);
    arena_release(w->prebuilt_arena);
    arena_release(w->substep_arena);
    arena_release(w->step_arena);
    arena_release(w->arena);
}

internal void phys_world_add_collision_record(PHYS_World* w, PHYS_CollisionSubstepRecord info) {
    PHYS_CollisionSubstepRecordNode* n = push_array(w->substep_arena, PHYS_CollisionSubstepRecordNode, 1);
    n->v = info;
    stack_push(w->substep_collision_records, n);
}

// pooled array helpers
internal void phys_pooled_array_alloc(PHYS_PooledArray* pa, u32 item_size, u32 growth, u32 inital_capacity) {
    pa->v = os_allocate(inital_capacity*item_size);
    pa->versions = (s32*)os_allocate(inital_capacity*sizeof(s32));
    pa->length = 0;
    pa->capacity = inital_capacity;
    pa->growth = growth;
    pa->item_size = item_size;
    pa->free_arena = arena_alloc();
    pa->free = NULL;
}
internal void phys_pooled_array_release(PHYS_PooledArray* pa) {
    os_deallocate(pa->v);
    os_deallocate(pa->versions);
    arena_release(pa->free_arena);
}
internal void phys_pooled_array_adjust_allocation(PHYS_PooledArray* pa) {
    void* new_v = os_allocate(pa->capacity*pa->item_size);
    memcpy(new_v, pa->v, pa->length*pa->item_size);
    os_deallocate(pa->v);
    pa->v = new_v;

    s32* new_versions = (s32*)os_allocate(pa->capacity*sizeof(*pa->versions));
    memcpy(new_versions, pa->versions, pa->length*sizeof(*pa->versions));
    os_deallocate(pa->versions);
    pa->versions = new_versions;
}

internal PHYS_pooled_array_id phys_pooled_array_add(PHYS_PooledArray* pa) {
    PHYS_pooled_array_id new_id;
    if (pa->free != NULL) {
        new_id.idx = pa->free->v;
        new_id.version = -pa->versions[pa->free->v];
        stack_pop(pa->free);
        arena_pop(pa->free_arena, sizeof(*pa->free));
    } else {
        new_id.idx = pa->length;
        new_id.version = 1;
        pa->length++;
    
        // @todo reserve large vaddress space and commit pages as needed
        if (pa->length > pa->capacity) {
            pa->capacity *= pa->growth;
            phys_pooled_array_adjust_allocation(pa);
        }
    }

    pa->versions[new_id.idx] = new_id.version;
    
    return new_id;
}
internal void phys_pooled_array_remove(PHYS_PooledArray* pa, PHYS_pooled_array_id id) {
    Assert(id.idx >= 0 && id.idx < pa->length);
    pa->versions[id.idx] = -(pa->versions[id.idx] + 1);
    
    // @todo mask off usage to decrease size if possible?
    if (id.idx == pa->length-1) {
        pa->length--;
        return;
    }

    PHYS_U32Node* n = push_array(pa->free_arena, PHYS_U32Node, 1);
    n->v = id.idx;
    stack_push(pa->free, n);
}

// body api
shared_function PHYS_body_id phys_world_add_body(PHYS_World* w, PHYS_Body body) {
    // initialise rotation if not set
    if (dot_4f32(body.rotation, body.rotation) < EPSILON_F32) {
        body.rotation = make_identity_quat();
    }

    PHYS_pooled_array_id pa_id = phys_pooled_array_add(&w->bodies);
    PHYS_Body* v = (PHYS_Body*)w->bodies.v;
    v[pa_id.idx] = body;
    return (PHYS_body_id){.idx=pa_id.idx, .version=(u32)pa_id.version};
}
shared_function void phys_world_remove_body(PHYS_World* w, PHYS_body_id id) {
    phys_pooled_array_remove(&w->bodies, (PHYS_pooled_array_id){.idx=id.idx, .version=(s32)id.version});
}
shared_function PHYS_Body* phys_world_resolve_body_unchecked(PHYS_World* w, PHYS_body_id id) {
    Assert(id.idx >= 0 && id.idx < w->bodies.length);
    Assert(w->bodies.versions[id.idx] == id.version);
    return &((PHYS_Body*)w->bodies.v)[id.idx];
}
shared_function PHYS_Body* phys_world_resolve_body(PHYS_World* w, PHYS_body_id id) {
    Assert(id.idx >= 0 && id.idx < w->bodies.length);
    if (w->bodies.versions[id.idx] != id.version) {
        return NULL;
    }
    return &((PHYS_Body*)w->bodies.v)[id.idx];
}

// collider api
shared_function PHYS_collider_id phys_world_add_collider(PHYS_World* w, PHYS_Collider collider) {
    Assert(phys_world_valid_radius(w, collider.base.r));
    if (phys_collider_layers_equal(collider.base.layer, PHYS_ColliderLayer_Invalid))
        collider.base.layer = PHYS_ColliderLayer_All; // @todo layer context?

    PHYS_pooled_array_id pa_id = phys_pooled_array_add(&w->colliders);
    PHYS_Collider* v = (PHYS_Collider*)w->colliders.v;
    v[pa_id.idx] = collider;
    return (PHYS_collider_id){.idx=pa_id.idx, .version=(u32)pa_id.version};
}
shared_function void phys_world_remove_collider(PHYS_World* w, PHYS_collider_id id) {
    phys_pooled_array_remove(&w->colliders, (PHYS_pooled_array_id){.idx=id.idx, .version=(s32)id.version});
}
shared_function PHYS_Collider* phys_world_resolve_collider_unchecked(PHYS_World* w, PHYS_collider_id id) {
    Assert(id.idx >= 0 && id.idx < w->colliders.length);
    Assert(w->colliders.versions[id.idx] == id.version);
    return &((PHYS_Collider*)w->colliders.v)[id.idx];
}
shared_function PHYS_Collider* phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id id) {
    Assert(id.idx >= 0 && id.idx < w->colliders.length);
    if (w->colliders.versions[id.idx] != id.version) {
        return NULL;
    }
    return &((PHYS_Collider*)w->colliders.v)[id.idx];
}

// constraint api
shared_function PHYS_constraint_id phys_world_add_constraint(PHYS_World* w, PHYS_Constraint constraint) {
    PHYS_pooled_array_id pa_id = phys_pooled_array_add(&w->constraints);
    PHYS_Constraint* v = (PHYS_Constraint*)w->constraints.v;
    v[pa_id.idx] = constraint;
    return (PHYS_constraint_id){.idx=pa_id.idx, .version=(u32)pa_id.version};
}
shared_function void phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id id) {
    phys_pooled_array_remove(&w->constraints, (PHYS_pooled_array_id){.idx=id.idx, .version=(s32)id.version});
}
shared_function PHYS_Constraint* phys_world_resolve_constraint_unchecked(PHYS_World* w, PHYS_constraint_id id) {
    Assert(id.idx >= 0 && id.idx < w->constraints.length);
    Assert(w->constraints.versions[id.idx] == id.version);
    return &((PHYS_Constraint*)w->constraints.v)[id.idx];
}
shared_function PHYS_Constraint* phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id id) {
    Assert(id.idx >= 0 && id.idx < w->constraints.length);
    if (w->constraints.versions[id.idx] != id.version) {
        return NULL;
    }
    return &((PHYS_Constraint*)w->constraints.v)[id.idx];
}

// dependent constraint api
shared_function PHYS_dependent_constraint_id phys_world_add_dependent_constraint(PHYS_World* w, PHYS_DependentConstraint dependent_constraint) {
    PHYS_pooled_array_id pa_id = phys_pooled_array_add(&w->dependent_constraints);
    PHYS_DependentConstraint* v = (PHYS_DependentConstraint*)w->dependent_constraints.v;
    v[pa_id.idx] = dependent_constraint;
    return (PHYS_dependent_constraint_id){.idx=pa_id.idx, .version=(u32)pa_id.version};
}
shared_function void phys_world_remove_dependent_constraint(PHYS_World* w, PHYS_dependent_constraint_id id) {
    phys_pooled_array_remove(&w->dependent_constraints, (PHYS_pooled_array_id){.idx=id.idx, .version=(s32)id.version});
}
shared_function PHYS_DependentConstraint* phys_world_resolve_dependent_constraint_unchecked(PHYS_World* w, PHYS_dependent_constraint_id id) {
    Assert(id.idx >= 0 && id.idx < w->dependent_constraints.length);
    Assert(w->dependent_constraints.versions[id.idx] == id.version);
    return &((PHYS_DependentConstraint*)w->dependent_constraints.v)[id.idx];
}
shared_function PHYS_DependentConstraint* phys_world_resolve_dependent_constraint(PHYS_World* w, PHYS_dependent_constraint_id id) {
    Assert(id.idx >= 0 && id.idx < w->dependent_constraints.length);
    if (w->dependent_constraints.versions[id.idx] != id.version) {
        return NULL;
    }
    return &((PHYS_DependentConstraint*)w->dependent_constraints.v)[id.idx];
}

// asserts
internal b32 phys_world_valid_radius(PHYS_World* w, f32 d) {
    if (w->min_r) {
        return d >= w->min_r;
    }
    return true;
}

// layers
shared_function b32 phys_collider_layers_overlap(PHYS_ColliderLayer l1, PHYS_ColliderLayer l2) {
    return (l1.group&l2.mask) && (l2.group&l1.mask);
}
shared_function b32 phys_collider_layers_equal(PHYS_ColliderLayer l1, PHYS_ColliderLayer l2) {
    return l1.v == l2.v;
}