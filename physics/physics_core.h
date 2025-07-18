#pragma once

// Units are generally assumed to be m,kg,seconds (MKS),
typedef struct PHYS_Plane_f32 PHYS_Plane_f32;
struct PHYS_Plane_f32 {
    vec3_f32 n;
    vec3_f32 p;
};

typedef struct PHYS_Particle PHYS_Particle;
struct PHYS_Particle {
    f32 radius;
    f32 inv_mass;

    vec3_f32 position;
    vec3_f32 prev_position;
    vec3_f32 velocity;
};

typedef struct PHYS_World PHYS_World;
struct PHYS_World {
    Arena* arena;
    u64 substeps;
    f64 compliance;
    f32 little_g;
    int num_particles;
    PHYS_Particle* particles;
};

PHYS_World* phys_world_create(Arena* arena);

void phys_world_step(PHYS_World* world, f32 dt);

// constraints
static const PHYS_Plane_f32 AREA_BOUNDARIES[] = {
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = +1.0, .y = 0.0, .z = 0.0 },
        .p = (vec3_f32) { .x = -9.0, .y = 0.0, .z = 0.0 },
    },
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = 0.0, .y = +1.0, .z = 0.0 },
        .p = (vec3_f32) { .x = 0.0, .y = -9.0, .z = 0.0 },
    },
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = 0.0, .y = 0.0, .z = +1.0 },
        .p = (vec3_f32) { .x = 0.0, .y = 0.0, .z = -9.0 },
    },
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = -1.0, .y = 0.0, .z = 0.0 },
        .p = (vec3_f32) { .x = +9.0, .y = 0.0, .z = 0.0 },
    },
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = 0.0, .y = -1.0, .z = 0.0 },
        .p = (vec3_f32) { .x = 0.0, .y = +9.0, .z = 0.0 },
    },
    (PHYS_Plane_f32) {
        .n = (vec3_f32) { .x = 0.0, .y = 0.0, .z = -1.0 },
        .p = (vec3_f32) { .x = 0.0, .y = 0.0, .z = +9.0 },
    },
};
static const int NUM_AREA_BOUNDARIES = sizeof(AREA_BOUNDARIES) / sizeof(PHYS_Plane_f32);

static void phys_sphere_area_collision_constraint(f64 a_dt2, PHYS_Plane_f32 plane, vec3_f32* x, f32 r, f32 w);
static void phys_particle_particle_collision_constraint(f64 a_dt2, const PHYS_Particle* p2, PHYS_Particle* p1);