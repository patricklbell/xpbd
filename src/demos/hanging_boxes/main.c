#include "../demos_main.h"
#include "../demos_main.c"

typedef struct HangingBox HangingBox;
struct HangingBox {
    PHYS_RigidBody rigid_body;
    vec3_f32 extents;
};

typedef struct HangingBoxesState HangingBoxesState;
struct HangingBoxesState {
    R_Handle cube_vertices;
    R_Handle cube_indices;

    DEMOS_Camera camera;

    PHYS_World* world;

    PHYS_body_id anchor_id;
    PHYS_constraint_id anchor_to_box1;
    HangingBox box1;
    PHYS_constraint_id box1_to_box2;
    HangingBox box2;
    
    f64 time;
};
static HangingBoxesState s;

int demos_init_hook(DEMOS_CommonState* cs) {
    MS_MeshResult cube = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"));
    if (cube.error.length != 0) {
        fprintf(stderr, "%s\n", cube.error.data);
        return 1;
    }
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, cube.v.vertices_count*sizeof(*cube.v.vertices), cube.v.vertices);
    s.cube_indices  = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, cube.v.indices_count*sizeof(*cube.v.indices), cube.v.indices);

    s.camera.eye    = (vec3_f32){.x = 0,.y =-10,.z =40};
    s.camera.target = (vec3_f32){.x = 0,.y =-10,.z = 0};

    {
        s.world = phys_world_make((PHYS_WorldSettings){
            .damping = 0.01,
        }); 

        s.anchor_id = phys_world_add_body(s.world, (PHYS_Body){
            .position = make_3f32(0,0,0),
            .inv_mass = 0.f,
            .no_gravity = 1,
        });

        {
            s.box1.extents = make_3f32(1,1,1);
            PHYS_Box_Settings box_settings = {
                .mass = 1,
                .center = make_3f32(0,-10,0),
                .extents = s.box1.extents,
            };
            s.box1.rigid_body = phys_world_add_box(s.world, box_settings);
        }

        s.anchor_to_box1 = phys_world_add_constraint(s.world, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = 0.0001f,
                .b1 = s.anchor_id,
                .b2 = s.box1.rigid_body.body_id,
                .d = 11.f,

                .is_offset = 1,
                .offset2 = make_3f32(0,1,0),
            }
        });

        {
            s.box2.extents = make_3f32(1,1,1);
            PHYS_Box_Settings box_settings = {
                .mass = 1,
                .center = make_3f32(0,-20,0),
                .linear_velocity = make_3f32(1,0,0),
                .extents = s.box2.extents,
            };
            s.box2.rigid_body = phys_world_add_box(s.world, box_settings);
        }

        s.box1_to_box2 = phys_world_add_constraint(s.world, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = 0.0001f,
                .b1 = s.box1.rigid_body.body_id,
                .b2 = s.box2.rigid_body.body_id,
                .d = 9.f,

                .is_offset = 1,
                .offset1 = make_3f32(0,-1,0),
                .offset2 = make_3f32(0,1,0),
            }
        });
    }

    s.time = os_now_seconds();
    return 0;
}

void d_hanging_box(HangingBox* hanging_box) {
    PHYS_Body* body = phys_world_resolve_body(s.world, hanging_box->rigid_body.body_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(body->position),
        make_rotate_4x4f32(normalize_4f32(body->rotation))),
        make_scale_4x4f32(hanging_box->extents)
    );
    d_mesh(s.cube_vertices, s.cube_indices, t);
}

void demos_frame_hook(DEMOS_CommonState* cs) {
    f64 ntime = os_now_seconds();
    f64 dt = ntime - s.time;
    s.time = ntime;

    demos_camera_controls_orbit(cs->window, dt, &s.camera);

    phys_world_step(s.world, dt);
    
    d_begin_pipeline();
    {
        d_begin_3d_pass_camera(cs->window, &s.camera);
        
        d_hanging_box(&s.box1);
        d_hanging_box(&s.box2);
    }
    d_submit_pipeline(cs->window, cs->rwindow);
}

void demos_shutdown_hook(DEMOS_CommonState* cs) {
    phys_world_cleanup(s.world);
}