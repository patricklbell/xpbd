PHYS_World* phys_world_create(Arena* arena) {
    PHYS_World* w = push_array(arena, PHYS_World, 1);
    *w = (PHYS_World){
        .arena = arena,
        .substeps = 32,
        .compliance = 0.001,
        .little_g = -9.8,
        .num_particles = 10,
        .particles = NULL,
    };

    w->particles = push_array(arena, PHYS_Particle, w->num_particles);
    for EachIndex(i, w->num_particles) {
        w->particles[i] = (PHYS_Particle){
            .radius         = 0.5f+1.5f*rand_f32(),
            .inv_mass       = 1.0f,
            .position       = make_3f32(7*rand_f32(), 7*rand_f32(), 7*rand_f32()),
            .prev_position  = zero_struct,
            .velocity       = make_3f32(7*rand_f32(), 7*rand_f32(), 7*rand_f32()),
        };
    }

    return w;
}

// xpbd with substepping splits physics step into substeps which solve constraints
// n times. Each steps consists of:
//      - apply forces and update position,
//      - solve constraints on positions,
//      - determine velocity from delta after constraints have been applied.
static void phys_world_substep(PHYS_World* w, f64 dt) {
    const vec3_f32 a = {.x = 0, .y = w->little_g, .z = 0};
    
    for EachIndex(i, w->num_particles) {
        PHYS_Particle* p = &w->particles[i];

        // save the position before forces and constraints
        p->prev_position = p->position;
    
        // apply forces and update positions
        p->velocity = add_3f32(p->velocity, mul_3f32(a, dt));
        p->position = add_3f32(p->position, mul_3f32(p->velocity, dt));
    }

    // solve constraints
    f64 inv_dt = 1.f / dt;
    f64 a_dt2 = w->compliance*inv_dt*inv_dt; // stiffness term;
    for EachIndex(i, w->num_particles) {
        PHYS_Particle* p = &w->particles[i];

        for EachIndex(j, NUM_AREA_BOUNDARIES) {
            phys_sphere_area_collision_constraint(a_dt2, AREA_BOUNDARIES[j], &p->position, p->radius, p->inv_mass);
        }
        for EachIndex(j, w->num_particles) {
            if (i == j) continue;
            phys_particle_particle_collision_constraint(a_dt2, &w->particles[j], p);
        }
    }

    for EachIndex(i, w->num_particles) {
        PHYS_Particle* p = &w->particles[i];

        // calculate resultant velocities
        p->velocity = mul_3f32(sub_3f32(p->position, p->prev_position), inv_dt);
    }

}

void phys_world_step(PHYS_World* w, f32 dt) {
    f64 sdt = dt / (f64)w->substeps;
    for (u64 i = 0; i < w->substeps; ++i) {
        phys_world_substep(w, sdt);
    }
}

static void phys_sphere_area_collision_constraint(f64 a_dt2, PHYS_Plane_f32 p, vec3_f32* x, f32 r, f32 w) {
    f32 c = dot_3f32(sub_3f32(*x, p.p), p.n) - r;

    if (c < 0) {
        f32 l = -c*(w / (w + a_dt2));
        *x = add_3f32(*x, mul_3f32(p.n, l));
    }
}

static void phys_particle_particle_collision_constraint(f64 a_dt2, const PHYS_Particle* p2, PHYS_Particle* p1) {
    vec3_f32 d = sub_3f32(p1->position, p2->position);
    f32 d_length = length_3f32(d);
    f32 c = d_length - (p1->radius + p2->radius);

    if (c < 0) {
        f32 l = -c*(p1->inv_mass / (p1->inv_mass + p2->inv_mass + a_dt2));
        p1->position = add_3f32(p1->position, mul_3f32(d, l/d_length));
    }
}