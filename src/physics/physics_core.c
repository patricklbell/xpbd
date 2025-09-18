// corrections helpers
static f32 phys_body_inverse_inertia(PHYS_Body* b, vec3_f32 t_world) {
    if (!b->has_inertia) return b->inv_mass*dot_3f32(t_world, t_world);
    // direction of torque
    vec3_f32 nt = rot_quat(t_world, inv_quat(b->rotation));
    return dot_3f32(nt, elmul_3f32(b->inv_inertia, nt));
}
static void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 dp_world) {
    if (b->inv_mass <= 0.f) return;
    b->position = add_3f32(b->position, mul_3f32(dp_world, b->inv_mass));
}
static void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 dt_world) {
    if (!b->has_inertia) return;

    // stabilize rotation
    // dt_world = mul_3f32(dt_world, 0.5f);

    // torque in inertial frame
    vec3_f32 t_inertial = rot_quat(dt_world, inv_quat(b->rotation));
    // delta angle in world frame
    vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, t_inertial), b->rotation);
    // apply rotation correction (linearized approximation)
    vec4_f32 dr = mul_quat(make_axis_quat(dw), b->rotation);
    b->rotation = normalize_4f32(add_4f32(b->rotation, mul_4f32(dr, 0.5f)));
}

// velocity correct helpers
static void phys_body_apply_linear_velocity_correction(PHYS_Body* b, vec3_f32 corr) {
    if (b->inv_mass <= 0.f) return;
    // apply position correction
    b->linear_velocity = add_3f32(b->linear_velocity, mul_3f32(corr, b->inv_mass));
}
static void phys_body_apply_angular_velocity_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r) {
    if (!b->has_inertia) return;
    vec3_f32 t = rot_quat(cross_3f32(r, corr), inv_quat(b->rotation));
    vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, t), b->rotation);
    b->angular_velocity = add_3f32(b->angular_velocity, dw);
}

static f32 phys_lagrange_delta_no_update(f32 C, f32 w, f32 alpha) {
    return -C / (w + alpha);
}

static f32 phys_update_lagrange_multiplier_return_delta(f32 C, f32 w, f32 alpha, f32* l) {
    // f32 dl = (-C - alpha*(*l)) / (w + alpha);
    // using l in calculating dl causes the stiffness to become substep dependent,
    // @todo figure out why xpbd paper results don't match this observation.
    f32 dl = -C / (w + alpha);
    *l += dl;
    return dl;
}

static vec3_f32 phys_body_velocity_at_offset(PHYS_Body* b, vec3_f32 r) {
    return add_3f32(b->linear_velocity, cross_3f32(b->angular_velocity, r));
}

// solvers
static void phys_constraint_solve_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c->distance.b1);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c->distance.b2);

    vec3_f32 r1, r2;
    if (c->distance.is_offset) {
        r1 = rot_quat(c->distance.offset1, b1->rotation);
        r2 = rot_quat(c->distance.offset2, b2->rotation);
    }

    vec3_f32 dx = sub_3f32(
        c->distance.is_offset ? add_3f32(b2->position, r2) : b2->position,
        c->distance.is_offset ? add_3f32(b1->position, r1) : b1->position
    );
    f32 d = length_3f32(dx);
    
    f32 C = c->distance.d - d;
    if (c->distance.unilateral ? (C <= 0.f) : (C == 0.f)) return;    
    vec3_f32 n = (d != 0.f) ? mul_3f32(dx, sgn_f32(C)*1.f/d) : make_3f32(sgn_f32(C),0,0);
    C = abs_f32(C);

    f32 alpha = settings.inv_dt2*c->compliance;

    f32 w  = b1->inv_mass;
        w += b2->inv_mass;
    if (c->distance.is_offset) {
        w += phys_body_inverse_inertia(b1, cross_3f32(r1, n));
        w += phys_body_inverse_inertia(b2, cross_3f32(r2, n));
    }
    if (w <= 0.f) return;

    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, &c->l);

    vec3_f32 corr1 = mul_3f32(n, +dl);
    vec3_f32 corr2 = mul_3f32(n, -dl);
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    if (c->distance.is_offset) {
        phys_body_apply_angular_correction(b1, cross_3f32(r1, corr1));
        phys_body_apply_angular_correction(b2, cross_3f32(r2, corr2));
    }

    c->force = abs_f32(c->l) * settings.inv_dt;
}

