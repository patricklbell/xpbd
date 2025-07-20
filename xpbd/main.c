// headers
#include "../common/common_inc.h"
#include "../os/os_inc.h"
#include "../physics/physics_inc.h"
#include "../render/render_inc.h"
#include "../draw/draw.h"
#include "../mesh/mesh.h"
#include "../input/input.h"
#include "xpbd_controls.h"

// implementations
#include "../common/common_inc.c"
#include "../os/os_inc.c"
#include "../physics/physics_inc.c"
#include "../render/render_inc.c"
#include "../draw/draw.c"
#include "../mesh/mesh.c"
#include "../input/input.c"
#include "xpbd_controls.c"

int main() {
    ThreadCtx main_ctx;
    thread_equip(&main_ctx);

    Arena* main_arena = arena_alloc();
    MS_MeshResult sphere = ms_load_obj(main_arena, str_8("sphere.obj"));
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }

    // initialise the windowing api
    os_gfx_init();

    // open a window
    OS_Handle window = os_open_window();
    if (os_is_handle_zero(window)) {
        os_gfx_cleanup();
        return 1;
    }
    
    // initialize rendering api
    r_init();

    // equip window for rendering
    R_Handle rwindow = r_os_equip_window(window);

    R_Handle sphere_vertices = r_buffer_alloc(R_ResourceKind_Static, sphere.v.num_vertices*sizeof(*sphere.v.vertices), sphere.v.vertices);
    R_Handle sphere_indices  = r_buffer_alloc(R_ResourceKind_Static, sphere.v.num_indices*sizeof(*sphere.v.indices), sphere.v.indices);

    vec3_f32 eye    = (vec3_f32){.x = 0,.y = 0,.z =30};
    vec3_f32 target = (vec3_f32){.x = 0,.y = 0,.z = 0};

    PHYS_World* world = phys_world_make();

    static const int NUM_BALLS = 10;
    PHYS_Ball_Settings ball_settings[NUM_BALLS];
    PHYS_Ball balls[NUM_BALLS];
    {
        phys_world_add_box_boundary(world, (PHYS_BoxBoundary_Settings){.extents=make_3f32(6,6,6)});
        for EachElement(i, balls) {
            f32 radius = rand_f32()*1.0f + 0.5f;
            f32 density = 1.0f;
            ball_settings[i] = (PHYS_Ball_Settings){
                .radius=radius,
                .mass=radius*radius*radius*(3.f/4.f)*PI*density,
                .center=make_3f32(rand_f32()*6-3, 0, rand_f32()*6-3),
                .velocity=make_3f32(rand_f32()*12, 0, rand_f32()*12),
            };
            balls[i] = phys_world_add_ball(world, ball_settings[i]);
        }
    }

    f64 time = os_now_seconds();
    input_init();
    while (!input_has_quit()) {
        input_update(window);
        f64 ntime = os_now_seconds();
        f64 dt = ntime - time;
        time = ntime;
        
        phys_world_step(world, dt);

        xpbd_controls_orbit_camera(window, &eye, &target);
        
        d_begin_pipeline();
        {
            vec2_f32 window_size = os_window_size(window);
            rect_f32 viewport = make_rect_f32((vec2_f32){}, window_size);
            mat4x4_f32 projection = make_perspective_4x4f32(DegreesToRad(45), window_size.x / window_size.y, 0.1, 100.f);
            mat4x4_f32 view = make_look_at_4x4f32(eye, target, make_up_3f32());
            d_begin_3d_pass(viewport, view, projection);
            
            for EachElement(i, balls) {
                PHYS_Body* body = phys_world_resolve_body(world, balls[i].center);
                f32 radius = ball_settings[i].radius;

                mat4x4_f32 t = mul_4x4f32(
                    make_translate_4x4f32(body->position),
                    make_scale_4x4f32(make_3f32(radius, radius, radius))
                );
                d_mesh(sphere_vertices, sphere_indices, t);
            }
        }
        d_submit_pipeline(window, rwindow);
    }
    phys_world_cleanup(world);

    r_cleanup();
    os_close_window(window);
    os_gfx_cleanup();
}