static void solve_area_collision_constraint(const vec3_f32* area_normal, const vec3_f32* area_position, vec3_f32* x, f32 r) {
    f32 proj_d = dot_3f32(sub_3f32(*x, *area_position), *area_normal);

    if (proj_d < r) {
        *x = sub_3f32(*x, mul_3f32(*area_normal, proj_d - r));
    }
}

typedef struct AreaBoundary AreaBoundary;
struct AreaBoundary {
    vec3_f32 normal;
    vec3_f32 position;
};
static const AreaBoundary AREA_BOUNDARIES[] = {
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = +1.0, .y = 0.0, .z = 0.0 },
        .position   = (vec3_f32) { .x = -9.0, .y = 0.0, .z = 0.0 },
    },
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = 0.0, .y = +1.0, .z = 0.0 },
        .position   = (vec3_f32) { .x = 0.0, .y = -9.0, .z = 0.0 },
    },
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = 0.0, .y = 0.0, .z = +1.0 },
        .position   = (vec3_f32) { .x = 0.0, .y = 0.0, .z = -9.0 },
    },
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = -1.0, .y = 0.0, .z = 0.0 },
        .position   = (vec3_f32) { .x = +9.0, .y = 0.0, .z = 0.0 },
    },
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = 0.0, .y = -1.0, .z = 0.0 },
        .position   = (vec3_f32) { .x = 0.0, .y = +9.0, .z = 0.0 },
    },
    (AreaBoundary) {
        .normal     = (vec3_f32) { .x = 0.0, .y = 0.0, .z = -1.0 },
        .position   = (vec3_f32) { .x = 0.0, .y = 0.0, .z = +9.0 },
    },
};
static const int NUM_AREA_BOUNDARIES = sizeof(AREA_BOUNDARIES) / sizeof(AreaBoundary);

// xpbd with substepping splits physics step into substeps which solve constraints
// n times. Each steps consists of:
//      - apply forces and update position,
//      - solve constraints on positions,
//      - determine velocity from delta after constraints have been applied.
static void phys_substep(PhysicsState* state, f32 dt) {
    Ball* ball = &state->ball;
    const vec3_f32 a = {.x = 0, .y = state->little_g, .z = 0};

    vec3_f32* x = &ball->position;
    vec3_f32* v = &ball->velocity;

    // save position before forces and constraints
    ball->prev_position = *x;

    // apply forces and update positions
    *v = add_3f32(*v, mul_3f32(a, dt));
    *x = add_3f32(*x, mul_3f32(*v, dt));

    // solve constraints
    for EachIndex(i, NUM_AREA_BOUNDARIES) {
        solve_area_collision_constraint(&AREA_BOUNDARIES[i].normal, &AREA_BOUNDARIES[i].position, x, ball->radius);
    }

    // calculate resultant velocity
    *v = mul_3f32(sub_3f32(*x, ball->prev_position), 1.f / dt);
}

static const int NUM_SUBSTEPS = 128;
void phys_step(PhysicsState* state, f32 dt) {
    f32 sdt = dt / NUM_SUBSTEPS;
    for (int i = 0; i < NUM_SUBSTEPS; ++i) {
        phys_substep(state, sdt);
    }
}