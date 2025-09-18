#pragma once

// units
#define PHYS_UNIT_KG(n)  (((f32)(n)))
#define PHYS_UNIT_G(n)   (((f32)(n))*0.001f)
#define PHYS_UNIT_KM(n)  (((f32)(n))*1000f)
#define PHYS_UNIT_M(n)   (((f32)(n)))
#define PHYS_UNIT_CM(n)  (((f32)(n))*0.01f)
#define PHYS_UNIT_MM(n)  (((f32)(n))*0.001f)
#define PHYS_UNIT_N(n)   (((f32)(n)))
#define PHYS_UNIT_NM(n)  (((f32)(n)))
#define PHYS_UNIT_J(n)   (((f32)(n)))

// @note units are generally assumed to be m,kg,seconds (MKS),
typedef struct PHYS_World PHYS_World;

typedef u64 PHYS_body_id;
typedef struct PHYS_Body PHYS_Body;
struct PHYS_Body {
    vec3_f32    position;
    vec3_f32    prev_position;
    vec3_f32    linear_velocity;
    
    b32         is_particle;
    vec4_f32    rotation;
    vec4_f32    prev_rotation;
    vec3_f32    angular_velocity; // direction -> axis, length -> rad/s
    
    b32         no_gravity;
    f32         inv_mass;
    b32         has_inertia;
    vec3_f32    inv_inertia; // unit rotation should ensure moment of inertia is diagonal
    f32         restitution;
};

// correction helpers
static f32  phys_body_inverse_inertia(PHYS_Body* b, vec3_f32 t_world);
static void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 dp_world);
static void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 dt_world);

// velocity correction helpers
static void     phys_body_apply_linear_velocity_correction(PHYS_Body* b, vec3_f32 corr);
static void     phys_body_apply_angular_velocity_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r);
static vec3_f32 phys_body_velocity_at_offset(PHYS_Body* b, vec3_f32 r);

static f32 phys_lagrange_delta_no_update(f32 C, f32 w, f32 alpha);
static f32 phys_update_lagrange_multiplier_return_delta(f32 C, f32 w, f32 alpha, f32* l);

// constraints
typedef union PHYS_constraint_id {
    struct {
        u32 i;
        u32 version;
    };
    u64 v;
} PHYS_constraint_id;

typedef struct PHYS_Constraint_Distance PHYS_Constraint_Distance;
struct PHYS_Constraint_Distance {
    PHYS_body_id b1;
    PHYS_body_id b2;
    f32 d;

    b32 is_offset;
    vec3_f32 offset1;
    vec3_f32 offset2;

    b32 unilateral; // eg. string
};

typedef struct PHYS_Constraint_Volume PHYS_Constraint_Volume;
struct PHYS_Constraint_Volume {
    PHYS_body_id p[4];
    f32 v_rest;
};

typedef struct PHYS_Constraint_Hinge PHYS_Constraint_Hinge;
struct PHYS_Constraint_Hinge {
    PHYS_body_id b1;
    PHYS_body_id b2;
    vec3_f32 a1;
    vec3_f32 a2;
};

typedef enum PHYS_ConstraintType {
    PHYS_ConstraintType_Distance,
    PHYS_ConstraintType_Volume,
    PHYS_ConstraintType_Hinge,
    PHYS_ConstraintType_COUNT ENUM_CASE_UNUSED,
} PHYS_ConstraintType;

typedef struct PHYS_Constraint PHYS_Constraint;
struct PHYS_Constraint {
    PHYS_ConstraintType type;
    f32 l;
    f32 compliance;

    union {
        f32 force;
        vec3_f32 torque;
    };

    union {
        PHYS_Constraint_Distance        distance;
        PHYS_Constraint_Volume          volume;
        PHYS_Constraint_Hinge           hinge;
    };
};

typedef struct PHYS_ConstraintSolveSettings PHYS_ConstraintSolveSettings;
struct PHYS_ConstraintSolveSettings {
    PHYS_World* w;
    f64 inv_dt;
    f64 inv_dt2;
};

