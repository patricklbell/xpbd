#include "../demos_main.h"
#include "../demos_main.c"

typedef struct DEMOS_Ball DEMOS_Ball;
struct DEMOS_Ball {
    b32 is_cube;
    f32 radius;
    vec3_f32 color;
    PHYS_body_id center_id;
};

typedef struct DEMOS_BallsState DEMOS_BallsState;
struct DEMOS_BallsState {
    Arena* state_arena;
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;
    R_Handle cube_vertices;
    R_Handle cube_indices;
    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;

    DEMOS_Ball balls[10];
};
static DEMOS_BallsState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    phys_dbg_d_ctx->do_contact_points = true;
    phys_dbg_d_ctx->do_contact_manifold = true;
    phys_dbg_d_ctx->do_collider_normals = false;
    phys_dbg_d_ctx->do_colliders = true;
    phys_dbg_d_ctx->default_point_radius = 0.01;

    s.state_arena = arena_alloc();

    MS_LoadResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"), (MS_LoadSettings){});
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }
    s.sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*r_vertex_size(sphere.v.flags), sphere.v.vertices);
    s.sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);
    s.sphere_flags = sphere.v.flags;
    s.sphere_topology = sphere.v.topology;
    MS_LoadResult cube = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"), (MS_LoadSettings){});
    if (cube.error.length != 0) {
        fprintf(stderr, "%s\n", cube.error.data);
        return 1;
    }
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, cube.v.vertices_count*r_vertex_size(cube.v.flags), cube.v.vertices);
    s.cube_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, cube.v.indices_count*sizeof(*cube.v.indices), cube.v.indices);
    s.cube_flags = cube.v.flags;
    s.cube_topology = cube.v.topology;

    cs->camera.eye    = (vec3_f32){.x = 0,.y = 2,.z = 10};
    cs->camera.target = (vec3_f32){.x = 0,.y = 0,.z = 0};

    return 0;
}
void demos_cleanup_hook(DEMOS_CommonState* cs) {}

void demos_world_start_hook(PHYS_World* w) {
    w->restitution_calculation = PHYS_CoefficientCalculation_Max;
    w->dynamic_friction_calculation = PHYS_CoefficientCalculation_Max;
    
    srand(31415);
    for EachElement(i, s.balls) {
        f32 radius = rand_f32()*0.3f + 0.1f;
        f32 density = 1.0f;
        f32 resitution = rand_f32();
        
        PHYS_body_id center_id;
        if (i % 2 == 0) {
            PHYS_Ball_Settings settings = {
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .resitution = resitution,
                .center=make_3f32((i - ArrayLength(s.balls)/2.f)*1.f, 0, 0),
                .linear_velocity=make_3f32(0, rand_f32()*5, rand_f32()*5.0),
                .coefficient_of_dynamic_friction=0.2f,
                .can_rotate=true,
            };
            center_id = phys_world_add_ball(w, settings).body_id;
        } else {
            PHYS_Box_Settings settings = {
                .arena=s.state_arena,
                .center=make_3f32((i - ArrayLength(s.balls)/2.f)*1.f, 0, 0),
                .rotation=normalize_4f32(make_axis_quat(make_3f32(1.f-2.f*rand_f32(), 1.f-2.f*rand_f32(), 1.f-2.f*rand_f32()))),
                .extents=make_3f32(radius,radius,radius),
                .mass=radius*radius*radius*density,
                .resitution=resitution,
                .linear_velocity=make_3f32(0, rand_f32()*5, rand_f32()*5.0),
                .coefficient_of_dynamic_friction=0.2f,
            };
            center_id = phys_world_add_box(w, settings).body_id;
        }

        s.balls[i] = (DEMOS_Ball){
            .is_cube = i % 2 != 0,
            .center_id = center_id,
            .color = hsl_to_rgb(make_3f32(resitution,1.0,1.0)),
            .radius = radius,
        };
    }

    // s.balls[0] = (DEMOS_Ball){
    //     .is_cube = true,
    //     .center_id = phys_world_add_box(w, (PHYS_Box_Settings) {
    //         .arena=s.state_arena,
    //         .center=make_3f32(-2,0,0),
    //         // .rotation=make_angle_axis_quat(PI/4.f, make_3f32(sqrt(2)/2.f,0,sqrt(2)/2.f)),
    //         .extents=make_3f32(1,1,1),
    //         .mass=1,
    //         .resitution=1.f,
    //         .linear_velocity=make_3f32(5,0,0),
    //     }).body_id,
    //     .color = make_3f32(1,0,0),
    //     .radius = 1,
    // };
    // s.balls[1] = (DEMOS_Ball){
    //     .is_cube = true,
    //     .center_id = phys_world_add_box(w, (PHYS_Box_Settings) {
    //         .arena=s.state_arena,
    //         .center=make_3f32(2,0,0),
    //         .rotation=make_angle_axis_quat(PI/4.f, make_3f32(0,1,0)),
    //         .extents=make_3f32(1,1,1),
    //         .mass=1,
    //         .resitution=1.f,
    //         .linear_velocity=make_3f32(-5,0,0),
    //     }).body_id,
    //     .color = make_3f32(0,0,1),
    //     .radius = 1,
    // };

    phys_world_add_box_boundary(w, (PHYS_BoxBoundary_Settings){
        .arena = w->arena,
        .extents=make_3f32(ArrayLength(s.balls),3.5,3.5),
        // .extents=make_3f32(4,4,4),
    });
}
void demos_world_end_hook(PHYS_World* w) {
    arena_clear(s.state_arena);
}

// helpers
static void d_ball(PHYS_World* w, DEMOS_Ball* ball);

void demos_frame_hook(DEMOS_CommonState* cs) {
    for EachElement(i, s.balls) {
        d_ball(cs->w, &s.balls[i]);
    }
}

// helpers
static void d_ball(PHYS_World* w, DEMOS_Ball* ball) {
    PHYS_Body* center = phys_world_resolve_body(w, ball->center_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_rotate_4x4f32(center->rotation)),
        make_scale_4x4f32(make_3f32(ball->radius, ball->radius, ball->radius))
    );

    if (ball->is_cube) 
        d_pbr_mesh(
            s.cube_vertices, s.cube_flags, s.cube_indices, s.cube_topology,
            t, /*albedo*/ ball->color, /*roughness*/ 1.0, /*specular*/ make_3f32(0,0,0)
        );
    else
        d_pbr_mesh(
            s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology,
            t, /*albedo*/ ball->color, /*roughness*/ 1.0, /*specular*/ make_3f32(0,0,0)
        );
}