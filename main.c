// headers
#include "common/common_inc.h"
#include "os/os_inc.h"
#include "physics/physics_inc.h"
#include "render/render_inc.h"
#include "draw/draw.h"
#include "mesh/mesh.h"

// implementations
#include "common/common_inc.c"
#include "os/os_inc.c"
#include "physics/physics_inc.c"
#include "render/render_inc.c"
#include "draw/draw.c"
#include "mesh/mesh.c"

int main() {
    ThreadCtx main_ctx;
    thread_equip(&main_ctx);

    Arena* asset_arena = arena_alloc();
    MS_MeshResult sphere = ms_load_obj(asset_arena, str_8("sphere.obj"));
    if (sphere.error.length != 0) {
        fprintf(stderr, "%s\n", sphere.error.data);
        return 1;
    }

    PhysicsState physics_state = {
        .ball = {
            .radius = 3.f,
            .position       = zero_struct,
            .prev_position  = zero_struct,
            .velocity       = zero_struct,
        },
        .little_g = -9.8
    };

    vec3_f32 eye    = (vec3_f32) { .x = 0.f, .y = 0.f, .z =-15.f };
    vec3_f32 target = (vec3_f32) { .x = 0.f, .y = -6.f, .z = 0.f };
    vec3_f32 up     = (vec3_f32) { .x = 0.f, .y = 1.f, .z = 0.f };

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

    f64 time = os_now_seconds();
    while (!os_window_has_close_event(window))
    {
        f64 ntime = os_now_seconds();
        f64 dt = ntime - time;
        time = ntime;

        phys_step(&physics_state, dt);

        vec3_f32* x = &physics_state.ball.position;
        printf("%.3f %.3f %.3f\n", x->x, x->y, x->z);

        d_begin_pipeline();

        {
            vec2_f32 window_size = os_window_size(window);
            rect_f32 viewport = make_rect_f32((vec2_f32){}, window_size);
            mat4x4_f32 view = make_look_at_4x4f32(eye, target, up);
            mat4x4_f32 projection = make_perspective_4x4f32(DegreesToRad(45), window_size.x / window_size.y, 0.1, 100.f);
            d_begin_3d_pass(viewport, view, projection);

            d_mesh(sphere_vertices, sphere_indices, make_translate_4x4f32(*x));
        }

        d_submit_pipeline(window, rwindow);
    }

    r_cleanup();
    os_close_window(window);
    os_gfx_cleanup();
}