#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle bunny_vertices;
    R_Handle bunny_indices;

    VTK_Data bunny_vtk;
    MS_Mesh bunny_mesh;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_Softbody bunny_phys;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult bunny = vtk_load(cs->arena, ntstr8_lit("./data/bunny.vtk"), (VTK_LoadSettings){
        .calculate_volume_edges = 1,
    });
    if (bunny.error.length != 0) {
        fprintf(stderr, "%s\n", bunny.error.data);
        return 1;
    }
    s.bunny_vtk = bunny.v;
    
    s.camera.eye    = (vec3_f32){.x = 0,.y =-2,.z =15};
    s.camera.target = (vec3_f32){.x = 0,.y =-2,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){.substeps=8}); 

        s.bunny_phys = phys_world_add_tet_tri_softbody(s.world, (PHYS_TetTriSoftbody_Settings){
            .arena = cs->arena,
            .mass = 0.5f,
            .edge_compliance = 1.f,
            .volume_compliance = 0.f,
            .collider_compliance = 0.f,
            .center = make_3f32(0,0,0),
            .vertices               = s.bunny_vtk.points,
            .vertices_count         = s.bunny_vtk.points_count,
            .tet_edge_indices       = s.bunny_vtk.volume_edge_indices,
            .tet_edge_indices_count = s.bunny_vtk.volume_edge_indices_count,
            .tet_indices            = s.bunny_vtk.volume_indices,
            .tet_indices_count      = s.bunny_vtk.volume_indices_count,
            .surface_indices        = s.bunny_vtk.surface_indices,
            .surface_indices_count  = s.bunny_vtk.surface_indices_count,
        });

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(4,4,4)
        });
    }

    s.bunny_mesh.flags = R_VertexFlag_PN;
    s.bunny_mesh.topology = R_VertexTopology_Triangles;
    s.bunny_mesh.indices_count = s.bunny_vtk.surface_indices_count;
    s.bunny_mesh.vertices_count = s.bunny_vtk.surface_indices_count;
    s.bunny_mesh.indices = push_array(cs->arena, u32, s.bunny_mesh.indices_count);
    for EachIndex(i, s.bunny_mesh.indices_count) s.bunny_mesh.indices[i] = i;
    s.bunny_mesh.vertices = arena_push(cs->arena, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), r_vertex_align(s.bunny_mesh.flags));

    s.bunny_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), s.bunny_mesh.vertices);
    s.bunny_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, s.bunny_mesh.indices_count*sizeof(*s.bunny_mesh.indices), s.bunny_mesh.indices);

    s.time = os_now_seconds();
    return 0;
}

void d_sofbody(PHYS_World* world, MS_Mesh* mesh, PHYS_Softbody* softbody) {
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

    r_buffer_load(s.bunny_vertices, 0, mesh->vertices_count*r_vertex_size(mesh->flags), mesh->vertices);

    d_mesh(s.bunny_vertices, mesh->flags, s.bunny_indices, mesh->topology, make_diagonal_4x4f32(1.0f), make_3f32(1,0,0));
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    f64 pdt = 1.f/60.f;
    s.time = ntime;

    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.world, pdt);
    
    d_begin_pipeline();
    d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        d_sofbody(s.world, &s.bunny_mesh, &s.bunny_phys);
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}