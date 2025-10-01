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

static void phys_world_add_collision_record(PHYS_World* w, PHYS_CollisionSubstepRecord info);

typedef union PHYS_ConstraintNodeValue {
    PHYS_Constraint constraint;
    PHYS_DependentConstraint dependent_constraint;
} PHYS_ConstraintNodeValue;

typedef struct PHYS_ConstraintNode PHYS_ConstraintNode;
struct PHYS_ConstraintNode {
    PHYS_ConstraintNode* prev;
    PHYS_ConstraintNode* next;
    PHYS_ConstraintNodeValue v;
    PHYS_constraint_id id;
};

typedef struct PHYS_ConstraintList PHYS_ConstraintList;
struct PHYS_ConstraintList {
    PHYS_ConstraintNode* first;
    PHYS_ConstraintNode* last;
};

#define PHYS_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 16
#define PHYS_DEPENDENT_CONSTRAINT_MAP_DEFAULT_SLOTS_COUNT 8
typedef struct PHYS_ConstraintMap PHYS_ConstraintMap;
struct PHYS_ConstraintMap {
    PHYS_ConstraintList* slots;
    u32 slots_count;
    u32 max_idx;
    PHYS_ConstraintNode* free_chain;
    u32 length;
};

internal PHYS_constraint_id phys_constraint_map_add_value(PHYS_ConstraintMap* map, Arena* arena, PHYS_ConstraintNodeValue v);
internal PHYS_ConstraintNode* phys_constraint_map_get_node(PHYS_ConstraintMap* map, u32 i);
internal PHYS_ConstraintNodeValue* phys_constraint_map_get_value(PHYS_ConstraintMap* map, PHYS_constraint_id id);
internal void phys_constraint_map_delete(PHYS_ConstraintMap* map, PHYS_constraint_id id);

typedef struct PHYS_ColliderNode PHYS_ColliderNode;
struct PHYS_ColliderNode {
    PHYS_ColliderNode* next;
    PHYS_ColliderNode* prev;
    PHYS_Collider v;
    PHYS_collider_id id;
};

typedef struct PHYS_ColliderList PHYS_ColliderList;
struct PHYS_ColliderList {
    PHYS_ColliderNode* first;
    PHYS_ColliderNode* last;
};

#define PHYS_COLLIDER_MAP_DEFAULT_SLOTS_COUNT 64
typedef struct PHYS_ColliderMap PHYS_ColliderMap;
struct PHYS_ColliderMap {
    PHYS_ColliderList* slots;
    u32 slots_count;
    u32 max_idx;
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

    Arena* prebuilt_arena; // @todo manage user calls which require allocation separate to world

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
    PHYS_ConstraintMap dependent_constraints;
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

// asserts
internal b32 phys_world_valid_radius(PHYS_World* w, f32 d);