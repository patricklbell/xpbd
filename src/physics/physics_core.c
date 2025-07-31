// helpers
static f32 phys_body_generalized_inverse_mass(PHYS_Body* b, vec3_f32 r, vec3_f32 dC) {
    // direction of torque
    vec3_f32 nt = rot_quat(cross_3f32(r, dC), inv_quat(b->rotation));
    return b->inv_mass + dot_3f32(nt, mul_3x3f32(b->inv_inertia, nt));
}

static void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 corr) {
    // apply position correction
    vec3_f32 dp = mul_3f32(corr, b->inv_mass);
    b->position = add_3f32(b->position, dp);
}
static void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r) {
    // torque in inertial frame
    vec3_f32 t = rot_quat(cross_3f32(r, corr), inv_quat(b->rotation));
    // delta angle in world frame
    vec3_f32 dw = rot_quat(mul_3x3f32(b->inv_inertia, t), b->rotation);
    // apply rotation correction
    vec4_f32 dr = mul_quat(make_axis_quat(dw), b->rotation);
    b->rotation = normalize_4f32(add_4f32(b->rotation, mul_4f32(dr, 0.5f)));
}

// @note assumes dC is normalized
static void phys_2body_apply_correction_wo_offset(PHYS_Body* b1, PHYS_Body* b2, f32 alpha, f32 C, vec3_f32 dC, f32* l_out) {
    f32 w1 = b1->inv_mass;
    f32 w2 = b2->inv_mass;
    if (w1 + w2 == 0.f) return;
    *l_out = C / (w1 + w2 + alpha);

    vec3_f32 corr1 = mul_3f32(dC, -(*l_out));
    vec3_f32 corr2 = mul_3f32(dC, +(*l_out));
    
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
}

// @note assumes dC is normalized
static void phys_2body_apply_correction_wt_offset(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, f32 alpha, f32 C, vec3_f32 dC, f32* l_out) {
    f32 w1 = phys_body_generalized_inverse_mass(b1, r1, dC);
    f32 w2 = phys_body_generalized_inverse_mass(b2, r2, dC);
    if (w1 + w2 == 0.f) return;
    *l_out = C / (w1 + w2 + alpha);
    
    vec3_f32 corr1 = mul_3f32(dC, -(*l_out));
    vec3_f32 corr2 = mul_3f32(dC, +(*l_out));

    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    phys_body_apply_angular_correction(b1, corr1, r1);
    phys_body_apply_angular_correction(b2, corr2, r2);
}

// solvers
void phys_constrain_distance(PHYS_Constraint_Distance* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c->b1);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c->b2);

    vec3_f32 r1, r2;
    if (c->is_offset) {
        r1 = rot_quat(c->offset1, b1->rotation);
        r2 = rot_quat(c->offset2, b2->rotation);
    }

    vec3_f32 d = sub_3f32(
        c->is_offset ? add_3f32(b1->position, r1) : b1->position,
        c->is_offset ? add_3f32(b2->position, r2) : b2->position
    );
    f32 d_length = length_3f32(d);
    if (c->unilateral && d_length < c->d) return;
    f32 C = d_length - c->d;
    if (C == 0.f) return;
    vec3_f32 dC = mul_3f32(d, 1.f/d_length);
    f32 alpha = settings.inv_dt2*c->compliance;

    f32 l;
    if (c->is_offset) {
        phys_2body_apply_correction_wt_offset(b1, b2, r1, r2, alpha, C, dC, &l);
    } else {
        phys_2body_apply_correction_wo_offset(b1, b2, alpha, C, dC, &l);
    }

    c->force = abs_f32(l) * settings.inv_dt;
}

void phys_constrain_volume(PHYS_Constraint_Volume* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* p1 = phys_world_resolve_body(settings.w, c->p[0]);
    PHYS_Body* p2 = phys_world_resolve_body(settings.w, c->p[1]);
    PHYS_Body* p3 = phys_world_resolve_body(settings.w, c->p[2]);
    PHYS_Body* p4 = phys_world_resolve_body(settings.w, c->p[3]);

    vec3_f32 d21 = sub_3f32(p2->position, p1->position);
    vec3_f32 d31 = sub_3f32(p3->position, p1->position);
    vec3_f32 d41 = sub_3f32(p4->position, p1->position);
    vec3_f32 d32 = sub_3f32(p3->position, p2->position);
    vec3_f32 d42 = sub_3f32(p4->position, p2->position);
    vec3_f32 d43 = sub_3f32(p4->position, p3->position);

    vec3_f32 dC1 = cross_3f32(d42, d32);
    vec3_f32 dC2 = cross_3f32(d31, d41);
    vec3_f32 dC3 = cross_3f32(d41, d21);
    vec3_f32 dC4 = cross_3f32(d21, d31);

    f32 v = phys_tet_volume_axis(d21, d31, d41);
    f32 C = v - c->v_rest;
    if (C == 0.f) return;

    f32 alpha = settings.inv_dt2*c->compliance;
    f32 w  = p1->inv_mass*dot_3f32(dC1, dC1);
        w += p2->inv_mass*dot_3f32(dC2, dC2);
        w += p3->inv_mass*dot_3f32(dC3, dC3);
        w += p4->inv_mass*dot_3f32(dC4, dC4);
    if (w == 0.f) return;
    f32 l = C / (w + alpha);

    vec3_f32 corr1 = mul_3f32(dC1, -l);
    vec3_f32 corr2 = mul_3f32(dC2, -l);
    vec3_f32 corr3 = mul_3f32(dC3, -l);
    vec3_f32 corr4 = mul_3f32(dC4, -l);
    
    phys_body_apply_linear_correction(p1, corr1);
    phys_body_apply_linear_correction(p2, corr2);
    phys_body_apply_linear_correction(p3, corr3);
    phys_body_apply_linear_correction(p4, corr4);

    c->force = abs_f32(l) * settings.inv_dt;
}

