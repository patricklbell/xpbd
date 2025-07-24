// constraints
void phys_constrain_distance(PHYS_Constraint_Distance* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c->b1);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c->b2);

    vec3_f32 d = sub_3f32(b2->position, b1->position);
    f32 d_length = length_3f32(d);
    f32 C = d_length - c->d;
    vec3_f32 dC = mul_3f32(d, 1.f/d_length);
    f32 a_on_dt2 = settings.inv_dt2*c->compliance;
    f32 l = 1.f / (b1->inv_mass + b2->inv_mass + a_on_dt2);
    
    b1->position = add_3f32(b1->position, mul_3f32(dC, +C*b1->inv_mass*l));
    b2->position = add_3f32(b2->position, mul_3f32(dC, -C*b2->inv_mass*l));
}

void phys_constrain_volume(PHYS_Constraint_Volume* c, PHYS_ConstraintSolveSettings settings) {
    NotImplemented;
}

// colliders
static void phys_collide_spheres(const PHYS_Collider_Sphere* c1, PHYS_Collider_Sphere* c2, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->c);

    vec3_f32 d = sub_3f32(b2->position, b1->position);
    f32 d_length = length_3f32(d);
    if (d_length >= c1->r + c2->r) return;

    f32 C = d_length - (c1->r + c2->r);
    vec3_f32 dC = mul_3f32(d, 1.f/d_length);
    f32 a_on_dt2 = settings.inv_dt2*(c1->compliance + c2->compliance); // @todo physical correctness?
    f32 l = 1.f / (b1->inv_mass + b2->inv_mass + a_on_dt2);
    
    b1->position = add_3f32(b1->position, mul_3f32(dC, +C*b1->inv_mass*l));
    b2->position = add_3f32(b2->position, mul_3f32(dC, -C*b2->inv_mass*l));
}

static void phys_collide_sphere_with_plane(const PHYS_Collider_Sphere* c1, PHYS_Collider_Plane* c2, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->p);

    f32 d_length = dot_3f32(sub_3f32(b1->position, b2->position), c2->n);
    if (d_length >= c1->r) return;

    f32 C = d_length - c1->r;
    vec3_f32 dC = c2->n;
    f32 a_on_dt2 = settings.inv_dt2*(c1->compliance + c2->compliance); // @todo physical correctness?
    f32 l = 1.f / (b1->inv_mass + a_on_dt2);

    b1->position = add_3f32(b1->position, mul_3f32(dC, -C*b1->inv_mass*l));
}

