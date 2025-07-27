#include "../demos_main.h"
#include "../demos_main.c"

typedef struct SoftbodyState SoftbodyState;
struct SoftbodyState {
    DEMOS_Camera camera;

    PHYS_World* world;
    
    f64 time;
};
static SoftbodyState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    s.camera.eye    = (vec3_f32){.x = 0,.y =-10,.z =40};
    s.camera.target = (vec3_f32){.x = 0,.y =-10,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){}); 
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
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}