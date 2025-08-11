#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;

    R_Handle cloth_indices[2];
    R_Handle cloth_vertices[2];
    MS_Mesh cloth_mesh[2];

    VTK_Data cloth_vtk;

    DEMOS_Camera camera;

    Arena* state_arena;
    PHYS_World* world;
    PHYS_Cloth cloth_phys;
    PHYS_RigidBody ball_phys;
    
    f64 time;
};
static SoftbodyState s;

int demos_persistent_init_hook(DEMOS_CommonState* cs) {
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
    s.cloth_vtk = cloth.v;
    
    s.camera.eye    = (vec3_f32){.x = 0,.y =-1.3,.z = 1};
    s.camera.target = (vec3_f32){.x = 0,.y =-1.8,.z = 0};

    // setup visual mesh
    for EachElement(i, s.cloth_mesh) {
        s.cloth_mesh[i].flags = R_VertexFlag_PN;
        s.cloth_mesh[i].topology = R_VertexTopology_Triangles;
        geo_triangulate(
            cs->arena, GEO_Topology_Quad, /*cw*/ i,
            /*in*/ cloth.v.indices[VTK_CellType_Quad], cloth.v.indices_counts[VTK_CellType_Quad],
            /*out*/ &s.cloth_mesh[i].indices, &s.cloth_mesh[i].indices_count
        );
        s.cloth_mesh[i].vertices_count = cloth.v.points_count;
        s.cloth_mesh[i].vertices = arena_push(cs->arena, s.cloth_mesh[i].vertices_count*r_vertex_size(s.cloth_mesh[i].flags), r_vertex_align(s.cloth_mesh[i].flags));
    
        s.cloth_indices[i] = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Indices, s.cloth_mesh[i].indices_count*sizeof(*s.cloth_mesh[i].indices), s.cloth_mesh[i].indices);
        s.cloth_vertices[i] = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, s.cloth_mesh[i].vertices_count*r_vertex_size(s.cloth_mesh[i].flags), NULL);
    }

    // allocate state arena
    s.state_arena = arena_alloc();
    return 0;
}

void demos_state_init_hook() {
    {DeferResource(Temp scratch = scratch_begin_a(s.state_arena), scratch_end(scratch)) {
        u32* edge_indices;
        u32 edge_indices_count;
        geo_calculate_edges(scratch.arena,
            s.cloth_vtk.points_count*2, GEO_Topology_Quad, GEO_Connected_Ring,
            /*in*/ s.cloth_vtk.indices[VTK_CellType_Quad], s.cloth_vtk.indices_counts[VTK_CellType_Quad],
            /*out*/ &edge_indices, &edge_indices_count
        );
        
        f32 thickness = 0.01;
        s.world = phys_make_world((PHYS_WorldSettings){
            .substeps = 10,
            .linear_damping = 0.5,
            .min_collision_distance = thickness,
            .hashgrid_cell_size = 0.2,
            .hashgrid_object_size = thickness,
            .dynamic_friction_calculation = PHYS_CoefficientCalculation_Max,
        });

        PHYS_ClothFiber_Settings fibers_depth1[] = {
            {/*stretch-r*/ .compliance=0.f, .direction=make_3f32(1,0,0)},
            {/*stretch-u*/ .compliance=0.f, .direction=make_3f32(0,1,0)},
        };
        PHYS_ClothFiber_Settings fibers_depth2[] = {
            {/*shear-ur*/ .compliance=1.0f, .direction=normalize_3f32(make_3f32(1,1,0))},
            {/*shear-dr*/ .compliance=1.0f, .direction=normalize_3f32(make_3f32(1,-1,0))},
            {/*bend-rr*/ .compliance=3.f, .direction=make_3f32(1,0,0)},
            {/*bend-uu*/ .compliance=3.f, .direction=make_3f32(0,1,0)},
        };
        int fibers_counts[] = {ArrayLength(fibers_depth1), ArrayLength(fibers_depth2)};
        PHYS_ClothFiber_Settings* fibers[] = {fibers_depth1, fibers_depth2};
        u32 fiber_depth = ArrayLength(fibers);

        s.cloth_phys = phys_world_add_cloth(s.world, (PHYS_Cloth_Settings){
            .arena = s.state_arena,
            .thickness = thickness,
            .mass = 1.2f,
            .center = make_3f32(-0.2,-1,-0.2),
            .rotation = make_axis_angle_quat(PI/2.f, normalize_3f32(make_3f32(1,0,0))),
            .scale = mul_3f32(make_3f32(1.f,1.f,1.f), 1.f/2.4f),
            .vertices               = s.cloth_vtk.points,
            .vertices_count         = s.cloth_vtk.points_count,
            .edge_indices           = edge_indices,
            .edge_indices_count     = edge_indices_count,
            .fibers                 = fibers,
            .fibers_counts          = fibers_counts,
            .fiber_depth            = fiber_depth,
            .fiber_ratio_hint       = 6, // each body connects to 6 constraints
        });

        s.ball_phys = phys_world_add_ball(s.world, (PHYS_Ball_Settings){
            .center = make_3f32(0,-1.5,0),
            .mass = 10.f,
            .radius = 0.1f,
        });

        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(2,2,2),
        });
    }}

    s.time = os_now_seconds();
}

