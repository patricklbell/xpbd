#pragma once

// @note units are generally assumed to be m,kg,seconds (MKS),
typedef struct PHYS_World PHYS_World;

typedef u64 PHYS_body_id;
typedef struct PHYS_Body PHYS_Body;
struct PHYS_Body {
    vec3_f32    position;
    vec3_f32    prev_position;
    vec3_f32    linear_velocity;
    
    vec4_f32    rotation;
    vec4_f32    prev_rotation;
    vec3_f32    angular_velocity; // direction -> axis, length -> rad/s
    
    b32         no_gravity;
    f32         inv_mass;
    mat3x3_f32  inv_inertia;
};

// constraints
typedef union PHYS_constraint_id {
    struct {
        u32 id;
        u32 version;
    };
    u64 v;
} PHYS_constraint_id;

typedef struct PHYS_Constraint_Distance PHYS_Constraint_Distance;
struct PHYS_Constraint_Distance {
    f32 compliance;
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
    f32 compliance;
    PHYS_body_id b[4];
    f32 v;
};

typedef enum PHYS_ConstraintType {
    PHYS_ConstraintType_Distance,
    PHYS_ConstraintType_Volume,
    PHYS_ConstraintType_COUNT ENUM_CASE_UNUSED,
} PHYS_ConstraintType;

typedef struct PHYS_Constraint PHYS_Constraint;
struct PHYS_Constraint {
    PHYS_ConstraintType type;

    union {
        PHYS_Constraint_Distance        distance;
        PHYS_Constraint_Volume          volume;
    };
};

typedef struct PHYS_ConstraintSolveSettings PHYS_ConstraintSolveSettings;
struct PHYS_ConstraintSolveSettings {
    PHYS_World* w;
    f64 inv_dt2;
};

static void phys_body_apply_linear_correction(PHYS_Body* b, vec3_f32 corr);
static void phys_body_apply_angular_correction(PHYS_Body* b, vec3_f32 corr, vec3_f32 r);
static f32 phys_body_generalized_inverse_mass(PHYS_Body* b, vec3_f32 r, vec3_f32 dC);

static void phys_constrain_distance(PHYS_Constraint_Distance* c, PHYS_ConstraintSolveSettings settings);
static void phys_constrain_volume(PHYS_Constraint_Volume* c, PHYS_ConstraintSolveSettings settings);

// colliders
// @todo static colliders
typedef union PHYS_collider_id {
    struct {
        u32 id;
        u32 version;
    };
    u64 v;
} PHYS_collider_id;

typedef struct PHYS_Collider_Sphere PHYS_Collider_Sphere;
struct PHYS_Collider_Sphere {
    f32 compliance;
    PHYS_body_id c;
    f32 r;
};

typedef struct PHYS_Collider_RectCuboid PHYS_Collider_RectCuboid;
struct PHYS_Collider_RectCuboid {
    f32 compliance;
    PHYS_body_id c;
    vec3_f32 r;
};

typedef struct PHYS_Collider_Plane PHYS_Collider_Plane;
struct PHYS_Collider_Plane {
    f32 compliance;
    PHYS_body_id p;
    vec3_f32 n;
};

typedef enum PHYS_ColliderType {
    PHYS_ColliderType_Sphere,
    PHYS_ColliderType_RectCuboid,
    PHYS_ColliderType_Plane,
    PHYS_ColliderType_COUNT,
} PHYS_ColliderType;

typedef struct PHYS_Collider PHYS_Collider;
struct PHYS_Collider {
    PHYS_ColliderType type;

    union {
        PHYS_Collider_Sphere         sphere;
        PHYS_Collider_RectCuboid     rect_cuboid;
        PHYS_Collider_Plane          plane;
    };
};

static void phys_collide_spheres(const PHYS_Collider_Sphere* c1, PHYS_Collider_Sphere* c2, PHYS_ConstraintSolveSettings settings);
static void phys_collide_sphere_with_plane(const PHYS_Collider_Sphere* c1, PHYS_Collider_Plane* c2, PHYS_ConstraintSolveSettings settings);

