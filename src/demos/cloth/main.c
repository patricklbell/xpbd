#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_ClothState DEMO_ClothState;
struct DEMO_ClothState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;

    R_Handle cloth_indices;
    R_Handle cloth_vertices;
    MS_Mesh cloth_mesh;

    VTK_Data cloth_vtk;

    PHYS_Cloth cloth_phys;
    PHYS_RigidBody ball_phys;
};

global DEMO_ClothState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    MS_LoadResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"), (MS_LoadSettings){});
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.cstr);
        return 1;
    }
    s.sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*r_vertex_size(sphere.v.flags), sphere.v.vertices);
    s.sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);
    s.sphere_flags = sphere.v.flags;
    s.sphere_topology = sphere.v.topology;

    VTK_LoadResult cloth = vtk_load(cs->arena, (VTK_LoadSettings){.path=ntstr8_lit("./data/cloth.vtk")});
    if (cloth.error.length != 0) {
        fprintf(stderr, "%s\n", cloth.error.cstr);
        return 1;
    }
    s.cloth_vtk = cloth.v;
    
    // setup visual mesh
    s.cloth_mesh.flags = R_VertexFlag_PN;
    s.cloth_mesh.topology = R_VertexTopology_Triangles;
    geo_triangulate(
        cs->arena, GEO_Topology_Quad, /*cw*/ false,
        /*in*/ cloth.v.indices[VTK_CellType_Quad], cloth.v.indices_counts[VTK_CellType_Quad],
        /*out*/ &s.cloth_mesh.indices, &s.cloth_mesh.indices_count
    );
    s.cloth_mesh.vertices_count = cloth.v.points_count;
    s.cloth_mesh.vertices = arena_push(cs->arena, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), r_vertex_align(s.cloth_mesh.flags));

    s.cloth_indices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Indices, s.cloth_mesh.indices_count*sizeof(*s.cloth_mesh.indices), s.cloth_mesh.indices);
    s.cloth_vertices = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cloth_mesh.vertices_count*r_vertex_size(s.cloth_mesh.flags), NULL);

    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, -1.3, 1), /*target*/ make_3f32(0, -1.8, 0));

    // allocate state arena
    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {    
    f32 l = 0.5f;
    f32 subdivisions = 20.f; // @note just for calculating appropriate thickness
    f32 diameter = l/subdivisions;
    f32 radius = diameter*0.5f;
    w->substeps = 8;
    w->min_r = radius;
    w->min_v_mult = 0.4f;
    w->hashgrid_cell_size = diameter;
    w->hashgrid_obj_size = diameter;
    w->enable_particle_ground_plane = true;
    w->particle_ground_plane_height = -2.f;

    PHYS_ClothFiber_Settings fibers_depth1[] = {
        {/*stretch-r*/ .compliance=0.f, .direction=make_3f32(1,0,0)},
        {/*stretch-u*/ .compliance=0.f, .direction=make_3f32(0,1,0)},
    };
    PHYS_ClothFiber_Settings fibers_depth2[] = {
        {/*shear-ur*/ .compliance=0.0001f, .direction=normalize_3f32(make_3f32(1,1,0))},
        {/*shear-dr*/ .compliance=0.0001f, .direction=normalize_3f32(make_3f32(1,-1,0))},
        {/*bend-rr*/  .compliance=1.0f,    .direction=make_3f32(1,0,0)},
        {/*bend-uu*/  .compliance=1.0f,    .direction=make_3f32(0,1,0)},
    };
    int fibers_counts[] = {ArrayLength(fibers_depth1), ArrayLength(fibers_depth2)};
    PHYS_ClothFiber_Settings* fibers[] = {fibers_depth1, fibers_depth2};
    int fiber_depth = ArrayLength(fibers);

    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)) {
        u32* edge_indices;
        u32 edge_indices_count;
        geo_calculate_edges(scratch.arena,
            s.cloth_vtk.points_count*2, GEO_Topology_Quad, GEO_Connected_Ring,
            /*in*/ s.cloth_vtk.indices[VTK_CellType_Quad], s.cloth_vtk.indices_counts[VTK_CellType_Quad],
            /*out*/ &edge_indices, &edge_indices_count
        );
        s.cloth_phys = phys_world_add_cloth(w, (PHYS_Cloth_Settings){
            .mass = PHYS_UNIT_KG(1.2),
            .center = make_3f32(-l/2.f,-1,-l/2.f),
            .rotation = make_angle_axis_quat(PI_F32/2.f, normalize_3f32(make_3f32(1,0,0))),
            .scale = mul_3f32(make_3f32(1.f,1.f,1.f), l),

            .thickness = radius,

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

    s.ball_phys = phys_world_add_ball(w, (PHYS_Ball_Settings){
        .mass = PHYS_UNIT_KG(10),
        .center = make_3f32(0,-1.5,0),

        .radius = 0.1f,
        .is_particle = true, // @todo
    });
}
demos_hook void demos_world_end_hook(PHYS_World* w) {}

// 
// per-frame
// 
internal void d_ball(PHYS_World* w, PHYS_RigidBody* ball);
internal void d_cloth(PHYS_World* w, PHYS_Cloth* cloth, b32 bf);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    d_ball(cs->w, &s.ball_phys);
    d_cloth(cs->w, &s.cloth_phys, false);

    demos_d_begin_3d_pass_camera(cs->window, &cs->camera, /*debug*/ false, /*back_face*/ true);
    d_cloth(cs->w, &s.cloth_phys, true);
}

// helpers
internal void d_ball(PHYS_World* w, PHYS_RigidBody* ball) {
    PHYS_Body* center = phys_world_resolve_body_unchecked(w, ball->body_id);
    PHYS_Collider* collider = phys_world_resolve_collider_unchecked(w, ball->collider_id);
    f32 radius = collider->base.r;

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(radius, radius, radius))
    );
    d_pbr_mesh(
        s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology,
        t, make_3f32(0.7,0.7,0.7), 1.0, make_3f32(0,0,0)
    );
}

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