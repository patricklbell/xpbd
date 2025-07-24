#include "../demos_common.h"
#include "../demos_common.c"


typedef struct HangingBox HangingBox;
struct HangingBox {
    PHYS_RigidBody rigid_body;
    vec3_f32 extents;
};

typedef struct HangingBoxesState HangingBoxesState;
struct HangingBoxesState {
    R_Handle cube_vertices;
    R_Handle cube_indices;

    vec3_f32 eye;
    vec3_f32 target;

    PHYS_World* world;

    PHYS_body_id anchor_id;
    PHYS_constraint_id anchor_to_box1;
    HangingBox box1;
    PHYS_constraint_id box1_to_box2;
    HangingBox box2;
    
    f64 time;
};
static HangingBoxesState demos_hanging_boxes;

int demos_init_hook(DEMOS_CommonState* cs) {
    HangingBoxesState* s = &demos_hanging_boxes;

    MS_MeshResult cube = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"));
    if (cube.error.length != 0) {
        fprintf(stderr, "%s\n", cube.error.data);
        return 1;
    }
    s->cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, cube.v.vertices_count*sizeof(*cube.v.vertices), cube.v.vertices);
    s->cube_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, cube.v.indices_count*sizeof(*cube.v.indices), cube.v.indices);

    s->eye    = (vec3_f32){.x = 0,.y =-4,.z =15};
    s->target = (vec3_f32){.x = 0,.y =-4,.z = 0};

    s->world = phys_world_make();    
    {
        s->anchor_id = phys_world_add_body(s->world, (PHYS_Body){
            .position = make_3f32(0,0,0),
            .inv_mass = 0.f,
            .no_gravity = true,
        });

        {
            s->box1.extents = make_3f32(0.5,0.5,0.5);
            PHYS_Box_Settings box_settings = {
                .mass = 0.125,
                .center = make_3f32(0,-2,0),
                .linear_velocity = make_3f32(1,0,0),
                .extents = s->box1.extents,
            };
            s->box1.rigid_body = phys_world_add_box(s->world, box_settings);
        }

        s->anchor_to_box1 = phys_world_add_constraint(s->world, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = 0.00001f,
                .b1 = s->anchor_id,
                .b2 = s->box1.rigid_body.body_id,
                .d = 2.f,
            }
        });

        {
            s->box2.extents = make_3f32(0.5,0.5,0.5);
            PHYS_Box_Settings box_settings = {
                .mass = 0.125,
                .center = make_3f32(0,-7,0),
                .linear_velocity = make_3f32(3,0,0),
                .extents = s->box2.extents,
            };
            s->box2.rigid_body = phys_world_add_box(s->world, box_settings);
        }

        s->box1_to_box2 = phys_world_add_constraint(s->world, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = 0.000001f,
                .b1 = s->box1.rigid_body.body_id,
                .b2 = s->box2.rigid_body.body_id,
                .d = 3.f,
            }
        });
    }

    s->time = os_now_seconds();
    return 0;
}

void d_hanging_box(HangingBoxesState* s, HangingBox* hanging_box) {
    PHYS_Body* body = phys_world_resolve_body(s->world, hanging_box->rigid_body.body_id);

    mat4x4_f32 t = mul_4x4f32(
        make_translate_4x4f32(body->position),
        make_scale_4x4f32(hanging_box->extents)
    );
    d_mesh(s->cube_vertices, s->cube_indices, t);
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    HangingBoxesState* s = &demos_hanging_boxes;
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
        
        d_hanging_box(s, &s->box1);
        d_hanging_box(s, &s->box2);
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    HangingBoxesState* s = &demos_hanging_boxes;
    phys_world_cleanup(s->world);
}