#include "../demos_main.h"
#include "../demos_main.c"

#define BALLS_COUNT 10

typedef struct Ball Ball;
struct Ball {
    f32 radius;
    vec3_f32 color;
    PHYS_body_id center_id;
};

typedef struct BallsState BallsState;
struct BallsState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;
    R_VertexFlag sphere_flags;
    R_VertexTopology sphere_topology;

    DEMOS_Camera camera;

    PHYS_World* world;
    Ball balls[BALLS_COUNT];
    f64 time;
};
static BallsState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    MS_LoadResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"), (MS_LoadSettings){});
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }
    s.sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*r_vertex_size(sphere.v.flags), sphere.v.vertices);
    s.sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);
    s.sphere_flags = sphere.v.flags;
    s.sphere_topology = sphere.v.topology;

    s.camera.eye    = (vec3_f32){.x = 0,.y = 0,.z =30};
    s.camera.target = (vec3_f32){.x = 0,.y = 0,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){});    
        
        phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(6,6,6)
        });
        for EachElement(i, s.balls) {
            f32 radius = rand_f32()*1.0f + 0.5f;
            f32 density = 1.0f;
            vec3_f32 linear_velocity = make_3f32(rand_f32()*10, rand_f32()*10, rand_f32()*10);
            PHYS_Ball_Settings settings = {
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .compliance = 0.0001f,
                .center=make_3f32(rand_f32()*6-3, 0, rand_f32()*6-3),
                .linear_velocity=linear_velocity,
            };
            PHYS_body_id center_id = phys_world_add_ball(s.world, settings).body_id;

            s.balls[i] = (Ball){
                .center_id = center_id,
                .color = normalize_3f32(linear_velocity),
                .radius = radius,
            };
        }
    }

    s.time = os_now_seconds();
    return 0;
}

static void d_ball(Ball* ball) {
    PHYS_Body* center = phys_world_resolve_body(s.world, ball->center_id);

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(ball->radius, ball->radius, ball->radius))
    );
    d_mesh(s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology, t, ball->color);
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
        for EachElement(i, s.balls) {
            d_ball(&s.balls[i]);
        }
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}