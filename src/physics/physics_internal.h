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

typedef struct PHYS_U32Node PHYS_U32Node;
struct PHYS_U32Node {
    PHYS_U32Node* next;
    u32 v;
};

typedef union PHYS_pooled_array_id {
    struct {
        u32 idx;
        s32 version;
    };
    u64 v;
} PHYS_pooled_array_id;

typedef struct PHYS_PooledArray PHYS_PooledArray;
struct PHYS_PooledArray {
    void* v;
    s32* versions;
    u32 length;
    u32 capacity;
    u32 growth;
    u32 item_size;
    Arena* free_arena;
    PHYS_U32Node* free;
};

internal void phys_pooled_array_alloc(PHYS_PooledArray* pa, u32 item_size, u32 growth, u32 inital_capacity);
internal void phys_pooled_array_release(PHYS_PooledArray* pa);
internal void phys_pooled_array_adjust_allocation(PHYS_PooledArray* pa);
internal PHYS_pooled_array_id phys_pooled_array_add(PHYS_PooledArray* pa);
internal void phys_pooled_array_remove(PHYS_PooledArray* pa, PHYS_pooled_array_id id);

#define PHYS_EachPA(array) (u32 idx = 0; idx < array.length; idx++) 
#define PHYS_EachPADef(array, obj_type, name)               if (array.versions[idx] < 0) {continue;} obj_type* name = &((obj_type*)array.v)[idx];
#define PHYS_EachPADefId(array, obj_type, id_type, name)    PHYS_EachPADef(array, obj_type, name); id_type name##_id = {.idx=idx, .version=(u32)array.versions[idx]};

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

    // @todo
    b32 enable_particle_ground_plane;
    f32 particle_ground_plane_height;

    PHYS_CoefficientCalculation restitution_calculation;
    PHYS_CoefficientCalculation static_friction_calculation;
    PHYS_CoefficientCalculation dynamic_friction_calculation;

    f32 min_r;
    f32 min_v_mult;
    f32 hashgrid_cell_size;
    f32 hashgrid_obj_size;

    PHYS_PooledArray bodies;
    PHYS_PooledArray colliders;
    PHYS_PooledArray constraints;
    PHYS_PooledArray dependent_constraints;

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