static void phys_constraint_solve_volume(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* p1 = phys_world_resolve_body(settings.w, c->volume.p[0]);
    PHYS_Body* p2 = phys_world_resolve_body(settings.w, c->volume.p[1]);
    PHYS_Body* p3 = phys_world_resolve_body(settings.w, c->volume.p[2]);
    PHYS_Body* p4 = phys_world_resolve_body(settings.w, c->volume.p[3]);

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

    f32 v = phys_tetrahedron_volume_axis(d21, d31, d41);
    f32 C = v - c->volume.v_rest;
    if (C == 0.f) return;

    f32 alpha = settings.inv_dt2*c->compliance;
    f32 w  = p1->inv_mass*dot_3f32(dC1, dC1);
        w += p2->inv_mass*dot_3f32(dC2, dC2);
        w += p3->inv_mass*dot_3f32(dC3, dC3);
        w += p4->inv_mass*dot_3f32(dC4, dC4);
    if (w == 0.f) return;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, &c->l);

    vec3_f32 corr1 = mul_3f32(dC1, dl);
    vec3_f32 corr2 = mul_3f32(dC2, dl);
    vec3_f32 corr3 = mul_3f32(dC3, dl);
    vec3_f32 corr4 = mul_3f32(dC4, dl);
    
    phys_body_apply_linear_correction(p1, corr1);
    phys_body_apply_linear_correction(p2, corr2);
    phys_body_apply_linear_correction(p3, corr3);
    phys_body_apply_linear_correction(p4, corr4);

    c->force = abs_f32(c->l) * settings.inv_dt;
}

static void phys_constraint_solve_hinge(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c->hinge.b1);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c->hinge.b2);

    vec3_f32 a1 = rot_quat(c->hinge.a1, b1->rotation);
    vec3_f32 a2 = rot_quat(c->hinge.a2, b2->rotation);

    vec3_f32 dq = cross_3f32(a1, a2);
    f32 C2 = dot_3f32(dq, dq);
    if (C2 == 0.f) return;
    f32 C = sqrt_f32(C2);
    vec3_f32 n = mul_3f32(dq, 1.f/C);

    f32 alpha = settings.inv_dt2*c->compliance;
    f32 w  = b1->inv_mass + phys_body_inverse_inertia(b1, n);
        w += b2->inv_mass + phys_body_inverse_inertia(b2, n);
    if (w <= 0.f) return;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, &c->l);

    vec3_f32 corr1 = mul_3f32(n, +dl);
    vec3_f32 corr2 = mul_3f32(n, -dl);
    phys_body_apply_angular_correction(b1, corr1);
    phys_body_apply_angular_correction(b2, corr2);

    c->torque = mul_3f32(n, abs_f32(c->l) * settings.inv_dt);
}

static void phys_constraint_solve(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    switch (c->type) {
        case PHYS_ConstraintType_Distance: {
            phys_constraint_solve_distance(c, settings);
        }break;
        case PHYS_ConstraintType_Volume: {
            phys_constraint_solve_volume(c, settings);
        }break;
        case PHYS_ConstraintType_Hinge: {
            phys_constraint_solve_hinge(c, settings);
        }break;
    }
}

// colliders 
static void phys_collision_apply_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n, f32 l) {
    vec3_f32 corr1 = mul_3f32(n, +l);
    vec3_f32 corr2 = mul_3f32(n, -l);
    
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    phys_body_apply_angular_correction(b1, cross_3f32(r1, corr1));
    phys_body_apply_angular_correction(b2, cross_3f32(r2, corr2));
}

