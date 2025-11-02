#pragma once

// 
// units
// 
// @note units are m,kg,seconds (MKS),
#define PHYS_UNIT_KG(n)  (((f32)(n)))
#define PHYS_UNIT_G(n)   (((f32)(n))*0.001f)
#define PHYS_UNIT_KM(n)  (((f32)(n))*1000f)
#define PHYS_UNIT_M(n)   (((f32)(n)))
#define PHYS_UNIT_CM(n)  (((f32)(n))*0.01f)
#define PHYS_UNIT_MM(n)  (((f32)(n))*0.001f)
#define PHYS_UNIT_N(n)   (((f32)(n)))
#define PHYS_UNIT_NM(n)  (((f32)(n)))
#define PHYS_UNIT_J(n)   (((f32)(n)))

typedef struct PHYS_World PHYS_World;

// 
// bodies
// 
typedef union PHYS_body_id {
    struct {
        u32 idx;
        u32 version;
    };
    u64 v;
} PHYS_body_id;

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

// 
// constraints
// 
typedef union PHYS_constraint_id {
    struct {
        u32 idx;
        u32 version;
    };
    u64 v;
} PHYS_constraint_id;

// dependent constraints (not processed by themselves)
typedef PHYS_constraint_id PHYS_dependent_constraint_id;

typedef struct PHYS_DependentConstraint_Target PHYS_DependentConstraint_Target;
struct PHYS_DependentConstraint_Target {
    f32 value;
};

typedef struct PHYS_DependentConstraint_Limits PHYS_DependentConstraint_Limits;
struct PHYS_DependentConstraint_Limits {
    f32 min;
    f32 max;
};

typedef struct PHYS_DependentConstraint PHYS_DependentConstraint;
struct PHYS_DependentConstraint {
    f32 l;
    f32 compliance;

    union {
        PHYS_DependentConstraint_Target target;
        PHYS_DependentConstraint_Limits limits;
    };
};

// applied constraints
typedef struct PHYS_Constraint_Distance PHYS_Constraint_Distance;
struct PHYS_Constraint_Distance {
    PHYS_body_id body1, body2;

    f32 d;
    b32 unilateral; // eg. string
};

typedef struct PHYS_Constraint_AdvancedDistance PHYS_Constraint_AdvancedDistance;
struct PHYS_Constraint_AdvancedDistance {
    PHYS_body_id body1, body2;

    vec3_f32 offset1, offset2;

    b32 is_projected;
    // @note relative to body1
    vec3_f32 axis;

    f32 d;
    b32 unilateral;
};

typedef struct PHYS_Constraint_LinearDOFs PHYS_Constraint_LinearDOFs;
struct PHYS_Constraint_LinearDOFs {
    PHYS_body_id body1, body2;

    // @note relative to body1
    vec3_f32 axes[3];
    PHYS_DependentConstraint_Limits limits[3];
};

typedef struct PHYS_Constraint_Volume PHYS_Constraint_Volume;
struct PHYS_Constraint_Volume {
    PHYS_body_id bodies[4];
    f32 v_rest;
};

typedef struct PHYS_Constraint_GlobalVolume PHYS_Constraint_GlobalVolume;
struct PHYS_Constraint_GlobalVolume {
    f32 v_rest;
    f32 k; // overpressure factor

    PHYS_body_id* surface_bodies;
    u32 surface_bodies_count;

    u32* surface_indices; // @note assumed to be triangles
    u32 surface_indices_count;
};

typedef struct PHYS_Constraint_Orientation PHYS_Constraint_Orientation;
struct PHYS_Constraint_Orientation {
    PHYS_body_id body1, body2;
};

typedef struct PHYS_Constraint_Hinge PHYS_Constraint_Hinge;
struct PHYS_Constraint_Hinge {
    PHYS_body_id body1;
    PHYS_body_id body2;
    vec3_f32 a1, b1;
    vec3_f32 a2, b2;

    PHYS_dependent_constraint_id limit_angle;
    PHYS_dependent_constraint_id target_angle;
};

typedef struct PHYS_Constraint_Swing PHYS_Constraint_Swing;
struct PHYS_Constraint_Swing {
    PHYS_body_id body1;
    PHYS_body_id body2;
    vec3_f32 a1;
    vec3_f32 a2;
    PHYS_DependentConstraint_Limits limits;
};

typedef struct PHYS_Constraint_Twist PHYS_Constraint_Twist;
struct PHYS_Constraint_Twist {
    PHYS_body_id body1;
    PHYS_body_id body2;
    vec3_f32 a1, b1;
    vec3_f32 a2, b2;
    PHYS_DependentConstraint_Limits limits;
};

typedef enum PHYS_ConstraintType {
    PHYS_ConstraintType_Distance,
    PHYS_ConstraintType_AdvancedDistance,
    PHYS_ConstraintType_LinearDOFs,
    PHYS_ConstraintType_Volume,
    PHYS_ConstraintType_GlobalVolume,
    PHYS_ConstraintType_Orientation,
    PHYS_ConstraintType_Hinge,
    PHYS_ConstraintType_Swing,
    PHYS_ConstraintType_Twist,
    PHYS_ConstraintType_COUNT ENUM_CASE_UNUSED,
} PHYS_ConstraintType;

typedef struct PHYS_Constraint PHYS_Constraint;
struct PHYS_Constraint {
    PHYS_ConstraintType type;

    f32 l;
    f32 compliance;

    union {
        PHYS_Constraint_Distance            distance;
        PHYS_Constraint_AdvancedDistance    advanced_distance;
        PHYS_Constraint_LinearDOFs          linear_dofs;
        PHYS_Constraint_Volume              volume;
        PHYS_Constraint_GlobalVolume        global_volume;
        PHYS_Constraint_Orientation         orientation;
        PHYS_Constraint_Hinge               hinge;
        PHYS_Constraint_Swing               swing;
        PHYS_Constraint_Twist               twist;
    };
};

