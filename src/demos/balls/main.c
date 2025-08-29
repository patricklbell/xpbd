#include "../demos_main.h"
#include "../demos_main.c"

#define BALLS_COUNT 9

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

static void on_slider_gravity(f32 value, void* data) {
    if (s.world != NULL) {
        s.world->little_g = value;
    }
}

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

    s.camera.eye    = (vec3_f32){.x = 0,.y =-2,.z = 10};
    s.camera.target = (vec3_f32){.x = 0,.y =-2,.z = 0};

    #if OS_WEB
        emcontrols_add((EMCONTROLS_Control){
            .type = EMCONTROLS_ControlType_Slider,
            .label = ntstr8_lit("gravity"),
            .on_slider = &on_slider_gravity,
            .slider_value = -10,
            .slider_min = -20,
            .slider_max = +20,
        });
    #endif

    return 0;
}

void demos_state_init_hook() {
    s.world = phys_make_world((PHYS_WorldSettings){
        .restitution_calculation = PHYS_CoefficientCalculation_Max,
        .dynamic_friction_calculation = PHYS_CoefficientCalculation_Max,
    });    
    
    srand(31415);
    for EachElement(i, s.balls) {
        f32 radius = rand_f32()*0.4f + 0.1f;
        f32 density = 1.0f;
        f32 resitution = rand_f32();
        PHYS_Ball_Settings settings = {
            .radius=radius,
            .mass=radius*radius*radius*(3.f/4.f)*PI*density,
            .resitution = resitution,
            .center=make_3f32((i - BALLS_COUNT/2)*1.f, 0, 0),
            .linear_velocity=make_3f32(0, rand_f32()*5, rand_f32()*5.0),
            .coefficient_of_dynamic_friction = 0.02,
        };
        PHYS_body_id center_id = phys_world_add_ball(s.world, settings).body_id;

        s.balls[i] = (Ball){
            .center_id = center_id,
            .color = hsl_to_rgb(make_3f32(resitution,1.0,1.0)),
            .radius = radius,
        };
    }

    phys_world_add_box_boundary(s.world, (PHYS_BoxBoundary_Settings){
        .extents=make_3f32(BALLS_COUNT,4,4),
    });

    s.time = os_now_seconds();
}

static void d_ball(Ball* ball);

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
        for EachElement(i, s.balls) {
            d_ball(&s.balls[i]);
        }
    }
    d_submit_pipeline(cs->window, cs->rwindow);
    r_window_end_frame(cs->window, cs->rwindow);
}

void demos_state_cleanup_hook() {
    phys_world_cleanup(s.world);
}

void demos_persistent_cleanup_hook(DEMOS_CommonState* cs) {
    return;
}

// helpers
static void d_ball(Ball* ball) {
    PHYS_Body* center = phys_world_resolve_body(s.world, ball->center_id);

    mat4x4_f32 t = matmul_4x4f32(
        make_translate_4x4f32(center->position),
        make_scale_4x4f32(make_3f32(ball->radius, ball->radius, ball->radius))
    );
    d_pbr_mesh(
        s.sphere_vertices, s.sphere_flags, s.sphere_indices, s.sphere_topology,
        t, /*albedo*/ ball->color, /*roughness*/ 1.0, /*specular*/ make_3f32(0,0,0)
    );
}