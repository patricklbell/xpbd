#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle cube_vertices;
    R_Handle cube_indices;

    VTK_Data cube_vtk;
    MS_Mesh cube_mesh;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_Softbody cube_phys;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult cube = vtk_load(cs->arena, ntstr8_lit("./data/cube.vtk"), (VTK_LoadSettings){
        .calculate_volume_edges = 1,
    });
    if (cube.error.length != 0) {
        fprintf(stderr, "%s\n", cube.error.data);
        return 1;
    }
    s.cube_vtk = cube.v;
    
    s.camera.eye    = (vec3_f32){.x = 0,.y = 0,.z =15};
    s.camera.target = (vec3_f32){.x = 0,.y = 0,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){.substeps=1}); 

        s.cube_phys = phys_world_add_tet_tri_softbody(s.world, (PHYS_TetTriSoftbody_Settings){
            .arena = cs->arena,
            .mass = 0.5f,
            .compliance = 0.01,
            .center = make_3f32(0,0,0),
            .vertices               = s.cube_vtk.points,
            .vertices_count         = s.cube_vtk.points_count,
            .tet_edge_indices       = s.cube_vtk.volume_edge_indices,
            .tet_edge_indices_count = s.cube_vtk.volume_edge_indices_count,
            .tet_indices            = s.cube_vtk.volume_indices,
            .tet_indices_count      = s.cube_vtk.volume_indices_count,
            .surface_indices        = s.cube_vtk.surface_indices,
            .surface_indices_count  = s.cube_vtk.surface_indices_count,
        });

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(4,4,4)
        });
    }

    s.cube_mesh.indices_count = s.cube_vtk.surface_indices_count;
    s.cube_mesh.vertices_count = s.cube_vtk.surface_indices_count;
    s.cube_mesh.indices = push_array(cs->arena, u32, s.cube_mesh.indices_count);
    for EachIndex(i, s.cube_mesh.indices_count) s.cube_mesh.indices[i] = i;
    s.cube_mesh.vertices = push_array(cs->arena, R_VertexLayout, s.cube_mesh.vertices_count);

    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cube_mesh.vertices_count*sizeof(*s.cube_mesh.vertices), s.cube_mesh.vertices);
    s.cube_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, s.cube_mesh.indices_count*sizeof(*s.cube_mesh.indices), s.cube_mesh.indices);

    s.time = os_now_seconds();
    return 0;
}

void d_sofbody(PHYS_World* world, MS_Mesh* mesh, PHYS_Softbody* softbody) {
    u32 vertex_offset = 0;
    for EachIndex(tri_i, softbody->triangle_colliders_count) {
        PHYS_Collider_Triangle* tri = &phys_world_resolve_collider(world, softbody->triangle_colliders[tri_i])->triangle;

        for EachElement(p_i, tri->p) {
            // CW -> CCW winding orders
            mesh->vertices[vertex_offset].position = phys_world_resolve_body(world, tri->p[p_i])->position;
            vertex_offset++;
        }
    }

    // recalculate normals with new positions
    ms_calculate_flat_normals(mesh, MS_Topology_Triangle);

    r_buffer_load(s.cube_vertices, 0, mesh->vertices_count*sizeof(*mesh->vertices), mesh->vertices);

    d_mesh(s.cube_vertices, s.cube_indices, make_diagonal_4x4f32(1.0f));
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    s.time = ntime;

    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.world, dt);
    
    d_begin_pipeline();
    d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        d_sofbody(s.world, &s.cube_mesh, &s.cube_phys);
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}