// colliders
static void phys_collide_spheres(const PHYS_Collider_Sphere* c1, PHYS_Collider_Sphere* c2, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->c);

    vec3_f32 d = sub_3f32(b1->position, b2->position);
    f32 d_length = length_3f32(d);
    if (d_length >= c1->r + c2->r) return;

    f32 C = d_length - (c1->r + c2->r);
    vec3_f32 dC = mul_3f32(d, 1.f/d_length);
    f32 compliance = c1->compliance + c2->compliance;
    f32 alpha = settings.inv_dt2*compliance;

    f32 l;
    phys_2body_apply_correction_wo_offset(b1, b2, alpha, C, dC, &l);

    if (compliance == 0.f) {
        b1->prev_position = b1->position;
        b2->prev_position = b2->position;
    }
}

static void phys_collide_sphere_with_plane(const PHYS_Collider_Sphere* c1, PHYS_Collider_Plane* c2, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->c);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->p);

    f32 d_length = dot_3f32(sub_3f32(b1->position, b2->position), c2->n);
    if (d_length >= c1->r) return;

    f32 C = d_length - c1->r;
    vec3_f32 dC = c2->n;
    f32 compliance = c1->compliance + c2->compliance;
    f32 alpha = settings.inv_dt2*compliance;

    f32 l;
    phys_2body_apply_correction_wo_offset(b1, b2, alpha, C, dC, &l);

    if (compliance == 0.f) {
        b1->prev_position = b1->position;
        b2->prev_position = b2->position;
    }
}

static void phys_collide_triangle_with_plane(const PHYS_Collider_Triangle* c1, PHYS_Collider_Plane* c2, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* p = phys_world_resolve_body(settings.w, c2->p);
    
    vec3_f32 dC = c2->n;
    f32 compliance = c1->compliance + c2->compliance;
    f32 alpha = settings.inv_dt2*compliance;

    for EachElement(i, c1->p) {
        PHYS_Body* v = phys_world_resolve_body(settings.w, c1->p[i]);

        f32 C = dot_3f32(sub_3f32(v->position, p->position), c2->n);
        if (C >= 0.f) continue;

        f32 l;
        phys_2body_apply_correction_wo_offset(v, p, alpha, C, dC, &l);
    
        if (compliance == 0.f) {
            v->prev_position = v->position;
        }
    }
}

// world
PHYS_World* phys_world_make(PHYS_WorldSettings settings) {
    Arena* arena = arena_alloc();
    PHYS_World* w = push_array(arena, PHYS_World, 1);

    *w = (PHYS_World){
        .arena = arena,
        .substeps = (!settings.substeps) ? 16 : settings.substeps,
        .little_g = (!settings.little_g) ? -10 : settings.little_g,
        .damping  = settings.damping,
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
        // don't compute delta for objects with infinite mass
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
        b->position = add_3f32(b->position, dp);
        vec4_f32 dr = mul_4f32(mul_quat(make_axis_quat(b->angular_velocity), b->rotation), dt);
        b->rotation = normalize_4f32(add_4f32(b->rotation, mul_4f32(dr, 0.5f)));
    }

    // step 2: solve constraints (including collisions)
    PHYS_ConstraintSolveSettings settings = (PHYS_ConstraintSolveSettings){
        .w = w,
        .inv_dt = inv_dt,
        .inv_dt2 = inv_dt*inv_dt,
    };
    // @todo collision query acceleration structure
    for EachIndex(sloti, w->colliders.slots_count) {
        for EachList(collideri_n, PHYS_ColliderNode, w->colliders.slots[sloti]) {
            PHYS_Collider* collideri = &collideri_n->v;
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
                    } else if (
                        collideri->type == PHYS_ColliderType_Triangle &&
                        colliderj->type == PHYS_ColliderType_Plane
                    ) {
                        PHYS_Collider_Triangle* triangle = &collideri->triangle;
                        PHYS_Collider_Plane* plane = &colliderj->plane;
                        phys_collide_triangle_with_plane(triangle, plane, settings);
                    }
                }
            }
        }
    }
    for EachIndex(slot, w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            PHYS_Constraint* constraint = &constraint_n->v;

            switch (constraint->type) {
                case PHYS_ConstraintType_Distance: {
                    phys_constrain_distance(&constraint->distance, settings);
                }break;
                case PHYS_ConstraintType_Volume: {
                    phys_constrain_volume(&constraint->volume, settings);
                }break;
            }
        }
    }

    // step 3: set linear & angular velocities to resultant velocity
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        if (b->inv_mass != 0.f) {
            vec3_f32 dp = sub_3f32(b->position, b->prev_position);
            vec4_f32 dr = mul_quat(b->rotation, inv_quat(b->prev_rotation));
            b->linear_velocity = mul_3f32(dp, inv_dt*Max(1.f - w->damping*dt, 0.f));
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

    // initialise rotation if not set
    if (dot_4f32(b.rotation, b.rotation) < FLT_EPSILON) {
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
        u32 new_id = w->colliders.max_id;
        w->colliders.max_id++;
        u32 new_slot = new_id % w->colliders.slots_count;
        
        new_node = push_array(w->arena, PHYS_ColliderNode, 1);
        new_node->id = new_id;
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
        new_node->id = new_id;
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