// world
PHYS_World* phys_world_make() {
    Arena* arena = arena_alloc();
    PHYS_World* w = push_array(arena, PHYS_World, 1);

    *w = (PHYS_World){
        .arena = arena,
        .substeps = 16,
        .little_g = -10.f,
        .colliders = (PHYS_ColliderMap){
            .slots = push_array(arena, PHYS_ColliderNode*, PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT),
            .slots_count = PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT,
            .free_chain = NULL
        },
        .constraints = (PHYS_ConstraintMap){
            .slots = push_array(arena, PHYS_ConstraintNode*, PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT),
            .slots_count = PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT,
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
        // don't compute delta for objects with infinite inertia
        if (b->inv_mass != 0.f) {
            // save the position and rotation before forces and constraints
            b->prev_position = b->position;
            b->prev_rotation = b->rotation;
        }

        // apply torques & linear forces
        if (!b->no_gravity) {
            b->linear_velocity = add_3f32(b->linear_velocity, mul_3f32(a_gravity, dt));
        }

        // add velocities
        vec3_f32 dp = mul_3f32(b->linear_velocity, dt);
        vec4_f32 dr = mul_4f32(mul_quat(make_quat_axis(b->angular_velocity), b->rotation), 0.5f*dt);
        b->position = add_3f32(b->position, dp);
        b->rotation = normalize_4f32(add_4f32(b->rotation, dr));
    }

    // step 2: solve constraints (including collisions)
    PHYS_ConstraintSolveSettings settings = (PHYS_ConstraintSolveSettings){
        .w = w,
        .inv_dt2 = inv_dt*inv_dt,
    };
    for EachIndex(slot, w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            PHYS_Constraint* constraint = &constraint_n->v;

            switch (constraint->type) {
                case PHYS_ConstraintType_Distance: {
                    phys_constrain_distance(&constraint->distance, settings);
                    break;
                }
                case PHYS_ConstraintType_Volume: {
                    phys_constrain_volume(&constraint->volume, settings);
                    break;
                }
            }
        }
    }
    for EachIndex(sloti, w->colliders.slots_count) {
        for EachList(collideri_n, PHYS_ColliderNode, w->colliders.slots[sloti]) {
            PHYS_Collider* collideri = &collideri_n->v;

            // @todo collision query acceleration structure
            for EachIndex(slotj, w->colliders.slots_count) {
                for EachList(colliderj_n, PHYS_ColliderNode, w->colliders.slots[slotj]) {
                    PHYS_Collider* colliderj = &colliderj_n->v;
                    if (collideri == colliderj)
                        continue;

                    if (
                        collideri->type == PHYS_ColliderType_Sphere &&
                        colliderj->type == PHYS_ColliderType_Plane
                    ) {
                        PHYS_Collider_Sphere* sphere = &collideri->sphere;
                        PHYS_Collider_Plane* plane = &colliderj->plane;
                        phys_collide_sphere_with_plane(sphere, plane, settings);
                    } else if (
                        collideri->type == PHYS_ColliderType_Sphere &&
                        colliderj->type == PHYS_ColliderType_Sphere
                    ) {
                        PHYS_Collider_Sphere* sphere_1 = &collideri->sphere;
                        PHYS_Collider_Sphere* sphere_2 = &colliderj->sphere;
                        phys_collide_spheres(sphere_1, sphere_2, settings);
                    }
                }
            }
        }
    }

    // step 3: set linear & angular velocities to resultant velocity
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        if (b->inv_mass != 0.f) {
            vec3_f32 dp = sub_3f32(b->position, b->prev_position);
            vec4_f32 dr = mul_quat(b->rotation, inv_quat(b->prev_rotation));
            b->linear_velocity = mul_3f32(dp, inv_dt);
            b->angular_velocity = mul_3f32(dr.xyz, 2.0*inv_dt*sgn_f32(dr.w));
        }
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

// collider api
PHYS_collider_id phys_world_add_collider(PHYS_World* w, PHYS_Collider c) {
    PHYS_ColliderNode* new_node;
    
    if (w->colliders.free_chain != NULL) {
        new_node = w->colliders.free_chain;
        stack_pop(w->colliders.free_chain);
    } else {
        u32 new_id = w->colliders.max_id;
        w->colliders.max_id++;
        u32 new_slot = new_id % w->colliders.slots_count;
        
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
    u32 slot = id % w->colliders.slots_count;
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

// constraint
PHYS_constraint_id phys_world_add_constraint(PHYS_World* w, PHYS_Constraint c) {
    PHYS_ConstraintNode* new_node;
    
    if (w->constraints.free_chain != NULL) {
        new_node = w->constraints.free_chain;
        stack_pop(w->constraints.free_chain);
    } else {
        u32 new_id = w->constraints.max_id;
        w->constraints.max_id++;
        u32 new_slot = new_id % w->constraints.slots_count;
        
        new_node = push_array(w->arena, PHYS_ConstraintNode, 1);
        stack_push(w->constraints.slots[new_slot], new_node);
    }
    Assert(new_node != NULL);

    new_node->v = c;
    return (PHYS_constraint_id){
        .id = new_node->id,
        .version = new_node->version,
    };
}
static PHYS_ConstraintNode* phys_world_resolve_constraint_node(PHYS_World* w, u32 id) {
    u32 slot = id % w->constraints.slots_count;
    for EachList(n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
        if (n->id == id) {
            return n;
        }
    }
    return NULL;
}
void phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id col) {
    PHYS_ConstraintNode* n = phys_world_resolve_constraint_node(w, col.id);
    Assert(n != NULL);

    // invalidate old ids by increasing version
    n->version++;
    // add node to free chain for reuse
    stack_push(w->constraints.free_chain, n);
}
PHYS_Constraint* phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id col) {
    PHYS_ConstraintNode* n = phys_world_resolve_constraint_node(w, col.id);
    if (n != NULL && n->version != col.version) {
        return NULL;
    }
    return &n->v;
}

// helper objects
PHYS_RigidBody phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings){
    Assert(settings.mass > 0.f);
    PHYS_body_id center = phys_world_add_body(w, (PHYS_Body){
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .inv_mass = 1.f / settings.mass,
    });
    PHYS_collider_id sphere = phys_world_add_collider(w, (PHYS_Collider){
        .type = PHYS_ColliderType_Sphere,
        .sphere = (PHYS_Collider_Sphere){
            .compliance = settings.compliance,
            .c = center,
            .r = settings.radius,
        }
    });

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = sphere,
    };
}
PHYS_RigidBody phys_world_add_box(PHYS_World* w, PHYS_Box_Settings settings){
    Assert(settings.mass > 0.f);

    PHYS_body_id center = phys_world_add_body(w, (PHYS_Body){
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .angular_velocity = settings.angular_velocity,
        .inv_mass = 1.f / settings.mass,
        .inv_moment = phys_inv_moment_rect_cuboid(mul_3f32(settings.extents, 2.0), settings.mass),
    });
    PHYS_collider_id rect_cuboid = phys_world_add_collider(w, (PHYS_Collider){
        .type = PHYS_ColliderType_RectCuboid,
        .rect_cuboid = (PHYS_Collider_RectCuboid){
            .compliance = settings.compliance,
            .c = center,
            .r = settings.extents,
        }
    });

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = rect_cuboid,
    };
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

            box.positions[i] = phys_world_add_body(w, (PHYS_Body){
                .position = position,
                .no_gravity = 1,
                .inv_mass = 0.f,
            });
            box.areas[i] = phys_world_add_collider(w, (PHYS_Collider){
                .type = PHYS_ColliderType_Plane,
                .plane = (PHYS_Collider_Plane){
                    .compliance = 0.f,
                    .p = box.positions[i],
                    .n = normal,
                }
            });
            i++;
        }
    }

    return box;
}

void phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object) {
    phys_world_remove_collider(w, object->collider_id);
    phys_world_remove_body(w, object->body_id);
}
void phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary* object){
    for EachElement(i, object->areas) {
        phys_world_remove_collider(w, object->areas[i]);
        phys_world_remove_body(w, object->positions[i]);
    }
}