// rigid bodies
void phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object) {
    phys_world_remove_collider(w, object->collider_id);
    phys_world_remove_body(w, object->body_id);
}

PHYS_RigidBody phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings){
    Assert(settings.mass > 0.f);
    PHYS_body_id center = phys_world_add_body(w, (PHYS_Body){
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .inv_mass = 1.f / settings.mass,
    });
    PHYS_collider_id sphere = phys_world_add_collider(w, (PHYS_Collider){
        .type = PHYS_ColliderType_Sphere,
        .sphere = {
            .compliance = settings.compliance,
            .c = center,
            .r = settings.radius,
        }
    });

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = sphere,
    };
}

PHYS_RigidBody phys_world_add_box(PHYS_World* w, PHYS_Box_Settings settings){
    Assert(settings.mass > 0.f);

    PHYS_body_id center = phys_world_add_body(w, (PHYS_Body){
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .angular_velocity = settings.angular_velocity,
        .inv_mass = 1.f / settings.mass,
        .inv_inertia = phys_inv_moment_rect_cuboid(mul_3f32(settings.extents, 2.0), settings.mass),
    });
    PHYS_collider_id rect_cuboid = phys_world_add_collider(w, (PHYS_Collider){
        .type = PHYS_ColliderType_RectCuboid,
        .rect_cuboid = {
            .compliance = settings.compliance,
            .c = center,
            .r = settings.extents,
        }
    });

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = rect_cuboid,
    };
}

// box boundary
PHYS_BoxBoundary phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings){
    PHYS_BoxBoundary result;

    int i = 0;
    for EachIndex(dim, 3) {
        for (int offset = -1; offset <= 1; offset += 2) {
            vec3_f32 normal = zero_struct;
            normal.v[dim] = offset;

            vec3_f32 position = zero_struct;
            position.v[dim] = -offset*settings.extents.v[dim];
            position = add_3f32(settings.center, position);

            result.positions[i] = phys_world_add_body(w, (PHYS_Body){
                .position = position,
                .no_gravity = 1,
                .inv_mass = 0.f,
            });
            result.areas[i] = phys_world_add_collider(w, (PHYS_Collider){
                .type = PHYS_ColliderType_Plane,
                .plane = {
                    .compliance = 0.f,
                    .p = result.positions[i],
                    .n = normal,
                }
            });
            i++;
        }
    }

    return result;
}

void phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary* object){
    for EachElement(i, object->areas) {
        phys_world_remove_collider(w, object->areas[i]);
        phys_world_remove_body(w, object->positions[i]);
    }
}

// softbody
PHYS_Softbody phys_world_add_tet_tri_softbody(PHYS_World* w, PHYS_TetTriSoftbody_Settings settings) {
    PHYS_Softbody result;

    // @todo angular velocity

    // vertices
    result.vertices_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = settings.vertices[vert_i],
            .linear_velocity = settings.linear_velocity,
            .inv_mass = 0.0f, // will be filled when constructing tets
        });
    }

    // surface collider
    const static int tri_size = 3;
    result.triangle_colliders_count = settings.surface_indices_count / tri_size;
    result.triangle_colliders = push_array(settings.arena, PHYS_collider_id, result.triangle_colliders_count);
    for (int tri_i = 0; tri_i < result.triangle_colliders_count; tri_i++) {
        u32 v1 = settings.surface_indices[tri_i*tri_size + 0];
        u32 v2 = settings.surface_indices[tri_i*tri_size + 1];
        u32 v3 = settings.surface_indices[tri_i*tri_size + 2];

        result.triangle_colliders[tri_i] = phys_world_add_collider(w, (PHYS_Collider){
            .type = PHYS_ColliderType_Triangle,
            .triangle = {
                .compliance = 0.f,
                .p = {
                    result.vertices[v1],
                    result.vertices[v2],
                    result.vertices[v3]
                }
            }
        });
    }

    // edge constraints
    const static int edge_size = 2;
    result.distance_constraints_count = settings.tet_edge_indices_count / edge_size;
    result.distance_constraints = push_array(settings.arena, PHYS_constraint_id, result.distance_constraints_count);
    for (int edge_i = 0; edge_i < result.distance_constraints_count; edge_i++) {
        u32 v1 = settings.tet_edge_indices[edge_i*edge_size + 0];
        u32 v2 = settings.tet_edge_indices[edge_i*edge_size + 1];

        result.distance_constraints[edge_i] = phys_world_add_constraint(w, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = settings.edge_compliance,
                .b1 = result.vertices[v1],
                .b2 = result.vertices[v2],
                .d = length_3f32(sub_3f32(settings.vertices[v1], settings.vertices[v2]))
            }
        });
    }

    f32 total_volume = 0.f;

    // volume constraints
    const static int tet_size = 4;
    result.volume_constraints_count = settings.tet_indices_count / tet_size;
    result.volume_constraints = push_array(settings.arena, PHYS_constraint_id, result.volume_constraints_count);
    for (int tet_i = 0; tet_i < result.volume_constraints_count; tet_i++) {
        u32 v1 = settings.tet_indices[tet_i*tet_size + 0];
        u32 v2 = settings.tet_indices[tet_i*tet_size + 1];
        u32 v3 = settings.tet_indices[tet_i*tet_size + 2];
        u32 v4 = settings.tet_indices[tet_i*tet_size + 3];

        f32 v_rest = phys_tet_volume(
            settings.vertices[v1],
            settings.vertices[v2],
            settings.vertices[v3],
            settings.vertices[v4]
        );

        // distribute mass among vertices (normalize and inverse after)
        total_volume+=v_rest;
        for (int offset = 0; offset < tet_size; offset++) {
            u32 v = settings.tet_indices[tet_i*tet_size + offset];
            PHYS_Body* b = phys_world_resolve_body(w, result.vertices[v]);
            b->inv_mass += v_rest / (f32)tet_size;
        }

        result.volume_constraints[tet_i] = phys_world_add_constraint(w, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Volume,
            .volume = {
                .compliance = settings.volume_compliance,
                .p = {
                    result.vertices[v1],
                    result.vertices[v2],
                    result.vertices[v3],
                    result.vertices[v4]
                },
                .v_rest = v_rest,
            }
        });
    }

    // normalise and invert vertex masses
    for EachIndex(vert_i, result.vertices_count) {
        PHYS_Body* b = phys_world_resolve_body(w, result.vertices[vert_i]);
        b->inv_mass = 1.f / (settings.mass*b->inv_mass/total_volume);
    }

    return result;
}

