void d_begin_pipeline() {
    Arena* arena;
    if (d_thread_ctx == NULL) {
        arena = arena_alloc();
        d_thread_ctx = push_array(arena, D_ThreadCtx, 1);
    } else {
        arena = d_thread_ctx->arena;
        arena_clear(d_thread_ctx->arena);
    }

    d_thread_ctx = push_array(arena, D_ThreadCtx, 1);
    d_thread_ctx->arena = arena;
}

void d_submit_pipeline(OS_Handle window, R_Handle rwindow) {
    r_submit(window, &d_thread_ctx->passes);
}

R_PassParams_3D* d_make_3d_pass(rect_f32 viewport, mat4x4_f32 view, mat4x4_f32 projection, b32 debug) {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &d_thread_ctx->passes, debug ? R_PassKind_3DDebug : R_PassKind_3D);
    R_PassParams_3D *params = pass->params_3d;
    params->viewport = viewport;
    params->clip = viewport;
    params->view = view;
    params->projection = projection;
    return params;
}

void* d_3d(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, R_Mesh3DMaterial material, void* instance, u64 instance_size, b32 debug) {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &d_thread_ctx->passes, debug ? R_PassKind_3DDebug : R_PassKind_3D);
    R_PassParams_3D *params = pass->params_3d;

    // make batch hash map
    if(params->mesh_batches.slots_count == 0) {
        params->mesh_batches.slots_count = 1024;
        params->mesh_batches.slots = push_array(d_thread_ctx->arena, R_BatchGroup3DMapNode*, params->mesh_batches.slots_count);
    }

    // hash batch group params
    u64 hash = 0;
    u64 slot_idx = 0;
    {
        struct {
            R_Handle mesh_vertices;
            R_VertexFlag mesh_flags;
            R_Handle mesh_indices;
            R_VertexTopology mesh_topology;
            R_Mesh3DMaterial mesh_material;
        } buffer = {
            .mesh_vertices = vertices,
            .mesh_flags = flags,
            .mesh_indices = indices,
            .mesh_topology = topology,
            .mesh_material = material,
        };
        hash = hash_u64((u8*)&buffer, sizeof(buffer));
        slot_idx = hash % params->mesh_batches.slots_count;
    }

    // check map for matching hash
    R_BatchGroup3DMapNode *node = 0;
    {
        for(R_BatchGroup3DMapNode *n = params->mesh_batches.slots[slot_idx]; n != 0; n = n->next) {
            if (n->hash == hash) {
                node = n;
                break;
            }
        }

        // if there is no matching batch group, make one
        if (node == 0) {
            node = push_array(d_thread_ctx->arena, R_BatchGroup3DMapNode, 1);
            stack_push(params->mesh_batches.slots[slot_idx], node);
            node->hash = hash;
            node->batches = r_batch_list_make(instance_size);
            node->params.mesh_vertices = vertices;
            node->params.mesh_flags = flags;
            node->params.mesh_indices = indices;
            node->params.mesh_topology = topology;
            node->params.mesh_material = material;
            node->params.batch_transform = make_diagonal_4x4f32(1.f);
        }
    }

    void *inst = r_batch_list_push_inst(d_thread_ctx->arena, &node->batches, 128);
    memcpy(inst, instance, instance_size);
    return inst;
}

// wrappers
R_Mesh3DInstance* d_lambertian_mesh(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, mat4x4_f32 transform, vec3_f32 color) {
    R_Mesh3DInstance instance = {
        .transform = transform,
        .color = color,
    };
    return (R_Mesh3DInstance*)d_3d(vertices, flags, indices, topology, R_Mesh3DMaterial_Lambertian, &instance, sizeof(instance), false);
}
R_PBRMesh3DInstance* d_pbr_mesh(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, mat4x4_f32 transform, vec3_f32 albedo, f32 roughness, vec3_f32 specular) {
    R_PBRMesh3DInstance instance = {
        .transform = transform,
        .albedo_roughness = {.xyz = albedo, ._w = roughness},
        .specular = specular,
    };
    return (R_PBRMesh3DInstance*)d_3d(vertices, flags, indices, topology, R_Mesh3DMaterial_DieletricPBR, &instance, sizeof(instance), false);
}

void d_debug(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology) {
    R_Mesh3DInstance instance = {
        .transform = make_diagonal_4x4f32(1.f),
        .color = make_3f32(1.f,1.f,1.f),
    };
    d_3d(vertices, flags, indices, topology, R_Mesh3DMaterial_Debug, &instance, sizeof(instance), true);
}
void d_splat(R_Handle vertices, R_VertexFlag flags) {
    R_Mesh3DInstance instance = {
        .transform = make_diagonal_4x4f32(1.f),
        .color = make_3f32(1.f,1.f,1.f),
    };
    d_3d(vertices, flags, r_zero_handle(), R_VertexTopology_Points, R_Mesh3DMaterial_Splat, &instance, sizeof(instance), true);
}