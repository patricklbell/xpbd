// xpbd with substepping splits physics step into substeps which solve constraints
// n times. Each steps consists of:
//      - apply forces and update position,
//      - solve constraints on positions,
//      - determine linear & angular velocity from delta after constraints 
//        have been applied.
shared_function(phys_world_step)
void phys_world_step(PHYS_World* w, f32 dt) {ZoneScoped;
    w->hashgrid_info = NULL;
    w->brute_info = NULL;
    arena_clear(w->step_arena);

    f32 sdt = dt/(f32)w->substeps;
    f32 max_lin_v = w->min_v_mult*w->min_r/sdt;

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

    // @todo sorting inplace now that colliders are stored flat
    {ZoneScopedN("sorting colliders");
    for PHYS_EachPA(w->colliders) {
        PHYS_EachPADefId(w->colliders, PHYS_Collider, PHYS_collider_id, collider);

        Assert(phys_world_valid_radius(w, collider->base.r));
        
        // if collider is small enough, put it in the hashgrid
        if (collider->base.r <= w->hashgrid_obj_size) {
            if (w->hashgrid_info_count >= hashgrid_info_capacity) {
                hashgrid_info_capacity *= PHYS_PER_FRAME_DYNAMIC_ARRAY_GROWTH_RATE;
                PHYS_CachedHashgridInfo* tmp = push_array(w->step_arena, PHYS_CachedHashgridInfo, hashgrid_info_capacity);
                memcpy(tmp, w->hashgrid_info, sizeof(*tmp)*w->hashgrid_info_count);
                w->hashgrid_info = tmp;
            }
            w->hashgrid_info[w->hashgrid_info_count] = (PHYS_CachedHashgridInfo){
                .position = phys_world_resolve_body(w, collider->base.p)->position,
                .collider = collider_id,
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
                .collider = collider_id,
            };
            w->brute_info_count++;
        }
    }}

    // build hashgrid and query self collisions (broadphase)
    if (w->hashgrid_info_count) {
        w->hashgrid = hg_build_hashgrid(
            w->step_arena, w->hashgrid_cell_size,
            &w->hashgrid_info->position, sizeof(*w->hashgrid_info), w->hashgrid_info_count
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
    {ZoneScopedN("lagrange multiplier");
    for PHYS_EachPA(w->constraints) {
        PHYS_EachPADef(w->constraints, PHYS_Constraint, constraint);
        constraint->l = 0.f;
    }
    for PHYS_EachPA(w->dependent_constraints) {
        PHYS_EachPADef(w->dependent_constraints, PHYS_DependentConstraint, dependent_constraint);
        dependent_constraint->l = 0.f;
    }}

    for EachIndex(i, w->substeps) {
        phys_world_substep(w, sdt);
    }

    #if PHYS_DBG_D_STEP
        if (phys_dbg_d_ctx->do_colliders)
            phys_dbg_d_colliders(w, NULL, 0);
        if (phys_dbg_d_ctx->do_constraints)
            phys_dbg_d_constraints(w, NULL, 0);
        if (phys_dbg_d_ctx->do_bodies)
            phys_dbg_d_bodies(w);
    #endif
}

internal void phys_world_substep(PHYS_World* w, f32 dt) {ZoneScoped;
    w->substep_collision_records = NULL;
    arena_clear(w->substep_arena);

    f32 inv_dt = 1.f / dt;
    f32 max_lin_v = w->min_v_mult*w->min_r*inv_dt;
    f32 max_lin_v2 = max_lin_v*max_lin_v;

    const vec3_f32 a_gravity = {.x = 0, .y = w->little_g, .z = 0};
    
    // step 1: apply external forces
    {ZoneScopedN("step 1");
    for PHYS_EachPA(w->bodies) {
        PHYS_EachPADef(w->bodies, PHYS_Body, b);

        b->prev_position = b->position;
        b->prev_rotation = b->rotation;

        // apply torques & linear forces
        if (!b->no_gravity) {
            b->linear_velocity = add_3f32(b->linear_velocity, mul_3f32(a_gravity, dt));
        }
        if (b->has_inertia) {
            vec3_f32 w_inertial = rot_quat(b->angular_velocity, inv_quat(b->rotation));
            vec3_f32 n_inertial = cross_3f32(w_inertial, eldiv_3f32(w_inertial, b->inv_inertia));
            vec3_f32 n = rot_quat(n_inertial, b->rotation);
            vec3_f32 t_ext = make_3f32(0,0,0); // @todo apply external torques
            vec3_f32 dT = mul_3f32(sub_3f32(t_ext, n), dt);
            vec3_f32 dT_inertial = rot_quat(dT, inv_quat(b->rotation));
            vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, dT_inertial), b->rotation);

            b->angular_velocity = add_3f32(b->angular_velocity, dw);
        }

        // enforce maximum velocity to avoid skipping collisions
        if (w->min_r) {
            f32 lin_v2 = length2_3f32(b->linear_velocity);
            if (lin_v2 > max_lin_v2) {
                // @todo logging
                // fprintf(stderr, "[body %.4d] exceeded max linear velocity\n", i);
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
    }}

    // step 2: solve constraints (including collisions)
    PHYS_ConstraintSolveSettings settings = (PHYS_ConstraintSolveSettings){
        .w = w,
        .inv_dt = inv_dt,
        .inv_dt2 = inv_dt*inv_dt,
    };
    {ZoneScopedN("step 2");
    for PHYS_EachPA(w->constraints) {
        PHYS_EachPADef(w->constraints, PHYS_Constraint, constraint);
        phys_constraint_solve(constraint, settings);
    }

    // @todo ad-hoc group plane for particles with no restitution
    if (w->enable_particle_ground_plane) {
        phys_collision_ground_plane_solve(w);
    }
        
    // brute x brute
    // @todo bvh or octree
    for EachIndexU32(i, w->brute_info_count) {
        for (u32 j = i+1; j < w->brute_info_count; j++) {
            phys_collision_solve(w->brute_info[i].collider, w->brute_info[j].collider, settings);
        }
    }
    // brute x hashgrid
    // @todo bvh or octree
    for EachIndexU32(j, w->brute_info_count) {
        for EachIndexU32(i, w->hashgrid_info_count) {
            phys_collision_solve(w->hashgrid_info[i].collider, w->brute_info[j].collider, settings);
        }
    }
    // hashgrid x hashgrid
    HG_BatchQueryResult* q = &w->hashgrid_self_collisions;
    for EachIndexU32(object_i, q->object_count) {
        u32 hits_beg = q->object_hits_start[object_i  ];
        u32 hits_end = q->object_hits_start[object_i+1];

        for (u32 hit_j = hits_beg; hit_j < hits_end; hit_j++) {
            phys_collision_solve(w->hashgrid_info[object_i].collider, (PHYS_collider_id){.v=q->hits_data[hit_j]}, settings);
        }
    }
    }

    // step 3: set linear & angular velocities to resultant velocity
    {ZoneScopedN("step 3");
    for PHYS_EachPA(w->bodies) {
        PHYS_EachPADef(w->bodies, PHYS_Body, b);

        if (b->inv_mass > 0.f) {
            b->linear_velocity = mul_3f32(sub_3f32(b->position, b->prev_position), inv_dt);
            if (!b->is_particle) {
                vec4_f32 dr = mul_quat(b->rotation, inv_quat(b->prev_rotation));
                // @note linearized approximation
                b->angular_velocity = mul_3f32(dr.xyz, 2.f*inv_dt*sgn_f32(dr.w));
            }
        }
    }}

    // step 4: solve velocities
    {ZoneScopedN("step 4");
    for EachList(n, PHYS_CollisionSubstepRecordNode, w->substep_collision_records) {
        PHYS_CollisionSubstepRecord* record = &n->v;

        PHYS_Body* b1 = record->b1;
        PHYS_Body* b2 = record->b2;
        f32 restitution = phys_calculate_coeffcient(
            b1->restitution, b2->restitution,
            settings.w->restitution_calculation
        );

        // @note these velocities are AFTER the update (step 3)
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

        f32 dv_l2 = length2_3f32(dv);
        if (dv_l2 > EPSILON_F32) {
            // apply velocity update
            f32 w = phys_collision_generalized_inverse_mass(b1, b2, record->r1, record->r2, mul_3f32(dv, 1.f/sqrt_f32(dv_l2)));
            phys_collision_apply_velocity_corrections(b1, b2, record->r1, record->r2, dv, 1.f/w);
        }
    }}
}

// corrections helpers
internal f32 phys_body_inverse_inertia(PHYS_Body* b, vec3_f32 t_world) {
    if (!b->has_inertia) return 0.f;
    // direction of torque
    vec3_f32 nt = rot_quat(t_world, inv_quat(b->rotation));
    return dot_3f32(nt, elmul_3f32(b->inv_inertia, nt));
}
internal void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 dp_world) {
    // TracyPlot("lin_correction", length_3f32(dp_world)*b->inv_mass);
    if (b->inv_mass <= 0.f) return;
    b->position = add_3f32(b->position, mul_3f32(dp_world, b->inv_mass));
}
internal void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 dt_world) {
    if (!b->has_inertia) return;

    // torque in inertial frame
    vec3_f32 t_inertial = rot_quat(dt_world, inv_quat(b->rotation));
    // delta angle in world frame
    vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, t_inertial), b->rotation);
    // stabilize rotation
    dw = mul_3f32(dw, 0.5f);
    // apply rotation correction (linearized approximation)
    vec4_f32 dr = mul_quat(make_axis_quat(dw), b->rotation);
    b->rotation = normalize_4f32(add_4f32(b->rotation, mul_4f32(dr, 0.5f)));
}

