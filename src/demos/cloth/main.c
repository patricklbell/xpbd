#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;

    R_Handle cloth_vertices;

    MS_Mesh cloth_mesh;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_DBG_DrawContext phys_dbg_draw_ctx;
    PHYS_Cloth cloth_phys;
    PHYS_RigidBody ball_phys;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    MS_LoadResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"), (MS_LoadSettings){});
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }
    s.sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*r_vertex_size(sphere.v.flags), sphere.v.vertices);
    s.sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);
    s.sphere_flags = sphere.v.flags;
    s.sphere_topology = sphere.v.topology;

    VTK_LoadResult cloth = vtk_load(cs->arena, ntstr8_lit("./data/cloth.vtk"), (VTK_LoadSettings){});
    if (cloth.error.length != 0) {
        fprintf(stderr, "%s\n", cloth.error.data);
        return 1;
    }
    
    s.camera.eye    = (vec3_f32){.x = 0,.y =-2,.z =5};
    s.camera.target = (vec3_f32){.x = 0,.y =-2,.z = 0};

    {DeferResource(Temp scratch = scratch_begin_a(cs->arena), scratch_end(scratch)) {
        s.world = phys_world_make((PHYS_WorldSettings){
            .substeps=2,
            .min_collision_distance = 0.001,
        }); 
        s.phys_dbg_draw_ctx = phys_dbg_d_make_context(s.world, dbgdraw_edge_batch, dbgdraw_point_batch);
        s.phys_dbg_draw_ctx.draw_forces = 1;
        s.phys_dbg_draw_ctx.min_force_color_hsl = make_3f32(240.f/360.f, 1.0, 0.5);
        s.phys_dbg_draw_ctx.max_force_color_hsl = make_3f32(000.f/360.f, 1.0, 0.5);
        s.phys_dbg_draw_ctx.body_radius = 0.01;

        u32 edge_indices_count;
        u32* edge_indices;
        geo_calculate_edges(scratch.arena,
            cloth.v.points_count*2, GEO_Topology_Quad, GEO_Connected_Ring,
            /*in*/ cloth.v.indices[VTK_CellType_Quad], cloth.v.indices_counts[VTK_CellType_Quad],
            /*out*/ &edge_indices, &edge_indices_count
        );

        PHYS_ClothFiber_Settings fibers_depth1[] = {
            {/*stretch-r*/ .compliance=0.f, .direction=make_3f32(1,0,0)},
            {/*stretch-u*/ .compliance=0.f, .direction=make_3f32(0,1,0)},
        };
        PHYS_ClothFiber_Settings fibers_depth2[] = {
            {/*shear-ur*/ .compliance=0.0001f, .direction=normalize_3f32(make_3f32(1,1,0))},
            {/*shear-dr*/ .compliance=0.0001f, .direction=normalize_3f32(make_3f32(1,-1,0))},
            {/*bend-rr*/ .compliance=1.f, .direction=make_3f32(1,0,0)},
            {/*bend-uu*/ .compliance=1.f, .direction=make_3f32(0,1,0)},
        };
        int fibers_counts[] = {ArrayLength(fibers_depth1), ArrayLength(fibers_depth2)};
        PHYS_ClothFiber_Settings* fibers[] = {fibers_depth1, fibers_depth2};
        u32 fiber_depth = ArrayLength(fibers);

        s.cloth_phys = phys_world_add_cloth(s.world, (PHYS_Cloth_Settings){
            .arena = cs->arena,
            .thickness = 0.01,
            .mass = 0.2f,
            .center = make_3f32(-0.5,0,-0.5),
            .rotation = make_axis_angle_quat(PI/2.f, normalize_3f32(make_3f32(1,0,0))),
            .vertices               = cloth.v.points,
            .vertices_count         = cloth.v.points_count,
            .edge_indices           = edge_indices,
            .edge_indices_count     = edge_indices_count,
            .fibers                 = fibers,
            .fibers_counts          = fibers_counts,
            .fiber_depth            = fiber_depth,
            .fiber_ratio_hint       = 6, // each body connects to 6 constraints
        });

        s.ball_phys = phys_world_add_ball(s.world, (PHYS_Ball_Settings){
            .center = make_3f32(0,-1,0),
            .mass=0.2f,
            .radius = 0.2f,
        });

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(2,2,2),
        });
    }}

    s.time = os_now_seconds();
    return 0;
}

static void d_ball(PHYS_RigidBody* ball) {
    PHYS_Body* center = phys_world_resolve_body(s.world, ball->body_id);
    PHYS_Collider* collider = phys_world_resolve_collider(s.world, ball->collider_id);
    f32 radius = collider->sphere.r;

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(radius, radius, radius))
    );
    d_mesh(s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology, R_Mesh3DMaterial_Lambertian, t, make_3f32(1,0,0));
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    f64 pdt = 1.f/60.f;
    s.time = ntime;

    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.world, pdt);

    r_window_begin_frame(cs->window, cs->rwindow);
    
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        d_ball(&s.ball_phys);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    // @todo draw buckets
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        dbgdraw_begin();
        phys_dbg_d_constraints(&s.phys_dbg_draw_ctx, NULL, 0);
        phys_dbg_d_bodies(&s.phys_dbg_draw_ctx);
        dbgdraw_submit(cs->window, cs->rwindow);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}