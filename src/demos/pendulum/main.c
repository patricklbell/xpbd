#include "../demos_main.h"
#include "../demos_main.c"

typedef struct DEMO_PendulumArm DEMO_PendulumArm;
struct DEMO_PendulumArm {
    PHYS_RigidBody rb;
    vec3_f32 extents;
};

typedef struct DEMO_PendulumState DEMO_PendulumState;
struct DEMO_PendulumState {
    Arena* state_arena;

    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;
    R_Handle cube_vertices;
    R_Handle cube_indices;
    
    DEMO_PendulumArm arms[3]; 
};
static DEMO_PendulumState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    phys_dbg_d_ctx->color_mode = PHYS_DBG_DrawColorMode_Type;
    phys_dbg_d_ctx->do_contact_points = true;
    phys_dbg_d_ctx->do_contact_manifold = true;
    phys_dbg_d_ctx->do_collider_normals = true;
    phys_dbg_d_ctx->do_colliders = true;
    phys_dbg_d_ctx->default_point_radius = 0.01;

    MS_LoadResult res = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"), (MS_LoadSettings){});
    if (res.error.length != 0) {
        fprintf(stderr, "%s\n", res.error.data);
        return 1;
    }

    s.cube_flags = res.v.flags;
    s.cube_topology = res.v.topology;
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, res.v.vertices_count*r_vertex_size(s.cube_flags), res.v.vertices);
    s.cube_indices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, res.v.indices_count*sizeof(*res.v.indices), res.v.indices);

    cs->camera.eye    = (vec3_f32){.x = 0,.y = -1,.z = 5};
    cs->camera.target = (vec3_f32){.x = 0,.y = -1,.z = 0};

    s.state_arena = arena_alloc();
    return 0;
}
void demos_cleanup_hook(DEMOS_CommonState* cs) {}

void demos_world_start_hook(PHYS_World* w) {
    // bodies
    PHYS_body_id anchor_id = phys_world_add_body(w, (PHYS_Body){
        .position=make_3f32(0,0,0),
        .is_particle=true,
        .no_gravity=true,
    });

    // constraints
    f32 arm_half_length = 0.5, arm_half_width = 0.1, arm_half_depth = 0.05;
    f32 arm_bearing_inset = 0.1;
    f32 arm_inner_half_length = arm_half_length - arm_bearing_inset;
    PHYS_body_id prev_id = anchor_id;
    for EachElement(i, s.arms) {
        f32 z_offset = (i%2 == 0) ? -arm_half_depth : +arm_half_depth;

        s.arms[i].extents = make_3f32(arm_half_length,arm_half_width,arm_half_depth*0.9f);
        s.arms[i].rb = phys_world_add_box(w, (PHYS_Box_Settings){
            .arena=s.state_arena,
            .center=make_3f32((2*i+1)*arm_inner_half_length,0,z_offset),
            .extents=s.arms[i].extents,
            .linear_velocity=make_3f32(0,(i-1.f)*5.f,0),
            .mass=PHYS_UNIT_G(50),
        });
        PHYS_body_id curr_id = s.arms[i].rb.body_id;
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Distance,
            .distance={
                .b1=prev_id,
                .b2=curr_id,
                .d=0.f,
                .is_offset=true,
                .offset1=(i == 0) ? make_3f32(0,0,0) : make_3f32(+arm_inner_half_length,0,+z_offset),
                .offset2=                              make_3f32(-arm_inner_half_length,0,-z_offset),
            },
        });
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Hinge,
            .hinge={
                .b1=prev_id,
                .b2=curr_id,
                .a1=make_3f32(0,0,1),
                .a2=make_3f32(0,0,-1),
            },
        });

        prev_id = curr_id;
    }
}
void demos_world_end_hook(PHYS_World* w) {}

static void d_arm(PHYS_World* w, DEMO_PendulumArm* arm);

void demos_frame_hook(DEMOS_CommonState* cs) {
    for EachElement(i, s.arms) {
        d_arm(cs->w, &s.arms[i]);
    }
}

// helpers
static void d_arm(PHYS_World* w, DEMO_PendulumArm* arm) {
    PHYS_Body* body = phys_world_resolve_body(w, arm->rb.body_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(body->position),
        make_rotate_4x4f32(normalize_4f32(body->rotation))),
        make_scale_4x4f32(arm->extents)
    );
    d_pbr_mesh(
        s.cube_vertices, s.cube_flags, s.cube_indices, s.cube_topology,
        t, hsl_to_rgb(make_3f32(-PI/3.f, 0.9, 1)), 1.0, make_3f32(0,0,0)
    );
}