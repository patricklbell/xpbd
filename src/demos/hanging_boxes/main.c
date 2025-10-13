#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_HangingBox DEMO_HangingBox;
struct DEMO_HangingBox {
    PHYS_RigidBody rigid_body;
    vec3_f32 extents;
};

typedef struct DEMO_HangingBoxesState DEMO_HangingBoxesState;
struct DEMO_HangingBoxesState {
    R_Handle cube_vertices;
    R_Handle cube_indices;
    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;

    PHYS_body_id anchor_id;
    PHYS_constraint_id anchor_to_box1;
    DEMO_HangingBox box1;
    PHYS_constraint_id box1_to_box2;
    DEMO_HangingBox box2;
};

global DEMO_HangingBoxesState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
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

    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, -10, 40), /*target*/ make_3f32(0, -10, 0));
    cs->show_debug = true;

    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {
    s.anchor_id = phys_world_add_body(w, (PHYS_Body){
        .position = make_3f32(0,0,0),
        .no_gravity = true,
        .inv_mass = 0.f,
    });

    s.box1.extents = make_3f32(1,1,1);
    s.box1.rigid_body = phys_world_add_box(w, (PHYS_Box_Settings){
        .mass = PHYS_UNIT_KG(1),
        .center = make_3f32(0,-4,0),
        .extents = s.box1.extents,
    });

    s.anchor_to_box1 = phys_world_add_constraint(w, (PHYS_Constraint){
        .type = PHYS_ConstraintType_AdvancedDistance,
        .compliance = PHYS_UNIT_NM(0.0005),
        .advanced_distance = {
            .body1 = s.anchor_id,
            .body2 = s.box1.rigid_body.body_id,

            .offset2 = make_3f32(0,1,0),

            .d = 5.f,
        }
    });

    s.box2.extents = make_3f32(2,2,2);
    s.box2.rigid_body = phys_world_add_box(w, (PHYS_Box_Settings) {
        .mass = PHYS_UNIT_KG(10),
        .center = make_3f32(0,-15,0),
        .extents = s.box2.extents,
    });

    s.box1_to_box2 = phys_world_add_constraint(w, (PHYS_Constraint){
        .type = PHYS_ConstraintType_AdvancedDistance,
        .compliance = PHYS_UNIT_NM(0.0005),
        .advanced_distance = {
            .body1 = s.box1.rigid_body.body_id,
            .body2 = s.box2.rigid_body.body_id,
            
            .offset1 = make_3f32(0,-1,0),
            .offset2 = make_3f32(0,2,0),

            .d = 9.f,
        }
    });
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_hanging_box(PHYS_World* w, DEMO_HangingBox* hanging_box);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    d_hanging_box(cs->w, &s.box1);
    d_hanging_box(cs->w, &s.box2);
}

// helpers
internal void d_hanging_box(PHYS_World* w, DEMO_HangingBox* hanging_box) {
    PHYS_Body* body = phys_world_resolve_body_unchecked(w, hanging_box->rigid_body.body_id);

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