static void phys_collision_apply_velocity_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n, f32 l) {
    vec3_f32 corr1 = mul_3f32(n, +l);
    vec3_f32 corr2 = mul_3f32(n, -l);
    
    phys_body_apply_linear_velocity_correction(b1, corr1);
    phys_body_apply_linear_velocity_correction(b2, corr2);
    phys_body_apply_angular_velocity_correction(b1, corr1, r1);
    phys_body_apply_angular_velocity_correction(b2, corr2, r2);
}

static f32 phys_collision_generalized_inverse_mass(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n) {
    f32 w1 = b1->inv_mass + phys_body_inverse_inertia(b1, cross_3f32(r1, n));
    f32 w2 = b2->inv_mass + phys_body_inverse_inertia(b2, cross_3f32(r2, n));
    return w1 + w2;
}

static vec3_f32 phys_collision_total_velocity(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2) {
    return sub_3f32(
        phys_body_velocity_at_offset(b1, r1),
        phys_body_velocity_at_offset(b2, r2)
    );
}

b32 phys_collision_check_spheres(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Sphere* s1, PHYS_Collider_Sphere* s2) {
    return phys_contact_point_spheres(
        b1->position, b2->position, s1->base.r, s2->base.r,
        &out->d, &out->r1, &out->r2, &out->n
    );
}

static void phys_collision_SAT_min_max(vec3_f32 axis, PHYS_Body* b, PHYS_Collider* c, f32* min, f32* max) {
    *min = MAX_F32;
    *max = -MAX_F32;
    switch (c->base.type) {
        case PHYS_ColliderType_Sphere: {
            phys_SAT_sphere_min_max(axis, c->base.r, min, max);
        }break;
        case PHYS_ColliderType_Polytope: {
            phys_SAT_polytope_min_max(rot_quat(axis, inv_quat(b->rotation)), c->polytope.points, c->polytope.points_count, min, max);
        }break;
    }
    f32 center = dot_3f32(axis, b->position);
    *min += center;
    *max += center;
}
static b32 phys_collision_SAT_check_axis(PHYS_CollisionCheck* out, vec3_f32 axis, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider* c1, PHYS_Collider* c2) {
    f32 min1, max1, min2, max2;

    phys_collision_SAT_min_max(axis, b1, c1, &min1, &max1);
    phys_collision_SAT_min_max(axis, b2, c2, &min2, &max2);
    
    if (phys_SAT_check_collision_axis(axis, min1, max1, min2, max2, &out->d, &out->n)) {
        // @todo collision manifold
        out->r1 = make_3f32(0.f,0.f,0.f);
        out->r2 = make_3f32(0.f,0.f,0.f);
        return true;
    }

    return false;
}

b32 phys_collision_check_polytopes(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Polytope* p2) {
    out->d = MAX_F32;
    for EachIndex(ni, p1->normals_count) {
        if (!phys_collision_SAT_check_axis(out, rot_quat(p1->normals[ni], b1->rotation), b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)p2))
            return false;
    }
    for EachIndex(ni, p2->normals_count) {
        if (!phys_collision_SAT_check_axis(out, rot_quat(p2->normals[ni], b2->rotation), b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)p2))
            return false;
    }
    for GEO_EachEdge_Ring_Open(u1, v1, u32, p1->indices, p1->indices_count, p1->topology) {
        for GEO_EachEdge_Ring_Open(u2, v2, u32, p2->indices, p2->indices_count, p2->topology) {
            vec3_f32 e1 = rot_quat(sub_3f32(p1->points[v1], p1->points[u1]), b1->rotation);
            vec3_f32 e2 = rot_quat(sub_3f32(p2->points[v2], p2->points[u2]), b2->rotation);
            vec3_f32 axis = cross_3f32(e1, e2);

            f32 axisl2 = length2_3f32(axis);
            if (axisl2 <= EPSILON_F32) continue;
            axis = mul_3f32(axis, 1.f/sqrt_f32(axisl2));

            if (!phys_collision_SAT_check_axis(out, axis, b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)p2))
                return false;
        } GEO_EachEdge_Ring_Close;
    } GEO_EachEdge_Ring_Close;

    return true;
}
b32 phys_collision_check_polytope_sphere(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Sphere* s2) {
    out->d = MAX_F32;
    for EachIndex(ni, p1->normals_count) {
        if (!phys_collision_SAT_check_axis(out, rot_quat(p1->normals[ni], b1->rotation), b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)s2))
            return false;
    }
    return true;
}