// world
typedef struct PHYS_ConstraintNode PHYS_ConstraintNode;
struct PHYS_ConstraintNode {
    PHYS_ConstraintNode* next;
    PHYS_Constraint v;
    u32 version;
    u32 id;
};

#define PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 16
typedef struct PHYS_ConstraintMap PHYS_ConstraintMap;
struct PHYS_ConstraintMap {
    PHYS_ConstraintNode** slots;
    u32 slots_count;
    u32 max_id;
    PHYS_ConstraintNode* free_chain;
};

typedef struct PHYS_ColliderNode PHYS_ColliderNode;
struct PHYS_ColliderNode {
    PHYS_ColliderNode* next;
    PHYS_Collider v;
    u32 version;
    u32 id;
};

#define PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT 64
typedef struct PHYS_ColliderMap PHYS_ColliderMap;
struct PHYS_ColliderMap {
    PHYS_ColliderNode** slots;
    u32 slots_count;
    u32 max_id;
    PHYS_ColliderNode* free_chain;
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
    f32 damping;
};

typedef struct PHYS_World PHYS_World;
struct PHYS_World {
    Arena* arena;

    u64 substeps;    
    f32 little_g;
    f32 damping;
    PHYS_ColliderMap colliders;
    PHYS_ConstraintMap constraints;
    PHYS_BodyDynamicArray bodies;
};

PHYS_World*         phys_world_make(PHYS_WorldSettings settings);
void                phys_world_cleanup(PHYS_World* w);
void                phys_world_step(PHYS_World* w, f64 dt);
static void         phys_world_substep(PHYS_World* w, f64 dt);

PHYS_body_id        phys_world_add_body(PHYS_World* w, PHYS_Body b);
void                phys_world_remove_body(PHYS_World* w, PHYS_body_id dp);
PHYS_Body*          phys_world_resolve_body(PHYS_World* w, PHYS_body_id dp);

PHYS_collider_id    phys_world_add_collider(PHYS_World* w, PHYS_Collider c);
void                phys_world_remove_collider(PHYS_World* w, PHYS_collider_id col);
PHYS_Collider*      phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id col);

PHYS_constraint_id  phys_world_add_constraint(PHYS_World* w, PHYS_Constraint c);
void                phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id col);
PHYS_Constraint*    phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id col);

// helper objects
typedef struct PHYS_RigidBody_Settings PHYS_RigidBody_Settings;
struct PHYS_RigidBody_Settings {
    PHYS_Body       body;
    PHYS_Collider   collider;
};

typedef struct PHYS_RigidBody PHYS_RigidBody;
struct PHYS_RigidBody {
    PHYS_body_id        body_id;
    PHYS_collider_id    collider_id;
};

typedef struct PHYS_Ball_Settings PHYS_Ball_Settings;
struct PHYS_Ball_Settings {
    f32 radius;
    f32 mass;
    f32 compliance;
    vec3_f32 center;
    vec3_f32 linear_velocity;
};

typedef struct PHYS_Box_Settings PHYS_Box_Settings;
struct PHYS_Box_Settings {
    f32 mass;
    f32 compliance;
    vec3_f32 center;
    vec3_f32 extents;
    vec3_f32 linear_velocity;
    vec3_f32 angular_velocity;
};

typedef struct PHYS_BoxBoundary PHYS_BoxBoundary;
struct PHYS_BoxBoundary {
    PHYS_collider_id areas[6];
    PHYS_body_id positions[6];
};

typedef struct PHYS_BoxBoundary_Settings PHYS_BoxBoundary_Settings;
struct PHYS_BoxBoundary_Settings {
    vec3_f32 center;
    vec3_f32 extents;
};

PHYS_RigidBody      phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings);
PHYS_RigidBody      phys_world_add_box(PHYS_World* w, PHYS_Box_Settings settings);
PHYS_BoxBoundary    phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings);

void                phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object);
void                phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary* object);
