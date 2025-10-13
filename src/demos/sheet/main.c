#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_SheetState DEMO_SheetState;
struct DEMO_SheetState {
    MS_Mesh cloth_mesh;
    R_Handle cloth_vertices;
    R_Handle cloth_indices;

    u32 x, y;
    PHYS_Cloth cloth_phys;
};

global DEMO_SheetState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, -0.5, 1), /*target*/ make_3f32(0, -1, 0));

    // setup visual mesh
    s.x = 30;
    s.y = 200;
    
    s.cloth_mesh.flags = R_VertexFlag_PN;
    s.cloth_mesh.topology = R_VertexTopology_Triangles;
    geo_triangulate_quad(
        cs->arena, /*cw*/ false, s.x, s.y,
        /*out*/ &s.cloth_mesh.indices, &s.cloth_mesh.indices_count
    );
    s.cloth_mesh.vertices_count = s.x*s.y;
    s.cloth_mesh.vertices = arena_push(cs->arena, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), r_vertex_align(s.cloth_mesh.flags));
    
    s.cloth_indices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, s.cloth_mesh.indices_count*sizeof(*s.cloth_mesh.indices), s.cloth_mesh.indices);
    s.cloth_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), NULL);

    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {
    f32 diameter = 0.01;
    f32 radius = diameter*0.5f;
    w->substeps = 8;
    w->min_r = radius;
    w->min_v_mult = 0.4f;
    w->hashgrid_cell_size = diameter;
    w->hashgrid_obj_size = diameter;
    w->enable_particle_ground_plane = true;
    w->particle_ground_plane_height = -1.f;

    s.cloth_phys = phys_world_add_sheet(w, (PHYS_Sheet_Settings){
        .mass = PHYS_UNIT_KG(1),

        .thickness = radius,
        .spacing = diameter,
        .x = s.x,
        .y = s.y,
        .stretch_compliance = 0.f,
        .shear_compliance = 0.0001f,
        .bend_compliance = 1.f,
    });
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_cloth(PHYS_World* w, PHYS_Cloth* cloth, b32 back_face);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    d_cloth(cs->w, &s.cloth_phys, false);

    demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ false, /*back_face*/ true);
    d_cloth(cs->w, &s.cloth_phys, true);
}

// helpers
internal void d_cloth(PHYS_World* w, PHYS_Cloth* cloth, b32 bf) {
    MS_Mesh* m = &s.cloth_mesh;

    vec3_f32* p_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_P), vec3_f32);
    vec3_f32* n_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_N), vec3_f32);
    u64 p_stride = r_vertex_stride(m->flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(m->flags, R_VertexFlag_N);
    
    for EachIndex(idx, cloth->vertices_count) {
        vec3_f32 body_p = phys_world_resolve_body_unchecked(w, cloth->vertices[idx])->position;
        *OffsetPtr(p_start, idx*p_stride, R_VertexType_P) = body_p;
        *OffsetPtr(n_start, idx*n_stride, R_VertexType_N) = make_3f32(0,0,0);
    }

    geo_calculate_smooth_normals(
        GEO_Topology_Triangle,
        p_start, p_stride, m->vertices_count,
        m->indices, m->indices_count,
        n_start, n_stride
    );

    r_buffer_load(&s.cloth_vertices, 0, m->vertices_count*r_vertex_size(m->flags), m->vertices);

    if (bf) {
        d_pbr_mesh_bf(
            s.cloth_vertices, m->flags, s.cloth_indices, m->topology,
            make_diagonal_4x4f32(1.f), make_3f32(0.7,0.7,0), 1.0, make_3f32(0,0,0)
        );
    } else {
        d_pbr_mesh(
            s.cloth_vertices, m->flags, s.cloth_indices, m->topology,
            make_diagonal_4x4f32(1.f), make_3f32(1,0,0), 1.0, make_3f32(0,0,0)
        );
    }
}
