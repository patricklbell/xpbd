#include "../demos_main.h"
#include "../demos_main.c"

#define BALLS_COUNT 10

typedef struct BallsState BallsState;
struct BallsState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;

    DEMOS_Camera camera;

    PHYS_World* world;
    PHYS_Ball_Settings ball_settings[BALLS_COUNT];
    PHYS_RigidBody balls[BALLS_COUNT];
    f64 time;
};
static BallsState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    MS_LoadResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"), (MS_LoadSettings){});
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }
    s.sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*sizeof(*sphere.v.vertices), sphere.v.vertices);
    s.sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);

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
            s.ball_settings[i] = (PHYS_Ball_Settings){
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .compliance = 0.0001f,
                .center=make_3f32(rand_f32()*6-3, 0, rand_f32()*6-3),
                .linear_velocity=make_3f32(rand_f32()*12, 0, rand_f32()*12),
            };
            s.balls[i] = phys_world_add_ball(s.world, s.ball_settings[i]);
        }
    }

    s.time = os_now_seconds();
    return 0;
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
        for EachElement(i, s.balls) {
            PHYS_Body* body = phys_world_resolve_body(s.world, s.balls[i].body_id);
            f32 radius = s.ball_settings[i].radius;

            mat4x4_f32 t = matmul_4x4f32(
                make_translate_4x4f32(body->position),
                make_scale_4x4f32(make_3f32(radius, radius, radius))
            );
            d_mesh(s.sphere_vertices, s.sphere_indices, t);
        }
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}