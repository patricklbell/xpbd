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

R_PassParams_3D* d_begin_3d_pass(rect_f32 viewport, mat4x4_f32 view, mat4x4_f32 projection) {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &d_thread_ctx->passes, R_PassKind_3D);
    R_PassParams_3D *params = pass->params_3d;
    params->viewport = viewport;
    params->clip = viewport;
    params->view = view;
    params->projection = projection;
    return params;
}

R_Mesh3DInstance* d_mesh(R_Handle vertices, R_VertexFlag flags, R_Handle indices, R_VertexTopology topology, R_Mesh3DMaterial material, mat4x4_f32 transform, vec3_f32 color) {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &d_thread_ctx->passes, R_PassKind_3D);
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
            node->batches = r_batch_list_make(sizeof(R_Mesh3DInstance));
            node->params.mesh_vertices = vertices;
            node->params.mesh_flags = flags;
            node->params.mesh_indices = indices;
            node->params.mesh_topology = topology;
            node->params.mesh_material = material;
            node->params.batch_transform = make_diagonal_4x4f32(1.f);
        }
    }

    R_Mesh3DInstance *inst = (R_Mesh3DInstance*)r_batch_list_push_inst(d_thread_ctx->arena, &node->batches, 256);
    inst->transform = transform;
    inst->color = color;
    return inst;
}