// velocity correct helpers
internal void phys_body_apply_linear_velocity_correction(PHYS_Body* b, vec3_f32 corr) {
    if (b->inv_mass <= 0.f) return;
    // apply position correction
    b->linear_velocity = add_3f32(b->linear_velocity, mul_3f32(corr, b->inv_mass));
}
internal void phys_body_apply_angular_velocity_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r) {
    if (!b->has_inertia) return;
    vec3_f32 t = rot_quat(cross_3f32(r, corr), inv_quat(b->rotation));
    vec3_f32 dw = rot_quat(elmul_3f32(b->inv_inertia, t), b->rotation);
    b->angular_velocity = add_3f32(b->angular_velocity, dw);
}

force_inline internal f32 phys_lagrange_delta_no_update(f32 C, f32 w, f32 alpha) {
    return -C / (w + alpha);
}

internal f32 phys_update_lagrange_multiplier_return_delta(f32 C, f32 w, f32 alpha, f32* l) {
    // f32 dl = (-C - alpha*(*l)) / (w + alpha);
    // using l in calculating dl causes the stiffness to become substep dependent,
    // @todo figure out why xpbd paper results don't match this observation.
    f32 dl = -C / (w + alpha);
    *l += dl;
    return dl;
}

internal vec3_f32 phys_body_velocity_at_offset(PHYS_Body* b, vec3_f32 r) {
    return add_3f32(b->linear_velocity, cross_3f32(b->angular_velocity, r));
}

