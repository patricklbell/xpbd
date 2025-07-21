// constraints
static void phys_constrain_static(PHYS_Constraint_StaticDistance c, PHYS_ConstraintSettings settings) {
    NotImplemented;
}

void phys_constrain_distance(PHYS_Constraint_Distance c, PHYS_ConstraintSettings settings) {
    NotImplemented;
}

void phys_constrain_volume(PHYS_Constraint_Volume c, PHYS_ConstraintSettings settings) {
    NotImplemented;
}

// colliders
static void phys_collide_dynamic_spheres(const PHYS_Collider_DynamicSphere* c1, PHYS_Collider_DynamicSphere* c2, PHYS_ConstraintSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->c);

    vec3_f32 d = sub_3f32(b2->position, b1->position);
    f32 d_length = length_3f32(d);
    if (d_length >= c1->r + c2->r) return;

    f32 c = d_length - (c1->r + c2->r);
    vec3_f32 dc = mul_3f32(d, 1.f/d_length);
    f32 l = 1.f / (b1->inv_mass + b2->inv_mass + settings.a_dt2);
    
    b1->position = add_3f32(b1->position, mul_3f32(dc, +c*b1->inv_mass*l));
    b2->position = add_3f32(b2->position, mul_3f32(dc, -c*b2->inv_mass*l));
}

static void phys_collide_dynamic_sphere_with_static_plane(const PHYS_Collider_DynamicSphere* c1, PHYS_Collider_StaticPlane* c2, PHYS_ConstraintSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);

    f32 d_length = dot_3f32(sub_3f32(b1->position, c2->p), c2->n);
    if (d_length >= c1->r) return;

    f32 c = d_length - c1->r;
    f32 l = 1.f / (b1->inv_mass + settings.a_dt2);

    b1->position = add_3f32(b1->position, mul_3f32(c2->n, -c*b1->inv_mass*l));
}

// world
PHYS_World* phys_world_make() {
    Arena* arena = arena_alloc();
    PHYS_World* w = push_array(arena, PHYS_World, 1);

    *w = (PHYS_World){
        .arena = arena,
        .substeps = 32,
        .little_g = -10.f,
        .compliance = 0.001f,
        .colliders = (PHYS_ColliderMap){
            .slots = push_array(arena, PHYS_ColliderNode*, PHYS_COLLIDER_MAP_NUM_SLOTS),
            .num_slots = PHYS_COLLIDER_MAP_NUM_SLOTS,
            .free_chain = NULL
        },
        .constraints = (PHYS_ConstraintMap){
            .slots = push_array(arena, PHYS_ConstraintNode*, PHYS_CONSTRAINT_MAP_NUM_SLOTS),
            .num_slots = PHYS_CONSTRAINT_MAP_NUM_SLOTS,
            .free_chain = NULL
        },
        .bodies = (PHYS_BodyDynamicArray){
            .v = (PHYS_Body*)os_allocate(PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY*sizeof(PHYS_Body)),
            .length = 0,
            .capacity = PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY,
        }
    };

    return w;
}

void phys_world_cleanup(PHYS_World* w) {
    os_deallocate(w->bodies.v);
    arena_release(w->arena);
}

// xpbd with substepping splits physics step into substeps which solve constraints
// n times. Each steps consists of:
//      - apply forces and update position,
//      - solve constraints on positions,
//      - determine linear & angular velocity from delta after constraints 
//        have been applied.
void phys_world_step(PHYS_World* w, f64 dt) {
    f64 sdt = dt / (f64)w->substeps;
    for (u64 i = 0; i < w->substeps; ++i) {
        phys_world_substep(w, sdt);
    }
}

