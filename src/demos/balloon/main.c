#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_Inflatable DEMO_Inflatable;
struct DEMO_Inflatable {
    R_Handle indices;
    R_Handle vertices;
    MS_Mesh mesh;
    VTK_Data vtk;
    PHYS_Cloth phys;
};

typedef struct DEMO_BalloonState DEMO_BalloonState;
struct DEMO_BalloonState {
    DEMO_Inflatable inflatables[2];
};

global DEMO_BalloonState s;

// 
// initialization/cleanup
//
b8 demo_balloon_load_inflatable_vtk(Arena* arena, NTString8 path, DEMO_Inflatable* i) {
    VTK_LoadResult vtk_result = vtk_load(arena, path, (VTK_LoadSettings){});
    if (vtk_result.error.length != 0) {
        fprintf(stderr, "%s\n", vtk_result.error.data);
        return false;
    }
    i->vtk = vtk_result.v;
    
    // setup visual mesh
    i->mesh.flags = R_VertexFlag_PN;
    i->mesh.topology = R_VertexTopology_Triangles;
    i->mesh.indices = i->vtk.indices[VTK_CellType_Triangle];
    i->mesh.indices_count = i->vtk.indices_counts[VTK_CellType_Triangle];
    i->mesh.vertices_count = i->vtk.points_count;
    i->mesh.vertices = arena_push(arena, i->mesh.vertices_count*r_vertex_size(i->mesh.flags), r_vertex_align(i->mesh.flags));

    i->indices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Indices, i->mesh.indices_count*sizeof(*i->mesh.indices), i->mesh.indices);
    i->vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, i->mesh.vertices_count*r_vertex_size(i->mesh.flags), NULL);
    return true;
}
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    b8 res = true;
    res &= demo_balloon_load_inflatable_vtk(cs->arena, ntstr8_lit("./data/sphere.vtk"), &s.inflatables[0]);
    res &= demo_balloon_load_inflatable_vtk(cs->arena, ntstr8_lit("./data/cube.vtk"), &s.inflatables[1]);
    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, 0, 6), /*target*/ make_3f32(0, 0, 0));

    return !res;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

void demo_balloon_phys_world_add_inflatable(PHYS_World* w, DEMO_Inflatable* i, PHYS_ColliderLayer layer, vec3_f32 position, f32 thickness, f32 volume) {
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        u32* edge_indices;
        u32 edge_indices_count;
        geo_calculate_edges(scratch.arena,
            i->vtk.points_count*2, GEO_Topology_Triangle, GEO_Connected_Ring,
            /*in*/ i->vtk.indices[VTK_CellType_Triangle], i->vtk.indices_counts[VTK_CellType_Triangle],
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

        i->phys = phys_world_add_cloth(w, (PHYS_Cloth_Settings){
            .mass = PHYS_UNIT_G(500),
            .center = position,

            .thickness = thickness,
            .layer = layer,

            .vertices               = i->vtk.points,
            .vertices_count         = i->vtk.points_count,

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
            .v_rest = volume, // r = 1
            .k = 0.9,
            
            .surface_bodies = i->phys.vertices,
            .surface_bodies_count = i->phys.vertices_count,
            .surface_indices = i->vtk.indices[VTK_CellType_Triangle],
            .surface_indices_count = i->vtk.indices_counts[VTK_CellType_Triangle],
        }
    });
}
demos_hook void demos_world_start_hook(PHYS_World* w) {
    f32 thickness = 1.f/20.f;

    f32 diameter = 0.1;
    f32 radius = diameter*0.5f;
    w->substeps = 8;
    w->min_r = radius;
    w->hashgrid_cell_size = diameter;
    w->hashgrid_obj_size = diameter;
    w->enable_particle_ground_plane = true;
    w->particle_ground_plane_height = -2;

    demo_balloon_phys_world_add_inflatable(w, &s.inflatables[0], (PHYS_ColliderLayer){.mask=4u,.group=1u&2u}, make_3f32(-1,0,0), radius, (4.f/3.f)*PI);
    demo_balloon_phys_world_add_inflatable(w, &s.inflatables[1], (PHYS_ColliderLayer){.mask=2u,.group=1u&4u}, make_3f32(+1,0,0), radius, 1.f*1.f*1.f);
    // phys_world_add_box_boundary(w, (PHYS_BoxBoundary_Settings){
    //     .extents=make_3f32(2,2,2),
    //     .layer=PHYS_ColliderLayer_1_No1, // no raycast and no self collision
    // });
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_inflatable(PHYS_World* w, DEMO_Inflatable* i);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    d_inflatable(cs->w, &s.inflatables[0]);
    d_inflatable(cs->w, &s.inflatables[1]);
}

// helpers
internal void d_inflatable(PHYS_World* w, DEMO_Inflatable* i) {
    vec3_f32* p_start = OffsetPtr(i->mesh.vertices, r_vertex_offset(i->mesh.flags, R_VertexFlag_P), vec3_f32);
    vec3_f32* n_start = OffsetPtr(i->mesh.vertices, r_vertex_offset(i->mesh.flags, R_VertexFlag_N), vec3_f32);
    u64 p_stride = r_vertex_stride(i->mesh.flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(i->mesh.flags, R_VertexFlag_N);
    
    for EachIndex(idx, i->phys.vertices_count) {
        vec3_f32 body_p = phys_world_resolve_body_unchecked(w, i->phys.vertices[idx])->position;
        *OffsetPtr(p_start, idx*p_stride, R_VertexType_P) = body_p;
        *OffsetPtr(n_start, idx*n_stride, R_VertexType_N) = make_3f32(0,0,0);
    }

    geo_calculate_smooth_normals(
        GEO_Topology_Triangle,
        p_start, p_stride, i->mesh.vertices_count,
        i->mesh.indices, i->mesh.indices_count,
        n_start, n_stride
    );

    r_buffer_load(&i->vertices, 0, i->mesh.vertices_count*r_vertex_size(i->mesh.flags), i->mesh.vertices);

    srand(i->vertices.v32[0]);
    vec3_f32 color = hsl_to_rgb(make_3f32(rand_f32(),1.0,1.0));

    d_pbr_mesh(
        i->vertices, i->mesh.flags, i->indices, i->mesh.topology,
        make_diagonal_4x4f32(1.f), color, 0.2, make_3f32(0.5,0.5,0.5)
    );
}