internal void phys_constraint_apply_two_bodies_linear_correction(
    f32 compliance, f32* l, PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2,
    f32 C, vec3_f32 n, f32 w
) {
    f32 alpha = settings.inv_dt2*compliance;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, l);

    vec3_f32 corr1 = mul_3f32(n, +dl);
    vec3_f32 corr2 = mul_3f32(n, -dl);
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
}

internal void phys_constraint_apply_two_bodies_linear_offset_correction(
    f32 compliance, f32* l, PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2,
    f32 C, vec3_f32 n, vec3_f32 r1, vec3_f32 r2
) {
    f32 w  = b1->inv_mass + phys_body_inverse_inertia(b1, cross_3f32(r1, n));
        w += b2->inv_mass + phys_body_inverse_inertia(b2, cross_3f32(r2, n));
    if (w <= 0.f) return;
    
    f32 alpha = settings.inv_dt2*compliance;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, l);

    vec3_f32 corr1 = mul_3f32(n, +dl);
    vec3_f32 corr2 = mul_3f32(n, -dl);
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    phys_body_apply_angular_correction(b1, cross_3f32(r1, corr1));
    phys_body_apply_angular_correction(b2, cross_3f32(r2, corr2));
}

// solvers
internal void phys_constraint_solve_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->distance.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->distance.body2);
    f32 w = body1->inv_mass + body2->inv_mass;
    if (w <= 0.f) return;

    vec3_f32 dr = sub_3f32(body2->position, body1->position);
    f32 r2 = length2_3f32(dr);
    if (r2 == 0.f) return;
    f32 r = sqrt_f32(r2);
    
    f32 C = c->distance.d - r;
    if (c->distance.unilateral ? (C <= 0.f) : (C == 0.f)) return;    
    vec3_f32 n = mul_3f32(dr, 1.f/r); // @note not normalized so lagrange is not correct

    phys_constraint_apply_two_bodies_linear_correction(
        c->compliance, &c->l, settings, body1, body2,
        C, n, w
    );
}

internal void phys_constraint_solve_advanced_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->advanced_distance.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->advanced_distance.body2);

    vec3_f32 r1 = rot_quat(c->advanced_distance.offset1, body1->rotation);
    vec3_f32 r2 = rot_quat(c->advanced_distance.offset2, body2->rotation);
    vec3_f32 p1 = add_3f32(body1->position, r1);
    vec3_f32 p2 = add_3f32(body2->position, r2);

    // allowed axis, or just the normalized axis of separation
    vec3_f32 n;
    if (c->advanced_distance.is_projected) {
        n = rot_quat(c->advanced_distance.axis, body1->rotation);
    } else {
        vec3_f32 dr = sub_3f32(p1, p2);
        f32 r2 = length2_3f32(dr);
        n = (r2 != 0.f) ? mul_3f32(dr, 1.f/sqrt_f32(r2)) : make_3f32(1.f,0.f,0.f);
    }
    
    // projected positions
    f32 pp1 = dot_3f32(p1, n);
    f32 pp2 = dot_3f32(p2, n);

    f32 r = abs_f32(pp2 - pp1);
    
    f32 C = r - c->advanced_distance.d;
    if (c->advanced_distance.unilateral ? (C <= 0.f) : (C == 0.f)) return;
    if (c->advanced_distance.is_projected && pp1 < pp2) C *= -1.f;
    
    n = mul_3f32(n, sgn_f32(C));
    C = abs_f32(C);

    phys_constraint_apply_two_bodies_linear_offset_correction(
        c->compliance, &c->l, settings, body1, body2,
        C, n, r1, r2
    );
}

internal void phys_constraint_solve_linear_dofs(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->linear_dofs.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->linear_dofs.body2);
    f32 w = body1->inv_mass + body2->inv_mass;
    if (w <= 0.f) return;

    vec3_f32 dr = sub_3f32(body2->position, body1->position);

    vec3_f32 corr = {0};
    for EachElement(idx, c->linear_dofs.axes) {
        // project separation onto axis
        vec3_f32 a_axis = rot_quat(c->linear_dofs.axes[idx], body1->rotation);
        f32 a = dot_3f32(dr, a_axis);

        // apply limit condition
        f32 a_corr = a - Clamp(a, c->linear_dofs.limits[idx].min, c->linear_dofs.limits[idx].max);
        if (abs_f32(a_corr) < EPSILON_F32)
            continue;

        corr = add_3f32(corr, mul_3f32(a_axis, -a_corr));
    }
    
    f32 C = length_3f32(corr);
    if (C == 0.f) return;
    vec3_f32 n = mul_3f32(corr, 1.f/C);

    phys_constraint_apply_two_bodies_linear_correction(
        c->compliance, &c->l, settings, body1, body2,
        C, n, w
    );
}