// settings
typedef struct PHYS_ConstraintSolveSettings PHYS_ConstraintSolveSettings;
struct PHYS_ConstraintSolveSettings {
    PHYS_World* w;
    f32 inv_dt;
    f32 inv_dt2;
};

// 
// colliders
// 
// @todo static colliders
typedef union PHYS_collider_id {
    struct {
        u32 idx;
        u32 version;
    };
    u64 v;
} PHYS_collider_id;

typedef enum PHYS_ColliderType {
    PHYS_ColliderType_Sphere,
    PHYS_ColliderType_Polytope,
    PHYS_ColliderType_COUNT ENUM_CASE_UNUSED,
} PHYS_ColliderType;

typedef union PHYS_ColliderLayer {
    struct {
        u32 mask;
        u32 group;
    };
    u64 v;
} PHYS_ColliderLayer;

#define PHYS_ColliderLayer_Invalid  ((PHYS_ColliderLayer){.v=0})
#define PHYS_ColliderLayer_1        ((PHYS_ColliderLayer){.mask=~0u,.group= 1u})
#define PHYS_ColliderLayer_1_No1    ((PHYS_ColliderLayer){.mask=~1u,.group= 1u})
#define PHYS_ColliderLayer_All_No1  ((PHYS_ColliderLayer){.mask=~1u,.group=~0u})
#define PHYS_ColliderLayer_All      ((PHYS_ColliderLayer){.mask=~0u,.group=~0u})


shared_function(phys_collider_layers_overlap)
b32 phys_collider_layers_overlap(PHYS_ColliderLayer l1, PHYS_ColliderLayer l2);

shared_function(phys_collider_layers_overlap)
b32 phys_collider_layers_equal(PHYS_ColliderLayer l1, PHYS_ColliderLayer l2);

typedef struct PHYS_Collider_Base PHYS_Collider_Base;
struct PHYS_Collider_Base {
    PHYS_ColliderType type;
    PHYS_body_id p;
    f32 r;
    f32 static_friction;
    f32 dynamic_friction;
    PHYS_ColliderLayer layer;
};

// @note assumed that faces are CCW and edges are joined in a ring
typedef struct PHYS_Collider_Polytope PHYS_Collider_Polytope;
struct PHYS_Collider_Polytope {
    PHYS_Collider_Base base;
    
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

typedef struct PHYS_CollisionCheck PHYS_CollisionCheck;
struct PHYS_CollisionCheck {
    vec3_f32 r1, r2, n;
    f32 d;
    u32 f1, f2;
};

// coefficients
typedef enum PHYS_CoefficientCalculation {
    PHYS_CoefficientCalculation_Min = 0,
    PHYS_CoefficientCalculation_Max,
    PHYS_CoefficientCalculation_Average,
} PHYS_CoefficientCalculation;

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

// 
// core api
// 

shared_function(phys_make_world)
PHYS_World* phys_make_world(PHYS_WorldSettings settings);


shared_function(phys_world_cleanup)
void phys_world_cleanup(PHYS_World* w);


shared_function(phys_world_add_body)
PHYS_body_id phys_world_add_body(PHYS_World* w, PHYS_Body b);

shared_function(phys_world_remove_body)
void phys_world_remove_body(PHYS_World* w, PHYS_body_id id);

shared_function(phys_world_resolve_body_unchecked)
PHYS_Body* phys_world_resolve_body_unchecked(PHYS_World* w, PHYS_body_id id);

shared_function(phys_world_resolve_body)
PHYS_Body* phys_world_resolve_body(PHYS_World* w, PHYS_body_id id);


shared_function(phys_world_add_collider)
PHYS_collider_id phys_world_add_collider(PHYS_World* w, PHYS_Collider c);

shared_function(phys_world_remove_collider)
void phys_world_remove_collider(PHYS_World* w, PHYS_collider_id id);

shared_function(phys_world_resolve_collider_unchecked)
PHYS_Collider* phys_world_resolve_collider_unchecked(PHYS_World* w, PHYS_collider_id id);

shared_function(phys_world_resolve_collider)
PHYS_Collider* phys_world_resolve_collider(PHYS_World* w, PHYS_collider_id id);


shared_function(phys_world_add_constraint)
PHYS_constraint_id phys_world_add_constraint(PHYS_World* w, PHYS_Constraint c);

shared_function(phys_world_remove_constraint)
void phys_world_remove_constraint(PHYS_World* w, PHYS_constraint_id id);

shared_function(phys_world_resolve_constraint_unchecked)
PHYS_Constraint* phys_world_resolve_constraint_unchecked(PHYS_World* w, PHYS_constraint_id id);

shared_function(phys_world_resolve_constraint)
PHYS_Constraint* phys_world_resolve_constraint(PHYS_World* w, PHYS_constraint_id id);


shared_function(phys_world_add_dependent_constraint)
PHYS_dependent_constraint_id phys_world_add_dependent_constraint(PHYS_World* w, PHYS_DependentConstraint c);

shared_function(phys_world_remove_dependent_constraint)
void phys_world_remove_dependent_constraint(PHYS_World* w, PHYS_dependent_constraint_id id);

shared_function(phys_world_resolve_dependent_constraint_unchecked)
PHYS_DependentConstraint* phys_world_resolve_dependent_constraint_unchecked(PHYS_World* w, PHYS_dependent_constraint_id id);

shared_function(phys_world_resolve_dependent_constraint)
PHYS_DependentConstraint* phys_world_resolve_dependent_constraint(PHYS_World* w, PHYS_dependent_constraint_id id);