// solvers
static void phys_collision_solve_narrow(PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2, PHYS_CollisionCheck* check, f32 static_friction, f32 dynamic_friction) {
    // calculate lagrange multiplier for condition correction (along normal)
    f32 w_n = phys_collision_generalized_inverse_mass(b1, b2, check->r1, check->r2, check->n);
    f32 l_n = phys_lagrange_delta_no_update(check->d, w_n, 0.f);

    // apply static friction
    if (static_friction > 0.f) {
        vec3_f32 pc1 = add_3f32(b1->position, (b1->is_particle) ? check->r1 : rot_quat(check->r1, b1->rotation));
        vec3_f32 pc2 = add_3f32(b2->position, (b2->is_particle) ? check->r2 : rot_quat(check->r2, b2->rotation));
        vec3_f32 diff_c = sub_3f32(pc1, pc2);
        Assert(abs_f32(check->d - dot_3f32(diff_c, check->n)) < 10.f*EPSILON_F32);

        vec3_f32 pp1 = add_3f32(b1->prev_position, (b1->is_particle) ? check->r1 : rot_quat(check->r1, b1->prev_rotation));
        vec3_f32 pp2 = add_3f32(b2->prev_position, (b2->is_particle) ? check->r2 : rot_quat(check->r2, b2->prev_rotation));
        vec3_f32 diff_p = sub_3f32(pp1, pp2);

        // per-step tangent velocity
        vec3_f32 dp = sub_3f32(diff_c, diff_p);
        vec3_f32 dp_t = sub_3f32(dp, mul_3f32(check->n, dot_3f32(dp, check->n)));

        f32 dp_t_length = length_3f32(dp_t);
        vec3_f32 t = mul_3f32(dp_t, 1.f/dp_t_length);
        
        f32 w_t = phys_collision_generalized_inverse_mass(b1, b2, check->r1, check->r2, t);
        f32 l_t = phys_lagrange_delta_no_update(dp_t_length, w_t, 0.f);

        // determine if static friction is active
        if (static_friction*l_t < l_n) {
            // set tangential velocity to zero
            phys_collision_apply_corrections(b1, b2, check->r1, check->r2, t, l_t);
        }
    }

    // apply collision condition
    phys_collision_apply_corrections(b1, b2, check->r1, check->r2, check->n, l_n);

    // record collision for velocity update
    phys_world_add_collision_record(settings.w, (PHYS_CollisionSubstepRecord){
        .dynamic_friction = dynamic_friction,
        .b1 = b1,
        .b2 = b2,
        .r1 = check->r1,
        .r2 = check->r2,
        .n  = check->n,
        .f_n = l_n * settings.inv_dt2,
        .v_n = dot_3f32(check->n, phys_collision_total_velocity(b1, b2, check->r1, check->r2))
    });

    #if PHYS_DBG_D_STEP
        if (phys_dbg_d_ctx->do_contact_points) {
            PHYS_DBG_D_DRAW_EDGE(b1->position, add_3f32(b1->position, check->r1), make_3f32(1,0,0));
            PHYS_DBG_D_DRAW_EDGE(b2->position, add_3f32(b2->position, check->r2), make_3f32(0,0,1));
            PHYS_DBG_D_DRAW_NORMAL(add_3f32(b1->position, check->r1), check->n, make_3f32(0,1,0));
            PHYS_DBG_D_DRAW_NORMAL(add_3f32(b2->position, check->r2), mul_3f32(check->n,-1.f), make_3f32(0,1,0));
        }
    #endif
}
static void phys_collision_solve(PHYS_collider_id id1, PHYS_collider_id id2, PHYS_ConstraintSolveSettings settings) {
    if (id1.v == id2.v)
        return;

    PHYS_Collider* c1 = phys_world_resolve_collider(settings.w, id1);
    PHYS_Collider* c2 = phys_world_resolve_collider(settings.w, id2);

    if (c1->base.layer == PHYS_ColliderLayer_NoSelf && c2->base.layer == PHYS_ColliderLayer_NoSelf)
        return;
    
    Assert(phys_world_valid_radius(settings.w, c1->base.r));
    Assert(phys_world_valid_radius(settings.w, c2->base.r));

    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->base.p);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->base.p);

    f32 static_friction = phys_calculate_coeffcient(
        c1->base.static_friction, c2->base.static_friction,
        settings.w->static_friction_calculation
    );
    f32 dynamic_friction = phys_calculate_coeffcient(
        c1->base.dynamic_friction, c2->base.dynamic_friction,
        settings.w->dynamic_friction_calculation
    );

    // compute collision contact points
    PHYS_CollisionCheck check = {0};
    for (int i = 0; i < 2; i++) {
        if (
            (
                c1->base.type == PHYS_ColliderType_Sphere &&
                c2->base.type == PHYS_ColliderType_Sphere &&
                phys_collision_check_spheres(&check, b1, b2, &c1->sphere, &c2->sphere)
            ) || (
                c1->base.type == PHYS_ColliderType_Polytope &&
                c2->base.type == PHYS_ColliderType_Polytope &&
                phys_collision_check_polytopes(&check, b1, b2, &c1->polytope, &c2->polytope)
            ) || (
                c1->base.type == PHYS_ColliderType_Polytope &&
                c2->base.type == PHYS_ColliderType_Sphere &&
                phys_collision_check_polytope_sphere(&check, b1, b2, &c1->polytope, &c2->sphere)
            )
        ) {
            phys_collision_solve_narrow(settings, b1, b2, &check, static_friction, dynamic_friction);
            break;
        }

        // swap 1 and 2
        PHYS_Collider* ctmp = c1; c1 = c2; c2 = ctmp;
        PHYS_Body* btmp = b1; b1 = b2; b2 = btmp;
    }
}