static void phys_world_substep(PHYS_World* w, f64 dt) {
    f64 inv_dt = 1.f / dt;

    // step 1: apply external forces
    const vec3_f32 a_gravity = {.x = 0, .y = w->little_g, .z = 0};
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        // save the position before forces and constraints
        b->prev_position = b->position;
    
        // apply forces and update positions
        if (!b->no_gravity)
            b->velocity = add_3f32(b->velocity, mul_3f32(a_gravity, dt));
        b->position = add_3f32(b->position, mul_3f32(b->velocity, dt));
    }

    // step 2: solve constraints (including collisions)
    f64 a_dt2 = w->compliance*inv_dt*inv_dt; // stiffness term;
    PHYS_ConstraintSettings settings = (PHYS_ConstraintSettings){
        .w = w,
        .a_dt2 = a_dt2,
    };

    for EachIndex(slot, w->constraints.num_slots) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            PHYS_Constraint* constraint = &constraint_n->v;

            switch (constraint->type) {
                case PHYS_ConstraintType_StaticDistance: {
                    phys_constrain_static(constraint->static_distance, settings);
                    break;
                }
                case PHYS_ConstraintType_Distance: {
                    phys_constrain_distance(constraint->distance, settings);
                    break;
                }
                case PHYS_ConstraintType_Volume: {
                    phys_constrain_volume(constraint->volume, settings);
                    break;
                }
            }
        }
    }
    for EachIndex(sloti, w->colliders.num_slots) {
        for EachList(collideri_n, PHYS_ColliderNode, w->colliders.slots[sloti]) {
            PHYS_Collider* collideri = &collideri_n->v;

            // @todo collision query acceleration structure
            for EachIndex(slotj, w->colliders.num_slots) {
                for EachList(colliderj_n, PHYS_ColliderNode, w->colliders.slots[slotj]) {
                    PHYS_Collider* colliderj = &colliderj_n->v;
                    if (collideri == colliderj)
                        continue;

                    if (
                        collideri->type == PHYS_ColliderType_DynamicSphere &&
                        colliderj->type == PHYS_ColliderType_StaticPlane
                    ) {
                        PHYS_Collider_DynamicSphere* dynamic_sphere = &collideri->dynamic_sphere;
                        PHYS_Collider_StaticPlane* static_plane = &colliderj->static_plane;
                        phys_collide_dynamic_sphere_with_static_plane(dynamic_sphere, static_plane, settings);
                    } else if (
                        collideri->type == PHYS_ColliderType_DynamicSphere &&
                        colliderj->type == PHYS_ColliderType_DynamicSphere
                    ) {
                        PHYS_Collider_DynamicSphere* dynamic_sphere_1 = &collideri->dynamic_sphere;
                        PHYS_Collider_DynamicSphere* dynamic_sphere_2 = &colliderj->dynamic_sphere;
                        phys_collide_dynamic_spheres(dynamic_sphere_1, dynamic_sphere_2, settings);
                    }
                }
            }
        }
    }

    // step 3: set linear velocities to resultant velocity
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        b->velocity = mul_3f32(sub_3f32(b->position, b->prev_position), inv_dt);
    }
}

static void phys_bodies_adjust_allocation(PHYS_World* w) {
    PHYS_Body* new_v = (PHYS_Body*)os_allocate(w->bodies.capacity*sizeof(PHYS_Body));
    memcpy(new_v, w->bodies.v, w->bodies.capacity*sizeof(PHYS_Body));
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

PHYS_collider_id phys_world_add_collider(PHYS_World* w, PHYS_Collider c) {
    PHYS_ColliderNode* new_node;
    
    if (w->colliders.free_chain != NULL) {
        new_node = w->colliders.free_chain;
        stack_pop(w->colliders.free_chain);
    } else {
        u32 new_id = w->colliders.max_id;
        w->colliders.max_id++;
        u32 new_slot = new_id % w->colliders.num_slots;
        
        new_node = push_array(w->arena, PHYS_ColliderNode, 1);
        stack_push(w->colliders.slots[new_slot], new_node);
    }
    Assert(new_node != NULL);

    new_node->v = c;
    return (PHYS_collider_id){
        .id = new_node->id,
        .version = new_node->version,
    };
}
static PHYS_ColliderNode* phys_world_resolve_collider_node(PHYS_World* w, u32 id) {
    u32 slot = id % w->colliders.num_slots;
    for EachList(n, PHYS_ColliderNode, w->colliders.slots[slot]) {
        if (n->id == id) {
            return n;
        }
    }
    return NULL;
}
void phys_world_remove_collider(PHYS_World* w, PHYS_collider_id col) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, col.id);
    Assert(n != NULL);

    // invalidate old ids by increasing version
    n->version++;
    // add node to free chain for reuse
    stack_push(w->colliders.free_chain, n);
}
PHYS_Collider* phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id col) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, col.id);
    if (n != NULL && n->version != col.version) {
        return NULL;
    }
    return &n->v;
}

// helper objects
PHYS_Ball phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings){
    Assert(settings.mass > 0.f);
    PHYS_body_id center = phys_world_add_body(w, (PHYS_Body){
        .position = settings.center,
        .velocity = settings.velocity,
        .inv_mass = 1.f / settings.mass,
    });
    PHYS_collider_id sphere = phys_world_add_collider(w, (PHYS_Collider){
        .type = PHYS_ColliderType_DynamicSphere,
        .dynamic_sphere = (PHYS_Collider_DynamicSphere){
            .c = center,
            .r = settings.radius,
        }
    });

    return (PHYS_Ball){
        .sphere = sphere,
        .center = center,
    };
}
void phys_world_remove_ball(PHYS_World* w, PHYS_Ball object){
    phys_world_remove_collider(w, object.sphere);
    phys_world_remove_body(w, object.center);
}
PHYS_BoxBoundary phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings){
    PHYS_BoxBoundary box;

    int i = 0;
    for EachIndex(dim, 3) {
        for (int offset = -1; offset <= 1; offset += 2) {
            vec3_f32 normal = zero_struct;
            normal.v[dim] = offset;

            vec3_f32 position = zero_struct;
            position.v[dim] = -offset*settings.extents.v[dim];
            position = add_3f32(settings.center, position);

            box.areas[i] = phys_world_add_collider(w, (PHYS_Collider){
                .type = PHYS_ColliderType_StaticPlane,
                .static_plane = (PHYS_Collider_StaticPlane){
                    .p = position,
                    .n = normal,
                }
            });
            i++;
        }
    }

    return box;
}
void phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary object){
    for EachElement(i, object.areas) {
        phys_world_remove_collider(w, object.areas[i]);
    }
}