internal void phys_constraint_solve_volume(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c->volume.bodies[0]);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c->volume.bodies[1]);
    PHYS_Body* b3 = phys_world_resolve_body(settings.w, c->volume.bodies[2]);
    PHYS_Body* b4 = phys_world_resolve_body(settings.w, c->volume.bodies[3]);

    vec3_f32 d21 = sub_3f32(b2->position, b1->position);
    vec3_f32 d31 = sub_3f32(b3->position, b1->position);
    vec3_f32 d41 = sub_3f32(b4->position, b1->position);
    vec3_f32 d32 = sub_3f32(b3->position, b2->position);
    vec3_f32 d42 = sub_3f32(b4->position, b2->position);

    vec3_f32 dC1 = cross_3f32(d42, d32);
    vec3_f32 dC2 = cross_3f32(d31, d41);
    vec3_f32 dC3 = cross_3f32(d41, d21);
    vec3_f32 dC4 = cross_3f32(d21, d31);

    f32 v = phys_tetrahedron_volume_axis(d21, d31, d41);
    f32 C = v - c->volume.v_rest;
    if (C == 0.f) return;

    f32 alpha = settings.inv_dt2*c->compliance;
    f32 w  = b1->inv_mass*length2_3f32(dC1);
        w += b2->inv_mass*length2_3f32(dC2);
        w += b3->inv_mass*length2_3f32(dC3);
        w += b4->inv_mass*length2_3f32(dC4);
    if (w == 0.f) return;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, &c->l);

    vec3_f32 corr1 = mul_3f32(dC1, dl);
    vec3_f32 corr2 = mul_3f32(dC2, dl);
    vec3_f32 corr3 = mul_3f32(dC3, dl);
    vec3_f32 corr4 = mul_3f32(dC4, dl);
    
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    phys_body_apply_linear_correction(b3, corr3);
    phys_body_apply_linear_correction(b4, corr4);
}

// https://matthias-research.github.io/pages/publications/posBasedDyn.pdf
// 4.4 Cloth Balloons modified to use correct Lagrange multiplier for XPBD
internal void phys_constraint_solve_global_volume(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        vec3_f32* dC = push_array(scratch.arena, vec3_f32, c->global_volume.surface_bodies_count);

        f32 v = 0.f;
        for (u32 idx = 0; idx < c->global_volume.surface_indices_count; idx += GEO_Topology_Triangle) {
            u32 i1 = c->global_volume.surface_indices[idx+0];
            u32 i2 = c->global_volume.surface_indices[idx+1];
            u32 i3 = c->global_volume.surface_indices[idx+2];
            vec3_f32 p1 = phys_world_resolve_body(settings.w, c->global_volume.surface_bodies[i1])->position;
            vec3_f32 p2 = phys_world_resolve_body(settings.w, c->global_volume.surface_bodies[i2])->position;
            vec3_f32 p3 = phys_world_resolve_body(settings.w, c->global_volume.surface_bodies[i3])->position;
            
            vec3_f32 p2xp3 = cross_3f32(p2, p3);
            vec3_f32 p3xp1 = cross_3f32(p3, p1);
            vec3_f32 p1xp2 = cross_3f32(p1, p2);
            v += dot_3f32(p1xp2, p3); // @note ~tetrahedron volume

            dC[i1] = add_3f32(dC[i1], p2xp3);
            dC[i2] = add_3f32(dC[i2], p3xp1);
            dC[i3] = add_3f32(dC[i3], p1xp2);
        }

        f32 C = v - 6.f*c->global_volume.v_rest*c->global_volume.k;
        if (C == 0.f) return;

        f32 alpha = settings.inv_dt2*c->compliance;
        f32 w = 0.f;
        for EachIndexU32(idx, c->global_volume.surface_bodies_count) {
            PHYS_Body* b = phys_world_resolve_body(settings.w, c->global_volume.surface_bodies[idx]);
            w += b->inv_mass*length2_3f32(dC[idx]);
        }
        if (w == 0.f) return;
        f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, &c->l);

        for EachIndexU32(idx, c->global_volume.surface_bodies_count) {
            PHYS_Body* b = phys_world_resolve_body(settings.w, c->global_volume.surface_bodies[idx]);
            phys_body_apply_linear_correction(b, mul_3f32(dC[idx], dl));
        }
    }}
}

internal void phys_constraint_apply_two_bodies_angular_correction(
    f32 compliance, f32* l, PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2,
    vec3_f32 dq
) {
    f32 C2 = dot_3f32(dq, dq);
    if (C2 == 0.f) return;
    f32 C = sqrt_f32(C2);
    vec3_f32 n = mul_3f32(dq, 1.f/C);

    f32 alpha = settings.inv_dt2*compliance;
    f32 w  = phys_body_inverse_inertia(b1, n);
        w += phys_body_inverse_inertia(b2, n);
    if (w <= 0.f) return;
    f32 dl = phys_update_lagrange_multiplier_return_delta(C, w, alpha, l);

    vec3_f32 corr1 = mul_3f32(n, +dl);
    vec3_f32 corr2 = mul_3f32(n, -dl);
    phys_body_apply_angular_correction(b1, corr1);
    phys_body_apply_angular_correction(b2, corr2);
}

// https://matthias-research.github.io/pages/publications/PBDBodies.pdf
// Algorithm 3: Handling joint limits
internal void phys_constraint_apply_two_bodies_limit_angle(
    f32 compliance, f32* l, PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2,
    vec3_f32 n, vec3_f32 n1, vec3_f32 n2, f32 alpha, f32 beta
) {
    #if PHYS_DBG_D_STEP
        if (phys_dbg_d_ctx->do_limit_angle) {
            PHYS_DBG_D_DRAW_DANGLE(b1->position,n1,n2,make_3f32(1,0,0));
            PHYS_DBG_D_DRAW_NORMAL(b1->position, n, make_3f32(0,1,0));
        }
    #endif

    f32 phi = asin_f32(Clamp(dot_3f32(cross_3f32(n1, n2), n), -1.f,1.f)); // @note clamp for numerical errors

    if (dot_3f32(n1, n2) < 0.f)
        phi = PI_F32 - phi;
    if (phi > +PI_F32)
        phi -= 2.f*PI_F32;
    if (phi < -PI_F32)
        phi += 2.f*PI_F32;

    if (phi >= alpha && phi <= beta) {
        return;
    }

    phi = Clamp(phi, alpha, beta);

    vec3_f32 n_target = rot_quat(n1, make_angle_axis_quat(phi, n));
    // @todo figure out why torque needed to be flipped from paper
    phys_constraint_apply_two_bodies_angular_correction(compliance, l, settings, b1, b2, cross_3f32(n2, n_target));

    #if PHYS_DBG_D_STEP
        if (phys_dbg_d_ctx->do_limit_angle) {
            phys_dbg_d_sector(b1->position, n1, n, phi, make_3f32(0,0,1), phys_dbg_d_ctx->default_normal_length*0.3f);
            PHYS_DBG_D_DRAW_NORMAL(b1->position, n_target, make_3f32(0,0,1));
            PHYS_DBG_D_DRAW_NORMAL(b1->position, cross_3f32(n2, n_target), make_3f32(1,0,1));
        }
    #endif
}

