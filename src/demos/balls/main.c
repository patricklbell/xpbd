#include "../demos_common.h"
#include "../demos_common.c"

#define BALLS_COUNT 10

typedef struct BallsState BallsState;
struct BallsState {
    R_Handle sphere_vertices;
    R_Handle sphere_indices;

    vec3_f32 eye;
    vec3_f32 target;

    PHYS_World* world;
    PHYS_Ball_Settings ball_settings[BALLS_COUNT];
    PHYS_Ball balls[BALLS_COUNT];
    f64 time;
};
static BallsState demos_balls_state;

int demos_init_hook(DEMOS_CommonState* cs) {
    MS_MeshResult sphere = ms_load_obj(cs->arena, ntstr8_lit("./data/sphere.obj"));
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }

    BallsState* s = &demos_balls_state;
    s->sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, sphere.v.vertices_count*sizeof(*sphere.v.vertices), sphere.v.vertices);
    s->sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, sphere.v.indices_count*sizeof(*sphere.v.indices), sphere.v.indices);

    s->eye    = (vec3_f32){.x = 0,.y = 0,.z =30};
    s->target = (vec3_f32){.x = 0,.y = 0,.z = 0};

    s->world = phys_world_make();    
    {
        phys_world_add_box_boundary(s->world, (PHYS_BoxBoundary_Settings){
            .extents=make_3f32(6,6,6)
        });
        for EachElement(i, s->balls) {
            f32 radius = rand_f32()*1.0f + 0.5f;
            f32 density = 1.0f;
            s->ball_settings[i] = (PHYS_Ball_Settings){
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .center=make_3f32(rand_f32()*6-3, 0, rand_f32()*6-3),
                .linear_velocity=make_3f32(rand_f32()*12, 0, rand_f32()*12),
            };
            s->balls[i] = phys_world_add_ball(s->world, s->ball_settings[i]);
        }
    }

    s->time = os_now_seconds();
    return 0;
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    BallsState* s = &demos_balls_state;
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s->time;
    s->time = ntime;

    demos_controls_orbit_camera(cs->window, dt, &s->eye, &s->target);

    phys_world_step(s->world, dt);
    
    d_begin_pipeline();
    {
        vec2_f32 window_size = os_gfx_window_size(cs->window);
        rect_f32 viewport = make_rect_f32((vec2_f32){}, window_size);
        mat4x4_f32 projection = make_perspective_4x4f32(DegreesToRad(45), window_size.x / window_size.y, 0.1, 100.f);
        mat4x4_f32 view = make_look_at_4x4f32(s->eye, s->target, make_up_3f32());
        d_begin_3d_pass(viewport, view, projection);
        
        for EachElement(i, s->balls) {
            PHYS_Body* body = phys_world_resolve_body(s->world, s->balls[i].center);
            f32 radius = s->ball_settings[i].radius;

            mat4x4_f32 t = mul_4x4f32(
                make_translate_4x4f32(body->position),
                make_scale_4x4f32(make_3f32(radius, radius, radius))
            );
            d_mesh(s->sphere_vertices, s->sphere_indices, t);
        }
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    BallsState* s = &demos_balls_state;
    phys_world_cleanup(s->world);
}