static void phys_world_add_collision_record(PHYS_World* w, PHYS_CollisionSubstepRecord info) {
    PHYS_CollisionSubstepRecordNode* n = push_array(w->substep_arena, PHYS_CollisionSubstepRecordNode, 1);
    n->v = info;
    stack_push(w->substep_collision_records, n);
}

// coefficients
static f32 phys_calculate_coeffcient(f32 x1, f32 x2, PHYS_CoefficientCalculation method) {
    switch (method) {
        case PHYS_CoefficientCalculation_Average:   return (x1 + x2)/2.f;
        case PHYS_CoefficientCalculation_Min:       return Min(x1,x2);
        case PHYS_CoefficientCalculation_Max:       return Max(x1,x2);
    }
    NotImplemented;
}

// world
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

// xpbd with substepping splits physics step into substeps which solve constraints
// n times. Each steps consists of:
//      - apply forces and update position,
//      - solve constraints on positions,
//      - determine linear & angular velocity from delta after constraints 
//        have been applied.
void phys_world_step(PHYS_World* w, f64 dt) {
    w->hashgrid_info = NULL;
    w->brute_info = NULL;
    arena_clear(w->step_arena);

    f64 sdt = dt / (f64)w->substeps;
    f32 max_lin_v = w->min_r / sdt;

    // build cached queries
    // use previous frame's count for allocating or just split evenly
    if (!w->hashgrid_info_count)
        w->hashgrid_info_count = w->colliders.length/2;
    if (!w->brute_info_count)
        w->brute_info_count = w->colliders.length/2;
    u32 hashgrid_info_capacity = w->hashgrid_info_count;
    u32 brute_info_capacity = w->brute_info_count;
    w->hashgrid_info = push_array(w->step_arena, PHYS_CachedHashgridInfo, hashgrid_info_capacity);
    w->brute_info = push_array(w->step_arena, PHYS_CachedBruteInfo, brute_info_capacity);

    // fill in data
    w->hashgrid_info_count = 0;
    w->brute_info_count = 0;

    for EachIndex(slot, w->colliders.slots_count) {
        for EachList(collider_n, PHYS_ColliderNode, w->colliders.slots[slot]) {
            PHYS_Collider* collider = &collider_n->v;

            // if sphere is small enough, put it in the hashgrid
            if (collider->base.r <= w->hashgrid_obj_r && collider->base.type == PHYS_ColliderType_Sphere) {
                if (w->hashgrid_info_count >= hashgrid_info_capacity) {
                    hashgrid_info_capacity *= PHYS_PER_FRAME_DYNAMIC_ARRAY_GROWTH_RATE;
                    PHYS_CachedHashgridInfo* tmp = push_array(w->step_arena, PHYS_CachedHashgridInfo, hashgrid_info_capacity);
                    memcpy(tmp, w->hashgrid_info, sizeof(*tmp)*w->hashgrid_info_count);
                    w->hashgrid_info = tmp;
                }
                w->hashgrid_info[w->hashgrid_info_count] = (PHYS_CachedHashgridInfo){
                    .position = phys_world_resolve_body(w, collider->base.p)->position,
                    .collider = collider_n->id,
                };
                w->hashgrid_info_count++;
            // otherwise put it in brute
            } else {
                if (w->brute_info_count >= brute_info_capacity) {
                    brute_info_capacity *= PHYS_PER_FRAME_DYNAMIC_ARRAY_GROWTH_RATE;
                    PHYS_CachedBruteInfo* tmp = push_array(w->step_arena, PHYS_CachedBruteInfo, brute_info_capacity);
                    memcpy(tmp, w->brute_info, sizeof(*tmp)*w->brute_info_count);
                    w->brute_info = tmp;
                }
                w->brute_info[w->brute_info_count] = (PHYS_CachedBruteInfo){
                    .collider = collider_n->id,
                };
                w->brute_info_count++;
            }
        }
    }

    // build hashgrid and query self collisions (broadphase)
    if (w->hashgrid_info_count) {
        w->hashgrid = hg_build_hashgrid(
            w->step_arena, w->hashgrid_cell_r,
            &w->hashgrid_info->position, sizeof(*w->hashgrid_info),
            w->hashgrid_info_count
        );
        w->hashgrid_self_collisions = hg_hashgrid_batch_query(
            &w->hashgrid, w->step_arena, Max(w->hashgrid_self_collisions.hits_count, 64),
            max_lin_v*dt,
            &w->hashgrid_info->position, sizeof(*w->hashgrid_info),
            &w->hashgrid_info->collider.v, sizeof(*w->hashgrid_info),
            w->hashgrid_info_count
        );
        StaticAssert(sizeof(PHYS_collider_id) == sizeof(u64), phys_hashgrid_collider_id_data_size);
    } else {
        w->hashgrid_self_collisions.object_count = 0;
    }

    // set constraint lagrange multipliers back to zero (Gauss-seidel)
    for EachIndex(slot, w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            constraint_n->v.l = 0.f;
        }
    }

    for EachIndex(i, w->substeps) {
        phys_world_substep(w, sdt);
    }
}

