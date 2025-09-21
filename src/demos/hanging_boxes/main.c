#include "../demos_main.h"
#include "../demos_main.c"

typedef struct HangingBox HangingBox;
struct HangingBox {
    PHYS_RigidBody rigid_body;
    vec3_f32 extents;
};

typedef struct HangingBoxesState HangingBoxesState;
struct HangingBoxesState {
    Arena* state_arena;
    R_Handle cube_vertices;
    R_Handle cube_indices;
    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;

    PHYS_body_id anchor_id;
    PHYS_constraint_id anchor_to_box1;
    HangingBox box1;
    PHYS_constraint_id box1_to_box2;
    HangingBox box2;
};
static HangingBoxesState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    phys_dbg_d_ctx->color_mode = PHYS_DBG_DrawColorMode_Force;
    phys_dbg_d_ctx->do_constraints = true;
    phys_dbg_d_ctx->do_colliders = true;
    phys_dbg_d_ctx->do_contact_points = true;
    phys_dbg_d_ctx->do_contact_manifold = true;

    MS_LoadResult cube = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"), (MS_LoadSettings){});
    if (cube.error.length != 0) {
        fprintf(stderr, "%s\n", cube.error.data);
        return 1;
    }
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, cube.v.vertices_count*r_vertex_size(cube.v.flags), cube.v.vertices);
    s.cube_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, cube.v.indices_count*sizeof(*cube.v.indices), cube.v.indices);
    s.cube_flags = cube.v.flags;
    s.cube_topology = cube.v.topology;

    cs->camera.eye    = (vec3_f32){.x = 0,.y =-10,.z =40};
    cs->camera.target = (vec3_f32){.x = 0,.y =-10,.z = 0};
    cs->show_debug = true;

    s.state_arena = arena_alloc();
    return 0;
}
void demos_cleanup_hook(DEMOS_CommonState* cs) {}

void demos_world_start_hook(PHYS_World* w) {
    s.anchor_id = phys_world_add_body(w, (PHYS_Body){
        .position = make_3f32(0,0,0),
        .inv_mass = 0.f,
        .no_gravity = true,
    });

    s.box1.extents = make_3f32(1,1,1);
    PHYS_Box_Settings box1_settings = {
        .arena = s.state_arena,
        .mass = 1,
        .center = make_3f32(0,-4,0),
        .extents = s.box1.extents,
    };
    s.box1.rigid_body = phys_world_add_box(w, box1_settings);

    s.anchor_to_box1 = phys_world_add_constraint(w, (PHYS_Constraint){
        .compliance = 0.05f,
        .type = PHYS_ConstraintType_Distance,
        .distance = {
            .b1 = s.anchor_id,
            .b2 = s.box1.rigid_body.body_id,
            .d = 5.f,

            .is_offset = true,
            .offset2 = make_3f32(0,1,0),
        }
    });

    s.box2.extents = make_3f32(1,1,1);
    PHYS_Box_Settings box2_settings = {
        .arena = s.state_arena,
        .mass = 1,
        .center = make_3f32(0,-15,0),
        .linear_velocity = make_3f32(10,0,0),
        .extents = s.box2.extents,
    };
    s.box2.rigid_body = phys_world_add_box(w, box2_settings);

    s.box1_to_box2 = phys_world_add_constraint(w, (PHYS_Constraint){
        .compliance = 0.05f,
        .type = PHYS_ConstraintType_Distance,
        .distance = {
            .b1 = s.box1.rigid_body.body_id,
            .b2 = s.box2.rigid_body.body_id,
            .d = 9.f,

            .is_offset = true,
            .offset1 = make_3f32(0,-1,0),
            .offset2 = make_3f32(1,1,1),
        }
    });

    // s.box3.extents = make_3f32(10,1,10);
    // PHYS_Box_Settings box3_settings = {
    //     .arena = s.state_arena,
    //     .center = make_3f32(0,-20,0),
    //     .extents = s.box3.extents,
    //     .mass = 0,
    //     .no_gravity = true,
    // };
    // s.box3.rigid_body = phys_world_add_box(w, box3_settings);
}
void demos_world_end_hook(PHYS_World* w) {
    arena_clear(s.state_arena);
}

static void d_hanging_box(PHYS_World* w, HangingBox* hanging_box);

void demos_frame_hook(DEMOS_CommonState* cs) {
    d_hanging_box(cs->w, &s.box1);
    d_hanging_box(cs->w, &s.box2);
}

// helpers
static void d_hanging_box(PHYS_World* w, HangingBox* hanging_box) {
    PHYS_Body* body = phys_world_resolve_body(w, hanging_box->rigid_body.body_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(body->position),
        make_rotate_4x4f32(normalize_4f32(body->rotation))),
        make_scale_4x4f32(hanging_box->extents)
    );
    d_pbr_mesh(
        s.cube_vertices, s.cube_flags, s.cube_indices, s.cube_topology,
        t, make_3f32(0,1,0), 1.0, make_3f32(0.1,0.1,0.1)
    );
}