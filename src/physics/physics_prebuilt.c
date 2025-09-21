// rigid bodies
void phys_world_remove_rigid_body(PHYS_World* w, PHYS_RigidBody* object) {
    phys_world_remove_collider(w, object->collider_id);
    phys_world_remove_body(w, object->body_id);
}

PHYS_RigidBody phys_world_add_ball(PHYS_World* w, PHYS_Ball_Settings settings) {
    Assert(phys_world_valid_radius(w, settings.radius));

    PHYS_Body body = {
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .inv_mass = (settings.mass > 0.f) ? 1.f / settings.mass : 0.f,
        .restitution = settings.resitution,
    };
    if (settings.can_rotate && settings.mass > 0.f) {
        body.has_inertia = true;
        body.inv_inertia = phys_inv_moment_spehere(settings.radius, settings.mass);
    }

    PHYS_body_id center = phys_world_add_body(w, body);
    PHYS_collider_id sphere = phys_world_add_collider(w, (PHYS_Collider){
        .sphere = {
            .base = {
                .type = PHYS_ColliderType_Sphere,
                .p = center,
                .r = settings.radius,
                .static_friction = settings.coefficient_of_static_friction,
                .dynamic_friction = settings.coefficient_of_dynamic_friction,
            }
        }
    });

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = sphere,
    };
}

PHYS_RigidBody phys_world_add_box(PHYS_World* w, PHYS_Box_Settings settings) {
    Assert(settings.arena != NULL);

    PHYS_Body body = {
        .position = settings.center,
        .linear_velocity = settings.linear_velocity,
        .rotation = (length_4f32(settings.rotation) == 0.f) ? make_identity_quat() : settings.rotation,
        .angular_velocity = settings.angular_velocity,
        .no_gravity = settings.no_gravity,
        .inv_mass = (settings.mass > 0.f) ? 1.f / settings.mass : 0.f,
        .restitution = settings.resitution,
    };
    if (settings.mass > 0.f) {
        body.has_inertia = true;
        body.inv_inertia = phys_inv_moment_rect_cuboid(mul_3f32(settings.extents, 2.0), settings.mass);
    }
    PHYS_body_id center = phys_world_add_body(w, body);

    PHYS_Collider collider = {
        .polytope = {
            .base = {
                .type = PHYS_ColliderType_Polytope,
                .p = center,
                .r = length_3f32(settings.extents),
                .layer = settings.collision_layer,
                .dynamic_friction = settings.coefficient_of_dynamic_friction,
                .static_friction = settings.coefficient_of_static_friction,
            },
            .topology = GEO_Topology_Quad,
            .points = push_array(settings.arena, vec3_f32, 8),
            .points_count = 8,
            .indices = push_array(settings.arena, u32, GEO_Topology_Quad*6),
            .indices_count = GEO_Topology_Quad*6,
            .normals = push_array(settings.arena, vec3_f32, 6),
            .normals_count = 6,
        }
    };
    PHYS_Collider_Polytope* p = &collider.polytope;

    // build box vertices @todo move to geo
    for (u32 i = 0; i < 8; i++) {
        vec3_f32 v = make_3f32((i&(1<<0))>>0, (i&(1<<1))>>1, (i&(1<<2))>>2);
        p->points[i] = elmul_3f32(sub_3f32(mul_3f32(v, 2.f), make_3f32(1.f,1.f,1.f)), settings.extents);
    }
    u32 indice_offset = 0, normals_offset = 0;
    for (int dim = 0; dim < 3; dim++) {
        u32 u = 1<<((dim + 1)%3), v = 1<<((dim + 2)%3);

        u32 cidx = 0b000;
        p->indices[indice_offset++] = cidx;
        p->indices[indice_offset++] = cidx|v;
        p->indices[indice_offset++] = cidx|u|v;
        p->indices[indice_offset++] = cidx|u;
        p->normals[normals_offset++].v[dim] = -1.f;

        // @note order switched to ensure CCW
        cidx = 0b111;
        p->indices[indice_offset++] = cidx;
        p->indices[indice_offset++] = cidx^u;
        p->indices[indice_offset++] = cidx^u^v;
        p->indices[indice_offset++] = cidx^v;
        p->normals[normals_offset++].v[dim] = +1.f;
    }

    PHYS_collider_id rect_cuboid = phys_world_add_collider(w, collider);

    return (PHYS_RigidBody){
        .body_id = center,
        .collider_id = rect_cuboid,
    };
}

