#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_SoftbodyBunny DEMO_SoftbodyBunny;
struct DEMO_SoftbodyBunny {
    PHYS_Softbody sb;
    R_Handle vertices;
    vec3_f32 color;
};

typedef struct DEMO_SoftbodyState DEMO_SoftbodyState;
struct DEMO_SoftbodyState {
    MS_Mesh bunny_mesh;
    VTK_Data bunny_vtk;

    DEMO_SoftbodyBunny bunnies[1];
};

global DEMO_SoftbodyState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult bunny = vtk_load(cs->arena, ntstr8_lit("./data/bunny.vtk"), (VTK_LoadSettings){});
    if (bunny.error.length != 0) {
        fprintf(stderr, "%s\n", bunny.error.data);
        return 1;
    }
    s.bunny_vtk = bunny.v;
    
    // setup visual meshes
    s.bunny_mesh.flags = R_VertexFlag_PN;
    s.bunny_mesh.topology = R_VertexTopology_Triangles;
    s.bunny_mesh.vertices_count = s.bunny_vtk.indices_counts[VTK_CellType_Triangle];
    s.bunny_mesh.vertices = arena_push(cs->arena, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), r_vertex_align(s.bunny_mesh.flags));
    for EachElement(i, s.bunnies) {
        s.bunnies[i].vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.bunny_mesh.vertices_count*r_vertex_size(s.bunny_mesh.flags), NULL);
    }

    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, -2, 15), /*target*/ make_3f32(0, -2, 0));

    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {
    w->substeps = 8;
    w->min_r = 0.05f;
    w->hashgrid_cell_size = 0.15f; // @todo
    w->hashgrid_obj_size = 0.15f;
    w->enable_particle_ground_plane = true;
    w->particle_ground_plane_height = -4.f;
    
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        // @note allocates to cs arena for use later
        u32* surface_body_indices;
        u32 surface_body_indices_count;
        geo_calculate_points(scratch.arena,
            /*in*/ s.bunny_vtk.points_count, s.bunny_vtk.indices[VTK_CellType_Triangle], s.bunny_vtk.indices_counts[VTK_CellType_Triangle],
            /*out*/ &surface_body_indices, &surface_body_indices_count
        );

        u32 volume_edge_indices_count;
        u32* volume_edge_indices;
        geo_calculate_edges(scratch.arena,
            s.bunny_vtk.points_count/2, GEO_Topology_Tetrahedron, GEO_Connected_Strongly,
            /*in*/ s.bunny_vtk.indices[VTK_CellType_Tetrahedron], s.bunny_vtk.indices_counts[VTK_CellType_Tetrahedron],
            /*out*/ &volume_edge_indices, &volume_edge_indices_count
        );
        
        f32 compliances[ArrayLength(s.bunnies)] = { 0.3f }; // {0.1f,0.3f,0.7f,1.f};
        for EachElement(i, s.bunnies) {
            s.bunnies[i].sb = phys_world_add_softbody(w, (PHYS_TetTriSoftbody_Settings){
                .mass = 0.5f,
                .center = make_3f32((i-ArrayLength(s.bunnies)/2.f+0.5)*4.5f,0,0),

                .edge_compliance = compliances[i],
                .volume_compliance = 0.001f,

                .vertices                       = s.bunny_vtk.points,
                .vertices_count                 = s.bunny_vtk.points_count,

                .tetrahedron_edge_indices       = volume_edge_indices,
                .tetrahedron_edge_indices_count = volume_edge_indices_count,
                .tetrahedron_indices            = s.bunny_vtk.indices[VTK_CellType_Tetrahedron],
                .tetrahedron_indices_count      = s.bunny_vtk.indices_counts[VTK_CellType_Tetrahedron],

                .surface_point_indices          = surface_body_indices,
                .surface_point_indices_count    = surface_body_indices_count,
            });
            s.bunnies[i].color = hsl_to_rgb(make_3f32(i/(2.f*ArrayLength(s.bunnies)),1,1));
        }
    }}
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_bunny(PHYS_World* w, DEMO_SoftbodyBunny* bunny);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    for EachElement(i, s.bunnies) {
        d_bunny(cs->w, &s.bunnies[i]);
    }
}

// helpers
internal void d_bunny(PHYS_World* w, DEMO_SoftbodyBunny* bunny) {
    MS_Mesh* m = &s.bunny_mesh;

    vec3_f32* p_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_P), vec3_f32);
    vec3_f32* n_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_N), vec3_f32);
    u64 p_stride = r_vertex_stride(m->flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(m->flags, R_VertexFlag_N);

    for EachIndex(idx, s.bunny_vtk.indices_counts[VTK_CellType_Triangle]) {
        u32 indice = s.bunny_vtk.indices[VTK_CellType_Triangle][idx];
        vec3_f32 body_p = phys_world_resolve_body_unchecked(w, bunny->sb.vertices[indice])->position;

        *OffsetPtr(p_start, idx*p_stride, R_VertexType_P) = body_p;
    }

    geo_calculate_flat_normals(p_start, p_stride, m->vertices_count, n_start, n_stride);

    r_buffer_load(&bunny->vertices, 0, m->vertices_count*r_vertex_size(m->flags), m->vertices);

    d_pbr_mesh(
        bunny->vertices, m->flags, r_zero_handle(), m->topology,
        make_diagonal_4x4f32(1.0f), bunny->color, 0.1, make_3f32(0.9,0.9,0.9)
    );
}