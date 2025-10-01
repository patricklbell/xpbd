#pragma once

// camera
typedef struct DEMOS_Camera DEMOS_Camera;
struct DEMOS_Camera {
    f32 fov, aspect_ratio, near, far;

    vec3_f32 eye;
    vec3_f32 target;

    mat4x4_f32 projection, view;
    mat4x4_f32 inv_projection, inv_view;
};
internal DEMOS_Camera demos_make_camera(vec2_f32 window_size, vec3_f32 eye, vec3_f32 target);
internal void demos_camera_move(DEMOS_Camera* cam, vec3_f32 eye);
internal void demos_camera_pan(DEMOS_Camera* cam, vec3_f32 pan);
internal void demos_camera_resize_window(DEMOS_Camera* cam, vec2_f32 window_size);

// controls
#define DEMOS_CONTROLS_ORBIT_ZOOM_MULT 0.0001f
#define DEMOS_CONTROLS_ORBIT_ZOOM_RATE 0.06f
#define DEMOS_CONTROLS_ORBIT_ZOOM_MAX 100.f
internal void demos_controls_camera_orbit(OS_Handle window, f32 dt, DEMOS_Camera* camera);

typedef struct DEMOS_PhysDragState DEMOS_PhysDragState;
struct DEMOS_PhysDragState {
    Arena* arena;
    b32 drag_active;
    vec3_f32 drag_plane_normal;
    vec3_f32 drag_plane_origin;
    PHYS_constraint_id drag;
    PHYS_body_id pin;
};
#define DEMOS_CONTROLS_PHYS_DRAG_MULT 1.f
internal b32 demos_controls_phys_drag(PHYS_World* w, OS_Handle window, DEMOS_Camera* camera, f32 compliance);

// rendering
internal R_PassParams_3D* demos_d_begin_3d_pass_camera(OS_Handle window, DEMOS_Camera* camera, b32 debug);