internal void phys_constraint_solve_orientation(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->orientation.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->orientation.body2);

    vec4_f32 q = mul_quat(body1->rotation, inv_quat(body2->rotation));
    phys_constraint_apply_two_bodies_angular_correction(
        c->compliance, &c->l, settings, body1, body2,
        mul_3f32(q.xyz, 2.f)
    );
}

internal void phys_constraint_solve_hinge(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    vec3_f32 a1, a2, b1, b2;
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->hinge.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->hinge.body2);

    a1 = rot_quat(c->hinge.a1, body1->rotation);
    a2 = rot_quat(c->hinge.a2, body2->rotation);
    phys_constraint_apply_two_bodies_angular_correction(
        c->compliance, &c->l, settings, body1, body2,
        cross_3f32(a1, a2)
    );

    if (c->hinge.target_angle.v) {
        PHYS_DependentConstraint* dc = phys_world_resolve_dependent_constraint(settings.w, c->hinge.target_angle);

        a1 = rot_quat(c->hinge.a1, body1->rotation);
        b1 = rot_quat(c->hinge.b1, body1->rotation);
        b2 = rot_quat(c->hinge.b2, body2->rotation);
        vec3_f32 b_target = rot_quat(b1, make_angle_axis_quat(dc->target.value, a1));

        phys_constraint_apply_two_bodies_angular_correction(
            dc->compliance, &dc->l, settings, body1, body2,
            cross_3f32(b_target, b2)
        );
    }
    if (c->hinge.limit_angle.v) {
        PHYS_DependentConstraint* dc = phys_world_resolve_dependent_constraint(settings.w, c->hinge.limit_angle);

        a1 = rot_quat(c->hinge.a1, body1->rotation);
        b1 = rot_quat(c->hinge.b1, body1->rotation);
        b2 = rot_quat(c->hinge.b2, body2->rotation);
        phys_constraint_apply_two_bodies_limit_angle(
            dc->compliance, &dc->l, settings, body1, body2,
            a1, b1, b2, dc->limits.min, dc->limits.max
        );
    }
}

internal void phys_constraint_solve_swing(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->swing.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->swing.body2);

    vec3_f32 a1 = rot_quat(c->swing.a1, body1->rotation);
    vec3_f32 a2 = rot_quat(c->swing.a2, body2->rotation);

    phys_constraint_apply_two_bodies_limit_angle(
        c->compliance, &c->l, settings, body1, body2,
        cross_3f32(a1, a2), a1, a2, c->swing.limits.min, c->swing.limits.max
    );
}

internal void phys_constraint_solve_twist(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    PHYS_Body* body1 = phys_world_resolve_body(settings.w, c->twist.body1);
    PHYS_Body* body2 = phys_world_resolve_body(settings.w, c->twist.body2);

    vec3_f32 a1 = rot_quat(c->twist.a1, body1->rotation);
    vec3_f32 b1 = rot_quat(c->twist.b1, body1->rotation);
    vec3_f32 a2 = rot_quat(c->twist.a2, body2->rotation);
    vec3_f32 b2 = rot_quat(c->twist.b2, body2->rotation);

    vec3_f32 n = mul_3f32(add_3f32(a1, a2), 1.f/(length_3f32(a1) + length_3f32(a2)));
    vec3_f32 n1 = sub_3f32(b1, mul_3f32(n, dot_3f32(n, b1)));
    vec3_f32 n2 = sub_3f32(b2, mul_3f32(n, dot_3f32(n, b2)));

    phys_constraint_apply_two_bodies_limit_angle(
        c->compliance, &c->l, settings, body1, body2,
        n, n1, n2, c->twist.limits.min, c->twist.limits.max
    );
}

internal void phys_constraint_solve(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings) {
    switch (c->type) {
        case PHYS_ConstraintType_Distance: {
            phys_constraint_solve_distance(c, settings);
        }break;
        case PHYS_ConstraintType_AdvancedDistance: {
            phys_constraint_solve_advanced_distance(c, settings);
        }break;
        case PHYS_ConstraintType_LinearDOFs: {
            phys_constraint_solve_linear_dofs(c, settings);
        }break;
        case PHYS_ConstraintType_Volume: {
            phys_constraint_solve_volume(c, settings);
        }break;
        case PHYS_ConstraintType_GlobalVolume: {
            phys_constraint_solve_global_volume(c, settings);
        }break;
        case PHYS_ConstraintType_Orientation: {
            phys_constraint_solve_orientation(c, settings);
        }break;
        case PHYS_ConstraintType_Hinge: {
            phys_constraint_solve_hinge(c, settings);
        }break;
        case PHYS_ConstraintType_Swing: {
            phys_constraint_solve_swing(c, settings);
        }break;
        case PHYS_ConstraintType_Twist: {
            phys_constraint_solve_twist(c, settings);
        }break;
    }
}