void phys_world_remove_softbody(PHYS_World* w, PHYS_Softbody object) {
    for EachIndex(i, object.volume_constraints_count) {
        phys_world_remove_constraint(w, object.volume_constraints[i]);
    }
    for EachIndex(i, object.distance_constraints_count) {
        phys_world_remove_constraint(w, object.distance_constraints[i]);
    }
    for EachIndex(i, object.triangle_colliders_count) {
        phys_world_remove_collider(w, object.triangle_colliders[i]);
    }
    for EachIndex(i, object.vertices_count) {
        phys_world_remove_body(w, object.vertices[i]);
    }
}

// cloth
PHYS_Cloth phys_world_add_cloth(PHYS_World* w, PHYS_Cloth_Settings settings) {
    PHYS_Cloth result;

    // vertices
    result.vertices_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = settings.vertices[vert_i],
            .linear_velocity = settings.linear_velocity,
            .inv_mass = 1.f/(settings.mass / (f32)settings.vertices_count),
        });
    }

    // surface collider
    const static int tri_size = 3;
    result.triangle_colliders_count = settings.surface_indices_count / tri_size;
    result.triangle_colliders = push_array(settings.arena, PHYS_collider_id, result.triangle_colliders_count);
    for (int tri_i = 0; tri_i < result.triangle_colliders_count; tri_i++) {
        u32 v1 = settings.surface_indices[tri_i*tri_size + 0];
        u32 v2 = settings.surface_indices[tri_i*tri_size + 1];
        u32 v3 = settings.surface_indices[tri_i*tri_size + 2];

        result.triangle_colliders[tri_i] = phys_world_add_collider(w, (PHYS_Collider){
            .type = PHYS_ColliderType_Triangle,
            .triangle = {
                .compliance = 0.f,
                .p = {
                    result.vertices[v1],
                    result.vertices[v2],
                    result.vertices[v3]
                },
                .two_sided = 1,
            }
        });
    }

    // edge constraints
    const static int edge_size = 2;
    result.distance_constraints_count = settings.edge_indices_count / edge_size;
    result.distance_constraints = push_array(settings.arena, PHYS_constraint_id, result.distance_constraints_count);
    for (int edge_i = 0; edge_i < result.distance_constraints_count; edge_i++) {
        u32 v1 = settings.edge_indices[edge_i*edge_size + 0];
        u32 v2 = settings.edge_indices[edge_i*edge_size + 1];

        result.distance_constraints[edge_i] = phys_world_add_constraint(w, (PHYS_Constraint){
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .compliance = 0.f,
                .b1 = result.vertices[v1],
                .b2 = result.vertices[v2],
                .d = length_3f32(sub_3f32(settings.vertices[v1], settings.vertices[v2]))
            }
        });
    }

    return result;
}

void phys_world_remove_cloth(PHYS_World* w, PHYS_Cloth object) {
    for EachIndex(i, object.distance_constraints_count) {
        phys_world_remove_constraint(w, object.distance_constraints[i]);
    }
    for EachIndex(i, object.triangle_colliders_count) {
        phys_world_remove_collider(w, object.triangle_colliders[i]);
    }
    for EachIndex(i, object.vertices_count) {
        phys_world_remove_body(w, object.vertices[i]);
    }
}