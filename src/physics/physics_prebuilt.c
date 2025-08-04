// rigid bodies
void phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object) {
    phys_world_remove_collider(w, object->collider_id);
    phys_world_remove_body(w, object->body_id);
}

PHYS_RigidBody phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings){
    Assert(settings.mass > 0.f);
    Assert(phys_world_valid_radius(w, settings.radius));

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

    if (length_4f32(settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }

    int i = 0;
    for EachIndex(dim, 3) {
        for (int offset = -1; offset <= 1; offset += 2) {
            vec3_f32 normal = zero_struct;
            normal.v[dim] = offset;

            vec3_f32 position = zero_struct;
            position.v[dim] = -offset*settings.extents.v[dim];
            position = add_3f32(settings.center, rot_quat(position, settings.rotation));

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
                    .n = rot_quat(normal, settings.rotation),
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
PHYS_Softbody phys_world_add_softbody(PHYS_World* w, PHYS_TetTriSoftbody_Settings settings) {
    PHYS_Softbody result;

    // @todo angular velocity
    if (length_4f32(settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }

    // vertices
    result.vertices_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = add_3f32(rot_quat(settings.vertices[vert_i], settings.rotation), settings.center),
            .linear_velocity = settings.linear_velocity,
            .inv_mass = 0.0f, // will be filled when constructing tets
        });
    }

    // surface sphere colliders
    result.sphere_colliders_count = settings.surface_point_indices_count;
    result.sphere_colliders = push_array(settings.arena, PHYS_collider_id, result.sphere_colliders_count);
    for (int surf_i = 0; surf_i < result.sphere_colliders_count; surf_i++) {
        u32 v = settings.surface_point_indices[surf_i];

        result.sphere_colliders[surf_i] = phys_world_add_collider(w, (PHYS_Collider){
            .type = PHYS_ColliderType_Sphere,
            .sphere = {
                .compliance = 0.f,
                .c = result.vertices[v],
                .r = w->min_r,
            }
        });
    }

    // edge constraints
    const static int edge_size = 2;
    result.distance_constraints_count = settings.tetrahedron_edge_indices_count / edge_size;
    result.distance_constraints = push_array(settings.arena, PHYS_constraint_id, result.distance_constraints_count);
    for (int edge_i = 0; edge_i < result.distance_constraints_count; edge_i++) {
        u32 v1 = settings.tetrahedron_edge_indices[edge_i*edge_size + 0];
        u32 v2 = settings.tetrahedron_edge_indices[edge_i*edge_size + 1];

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
    const static int tetrahedron_size = 4;
    result.volume_constraints_count = settings.tetrahedron_indices_count / tetrahedron_size;
    result.volume_constraints = push_array(settings.arena, PHYS_constraint_id, result.volume_constraints_count);
    for (int tetrahedron_i = 0; tetrahedron_i < result.volume_constraints_count; tetrahedron_i++) {
        u32 v1 = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + 0];
        u32 v2 = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + 1];
        u32 v3 = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + 2];
        u32 v4 = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + 3];

        f32 v_rest = phys_tetrahedron_volume(
            settings.vertices[v1],
            settings.vertices[v2],
            settings.vertices[v3],
            settings.vertices[v4]
        );

        // distribute mass among vertices (normalize and inverse after)
        total_volume+=v_rest;
        for (int offset = 0; offset < tetrahedron_size; offset++) {
            u32 v = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + offset];
            PHYS_Body* b = phys_world_resolve_body(w, result.vertices[v]);
            b->inv_mass += v_rest / (f32)tetrahedron_size;
        }

        result.volume_constraints[tetrahedron_i] = phys_world_add_constraint(w, (PHYS_Constraint){
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
    for EachIndex(i, object.sphere_colliders_count) {
        phys_world_remove_collider(w, object.sphere_colliders[i]);
    }
    for EachIndex(i, object.vertices_count) {
        phys_world_remove_body(w, object.vertices[i]);
    }
}

// cloth
static void phys_world_add_cloth_build_fiber_constraints(PHYS_World* w, PHYS_Cloth_Settings settings, GEO_EdgeMap* edges, GEO_NeighborMap* map, u32 root, u32 node, int depth) {
    if (depth >= settings.fiber_depth)
        return;

    // the hash for the edge we are currently checking
    GEO_EdgeMapHash hash = { .i=root };
    vec3_f32 pi = settings.vertices[hash.i];

    // explore each of the nodes neighbors to see how they connect to the root
    for EachList(neighbor_n, GEO_NeighborMapNode, map->points[node]) {
        hash.j = neighbor_n->v;
        if (hash.i == hash.j)
            continue;

        vec3_f32 pj = settings.vertices[hash.j];
        vec3_f32 d = sub_3f32(pj, pi);
        vec3_f32 dn = normalize_3f32(d);

        f32 compliance = 0.f;
        b32 active = 0;
        for EachIndex(fiber_i, settings.fibers_counts[depth]) {
            f32 align = dot_3f32(dn, settings.fibers[depth][fiber_i].direction);
            if (abs_f32(1.f - align) > EPSILON_F32)
                continue;

            active = 1;
            compliance+=settings.fibers[depth][fiber_i].compliance;
        }

        if (active) {
            // ensure order indepedence
            if (hash.i > hash.j) {
                u32 temp = hash.i;
                hash.i = hash.j;
                hash.j = temp;
            }
            
            GEO_EdgeMapNode* edge = geo_edge_map_add_edge(edges, hash);
            edge->data += compliance;
        }

        // follow the edge check it's neighbors
        phys_world_add_cloth_build_fiber_constraints(w, settings, edges, map, root, hash.j, depth+1);
    }
}

PHYS_Cloth phys_world_add_cloth(PHYS_World* w, PHYS_Cloth_Settings settings) {
    PHYS_Cloth result;

    // solve settings
    if (length_4f32(settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }
    if (settings.thickness == 0.f) {
        settings.thickness = w->min_r;
    }
    settings.fiber_ratio_hint = Max(settings.fiber_ratio_hint, 1);
    Assert(phys_world_valid_radius(w, settings.thickness));

    // vertices + colliders
    result.vertices_count = settings.vertices_count;
    result.sphere_colliders_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    result.sphere_colliders = push_array(settings.arena, PHYS_collider_id, result.sphere_colliders_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = add_3f32(rot_quat(settings.vertices[vert_i], settings.rotation), settings.center),
            .linear_velocity = settings.linear_velocity,
            .inv_mass = 1.f/(settings.mass / (f32)settings.vertices_count), // @todo area
        });

        result.sphere_colliders[vert_i] = phys_world_add_collider(w, (PHYS_Collider){
            .type = PHYS_ColliderType_Sphere,
            .sphere = {
                .compliance = 0.f,
                .c = result.vertices[vert_i],
                .r = settings.thickness,
            }
        });
    }

    {DeferResource(Temp scratch = scratch_begin_a(settings.arena), scratch_end(scratch)){
        // build neighbor map from edges
        GEO_NeighborMap neighbors = geo_make_neighbor_map(scratch.arena, settings.vertices_count);
        geo_neighbor_map_add_indices(
            &neighbors, GEO_Topology_Edge, GEO_Connected_Strongly,
            settings.edge_indices, settings.edge_indices_count
        );
    
        // build set of constrained edges
        GEO_EdgeMap edges = geo_make_edge_map(scratch.arena, settings.fiber_ratio_hint*settings.vertices_count);
        for EachIndex(i0, neighbors.points_count) {
            phys_world_add_cloth_build_fiber_constraints(
                w, settings, &edges, &neighbors,
                /*root*/ i0, /*node*/ i0, /*depth*/ 0
            );
        }

        // create deduplicated constraints
        result.distance_constraints_count = edges.edge_count;
        result.distance_constraints = push_array(settings.arena, PHYS_constraint_id, result.distance_constraints_count);

        u32 distance_constraint_offset = 0;
        for EachIndex(slot, edges.slots_count) {
            for EachList(n_edge, GEO_EdgeMapNode, edges.slots[slot]) {
                vec3_f32 pi = settings.vertices[n_edge->hash.i];
                vec3_f32 pj = settings.vertices[n_edge->hash.j];

                result.distance_constraints[distance_constraint_offset] = phys_world_add_constraint(w, (PHYS_Constraint){
                    .type = PHYS_ConstraintType_Distance,
                    .distance = {
                        .compliance = n_edge->data,
                        .b1 = result.vertices[n_edge->hash.i],
                        .b2 = result.vertices[n_edge->hash.j],
                        .d = length_3f32(sub_3f32(pj, pi)),
                    }
                });
                distance_constraint_offset++;
            }
        }

    }}

    return result;
}

void phys_world_remove_cloth(PHYS_World* w, PHYS_Cloth object) {
    for EachIndex(i, object.distance_constraints_count) {
        phys_world_remove_constraint(w, object.distance_constraints[i]);
    }
    for EachIndex(i, object.sphere_colliders_count) {
        phys_world_remove_collider(w, object.sphere_colliders[i]);
    }
    for EachIndex(i, object.vertices_count) {
        phys_world_remove_body(w, object.vertices[i]);
    }
}