// colliders 
internal void phys_collision_apply_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n, f32 l) {
    vec3_f32 corr1 = mul_3f32(n, +l);
    vec3_f32 corr2 = mul_3f32(n, -l);
    
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
    phys_body_apply_angular_correction(b1, cross_3f32(r1, corr1));
    phys_body_apply_angular_correction(b2, cross_3f32(r2, corr2));
}

internal void phys_collision_apply_velocity_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n, f32 l) {
    vec3_f32 corr1 = mul_3f32(n, +l);
    vec3_f32 corr2 = mul_3f32(n, -l);
    
    phys_body_apply_linear_velocity_correction(b1, corr1);
    phys_body_apply_linear_velocity_correction(b2, corr2);
    phys_body_apply_angular_velocity_correction(b1, corr1, r1);
    phys_body_apply_angular_velocity_correction(b2, corr2, r2);
}

internal f32 phys_collision_generalized_inverse_mass(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 n) {
    f32 w1 = b1->inv_mass + phys_body_inverse_inertia(b1, cross_3f32(r1, n));
    f32 w2 = b2->inv_mass + phys_body_inverse_inertia(b2, cross_3f32(r2, n));
    return w1 + w2;
}

internal vec3_f32 phys_collision_total_velocity(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2) {
    return sub_3f32(
        phys_body_velocity_at_offset(b1, r1),
        phys_body_velocity_at_offset(b2, r2)
    );
}

internal b32 phys_collision_check_spheres(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Sphere* s1, PHYS_Collider_Sphere* s2) {
    return phys_contact_point_spheres(
        b1->position, b2->position, s1->base.r, s2->base.r,
        &out->d, &out->r1, &out->r2, &out->n
    );
}

internal void phys_collision_SAT_min_max(vec3_f32 axis, PHYS_Body* b, PHYS_Collider* c, f32* min, f32* max, u32* min_face, u32* max_face) {
    switch (c->base.type) {
        case PHYS_ColliderType_Sphere: {
            phys_SAT_sphere_min_max(axis, c->base.r, min, max);
            *min_face = 0;
            *max_face = 1;
        }break;
        case PHYS_ColliderType_Polytope: {
            vec3_f32 local_axis = rot_quat(axis, inv_quat(b->rotation));

            phys_SAT_polytope_min_max_with_aligned_face(
                local_axis,
                c->polytope.points, c->polytope.indices, c->polytope.indices_count, c->polytope.normals, c->polytope.topology,
                min, max, min_face, max_face
            );
        }break;
    }
    
    f32 proj_position = dot_3f32(axis, b->position);
    *min += proj_position;
    *max += proj_position;
}

internal void phys_copy_indexed_buffer_to_polgyon(vec3_f32* in_points, u32* in_indices, GEO_Topology in_topology, GEO_Polygon* out_face) {
    out_face->topology = in_topology;
    for EachIndex(i, in_topology) {
        out_face->data[i] = in_points[in_indices[i]];
    }
}
internal GEO_Polygon phys_collision_SAT_get_supporting_face(vec3_f32 penetration_axis, u32 f, PHYS_Body* b, PHYS_Collider* c) {
    GEO_Polygon support = {.topology=GEO_Topology_Empty};

    switch (c->base.type) {
        case PHYS_ColliderType_Sphere: {
            Assert(f == 0 || f == 1);
            support.data[support.topology] = add_3f32(mul_3f32(penetration_axis, (f == 0) ? -c->base.r : +c->base.r), b->position);
            support.topology = IntToEnum(GEO_Topology, support.topology+1);
        }break;
        case PHYS_ColliderType_Polytope: {
            phys_copy_indexed_buffer_to_polgyon(c->polytope.points, &c->polytope.indices[f], c->polytope.topology, &support);
            for EachIndex(i, support.topology)
                support.data[i] =  phys_rotate_translate(support.data[i], b->rotation, b->position);
        }break;
    }

    return support;
}
internal void phys_collision_manifold_solve_narrow(PHYS_CollisionCheck* in_out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider* c1, PHYS_Collider* c2, PHYS_ConstraintSolveSettings settings, f32 static_friction, f32 dynamic_friction) {
    in_out->r1 = make_3f32(0.f,0.f,0.f);
    in_out->r2 = make_3f32(0.f,0.f,0.f);

    // only calculate offsets at least one body can have torque applied
    if (b1->has_inertia || b2->has_inertia) {
        GEO_Polygon support1 = phys_collision_SAT_get_supporting_face(in_out->n, in_out->f1, b1, c1);
        GEO_Polygon support2 = phys_collision_SAT_get_supporting_face(mul_3f32(in_out->n,-1.f), in_out->f2, b2, c2);
        
        #if PHYS_DBG_D_STEP
            if (phys_dbg_d_ctx->do_contact_manifold) {
                for GEO_EachEdge_Ring_Open(u, v, vec3_f32, support1.data, support1.topology, support1.topology) {
                    PHYS_DBG_D_DRAW_EDGE(u, v, make_3f32(1,0,0));
                } GEO_EachEdge_Ring_Close;
                for GEO_EachEdge_Ring_Open(u, v, vec3_f32, support2.data, support2.topology, support2.topology) {
                    PHYS_DBG_D_DRAW_EDGE(u, v, make_3f32(0,0,1));
                } GEO_EachEdge_Ring_Close;
            }
        #endif
         
        PHYS_ContactPairs cp = phys_manifold_between_faces(in_out->n, in_out->d, &support1, &support2);
        if (cp.count == 0)
            return;
 
        // apply all contact points
        for EachIndexU32(i, cp.count) {
            // in_out->d = dot_3f32(sub_3f32(cp.bodies[0][i], cp.bodies[1][i]), in_out->n);
            #if PHYS_DBG_D_STEP
                if (phys_dbg_d_ctx->do_contact_manifold) {
                    for EachIndexU32(i, cp.count) {
                        PHYS_DBG_D_DRAW_DPOINT(cp.bodies[0][i], (in_out->d > 0.f) ? make_3f32(1,0,0) : make_3f32(0,1,1));
                        PHYS_DBG_D_DRAW_DPOINT(cp.bodies[1][i], (in_out->d > 0.f) ? make_3f32(0,0,1) : make_3f32(1,1,0));
                    }
                }
            #endif

            in_out->r1 = add_3f32(in_out->r1, sub_3f32(cp.bodies[0][i], b1->position));
            in_out->r2 = add_3f32(in_out->r2, sub_3f32(cp.bodies[1][i], b2->position));    
        }
        in_out->r1 = mul_3f32(in_out->r1, 1.f/cp.count);
        in_out->r2 = mul_3f32(in_out->r2, 1.f/cp.count);
    }

    phys_collision_solve_narrow(settings, b1, b2, in_out, static_friction, dynamic_friction);
}

