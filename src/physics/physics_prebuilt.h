// rigid bodies
typedef struct PHYS_RigidBody PHYS_RigidBody;
struct PHYS_RigidBody {
    PHYS_body_id        body_id;
    PHYS_collider_id    collider_id;
};

void phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object);

typedef struct PHYS_Ball_Settings PHYS_Ball_Settings;
struct PHYS_Ball_Settings {
    f32 radius;
    f32 mass;
    f32 compliance;
    vec3_f32 center;
    vec3_f32 linear_velocity;
};

PHYS_RigidBody phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings);

typedef struct PHYS_Box_Settings PHYS_Box_Settings;
struct PHYS_Box_Settings {
    f32 mass;
    f32 compliance;
    vec3_f32 center;
    vec3_f32 extents;
    vec3_f32 linear_velocity;
    vec3_f32 angular_velocity;
};

PHYS_RigidBody phys_world_add_box(PHYS_World* w, PHYS_Box_Settings settings);

// box boundary
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

PHYS_BoxBoundary    phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings);
void                phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary* object);

// softbody
typedef struct PHYS_Softbody PHYS_Softbody;
struct PHYS_Softbody {
    u32 vertices_count;
    PHYS_body_id* vertices;

    u32 triangle_colliders_count;
    PHYS_collider_id* triangle_colliders;

    u32 distance_constraints_count;
    PHYS_constraint_id* distance_constraints;

    u32 volume_constraints_count;
    PHYS_constraint_id* volume_constraints;
};
typedef struct PHYS_TetTriSoftbody_Settings PHYS_TetTriSoftbody_Settings;
struct PHYS_TetTriSoftbody_Settings {
    Arena* arena;
    
    f32 mass;
    f32 edge_compliance;
    f32 volume_compliance;
    vec3_f32 center;
    vec3_f32 linear_velocity;

    vec3_f32* vertices;
    u32 vertices_count;

    u32* tet_edge_indices;
    u32 tet_edge_indices_count;
    u32* tet_indices;
    u32 tet_indices_count;

    u32* surface_indices;
    u32 surface_indices_count;
};

PHYS_Softbody   phys_world_add_tet_tri_softbody(PHYS_World* w, PHYS_TetTriSoftbody_Settings settings);
void            phys_world_remove_softbody(PHYS_World* w, PHYS_Softbody object);

// cloth
typedef struct PHYS_Cloth PHYS_Cloth;
struct PHYS_Cloth {
    u32 vertices_count;
    PHYS_body_id* vertices;

    u32 triangle_colliders_count;
    PHYS_collider_id* triangle_colliders;

    u32 distance_constraints_count;
    PHYS_constraint_id* distance_constraints;
};
typedef struct PHYS_Cloth_Settings PHYS_Cloth_Settings;
struct PHYS_Cloth_Settings {
    Arena* arena;
    
    f32 mass;
    vec3_f32 center;
    vec3_f32 linear_velocity;

    vec3_f32* vertices;
    u32 vertices_count;

    u32* edge_indices;
    u32 edge_indices_count;

    u32* surface_indices;
    u32 surface_indices_count;
};

PHYS_Cloth  phys_world_add_cloth(PHYS_World* w, PHYS_Cloth_Settings settings);
void        phys_world_remove_cloth(PHYS_World* w, PHYS_Cloth object);