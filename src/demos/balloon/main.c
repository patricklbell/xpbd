#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_BalloonState DEMO_BalloonState;
struct DEMO_BalloonState {
    R_Handle cloth_indices;
    R_Handle cloth_vertices;
    MS_Mesh cloth_mesh;

    VTK_Data cloth_vtk;

    PHYS_Cloth cloth_phys;
};

global DEMO_BalloonState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    VTK_LoadResult cloth = vtk_load(cs->arena, ntstr8_lit("./data/sphere.vtk"), (VTK_LoadSettings){});
    if (cloth.error.length != 0) {
        fprintf(stderr, "%s\n", cloth.error.data);
        return 1;
    }
    s.cloth_vtk = cloth.v;
    
    // setup visual mesh
    s.cloth_mesh.flags = R_VertexFlag_PN;
    s.cloth_mesh.topology = R_VertexTopology_Triangles;
    s.cloth_mesh.indices = cloth.v.indices[VTK_CellType_Triangle];
    s.cloth_mesh.indices_count = cloth.v.indices_counts[VTK_CellType_Triangle];
    s.cloth_mesh.vertices_count = cloth.v.points_count;
    s.cloth_mesh.vertices = arena_push(cs->arena, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), r_vertex_align(s.cloth_mesh.flags));

    s.cloth_indices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Indices, s.cloth_mesh.indices_count*sizeof(*s.cloth_mesh.indices), s.cloth_mesh.indices);
    s.cloth_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), NULL);

    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, 0, 4), /*target*/ make_3f32(0, 0, 0));

    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {
    f32 thickness = 1.f/20.f;

    w->substeps = 8;
    w->little_g = 0.f;
    w->min_r = thickness;
    w->hashgrid_cell_size = 10.f*thickness;
    w->hashgrid_obj_size = thickness;
    w->dynamic_friction_calculation = PHYS_CoefficientCalculation_Max;
    phys_dbg_d_ctx->body_radius = thickness;
    phys_dbg_d_ctx->do_constraints = true;
    phys_dbg_d_ctx->do_bodies = true;

    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        u32* edge_indices;
        u32 edge_indices_count;
        geo_calculate_edges(scratch.arena,
            s.cloth_vtk.points_count*2, GEO_Topology_Triangle, GEO_Connected_Ring,
            /*in*/ s.cloth_vtk.indices[VTK_CellType_Triangle], s.cloth_vtk.indices_counts[VTK_CellType_Triangle],
            /*out*/ &edge_indices, &edge_indices_count
        );

        PHYS_ClothFiber_Settings fibers_depth1[] = {
            {/*stretch*/ .compliance=0.f, .ignore_direction=true},
        };
        PHYS_ClothFiber_Settings fibers_depth2[] = {
            {/*shear & bend*/ .compliance=0.01f, .ignore_direction=true},
        };
        int fibers_counts[] = {ArrayLength(fibers_depth1), ArrayLength(fibers_depth2)};
        PHYS_ClothFiber_Settings* fibers[] = {fibers_depth1, fibers_depth2};
        int fiber_depth = ArrayLength(fibers);

        s.cloth_phys = phys_world_add_cloth(w, (PHYS_Cloth_Settings){
            .mass = PHYS_UNIT_G(500),

            .thickness = thickness,

            .vertices               = s.cloth_vtk.points,
            .vertices_count         = s.cloth_vtk.points_count,

            .edge_indices           = edge_indices,
            .edge_indices_count     = edge_indices_count,

            .fibers                 = fibers,
            .fibers_counts          = fibers_counts,
            .fiber_depth            = fiber_depth,
            .fiber_ratio_hint       = 6, // each body connects to 6 constraints
        });
    }}

    phys_world_add_constraint(w, (PHYS_Constraint){
        .type = PHYS_ConstraintType_GlobalVolume,
        .compliance = 0.1f,
        .global_volume = {
            .surface_bodies = s.cloth_phys.vertices,
            .surface_bodies_count = s.cloth_phys.vertices_count,
            .surface_indices = s.cloth_vtk.indices[VTK_CellType_Triangle],
            .surface_indices_count = s.cloth_vtk.indices_counts[VTK_CellType_Triangle],
            .v_rest = 0.6*(4.f/3.f)*PI, // r = 1
        }
    });
    phys_world_add_box_boundary(w, (PHYS_BoxBoundary_Settings){
        .extents=make_3f32(2,2,2),
        .layer=PHYS_ColliderLayer_1_No1, // no raycast and no self collision
    });
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_cloth(PHYS_World* w, PHYS_Cloth* cloth);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    d_cloth(cs->w, &s.cloth_phys);
}

// helpers
internal void d_cloth(PHYS_World* w, PHYS_Cloth* cloth) {
    MS_Mesh* m = &s.cloth_mesh;

    vec3_f32* p_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_P), vec3_f32);
    vec3_f32* n_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_N), vec3_f32);
    u64 p_stride = r_vertex_stride(m->flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(m->flags, R_VertexFlag_N);
    
    for EachIndex(i, cloth->vertices_count) {
        vec3_f32 body_p = phys_world_resolve_body_unchecked(w, cloth->vertices[i])->position;
        *OffsetPtr(p_start, i*p_stride, R_VertexType_P) = body_p;
        *OffsetPtr(n_start, i*n_stride, R_VertexType_N) = make_3f32(0,0,0);
    }

    geo_calculate_smooth_normals(
        GEO_Topology_Triangle,
        p_start, p_stride, m->vertices_count,
        m->indices, m->indices_count,
        n_start, n_stride
    );

    r_buffer_load(&s.cloth_vertices, 0, m->vertices_count*r_vertex_size(m->flags), m->vertices);

    d_pbr_mesh(
        s.cloth_vertices, m->flags, s.cloth_indices, m->topology,
        make_diagonal_4x4f32(1.f), make_3f32(0,1,0), 0.5, make_3f32(0,1,0)
    );
}