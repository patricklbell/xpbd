#include "../demos_main.h"
#include "../demos_main.c"

typedef struct Ball Ball;
struct Ball {
    f32 radius;
    vec3_f32 color;
    PHYS_body_id center_id;
};

#define BALLS_COUNT 9

typedef struct BallsState BallsState;
struct BallsState {
    Arena* state_arena;
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;

    Ball balls[BALLS_COUNT];
};
static BallsState s;

// helpers
static void d_ball(PHYS_World* w, Ball* ball);

int demos_init_hook(DEMOS_CommonState* cs) {
    phys_dbg_d_ctx->do_contact_points = true;
    phys_dbg_d_ctx->do_collider_normals = true;

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

    cs->camera.eye    = (vec3_f32){.x = 0,.y =-2,.z = 10};
    cs->camera.target = (vec3_f32){.x = 0,.y =-3,.z = 0};

    return 0;
}
void demos_cleanup_hook(DEMOS_CommonState* cs) {}

void demos_world_start_hook(PHYS_World* w) {
    w->restitution_calculation = PHYS_CoefficientCalculation_Max,
    w->dynamic_friction_calculation = PHYS_CoefficientCalculation_Max,
    
    srand(31415);
    for EachElement(i, s.balls) {
        f32 radius = rand_f32()*0.4f + 0.1f;
        f32 density = 1.0f;
        f32 resitution = rand_f32();
        
        PHYS_body_id center_id;
        if (i % 2 == 0) {
            PHYS_Ball_Settings settings = {
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .resitution = resitution,
                .center=make_3f32((i - BALLS_COUNT/2)*1.f, 0, 0),
                .linear_velocity=make_3f32(0, rand_f32()*5, rand_f32()*5.0),
                .coefficient_of_dynamic_friction = 0.02,
            };
            center_id = phys_world_add_ball(w, settings).body_id;
        } else {
            PHYS_Box_Settings settings = {
                .arena=s.state_arena,
                .center=make_3f32((i - BALLS_COUNT/2)*1.f, 0, 0),
                .extents=make_3f32(radius,radius,radius),
                .mass=radius*radius*radius*density,
                .resitution=resitution,
                .linear_velocity=make_3f32(0, rand_f32()*5, rand_f32()*5.0),
            };
            center_id = phys_world_add_box(w, settings).body_id;
        }

        s.balls[i] = (Ball){
            .center_id = center_id,
            .color = hsl_to_rgb(make_3f32(resitution,1.0,1.0)),
            .radius = radius,
        };
    }

    phys_world_add_box_boundary(w, (PHYS_BoxBoundary_Settings){
        .arena = w->arena,
        .extents=make_3f32(BALLS_COUNT,4,4),
    });
}
void demos_world_end_hook(PHYS_World* w) {
    arena_clear(s.state_arena);
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    for EachElement(i, s.balls) {
        d_ball(cs->w, &s.balls[i]);
    }
}

// helpers
static void d_ball(PHYS_World* w, Ball* ball) {
    PHYS_Body* center = phys_world_resolve_body(w, ball->center_id);

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(ball->radius, ball->radius, ball->radius))
    );
    d_pbr_mesh(
        s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology,
        t, /*albedo*/ ball->color, /*roughness*/ 1.0, /*specular*/ make_3f32(0,0,0)
    );
}