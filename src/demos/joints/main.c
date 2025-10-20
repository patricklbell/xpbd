#include "demos/demos_main.h"
#include "demos/demos_main.c"

typedef struct DEMO_Object DEMO_Object;
struct DEMO_Object {
    PHYS_body_id body_id;

    vec3_f32 scale;

    R_VertexFlag r_flags;
    R_VertexTopology r_topology;
    R_Handle r_vertices;
    R_Handle r_indices;
};

typedef struct DEMO_JointsState DEMO_JointsState;
struct DEMO_JointsState {
    Arena* state_arena;

    R_VertexFlag cube_flags;
    R_VertexTopology cube_topology;
    R_Handle cube_vertices;
    R_Handle cube_indices;
    
    DEMO_Object objects[7]; 
};

global DEMO_JointsState s;

// 
// initialization/cleanup
// 
demos_hook int demos_init_hook(DEMOS_CommonState* cs) {
    phys_dbg_d_ctx->color_mode = PHYS_DBG_DrawColorMode_Type;
    phys_dbg_d_ctx->do_colliders = true;
    phys_dbg_d_ctx->do_limit_angle = true;
    phys_dbg_d_ctx->do_bodies = true;
    phys_dbg_d_ctx->do_constraints = true;

    MS_LoadResult res = ms_load_obj(cs->arena, ntstr8_lit("./data/cube.obj"), (MS_LoadSettings){});
    if (res.error.length != 0) {
        fprintf(stderr, "%s\n", res.error.cstr);
        return 1;
    }

    s.cube_flags = res.v.flags;
    s.cube_topology = res.v.topology;
    s.cube_vertices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Array, res.v.vertices_count*r_vertex_size(s.cube_flags), res.v.vertices);
    s.cube_indices = r_buffer_alloc(R_ResourceKind_Static, R_ResourceHint_Indices, res.v.indices_count*sizeof(*res.v.indices), res.v.indices);

    cs->camera = demos_make_camera(os_gfx_window_size(cs->window), /*eye*/ make_3f32(0, 1, 12), /*target*/ make_3f32(0, 0, 0));

    s.state_arena = arena_alloc();
    return 0;
}
demos_hook void demos_cleanup_hook(DEMOS_CommonState* cs) {}

