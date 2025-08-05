#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle bunny_vertices;
    MS_Mesh bunny_mesh;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_DBG_DrawContext phys_dbg_draw_ctx;

    PHYS_Softbody bunny_phys;
    u32* bunny_phys_surface_triangle_body_indices;
    u32 bunny_phys_surface_triangle_body_indices_count;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult bunny = vtk_load(cs->arena, ntstr8_lit("./data/bunny.vtk"), (VTK_LoadSettings){});
    if (bunny.error.length != 0) {
        fprintf(stderr, "%s\n", bunny.error.data);
        return 1;
    }
    
    s.camera.eye    = (vec3_f32){.x = 0,.y =-2,.z =15};
    s.camera.target = (vec3_f32){.x = 0,.y =-2,.z = 0};
    
    {DeferResource(Temp scratch = scratch_begin_a(cs->arena), scratch_end(scratch)) {
        s.world = phys_make_world((PHYS_WorldSettings){.substeps=8,.linear_damping=0.1}); 
        s.phys_dbg_draw_ctx = phys_dbg_d_make_context(s.world, dbgdraw_edge_batch, dbgdraw_point_batch);
        s.phys_dbg_draw_ctx.draw_forces = 1;
        s.phys_dbg_draw_ctx.min_force_color_hsl = make_3f32(240.f/360.f, 1.0, 0.5);
        s.phys_dbg_draw_ctx.max_force_color_hsl = make_3f32(000.f/360.f, 1.0, 0.5);

        // @note allocates to cs arena for use later
        u32* surface_body_indices;
        u32 surface_body_indices_count;
        geo_calculate_points(cs->arena,
            /*in*/ bunny.v.points_count, bunny.v.indices[VTK_CellType_Triangle], bunny.v.indices_counts[VTK_CellType_Triangle],
            /*out*/ &surface_body_indices, &surface_body_indices_count
        );

        u32 volume_edge_indices_count;
        u32* volume_edge_indices;
        geo_calculate_edges(scratch.arena,
            bunny.v.points_count/2, GEO_Topology_Tetrahedron, GEO_Connected_Strongly,
            /*in*/ bunny.v.indices[VTK_CellType_Tetrahedron], bunny.v.indices_counts[VTK_CellType_Tetrahedron],
            /*out*/ &volume_edge_indices, &volume_edge_indices_count
        );

        s.bunny_phys = phys_world_add_softbody(s.world, (PHYS_TetTriSoftbody_Settings){
            .arena = cs->arena,
            .mass = 0.5f,
            .edge_compliance = 0.5f,
            .volume_compliance = 0.f,
            .center = make_3f32(0,0,0),
            .vertices                       = bunny.v.points,
            .vertices_count                 = bunny.v.points_count,
            .tetrahedron_edge_indices       = volume_edge_indices,
            .tetrahedron_edge_indices_count = volume_edge_indices_count,
            .tetrahedron_indices            = bunny.v.indices[VTK_CellType_Tetrahedron],
            .tetrahedron_indices_count      = bunny.v.indices_counts[VTK_CellType_Tetrahedron],
            .surface_point_indices          = surface_body_indices,
            .surface_point_indices_count    = surface_body_indices_count,
        });
        s.bunny_phys_surface_triangle_body_indices = bunny.v.indices[VTK_CellType_Triangle];
        s.bunny_phys_surface_triangle_body_indices_count = bunny.v.indices_counts[VTK_CellType_Triangle];

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(4,4,4)
        });
    }}

    // setup visual mesh
    s.bunny_mesh.flags = R_VertexFlag_PN;
    s.bunny_mesh.topology = R_VertexTopology_Triangles;
    s.bunny_mesh.vertices_count = s.bunny_phys_surface_triangle_body_indices_count;
    s.bunny_mesh.vertices = arena_push(cs->arena, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), r_vertex_align(s.bunny_mesh.flags));
    s.bunny_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), NULL);

    s.time = os_now_seconds();
    return 0;
}

static void d_bunny() {
    MS_Mesh* m = &s.bunny_mesh;

    void* p_start = m->vertices + r_vertex_offset(m->flags, R_VertexFlag_P);
    void* n_start = m->vertices + r_vertex_offset(m->flags, R_VertexFlag_N);
    u64 p_stride = r_vertex_stride(m->flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(m->flags, R_VertexFlag_N);

    void* p = p_start;
    for EachIndex(i, s.bunny_phys_surface_triangle_body_indices_count) {
        u32 indice = s.bunny_phys_surface_triangle_body_indices[i];
        vec3_f32 body_p = phys_world_resolve_body(s.world, s.bunny_phys.vertices[indice])->position;

        *((R_VertexType_P*)p) = body_p;

        p+=p_stride;
    }

    geo_calculate_flat_normals(
        (vec3_f32*)p_start, p_stride, m->vertices_count,
        (vec3_f32*)n_start, n_stride
    );

    r_buffer_load(s.bunny_vertices, 0, m->vertices_count*r_vertex_size(m->flags), m->vertices);

    d_mesh(s.bunny_vertices, m->flags, r_zero_handle(), m->topology, R_Mesh3DMaterial_Lambertian, make_diagonal_4x4f32(1.0f), make_3f32(1,0,0));
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
        d_bunny();
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    // @todo draw buckets
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        dbgdraw_begin();
        PHYS_ConstraintType blacklist[] = {PHYS_ConstraintType_Volume};
        phys_dbg_d_constraints(&s.phys_dbg_draw_ctx, blacklist, ArrayLength(blacklist));
        dbgdraw_submit(cs->window, cs->rwindow);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}