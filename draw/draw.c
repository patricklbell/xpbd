void d_begin_frame() {
    Arena* arena;
    if (d_thread_ctx == NULL) {
        arena = arena_alloc();
    } else {
        arena = d_thread_ctx->arena;
        arena_clear(d_thread_ctx->arena);
    }

    d_thread_ctx = push_array(arena, D_ThreadCtx, 1);
    d_thread_ctx->arena = arena;
}

void d_end_frame(OS_Handle window, R_Handle rwindow) {
    r_window_begin_frame(window, rwindow);
    r_submit(window, &d_thread_ctx->passes);
    r_window_end_frame(window, rwindow);
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

R_Mesh3DInst* d_mesh(R_Handle mesh_vertices, R_Handle mesh_indices, mat4x4_f32 transform) {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &d_thread_ctx->passes, R_PassKind_3D);
    R_PassParams_3D *params = pass->params_3d;

    // make batch hash map
    if(params->mesh_batches.num_slots == 0) {
        params->mesh_batches.num_slots = 1024;
        params->mesh_batches.slots = push_array(d_thread_ctx->arena, R_BatchGroup3DMapNode *, params->mesh_batches.num_slots);
    }

    // hash batch group params
    u64 hash = 0;
    u64 slot_idx = 0;
    {
        u64 buffer[] = { mesh_vertices.v64, mesh_indices.v64 };
        hash = hash_u64((u8*)buffer, sizeof(buffer));
        slot_idx = hash % params->mesh_batches.num_slots;
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

        // if there is not matching batch group, make one
        if (node == 0) {
            node = push_array(d_thread_ctx->arena, R_BatchGroup3DMapNode, 1);
            stack_push(params->mesh_batches.slots[slot_idx], node);
            node->hash = hash;
            node->batches = r_batch_list_make(sizeof(R_Mesh3DInst));
            node->params.mesh_vertices = mesh_vertices;
            node->params.mesh_indices = mesh_indices;
            node->params.batch_transform = make_diagonal_4x4f32(1.f);
        }
    }

    R_Mesh3DInst *inst = (R_Mesh3DInst*)r_batch_list_push_inst(d_thread_ctx->arena, &node->batches, 256);
    inst->inst_transform = transform;
    return inst;
}