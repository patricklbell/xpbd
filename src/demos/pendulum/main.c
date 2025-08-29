#include "../demos_main.h"
#include "../demos_main.c"

typedef struct DEMO_PendulumState DEMO_PendulumState;
struct DEMO_PendulumState {
    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;
    R_Handle cube_vertices;
    R_Handle cube_indices;

    DEMOS_Camera camera;

    PHYS_World* phys_world;
    PHYS_DBG_DrawContext phys_dbg_d_ctx;
    PHYS_RigidBody rod1, rod2;
    
    f64 time;
};
static DEMO_PendulumState s;

int demos_persistent_init_hook(DEMOS_CommonState* cs) {
    MS_LoadResult res = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"), (MS_LoadSettings){});
    if (res.error.length != 0) {
        fprintf(stderr, "%s\n", res.error.data);
        return 1;
    }

    s.cube_flags = res.v.flags;
    s.cube_topology = res.v.topology;
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, res.v.vertices_count*r_vertex_size(s.cube_flags), res.v.vertices);
    s.cube_indices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, res.v.indices_count*sizeof(*res.v.indices), res.v.indices);

    s.camera.eye    = (vec3_f32){.x = 0,.y = -1,.z = 5};
    s.camera.target = (vec3_f32){.x = 0,.y = -1,.z = 0};

    return 0;
}

void demos_state_init_hook() {
    s.phys_world = phys_make_world((PHYS_WorldSettings){});
    s.phys_dbg_d_ctx = phys_dbg_d_make_context(s.phys_world, &dbgdraw_edge_batch, &dbgdraw_point_batch);
    s.phys_dbg_d_ctx.color_mode = PHYS_DBG_DrawColorMode_Type;
    s.phys_dbg_d_ctx.body_radius = 0.007;

    f32 hl = 0.6;
    f32 ht = 0.01;
    f32 inset = 0.05;

    // bodies
    PHYS_body_id anchor_id = phys_world_add_body(s.phys_world, (PHYS_Body){
        .position=make_3f32(inset,0,0),
        .is_particle=true,
        .no_gravity=true,
    });
    s.rod1 = phys_world_add_box(s.phys_world, (PHYS_Box_Settings){
        .center=make_3f32(hl,0,0),
        .extents=make_3f32(hl,hl/10,ht),
        .mass=PHYS_UNIT_G(500),
    });
    s.rod2 = phys_world_add_box(s.phys_world, (PHYS_Box_Settings){
        .center=make_3f32(3*hl-inset,0,0),
        .extents=make_3f32(hl,hl/10,ht),
        .linear_velocity=make_3f32(0,-10,0),
        .mass=PHYS_UNIT_G(500),
    });

    // constraints
    phys_world_add_constraint(s.phys_world, (PHYS_Constraint){
        .type=PHYS_ConstraintType_Distance,
        .distance={
            .b1=anchor_id,
            .b2=s.rod1.body_id,
            .d=0.f,
            .is_offset=true,
            .offset1=make_3f32(0,0,0),
            .offset2=make_3f32(-hl+inset,0,0),
        },
    });
    phys_world_add_constraint(s.phys_world, (PHYS_Constraint){
        .type=PHYS_ConstraintType_Hinge,
        .hinge={
            .b1=anchor_id,
            .b2=s.rod1.body_id,
            .a1=make_3f32(0,0,1),
            .a2=make_3f32(0,0,1),
        },
    });
    phys_world_add_constraint(s.phys_world, (PHYS_Constraint){
        .type=PHYS_ConstraintType_Distance,
        .distance={
            .b1=s.rod1.body_id,
            .b2=s.rod2.body_id,
            .d=0.f,
            .is_offset=true,
            .offset1=make_3f32( hl-inset,0,0),
            .offset2=make_3f32(-hl+inset,0,0),
        },
    });
    phys_world_add_constraint(s.phys_world, (PHYS_Constraint){
        .type=PHYS_ConstraintType_Hinge,
        .hinge={
            .b1=s.rod1.body_id,
            .b2=s.rod2.body_id,
            .a1=make_3f32(0,0,1),
            .a2=make_3f32(0,0,1),
        },
    });

    s.time = os_now_seconds();
}

static void d_box(PHYS_RigidBody* rb);

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    f64 pdt = 1.f/60.f;
    s.time = ntime;

    input_update(&cs->events);
    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.phys_world, pdt);

    r_window_begin_frame(cs->window, cs->rwindow);
    
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        d_box(&s.rod1);
        d_box(&s.rod2);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_state_cleanup_hook() {
    phys_world_cleanup(s.phys_world);
}

void demos_persistent_cleanup_hook(DEMOS_CommonState* cs) {
    return;
}

// helpers
static void d_box(PHYS_RigidBody* rb) {
    PHYS_Body* body = phys_world_resolve_body(s.phys_world, rb->body_id);
    PHYS_Collider* collider = phys_world_resolve_collider(s.phys_world, rb->collider_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(body->position),
        make_rotate_4x4f32(normalize_4f32(body->rotation))),
        make_scale_4x4f32(collider->rect_cuboid.r)
    );
    d_pbr_mesh(
        s.cube_vertices, s.cube_flags, s.cube_indices, s.cube_topology,
        t, make_3f32(1,1,1), 1.0, make_3f32(0.1,0.1,0.1)
    );
}