// solvers
static void phys_constraint_solve_distance(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_volume(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve_hinge(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);
static void phys_constraint_solve(PHYS_Constraint* c, PHYS_ConstraintSolveSettings settings);

// colliders
// @todo static colliders
typedef union PHYS_collider_id {
    struct {
        u32 i;
        u32 version;
    };
    u64 v;
} PHYS_collider_id;

typedef enum PHYS_ColliderType {
    PHYS_ColliderType_Sphere,
    PHYS_ColliderType_Polytope,
    PHYS_ColliderType_COUNT ENUM_CASE_UNUSED,
} PHYS_ColliderType;

typedef enum PHYS_ColliderLayer {
    PHYS_ColliderLayer_0,
    PHYS_ColliderLayer_NoSelf,
    PHYS_ColliderLayer_COUNT ENUM_CASE_UNUSED,
} PHYS_ColliderLayer;

typedef struct PHYS_Collider_Base PHYS_Collider_Base;
struct PHYS_Collider_Base {
    PHYS_ColliderType type;
    PHYS_body_id p;
    f32 r;
    f32 static_friction;
    f32 dynamic_friction;
    PHYS_ColliderLayer layer;
};

typedef struct PHYS_Collider_Polytope PHYS_Collider_Polytope;
struct PHYS_Collider_Polytope {
    PHYS_Collider_Base base;

    // @note assumed ring connection
    GEO_Topology topology;

    vec3_f32*   points;
    u32         points_count;
    u32*        indices;
    u32         indices_count;
    vec3_f32*   normals;
    u32         normals_count;
};

typedef struct PHYS_Collider_Sphere PHYS_Collider_Sphere;
struct PHYS_Collider_Sphere {
    PHYS_Collider_Base base;
};

typedef union PHYS_Collider PHYS_Collider;
union PHYS_Collider {
    PHYS_Collider_Base      base;
    PHYS_Collider_Sphere    sphere;
    PHYS_Collider_Polytope  polytope;
};

// helpers
static void     phys_collision_apply_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC, f32 l);
static void     phys_collision_apply_velocity_corrections(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC, f32 l);
static f32      phys_collision_generalized_inverse_mass(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2, vec3_f32 dC);
static vec3_f32 phys_collision_total_velocity(PHYS_Body* b1, PHYS_Body* b2, vec3_f32 r1, vec3_f32 r2);

typedef struct PHYS_CollisionCheck PHYS_CollisionCheck;
struct PHYS_CollisionCheck {
    vec3_f32 r1, r2, n;
    f32 d;
};
b32 phys_collision_check_spheres(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Sphere* s1, PHYS_Collider_Sphere* s2);
b32 phys_collision_check_polytopes(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Polytope* p2);
b32 phys_collision_check_polytope_sphere(PHYS_CollisionCheck* out, PHYS_Body* b1, PHYS_Body* b2, PHYS_Collider_Polytope* p1, PHYS_Collider_Sphere* s2);

// solvers
static void phys_collision_solve_narrow(PHYS_ConstraintSolveSettings settings, PHYS_Body* b1, PHYS_Body* b2, PHYS_CollisionCheck* check, f32 static_friction, f32 dynamic_friction);
static void phys_collision_solve(PHYS_collider_id id1, PHYS_collider_id id2, PHYS_ConstraintSolveSettings settings);

// coefficients
typedef enum PHYS_CoefficientCalculation {
    PHYS_CoefficientCalculation_Average = 0,
    PHYS_CoefficientCalculation_Min,
    PHYS_CoefficientCalculation_Max,
} PHYS_CoefficientCalculation;

static f32 phys_calculate_coeffcient(f32 x1, f32 x2, PHYS_CoefficientCalculation method);

// world
typedef struct PHYS_CollisionSubstepRecord PHYS_CollisionSubstepRecord;
struct PHYS_CollisionSubstepRecord {
    f32 dynamic_friction;
    PHYS_Body* b1;
    PHYS_Body* b2;
    vec3_f32 r1, r2, n;
    f32 f_n;
    f32 v_n;
};

typedef struct PHYS_CollisionSubstepRecordNode PHYS_CollisionSubstepRecordNode;
struct PHYS_CollisionSubstepRecordNode {
    PHYS_CollisionSubstepRecordNode* next;
    PHYS_CollisionSubstepRecord v;
};

typedef struct PHYS_ConstraintNode PHYS_ConstraintNode;
struct PHYS_ConstraintNode {
    PHYS_ConstraintNode* next;
    PHYS_Constraint v;
    PHYS_constraint_id id;
};

#define PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 16
typedef struct PHYS_ConstraintMap PHYS_ConstraintMap;
struct PHYS_ConstraintMap {
    PHYS_ConstraintNode** slots;
    u32 slots_count;
    u32 max_i;
    PHYS_ConstraintNode* free_chain;
};

typedef struct PHYS_ColliderNode PHYS_ColliderNode;
struct PHYS_ColliderNode {
    PHYS_ColliderNode* next;
    PHYS_Collider v;
    PHYS_collider_id id;
};

#define PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT 64
typedef struct PHYS_ColliderMap PHYS_ColliderMap;
struct PHYS_ColliderMap {
    PHYS_ColliderNode** slots;
    u32 slots_count;
    u32 max_i;
    PHYS_ColliderNode* free_chain;
    u32 length;
};

#define PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY 16
#define PHYS_BODY_DYNAMIC_ARRAY_GROWTH 2
typedef struct PHYS_BodyDynamicArray PHYS_BodyDynamicArray;
struct PHYS_BodyDynamicArray {
    PHYS_Body* v;
    PHYS_body_id length;
    PHYS_body_id capacity;
};

typedef struct PHYS_WorldSettings PHYS_WorldSettings;
struct PHYS_WorldSettings {
    u64 substeps;    
    f32 little_g;
    f32 min_collision_distance;
    f32 hashgrid_cell_size;
    f32 hashgrid_object_size;
    PHYS_CoefficientCalculation restitution_calculation;
    PHYS_CoefficientCalculation static_friction_calculation;
    PHYS_CoefficientCalculation dynamic_friction_calculation;
};

typedef struct PHYS_CachedHashgridInfo PHYS_CachedHashgridInfo;
struct PHYS_CachedHashgridInfo {
    vec3_f32 position;
    PHYS_collider_id collider;
};

typedef struct PHYS_CachedBruteInfo PHYS_CachedBruteInfo;
struct PHYS_CachedBruteInfo {
    PHYS_collider_id collider;
};

typedef struct PHYS_World PHYS_World;
struct PHYS_World {
    Arena* arena;
    Arena* step_arena;
    Arena* substep_arena;

    u64 substeps;    
    f32 little_g;

    PHYS_CoefficientCalculation restitution_calculation;
    PHYS_CoefficientCalculation static_friction_calculation;
    PHYS_CoefficientCalculation dynamic_friction_calculation;

    f32 min_r;
    f32 hashgrid_cell_r;
    f32 hashgrid_obj_r;

    PHYS_ColliderMap colliders;
    PHYS_ConstraintMap constraints;
    PHYS_BodyDynamicArray bodies;

    // per step
    HG_Hashgrid hashgrid;
    HG_BatchQueryResult hashgrid_self_collisions;
    u32 hashgrid_info_count;
    PHYS_CachedHashgridInfo* hashgrid_info;
    u32 brute_info_count;
    PHYS_CachedBruteInfo* brute_info;

    // per substep
    PHYS_CollisionSubstepRecordNode* substep_collision_records;
};

#define PHYS_PER_FRAME_DYNAMIC_ARRAY_GROWTH_RATE 2
#define PHYS_HG_TO_QUERY_R_RATIO 1.f

PHYS_World*         phys_make_world(PHYS_WorldSettings settings);
void                phys_world_cleanup(PHYS_World* w);
void                phys_world_step(PHYS_World* w, f64 dt);

PHYS_body_id        phys_world_add_body(PHYS_World* w, PHYS_Body b);
void                phys_world_remove_body(PHYS_World* w, PHYS_body_id dp);
PHYS_Body*          phys_world_resolve_body(PHYS_World* w, PHYS_body_id dp);

PHYS_collider_id    phys_world_add_collider(PHYS_World* w, PHYS_Collider c);
void                phys_world_remove_collider(PHYS_World* w, PHYS_collider_id col);
PHYS_Collider*      phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id col);

PHYS_constraint_id  phys_world_add_constraint(PHYS_World* w, PHYS_Constraint c);
void                phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id col);
PHYS_Constraint*    phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id col);

// internal
static void phys_world_substep(PHYS_World* w, f64 dt);
static void phys_world_add_collision_record(PHYS_World* w, PHYS_CollisionSubstepRecord info);

// asserts
b32 phys_world_valid_radius(PHYS_World* w, f32 d);