demos_hook void demos_world_start_hook(PHYS_World* w) {
    vec3_f32 offset = make_3f32(-5,3.5,0);
    PHYS_body_id ids[ArrayLength(s.objects)];
    vec3_f32 scales[ArrayLength(s.objects)];
    u32 obj_idx = 0;

    // 
    // Basic hinge
    // 
    {
        PHYS_body_id pin = phys_world_add_fixed_point(w, offset);
        vec3_f32 hinge_extents = make_3f32(1.0,0.4,0.1);
        PHYS_body_id hinge = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=add_3f32(offset, make_3f32(hinge_extents.x,0,0)),
            .extents=hinge_extents,
        }).body_id;

        // attach top of hinge to pin
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_AdvancedDistance,
            .advanced_distance={
                .body1=pin,
                .body2=hinge,
                .offset2=make_3f32(-hinge_extents.x,0,0),
                .d=0,
            }
        });
        // allow rotation only in z-axis
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Hinge,
            .hinge={
                .body1=pin,
                .body2=hinge,
                .a1=make_3f32(0,0,1),
                .a2=make_3f32(0,0,1),
            }
        });
        offset.x += 3.5f;

        ids[obj_idx] = hinge; scales[obj_idx++] = hinge_extents;
    }

    // 
    // Hinge limits and targets
    // 
    {
        PHYS_body_id pin = phys_world_add_fixed_point(w, offset);
        vec3_f32 hinge_extents = make_3f32(1.0,0.4,0.1);
        PHYS_body_id hinge = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=add_3f32(offset, make_3f32(hinge_extents.x,0,0)),
            .extents=hinge_extents,
        }).body_id;
    
        // attach top of hinge to pin
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_AdvancedDistance,
            .advanced_distance={
                .body1=pin,
                .body2=hinge,
                .offset2=make_3f32(-hinge_extents.x,0,0),
                .d=0,
            }
        });
        // allow rotation only in z axis and limit y axis to [-45,90] degrees
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Hinge,
            .hinge={
                .body1=pin,
                .body2=hinge,
                .a1=make_3f32(0,0,1), .b1=make_3f32(0,1,0),
                .a2=make_3f32(0,0,1), .b2=make_3f32(0,1,0),
                .limit_angle=phys_world_add_dependent_constraint(w, (PHYS_DependentConstraint){
                    .limits={.min=-PI_F32/4.f, .max=PI_F32/2.f}
                })
            }
        });
        offset.x += 3.5f;

        ids[obj_idx] = hinge; scales[obj_idx++] = hinge_extents;
    }

    //
    // Ball-in-socket
    // 
    {
        PHYS_body_id socket = phys_world_add_fixed_point(w, offset);
        vec3_f32 arm_extents = make_3f32(0.1,1.0,0.1);
        PHYS_body_id arm = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=add_3f32(offset, make_3f32(0,-arm_extents.y,0)),
            .extents=arm_extents,
        }).body_id;
    
        // attach top of arm to socket
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_AdvancedDistance,
            .advanced_distance={
                .body1=socket,
                .body2=arm,
                .offset2=make_3f32(0,arm_extents.y,0),
                .d=0,
            }
        });
        // restrict swing to 45 degrees from vertical
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Swing,
            .compliance=0.0001f,
            .swing={
                .body1=socket,
                .body2=arm,
                .a1=make_3f32(0,1,0),
                .a2=make_3f32(0,1,0),
                .limits={.min=-PI_F32/4.f, .max=+PI_F32/4.f}
            }
        });
        
        offset.x += 3.5f;
        ids[obj_idx] = arm; scales[obj_idx++] = arm_extents;
    }

    //
    // Ball-in-socket, Twist
    // 
    {
        PHYS_body_id socket = phys_world_add_fixed_point(w, offset);
        vec3_f32 arm_extents = make_3f32(0.1,1.0,0.1);
        PHYS_body_id arm = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=add_3f32(offset, make_3f32(0,-arm_extents.y,0)),
            .extents=arm_extents,
        }).body_id;
    
        // conenct top of arm to the socket
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_AdvancedDistance,
            .advanced_distance={
                .body1=socket,
                .body2=arm,
                .offset2=make_3f32(0,arm_extents.y,0),
                .d=0,
            }
        });
        // restrict swinging and twisting to certain limits
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Swing,
            .compliance=0.0001f,
            .swing={
                .body1=socket,
                .body2=arm,
                .a1=make_3f32(0,1,0),
                .a2=make_3f32(0,1,0),
                .limits={.min=-PI_F32/4.f, .max=+PI_F32/4.f}
            }
        });
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Twist,
            .compliance=0.001f,
            .twist={
                .body1=socket,
                .body2=arm,
                .a1=make_3f32(0,1,0), .b1=make_3f32(1,0,0),
                .a2=make_3f32(0,1,0), .b2=make_3f32(1,0,0),
                .limits={.min=-PI_F32/8.f, .max=+PI_F32/8.f}
            }
        });
        
        offset.x += 3.5f;
        ids[obj_idx] = arm; scales[obj_idx++] = arm_extents;
    }

    // second row
    offset.y -= 4;
    offset.x = -5;

    //
    // Revolute Joint
    // 
    {
        PHYS_body_id bearing = phys_world_add_fixed_point(w, offset);
        vec3_f32 pin_extents = make_3f32(0.1,0.7,0.1);
        PHYS_body_id pin = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=offset,
            .extents=pin_extents,
        }).body_id;
    
        // only allow movement in y-axis [-0.7,+0.7]
        phys_world_add_constraint(w, (PHYS_Constraint) {
            .type=PHYS_ConstraintType_LinearDOFs,
            .linear_dofs={
                .body1=bearing,
                .body2=pin,
                .axes   ={make_3f32(1,0,0),  make_3f32(0,1,0),         make_3f32(0,0,1) },
                .limits ={{.min=0,.max=0},   {.min=-0.7,.max=+0.7},    {.min=0,.max=0}  },
            }
        });
        // lock rotation to y-axis
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Hinge,
            .hinge={
                .body1=bearing,
                .body2=pin,
                .a1=make_3f32(0,1,0),
                .a2=make_3f32(0,1,0),
            }
        });
        
        offset.x += 3.5f;
        ids[obj_idx] = pin; scales[obj_idx++] = pin_extents;
    }

    //
    // Revolute Joint with Target
    // 
    {
        PHYS_body_id bearing = phys_world_add_fixed_point(w, offset);
        vec3_f32 pin_extents = make_3f32(0.1,0.7,0.1);
        PHYS_body_id pin = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=offset,
            .extents=pin_extents,
        }).body_id;
    
        // only allow movement in y-axis
        phys_world_add_constraint(w, (PHYS_Constraint) {
            .type=PHYS_ConstraintType_LinearDOFs,
            .linear_dofs={
                .body1=bearing,
                .body2=pin,
                // @note last axis doesn't have any effect since it is never parallel
                .axes   ={make_3f32(1,0,0),  make_3f32(0,0,1),  make_3f32(0,0,0) },
                .limits ={{.min=0,.max=0},   {.min=0,.max=0},   {.min=0,.max=0}  },
            }
        });
        // create a linear distance constraint only in y-axis
        phys_world_add_constraint(w, (PHYS_Constraint) {
            .type=PHYS_ConstraintType_AdvancedDistance,
            .compliance=0.1,
            .advanced_distance={
                .body1=bearing,
                .body2=pin,

                .is_projected=true,
                .axis=make_3f32(0,1,0),

                .d=0.f,
            }
        });
        // lock rotation to y-axis
        phys_world_add_constraint(w, (PHYS_Constraint){
            .type=PHYS_ConstraintType_Hinge,
            .hinge={
                .body1=bearing,
                .body2=pin,
                .a1=make_3f32(0,1,0),
                .a2=make_3f32(0,1,0),
            }
        });
        
        offset.x += 3.5f;
        ids[obj_idx] = pin; scales[obj_idx++] = pin_extents;
    }

    //
    // Prismatic Joint
    // 
    {
        PHYS_body_id collar = phys_world_add_fixed_point(w, offset);
        vec3_f32 arm_extents = make_3f32(0.1,0.7,0.1);
        PHYS_body_id arm = phys_world_add_box(w, (PHYS_Box_Settings){
            .mass=PHYS_UNIT_G(500),
            .center=offset,
            .extents=arm_extents,
        }).body_id;
    
        // lock orientations
        phys_world_add_constraint(w, (PHYS_Constraint) {
            .type=PHYS_ConstraintType_Orientation,
            .orientation={.body1=collar, .body2=arm}
        });
        // only allow movement in y-axis [-0.7,+0.7]
        phys_world_add_constraint(w, (PHYS_Constraint) {
            .type=PHYS_ConstraintType_LinearDOFs,
            .linear_dofs={
                .body1=collar,
                .body2=arm,
                .axes   ={make_3f32(1,0,0),  make_3f32(0,1,0),         make_3f32(0,0,1) },
                .limits ={{.min=0,.max=0},   {.min=-0.7,.max=+0.7},    {.min=0,.max=0}  },
            }
        });
        
        offset.x += 3.5f;
        ids[obj_idx] = arm; scales[obj_idx++] = arm_extents;
    }
    
    for EachElement(i, ids) {
        s.objects[i].body_id = ids[i];
        s.objects[i].scale = scales[i];
        s.objects[i].r_flags = s.cube_flags;
        s.objects[i].r_topology = s.cube_topology;
        s.objects[i].r_vertices = s.cube_vertices;
        s.objects[i].r_indices = s.cube_indices;
    }
}
demos_hook void demos_world_end_hook(PHYS_World* w) {
    arena_clear(s.state_arena);
}

// 
// per-frame
// 
internal void d_object(PHYS_World* w, DEMO_Object* obj, vec3_f32 color);

demos_hook void demos_frame_hook(DEMOS_CommonState* cs) {
    for EachElement(i, s.objects) {
        d_object(cs->w, &s.objects[i], make_3f32(1,0,0));
    }
}

// helpers
internal void d_object(PHYS_World* w, DEMO_Object* obj, vec3_f32 color) {
    PHYS_Body* body = phys_world_resolve_body_unchecked(w, obj->body_id);

    mat4x4_f32 t = matmul_4x4f32(matmul_4x4f32(
        make_translate_4x4f32(body->position),
        make_rotate_4x4f32(normalize_4f32(body->rotation))),
        make_scale_4x4f32(obj->scale)
    );
    d_pbr_mesh(
        obj->r_vertices, obj->r_flags, obj->r_indices, obj->r_topology,
        t, color, 1.0, make_3f32(0,0,0)
    );
}