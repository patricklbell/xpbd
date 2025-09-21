#pragma once

// camera
typedef struct DEMOS_Camera DEMOS_Camera;
struct DEMOS_Camera {
    vec3_f32 eye;
    vec3_f32 target;
};

#define DEMOS_CONTOLS_ORBIT_ZOOM_MULT 0.0001f
#define DEMOS_CONTOLS_ORBIT_ZOOM_RATE 0.06f
#define DEMOS_CONTOLS_ORBIT_ZOOM_MAX 100.f
void demos_camera_controls_orbit(OS_Handle window, f32 dt, DEMOS_Camera* camera);

// rendering
R_PassParams_3D* demos_d_begin_3d_pass_camera(OS_Handle window, DEMOS_Camera* camera, b32 debug);