// box boundary
PHYS_BoxBoundary phys_world_add_box_boundary(PHYS_World* w, PHYS_BoxBoundary_Settings settings){
    Assert(settings.arena != NULL);
    PHYS_BoxBoundary result;

    if (length_4f32(settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }

    int i = 0;
    for EachIndex(dim, 3) {
        for (int dir = -1; dir <= 1; dir += 2) {
            vec3_f32 normal = zero_struct;
            normal.v[dim] = dir;

            vec3_f32 position = zero_struct;
            position.v[dim] = -dir*settings.extents.v[dim];
            position = add_3f32(settings.center, rot_quat(position, settings.rotation));

            vec3_f32 area = settings.extents;
            area.v[dim] = 0;

            result.positions[i] = phys_world_add_body(w, (PHYS_Body){
                .position = position,
                .no_gravity = true,
                .inv_mass = 0.f,
                .restitution = settings.resitution,
            });
            PHYS_Collider collider = {
                .polytope = {
                    .base = {
                        .type = PHYS_ColliderType_Polytope,
                        .p = result.positions[i],
                        .r = length_3f32(area),
                        .layer = PHYS_ColliderLayer_NoSelf,
                    },
                    .topology = GEO_Topology_Quad,
                    .points = push_array(settings.arena, vec3_f32, GEO_Topology_Quad),
                    .points_count = GEO_Topology_Quad,
                    .indices = push_array(settings.arena, u32, GEO_Topology_Quad),
                    .indices_count = GEO_Topology_Quad,
                    .normals = push_array(settings.arena, vec3_f32, 1),
                    .normals_count = 1,
                }
            };
            PHYS_Collider_Polytope* p = &collider.polytope;

            p->normals[0] = normal;
            
            int point_idx = 0;
            int dimu = (dim+1)%3, dimv = (dim+2)%3;

            // @note multiply by dir to ensure CCW
            p->points[point_idx].v[dimu] = -settings.extents.v[dimu];
            p->points[point_idx].v[dimv] = -settings.extents.v[dimv];
            p->indices[point_idx] = point_idx;
            point_idx++;
            p->points[point_idx].v[dimu] = +dir*settings.extents.v[dimu];
            p->points[point_idx].v[dimv] = -dir*settings.extents.v[dimv];
            p->indices[point_idx] = point_idx;
            point_idx++;
            p->points[point_idx].v[dimu] = +settings.extents.v[dimu];
            p->points[point_idx].v[dimv] = +settings.extents.v[dimv];
            p->indices[point_idx] = point_idx;
            point_idx++;
            p->points[point_idx].v[dimu] = -dir*settings.extents.v[dimu];
            p->points[point_idx].v[dimv] = +dir*settings.extents.v[dimv];
            p->indices[point_idx] = point_idx;
            point_idx++;
            

            result.polytopes[i] = phys_world_add_collider(w, collider);

            i++;
        }
    }

    return result;
}

void phys_world_remove_box_boundary(PHYS_World* w, PHYS_BoxBoundary* object){
    for EachElement(i, object->polytopes) {
        phys_world_remove_collider(w, object->polytopes[i]);
    }
    for EachElement(i, object->positions) {
        phys_world_remove_body(w, object->positions[i]);
    }
}

// softbody
PHYS_Softbody phys_world_add_softbody(PHYS_World* w, PHYS_TetTriSoftbody_Settings settings) {
    PHYS_Softbody result;

    // @todo angular velocity
    if (dot_4f32(settings.rotation, settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }
    if (dot_3f32(settings.scale, settings.scale) == 0.f) {
        settings.scale = make_3f32(1,1,1);
    }
    Assert(settings.mass > 0.f);

    // vertices
    result.vertices_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = phys_scale_rotate_translate(settings.vertices[vert_i], settings.scale, settings.rotation, settings.center),
            .linear_velocity = settings.linear_velocity,
            .is_particle = true,
            .inv_mass = 0.0f, // will be filled when constructing tets
        });
    }

    // surface sphere colliders
    result.sphere_colliders_count = settings.surface_point_indices_count;
    result.sphere_colliders = push_array(settings.arena, PHYS_collider_id, result.sphere_colliders_count);
    for (int surf_i = 0; surf_i < result.sphere_colliders_count; surf_i++) {
        u32 v = settings.surface_point_indices[surf_i];

        result.sphere_colliders[surf_i] = phys_world_add_collider(w, (PHYS_Collider){
            .sphere = {
                .base = {
                    .type = PHYS_ColliderType_Sphere,
                    .p = result.vertices[v],
                    .r = w->min_r,
                    .dynamic_friction = 1.f,
                }
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

        f32 d = length_3f32(elmul_3f32(sub_3f32(settings.vertices[v1], settings.vertices[v2]), settings.scale));

        if (d < 2.f*w->min_r) {
            fprintf(stderr, "degenerate edge in softbody, moving points apart\n");
            d = 2.f*w->min_r;
        }
        result.distance_constraints[edge_i] = phys_world_add_constraint(w, (PHYS_Constraint){
            .compliance = settings.edge_compliance,
            .type = PHYS_ConstraintType_Distance,
            .distance = {
                .b1 = result.vertices[v1],
                .b2 = result.vertices[v2],
                .d = d,
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
            elmul_3f32(settings.vertices[v1], settings.scale),
            elmul_3f32(settings.vertices[v2], settings.scale),
            elmul_3f32(settings.vertices[v3], settings.scale),
            elmul_3f32(settings.vertices[v4], settings.scale)
        );

        // distribute mass among vertices (normalize and inverse after)
        total_volume+=v_rest;
        for (int offset = 0; offset < tetrahedron_size; offset++) {
            u32 v = settings.tetrahedron_indices[tetrahedron_i*tetrahedron_size + offset];
            PHYS_Body* b = phys_world_resolve_body(w, result.vertices[v]);
            b->inv_mass += v_rest / (f32)tetrahedron_size;
        }

        result.volume_constraints[tetrahedron_i] = phys_world_add_constraint(w, (PHYS_Constraint){
            .compliance = settings.volume_compliance,
            .type = PHYS_ConstraintType_Volume,
            .volume = {
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
        b32 active = false;
        for EachIndex(fiber_i, settings.fibers_counts[depth]) {
            f32 align = dot_3f32(dn, settings.fibers[depth][fiber_i].direction);
            if (abs_f32(1.f - align) > EPSILON_F32)
                continue;

            active = true;
            compliance+=settings.fibers[depth][fiber_i].compliance;
        }

        if (active) {
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
    if (dot_4f32(settings.rotation, settings.rotation) == 0.f) {
        settings.rotation = make_identity_quat();
    }
    if (dot_3f32(settings.scale, settings.scale) == 0.f) {
        settings.scale = make_3f32(1,1,1);
    }
    if (settings.thickness == 0.f) {
        settings.thickness = w->min_r;
    }
    settings.fiber_ratio_hint = Max(settings.fiber_ratio_hint, 1);
    Assert(phys_world_valid_radius(w, settings.thickness));
    Assert(settings.mass > 0.f);

    // introduces small instabilities to avoid unphysical behaviour
    f32 jitter = settings.thickness*0.01f;

    // vertices + colliders
    result.vertices_count = settings.vertices_count;
    result.sphere_colliders_count = settings.vertices_count;
    result.vertices = push_array(settings.arena, PHYS_body_id, result.vertices_count);
    result.sphere_colliders = push_array(settings.arena, PHYS_collider_id, result.sphere_colliders_count);
    for EachIndex(vert_i, result.vertices_count) {
        result.vertices[vert_i] = phys_world_add_body(w, (PHYS_Body){
            .position = add_3f32(
                phys_scale_rotate_translate(settings.vertices[vert_i], settings.scale, settings.rotation, settings.center),
                mul_3f32(make_3f32(rand_f32(),rand_f32(),rand_f32()), jitter)
            ),
            .is_particle = true,
            .linear_velocity = settings.linear_velocity,
            .inv_mass = 1.f/(settings.mass / (f32)settings.vertices_count), // @todo area
        });

        result.sphere_colliders[vert_i] = phys_world_add_collider(w, (PHYS_Collider){
            .sphere = {
                .base = {
                    .type = PHYS_ColliderType_Sphere,
                    .p = result.vertices[vert_i],
                    .r = settings.thickness,
                    .dynamic_friction = 1.f,
                }
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

                f32 d = length_3f32(elmul_3f32(sub_3f32(pj, pi), settings.scale));
                if (d < 2.f*settings.thickness) {
                    fprintf(stderr, "self collision at rest in cloth, skipping constraint\n");
                    continue;
                }
                result.distance_constraints[distance_constraint_offset] = phys_world_add_constraint(w, (PHYS_Constraint){
                    .compliance = n_edge->data,
                    .type = PHYS_ConstraintType_Distance,
                    .distance = {
                        .b1 = result.vertices[n_edge->hash.i],
                        .b2 = result.vertices[n_edge->hash.j],
                        .d = d,
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