static void phys_world_substep(PHYS_World* w, f64 dt) {
    w->substep_collision_records = NULL;
    arena_clear(w->substep_arena);

    f64 inv_dt = 1.f / dt;
    f32 max_lin_v = w->min_r*inv_dt;
    f32 max_lin_v2 = max_lin_v*max_lin_v;

    // step 1: apply external forces
    const vec3_f32 a_gravity = {.x = 0, .y = w->little_g, .z = 0};
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        b->prev_position = b->position;
        b->prev_rotation = b->rotation;

        // apply torques & linear forces
        if (!b->no_gravity) {
            b->linear_velocity = add_3f32(b->linear_velocity, mul_3f32(a_gravity, dt));
        }
        if (b->has_inertia) {
            vec3_f32 w_inertial = rot_quat(b->angular_velocity, inv_quat(b->rotation));
            vec3_f32 n = rot_quat(eldiv_3f32(w_inertial, b->inv_inertia), b->rotation);
            vec3_f32 t_ext = make_3f32(0,0,0); // @todo apply external torques
            vec3_f32 dT = mul_3f32(sub_3f32(t_ext, n), dt);
            vec3_f32 dT_inertial = rot_quat(dT, inv_quat(b->rotation));
            vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, dT_inertial), b->rotation);

            b->angular_velocity = add_3f32(b->angular_velocity, dw);
        }

        // enforce maximum velocity to avoid skipping collisions
        if (w->min_r) {
            f32 lin_v2 = dot_3f32(b->linear_velocity, b->linear_velocity);
            if (lin_v2 > max_lin_v2) {
                fprintf(stderr, "[body %.4d] exceeded max linear velocity\n", i);
                b->linear_velocity = mul_3f32(b->linear_velocity, max_lin_v/sqrt_f32(lin_v2));
            }
        }

        // integrate velocities
        b->position = add_3f32(b->position, mul_3f32(b->linear_velocity, dt));
        if (!b->is_particle) {
            vec4_f32 dr = mul_4f32(mul_quat(make_axis_quat(b->angular_velocity), b->rotation), dt);
            // @note linearized approximation
            b->rotation = normalize_4f32(add_4f32(b->rotation, mul_4f32(dr, 0.5f)));
        }
    }

    // step 2: solve constraints (including collisions)
    PHYS_ConstraintSolveSettings settings = (PHYS_ConstraintSolveSettings){
        .w = w,
        .inv_dt = inv_dt,
        .inv_dt2 = inv_dt*inv_dt,
    };

    // brute x brute
    // @todo bvh
    for EachIndex(i, w->brute_info_count) {
        for (int j = i+1; j < w->brute_info_count; j++) {
            phys_collision_solve(w->brute_info[i].collider, w->brute_info[j].collider, settings);
        }
    }
    // hashgrid x brute
    // @todo efficiency from colliding "particle" with object
    for EachIndex(i, w->hashgrid_info_count) {
        for EachIndex(j, w->brute_info_count) {
            phys_collision_solve(w->hashgrid_info[i].collider, w->brute_info[j].collider, settings);
        }
    }
    // hashgrid x hashgrid
    {
    HG_BatchQueryResult* q = &w->hashgrid_self_collisions;
    for EachIndex(object_i, q->object_count) {
        u32 hits_beg = q->object_hits_start[object_i  ];
        u32 hits_end = q->object_hits_start[object_i+1];

        for (u32 hit_j = hits_beg; hit_j < hits_end; hit_j++) {
            phys_collision_solve(w->hashgrid_info[object_i].collider, (PHYS_collider_id)q->hits_data[hit_j], settings);
        }
    }
    }
    
    for EachIndex(slot, w->constraints.slots_count) {
        for EachList(constraint_n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
            phys_constraint_solve(&constraint_n->v, settings);
        }
    }

    // step 3: set linear & angular velocities to resultant velocity
    for EachIndex(i, w->bodies.length) {
        PHYS_Body* b = &w->bodies.v[i];

        if (b->inv_mass > 0.f) {
            b->linear_velocity = mul_3f32(sub_3f32(b->position, b->prev_position), inv_dt);
            if (!b->is_particle) {
                vec4_f32 dr = mul_quat(b->rotation, inv_quat(b->prev_rotation));
                // @note linearized approximation
                b->angular_velocity = mul_3f32(dr.xyz, 2.0*inv_dt*sgn_f32(dr.w));
            }
        }
    }

    // step 4: solve velocities
    for EachList(n, PHYS_CollisionSubstepRecordNode, w->substep_collision_records) {
        PHYS_CollisionSubstepRecord* record = &n->v;

        PHYS_Body* b1 = record->b1;
        PHYS_Body* b2 = record->b2;
        f32 restitution = phys_calculate_coeffcient(
            b1->restitution, b2->restitution,
            settings.w->restitution_calculation
        );

        // @note this is the velocity after update (step 3)
        vec3_f32 v = phys_collision_total_velocity(b1, b2, record->r1, record->r2);
        f32 v_n = dot_3f32(record->n, v);

        // ensure velocity is correctly reflected from pre-update velocity
        if (abs_f32(v_n) <= 2*abs_f32(w->little_g)*dt) // @todo
            restitution = 0.f; // avoids jittering around low velocity collisions
        vec3_f32 dv = mul_3f32(record->n, -v_n + Min(-restitution*record->v_n, 0.f));

        if (record->dynamic_friction > 0.f) {
            // calculate dynamic friction along tangent
            vec3_f32 vt = sub_3f32(v, mul_3f32(record->n, v_n));
            f32 v_t = length_3f32(vt);

            if (v_t > 0.f) {
                f32 f_dynamic = dt*record->dynamic_friction*abs_f32(record->f_n);
                vec3_f32 dv_dynamic = mul_3f32(vt, -(1.f/v_t)*Min(f_dynamic, v_t));
                dv = add_3f32(dv, dv_dynamic);
            }
        }

        f32 dv_length2 = dot_3f32(dv, dv);
        if (dv_length2 > EPSILON_F32) {
            // apply velocity update
            f32 w = phys_collision_generalized_inverse_mass(b1, b2, record->r1, record->r2, mul_3f32(dv, 1.f/sqrt_f32(dv_length2)));
            phys_collision_apply_velocity_corrections(b1, b2, record->r1, record->r2, dv, 1.f/w);
        }
    }
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
        u32 new_i = w->colliders.max_i;
        w->colliders.max_i++;
        u32 new_slot = new_i % w->colliders.slots_count;
        
        new_node = push_array(w->arena, PHYS_ColliderNode, 1);
        new_node->id.i = new_i;
        stack_push(w->colliders.slots[new_slot], new_node);
    }
    Assert(new_node != NULL);

    new_node->v = c;
    
    w->colliders.length++;
    return new_node->id;
}
static PHYS_ColliderNode* phys_world_resolve_collider_node(PHYS_World* w, u32 i) {
    u32 slot = i % w->colliders.slots_count;
    for EachList(n, PHYS_ColliderNode, w->colliders.slots[slot]) {
        if (n->id.i == i) {
            return n;
        }
    }
    return NULL;
}
void phys_world_remove_collider(PHYS_World* w, PHYS_collider_id id) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, id.i);
    Assert(n != NULL);

    // invalidate old ids by increasing version
    n->id.version++;
    // add node to free chain for reuse
    stack_push(w->colliders.free_chain, n);

    w->colliders.length--;
}
PHYS_Collider* phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id id) {
    PHYS_ColliderNode* n = phys_world_resolve_collider_node(w, id.i);
    if (n != NULL && n->id.version != id.version) {
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
        u32 new_i = w->constraints.max_i;
        w->constraints.max_i++;
        u32 new_slot = new_i % w->constraints.slots_count;
        
        new_node = push_array(w->arena, PHYS_ConstraintNode, 1);
        new_node->id.i = new_i;
        stack_push(w->constraints.slots[new_slot], new_node);
    }
    Assert(new_node != NULL);

    new_node->v = c;
    return new_node->id;
}
static PHYS_ConstraintNode* phys_world_resolve_constraint_node(PHYS_World* w, u32 i) {
    u32 slot = i % w->constraints.slots_count;
    for EachList(n, PHYS_ConstraintNode, w->constraints.slots[slot]) {
        if (n->id.i == i) {
            return n;
        }
    }
    return NULL;
}
void phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_world_resolve_constraint_node(w, id.i);
    Assert(n != NULL);

    // invalidate old ids by increasing version
    n->id.version++;
    // add node to free chain for reuse
    stack_push(w->constraints.free_chain, n);
}
PHYS_Constraint* phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_world_resolve_constraint_node(w, id.i);
    if (n != NULL && n->id.version != id.version) {
        return NULL;
    }
    return &n->v;
}

// asserts
b32 phys_world_valid_radius(PHYS_World* w, f32 d) {
    if (w->min_r) {
        return d >= w->min_r;
    }
    return true;
}