static void d_ball(PHYS_RigidBody* ball);
static void d_cloth(PHYS_Cloth* cloth);

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    f64 pdt = 1.f/60.f;
    s.time = ntime;

    input_update(&cs->events);
    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.world, pdt);

    r_window_begin_frame(cs->window, cs->rwindow);
    
    d_begin_pipeline();
    demos_d_begin_3d_pass_camera(cs->window, &s.camera);
    {
        d_ball(&s.ball_phys);
        d_cloth(&s.cloth_phys);
    }
    d_submit_pipeline(cs->window, cs->rwindow);

    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_state_cleanup_hook() {
    phys_world_cleanup(s.world);
    arena_clear(s.state_arena);
}

void demos_persistent_cleanup_hook(DEMOS_CommonState* cs) {
    return;
}

// helpers
static void d_ball(PHYS_RigidBody* ball) {
    PHYS_Body* center = phys_world_resolve_body(s.world, ball->body_id);
    PHYS_Collider* collider = phys_world_resolve_collider(s.world, ball->collider_id);
    f32 radius = collider->r;

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(radius, radius, radius))
    );
    d_mesh(s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology, R_Mesh3DMaterial_Lambertian, t, make_3f32(0.7,0.7,0.7));
}

static void d_cloth(PHYS_Cloth* cloth) {
    for EachElement(i, s.cloth_mesh) {
        MS_Mesh* m = &s.cloth_mesh[i];

        vec3_f32* p_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_P), vec3_f32);
        vec3_f32* n_start = OffsetPtr(m->vertices, r_vertex_offset(m->flags, R_VertexFlag_N), vec3_f32);
        u64 p_stride = r_vertex_stride(m->flags, R_VertexFlag_P);
        u64 n_stride = r_vertex_stride(m->flags, R_VertexFlag_N);
        
        for EachIndex(i, cloth->vertices_count) {
            vec3_f32 body_p = phys_world_resolve_body(s.world, cloth->vertices[i])->position;
            *OffsetPtr(p_start, i*p_stride, R_VertexType_P) = body_p;
            *OffsetPtr(n_start, i*n_stride, R_VertexType_N) = make_3f32(0,0,0);
        }
    
        geo_calculate_smooth_normals(
            GEO_Topology_Triangle,
            p_start, p_stride, m->vertices_count,
            m->indices, m->indices_count,
            n_start, n_stride
        );
    
        r_buffer_load(s.cloth_vertices[i], 0, m->vertices_count*r_vertex_size(m->flags), m->vertices);
    
        vec3_f32 color = mul_3f32(normalize_3f32(make_3f32(0.5+0.5*i,0.5*i,0)), 2.f);
        d_mesh(s.cloth_vertices[i], m->flags, s.cloth_indices[i], m->topology, R_Mesh3DMaterial_Lambertian, make_diagonal_4x4f32(1.f), color);
    }
}