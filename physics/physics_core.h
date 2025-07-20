#pragma once

// @note units are generally assumed to be m,kg,seconds (MKS),
typedef struct PHYS_World PHYS_World;

// constraints
typedef u64 PHYS_body_id;
typedef struct PHYS_Body PHYS_Body;
struct PHYS_Body {
    vec3_f32    position;
    vec3_f32    prev_position;
    vec3_f32    velocity;
    f32         inv_mass;
    b32         no_gravity;
};

typedef struct PHYS_Constraint_StaticDistance PHYS_Constraint_StaticDistance;
struct PHYS_Constraint_StaticDistance {
    PHYS_body_id b;
    vec3_f32 p;
};

typedef struct PHYS_Constraint_Distance PHYS_Constraint_Distance;
struct PHYS_Constraint_Distance {
    PHYS_body_id b1;
    PHYS_body_id b2;
    f32 d;
};

typedef struct PHYS_Constraint_Volume PHYS_Constraint_Volume;
struct PHYS_Constraint_Volume {
    PHYS_body_id b[4];
    f32 d;
};

typedef enum PHYS_ConstraintType {
    PHYS_ConstraintType_StaticDistance,
    PHYS_ConstraintType_Distance,
    PHYS_ConstraintType_Volume,
    PHYS_ConstraintType_COUNT,
} PHYS_ConstraintType;

typedef struct PHYS_Constraint PHYS_Constraint;
struct PHYS_Constraint {
    PHYS_ConstraintType type;

    union {
        PHYS_Constraint_StaticDistance  static_distance;
        PHYS_Constraint_Distance        distance;
        PHYS_Constraint_Volume          volume;
    };
};

typedef struct PHYS_ConstraintSettings PHYS_ConstraintSettings;
struct PHYS_ConstraintSettings {
    PHYS_World* w;
    f64 a_dt2;
};

static void phys_constrain_static(PHYS_Constraint_StaticDistance c, PHYS_ConstraintSettings settings);
static void phys_constrain_distance(PHYS_Constraint_Distance c, PHYS_ConstraintSettings settings);
static void phys_constrain_volume(PHYS_Constraint_Volume c, PHYS_ConstraintSettings settings);

// colliders
typedef union PHYS_collider_id {
    struct {
        u32 id;
        u32 version;
    };
    u64 v;
} PHYS_collider_id;

typedef struct PHYS_Collider_DynamicSphere PHYS_Collider_DynamicSphere;
struct PHYS_Collider_DynamicSphere {
    PHYS_body_id c;
    f32 r;
};

typedef struct PHYS_Collider_StaticPlane PHYS_Collider_StaticPlane;
struct PHYS_Collider_StaticPlane {
    vec3_f32 p;
    vec3_f32 n;
};

typedef enum PHYS_ColliderType {
    PHYS_ColliderType_DynamicSphere,
    PHYS_ColliderType_StaticPlane,
    PHYS_ColliderType_COUNT,
} PHYS_ColliderType;

typedef struct PHYS_Collider PHYS_Collider;
struct PHYS_Collider {
    PHYS_ColliderType type;

    union {
        PHYS_Collider_DynamicSphere     dynamic_sphere;
        PHYS_Collider_StaticPlane       static_plane;
    };
};

static void phys_collide_dynamic_spheres(const PHYS_Collider_DynamicSphere* c1, PHYS_Collider_DynamicSphere* c2, PHYS_ConstraintSettings settings);
static void phys_collide_dynamic_sphere_with_static_plane(const PHYS_Collider_DynamicSphere* c1, PHYS_Collider_StaticPlane* c2, PHYS_ConstraintSettings settings);

// world
typedef struct PHYS_ConstraintNode PHYS_ConstraintNode;
struct PHYS_ConstraintNode {
    PHYS_ConstraintNode* next;
    PHYS_Constraint v;
    u32 version;
    u32 id;
};

#define PHYS_CONSTRAINT_MAP_NUM_SLOTS 16
typedef struct PHYS_ConstraintMap PHYS_ConstraintMap;
struct PHYS_ConstraintMap {
    PHYS_ConstraintNode** slots;
    u32 num_slots;
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

#define PHYS_COLLIDER_MAP_NUM_SLOTS 64
typedef struct PHYS_ColliderMap PHYS_ColliderMap;
struct PHYS_ColliderMap {
    PHYS_ColliderNode** slots;
    u32 num_slots;
    u32 max_id;
    PHYS_ColliderNode* free_chain;
};

#define PHYS_BODY_DYNAMIC_ARRAY_INITIAL_CAPACITY 1
#define PHYS_BODY_DYNAMIC_ARRAY_GROWTH 2
typedef struct PHYS_BodyDynamicArray PHYS_BodyDynamicArray;
struct PHYS_BodyDynamicArray {
    PHYS_Body* v;
    PHYS_body_id length;
    PHYS_body_id capacity;
};

typedef struct PHYS_World PHYS_World;
struct PHYS_World {
    Arena* arena;

    u64 substeps;    
    f32 little_g;
    f32 compliance;
    PHYS_ColliderMap colliders;
    PHYS_ConstraintMap constraints;
    PHYS_BodyDynamicArray bodies;
};

PHYS_World* phys_world_make();
void        phys_world_cleanup(PHYS_World* w);
void        phys_world_step(PHYS_World* w, f64 dt);
static void phys_world_substep(PHYS_World* w, f64 dt);

PHYS_body_id        phys_world_add_body(PHYS_World* w, PHYS_Body b);
void                phys_world_remove_body(PHYS_World* w, PHYS_body_id dp);
PHYS_Body*          phys_world_resolve_body(PHYS_World* w, PHYS_body_id dp);

PHYS_collider_id    phys_world_add_collider(PHYS_World* w, PHYS_Collider c);
void                phys_world_remove_collider(PHYS_World* w, PHYS_collider_id col);
PHYS_Collider*      phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id col);

// @todo add, remove, resolve constraint

// helper objects
typedef struct PHYS_Ball_Settings PHYS_Ball_Settings;
struct PHYS_Ball_Settings {
    f32 radius;
    f32 mass;
    vec3_f32 center;
    vec3_f32 velocity;
};

typedef struct PHYS_Ball PHYS_Ball;
struct PHYS_Ball {
    PHYS_collider_id sphere;
    PHYS_body_id center;
};

typedef struct PHYS_BoxBoundary PHYS_BoxBoundary;
struct PHYS_BoxBoundary {
    PHYS_collider_id areas[6];
};

typedef struct PHYS_BoxBoundary_Settings PHYS_BoxBoundary_Settings;
struct PHYS_BoxBoundary_Settings {
    vec3_f32 center;
    vec3_f32 extents;
};

PHYS_Ball           phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings);
void                phys_world_remove_ball(PHYS_World* w, PHYS_Ball object);
PHYS_BoxBoundary    phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings);
void                phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary object);