internal b32 phys_collision_SAT_check_axis(PHYS_CollisionCheck* out, vec3_f32 axis, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider* c1, PHYS_Collider* c2) {
    f32 min1, max1, min2, max2;
    u32 minf1, maxf1, minf2, maxf2;

    phys_collision_SAT_min_max(axis, b1, c1, &min1, &max1, &minf1, &maxf1);
    phys_collision_SAT_min_max(mul_3f32(axis, -1.f), b2, c2, &max2, &min2, &maxf2, &minf2);
    max2 *= -1.f;
    min2 *= -1.f;
    
    PHYS_SATCollisionForm order = phys_SAT_check_collision_axis(axis, min1, max1, min2, max2, &out->d, &out->n);
    if (order) {
        if (order == PHYS_SATCollisionForm_MaxMin) {
            out->f1 = maxf1;
            out->f2 = minf2;
        } else if (order == PHYS_SATCollisionForm_MinMax) {
            out->f1 = minf1;
            out->f2 = maxf2;
        }
        return true;
    }

    return false;
}

internal b32 phys_collision_check_polytopes(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Polytope* p2) {
    out->d = MAX_F32;
    for EachIndexU32(ni, p1->normals_count) {
        if (!phys_collision_SAT_check_axis(out, rot_quat(p1->normals[ni], b1->rotation), b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)p2))
            return false;
    }
    for EachIndexU32(ni, p2->normals_count) {
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
internal b32 phys_collision_check_polytope_sphere(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Sphere* s2) {
    out->d = MAX_F32;
    for EachIndexU32(ni, p1->normals_count) {
        if (!phys_collision_SAT_check_axis(out, rot_quat(p1->normals[ni], b1->rotation), b1, b2, (PHYS_Collider*)p1, (PHYS_Collider*)s2))
            return false;
    }

    return true;
}

// solvers
internal void phys_collision_solve_narrow(PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2, PHYS_CollisionCheck* check, f32 static_friction, f32 dynamic_friction) {
    #if PHYS_DBG_D_STEP
        if (phys_dbg_d_ctx->do_contact_points) {
            vec3_f32 r1w = add_3f32(check->r1, b1->position);
            vec3_f32 r2w = add_3f32(check->r2, b2->position);

            PHYS_DBG_D_DRAW_DPOINT(r1w, make_3f32(0.5,0,0));
            PHYS_DBG_D_DRAW_DPOINT(r2w, make_3f32(0,0,0.5));
            // PHYS_DBG_D_DRAW_EDGE(b1->position, r1w, make_3f32(1,0,0));
            // PHYS_DBG_D_DRAW_EDGE(b2->position, r2w, make_3f32(0,0,1));
            PHYS_DBG_D_DRAW_EDGE(r1w, add_3f32(r1w, mul_3f32(check->n,+10.f*check->d)), make_3f32(0.5,0,0));
            PHYS_DBG_D_DRAW_EDGE(r2w, add_3f32(r2w, mul_3f32(check->n,-10.f*check->d)), make_3f32(0,0,0.5));
        }
    #endif
    
    // @debug rough approximation of the maximum penetration distance per substep
    // f32 max_substep_d = length_3f32(sub_3f32(b1->linear_velocity, b2->linear_velocity))*settings.inv_dt;
    // if (check->d > max_substep_d){
    //     fprintf(stderr, "Too large a collision penetration, d=%f, max_d=%f\n", check->d, max_substep_d);
    // }
    
    // calculate lagrange multiplier for condition correction (along normal)
    f32 w_n = phys_collision_generalized_inverse_mass(b1, b2, check->r1, check->r2, check->n);
    f32 l_n = phys_lagrange_delta_no_update(check->d, w_n, 0.f);

    // apply internal friction
    if (static_friction > 0.f) {
        vec3_f32 pc1 = add_3f32(b1->position, check->r1);
        vec3_f32 pc2 = add_3f32(b2->position, check->r2);
        vec3_f32 diff_c = sub_3f32(pc1, pc2);
        Assert(abs_f32(check->d - dot_3f32(diff_c, check->n)) < 10.f*EPSILON_F32);

        // offsets in local body space
        vec3_f32 pr1 = (b1->is_particle) ? check->r1 : rot_quat(rot_quat(check->r1, inv_quat(b1->rotation)), b1->prev_rotation);
        vec3_f32 pr2 = (b2->is_particle) ? check->r2 : rot_quat(rot_quat(check->r2, inv_quat(b2->rotation)), b2->prev_rotation);
        vec3_f32 pp1 = add_3f32(b1->prev_position, pr1);
        vec3_f32 pp2 = add_3f32(b2->prev_position, pr2);
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

    vec3_f32 v = phys_collision_total_velocity(b1, b2, check->r1, check->r2);
    f32 v_n = dot_3f32(check->n, v);
    
    // record collision for velocity update
    phys_world_add_collision_record(settings.w, (PHYS_CollisionSubstepRecord){
        .dynamic_friction = dynamic_friction,
        .b1 = b1,
        .b2 = b2,
        .r1 = check->r1,
        .r2 = check->r2,
        .n  = check->n,
        .f_n = l_n*(f32)settings.inv_dt2,
        .v_n = v_n,
    });
}
internal void phys_collision_solve_narrow_particle(PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2, PHYS_CollisionCheck* check) {
    f32 w_n = b1->inv_mass + b2->inv_mass;
    f32 l_n = phys_lagrange_delta_no_update(check->d, w_n, 0.f);
    
    vec3_f32 corr1 = mul_3f32(check->n, +l_n);
    vec3_f32 corr2 = mul_3f32(check->n, -l_n);
    phys_body_apply_linear_correction(b1, corr1);
    phys_body_apply_linear_correction(b2, corr2);
}

internal void phys_collision_solve(PHYS_collider_id id1, PHYS_collider_id id2, PHYS_ConstraintSolveSettings settings) {
    if (id1.v == id2.v)
        return;

    PHYS_Collider* c1 = phys_world_resolve_collider(settings.w, id1);
    PHYS_Collider* c2 = phys_world_resolve_collider(settings.w, id2);

    if (!phys_collider_layers_overlap(c1->base.layer, c2->base.layer))
        return;
    
    Assert(phys_world_valid_radius(settings.w, c1->base.r));
    Assert(phys_world_valid_radius(settings.w, c2->base.r));

    PHYS_Body* b1 = phys_world_resolve_body(settings.w, c1->base.p);
    PHYS_Body* b2 = phys_world_resolve_body(settings.w, c2->base.p);

    // compute collision contact points
    PHYS_CollisionCheck check = zero_struct;
    for (int i = 0; i < 2; i++) {
        if (
            (
                c1->base.type == PHYS_ColliderType_Sphere &&
                c2->base.type == PHYS_ColliderType_Sphere &&
                phys_collision_check_spheres(&check, b1, b2, &c1->sphere, &c2->sphere)
            )
        ) {
            if (b1->is_particle && b2->is_particle) {
                phys_collision_solve_narrow_particle(settings, b1, b2, &check);
            } else {
                f32 static_friction = phys_calculate_coeffcient(c1->base.static_friction, c2->base.static_friction, settings.w->static_friction_calculation);
                f32 dynamic_friction = phys_calculate_coeffcient(c1->base.dynamic_friction, c2->base.dynamic_friction, settings.w->dynamic_friction_calculation);
                phys_collision_solve_narrow(settings, b1, b2, &check, static_friction, dynamic_friction);
            }
            break;
        }
        if (
             (
                c1->base.type == PHYS_ColliderType_Polytope &&
                c2->base.type == PHYS_ColliderType_Polytope &&
                phys_collision_check_polytopes(&check, b1, b2, &c1->polytope, &c2->polytope)
            ) || (
                c1->base.type == PHYS_ColliderType_Polytope &&
                c2->base.type == PHYS_ColliderType_Sphere &&
                phys_collision_check_polytope_sphere(&check, b1, b2, &c1->polytope, &c2->sphere)
            )
        ) {
            f32 static_friction = phys_calculate_coeffcient(c1->base.static_friction, c2->base.static_friction, settings.w->static_friction_calculation);
            f32 dynamic_friction = phys_calculate_coeffcient(c1->base.dynamic_friction, c2->base.dynamic_friction, settings.w->dynamic_friction_calculation);
            phys_collision_manifold_solve_narrow(&check, b1, b2, c1, c2, settings, static_friction, dynamic_friction);
            break;
        }
        
        // swap 1 and 2
        if (c1->base.type == c2->base.type)
            break;
        PHYS_Collider* ctmp = c1; c1 = c2; c2 = ctmp;
        PHYS_Body* btmp = b1; b1 = b2; b2 = btmp;
    }
}
internal void phys_collision_ground_plane_solve(PHYS_World* w) {ZoneScoped;
    for PHYS_EachPA(w->colliders) {
        PHYS_EachPADef(w->colliders, PHYS_Collider, collider);
        PHYS_Body* body = phys_world_resolve_body(w, collider->base.p);

        if (body->inv_mass <= 0.f)
            continue;
        if (body->position.y - collider->base.r < w->particle_ground_plane_height) {
            body->position = add_3f32(body->position, sub_3f32(body->prev_position, body->position));
            body->position.y = w->particle_ground_plane_height + collider->base.r;
        }
    }
}

// coefficients
internal f32 phys_calculate_coeffcient(f32 x1, f32 x2, PHYS_CoefficientCalculation method) {
    switch (method) {
        case PHYS_CoefficientCalculation_Average:   return (x1 + x2)/2.f;
        case PHYS_CoefficientCalculation_Min:       return Min(x1,x2);
        case PHYS_CoefficientCalculation_Max:       return Max(x1,x2);
    }
    NotImplemented;
    return x1;
}