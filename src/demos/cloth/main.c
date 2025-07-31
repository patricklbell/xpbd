#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle cloth_vertices;

    VTK_Data cloth_vtk;
    MS_Mesh cloth_mesh;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_DBG_DrawContext phys_dbg_draw_ctx;
    PHYS_Cloth cloth_phys;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult cloth = vtk_load(cs->arena, ntstr8_lit("./data/cloth.vtk"), (VTK_LoadSettings){
        .calculate_surface_edges = 1,
    });
    if (cloth.error.length != 0) {
        fprintf(stderr, "%s\n", cloth.error.data);
        return 1;
    }
    s.cloth_vtk = cloth.v;
    
    s.camera.eye    = (vec3_f32){.x = 0,.y =-2,.z =5};
    s.camera.target = (vec3_f32){.x = 0,.y =-2,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){.substeps=8}); 
        s.phys_dbg_draw_ctx = phys_dbg_d_make_context(s.world, dbgdraw_edge_batch, dbgdraw_point_batch);
        s.phys_dbg_draw_ctx.draw_forces = 1;
        s.phys_dbg_draw_ctx.min_force_color_hsl = make_3f32(240.f/360.f, 1.0, 0.5);
        s.phys_dbg_draw_ctx.max_force_color_hsl = make_3f32(000.f/360.f, 1.0, 0.5);

        s.cloth_phys = phys_world_add_cloth(s.world, (PHYS_Cloth_Settings){
            .arena = cs->arena,
            .mass = 0.2f,
            .center = make_3f32(0,0,0),
            .linear_velocity = make_3f32(0.5,0,0),
            .vertices               = s.cloth_vtk.points,
            .vertices_count         = s.cloth_vtk.points_count,
            .edge_indices           = s.cloth_vtk.surface_edge_indices,
            .edge_indices_count     = s.cloth_vtk.surface_edge_indices_count,
            .surface_indices        = s.cloth_vtk.surface_indices,
            .surface_indices_count  = s.cloth_vtk.surface_indices_count,
        });

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(2,2,2)
        });
    }

    s.cloth_mesh.flags = R_VertexFlag_PN;
    s.cloth_mesh.topology = R_VertexTopology_Triangles;
    s.cloth_mesh.vertices_count = s.cloth_vtk.surface_indices_count;
    s.cloth_mesh.vertices = arena_push(cs->arena, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), r_vertex_align(s.cloth_mesh.flags));
    s.cloth_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), s.cloth_mesh.vertices);

    s.time = os_now_seconds();
    return 0;
}

static void d_cloth(PHYS_World* world, MS_Mesh* mesh, PHYS_Cloth* softbody) {
    void* p = mesh->vertices + r_vertex_offset(mesh->flags, R_VertexFlag_P);
    u64 p_stride = r_vertex_stride(mesh->flags, R_VertexFlag_P);
    for EachIndex(tri_i, softbody->triangle_colliders_count) {
        PHYS_Collider_Triangle* tri = &phys_world_resolve_collider(world, softbody->triangle_colliders[tri_i])->triangle;

        for EachElement(p_i, tri->p) {
            *((R_VertexType_P*)p) = phys_world_resolve_body(world, tri->p[p_i])->position;
            p+=p_stride;
        }
    }

    // recalculate normals with new positions
    ms_calculate_flat_normals(mesh, mesh->topology);

    r_buffer_load(s.cloth_vertices, 0, mesh->vertices_count*r_vertex_size(mesh->flags), mesh->vertices);

    d_mesh(s.cloth_vertices, mesh->flags, r_zero_handle(), mesh->topology, R_Mesh3DMaterial_Lambertian, make_diagonal_4x4f32(1.0f), make_3f32(1,0,1));
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
        d_cloth(s.world, &s.cloth_mesh, &s.cloth_phys);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    // @todo draw buckets
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        dbgdraw_begin();
        phys_dbg_d_constraints(&s.phys_dbg_draw_ctx, NULL, 0);
        dbgdraw_submit(cs->window, cs->rwindow);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}