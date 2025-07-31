void dbgdraw_begin() {
    Arena* arena;
    if (dbgdraw_thread_ctx == NULL) {
        arena = arena_alloc();
        dbgdraw_thread_ctx = push_array(arena, DBGDRAW_ThreadCtx, 1);
    } else {
        arena = dbgdraw_thread_ctx->arena;
        arena_clear(dbgdraw_thread_ctx->arena);
    }

    dbgdraw_thread_ctx = push_array(arena, DBGDRAW_ThreadCtx, 1);
    dbgdraw_thread_ctx->arena = arena;
    dbgdraw_thread_ctx->edge_buffer = r_zero_handle();
}
void dbgdraw_submit(OS_Handle window, R_Handle rwindow) {
    // convert edges into flat vertex array
    R_VertexFlag edge_flags = R_VertexFlag_PC;
    u32 edges_size = dbgdraw_thread_ctx->edges.total_count*r_vertex_size(edge_flags);
    void* edges = arena_push(dbgdraw_thread_ctx->arena, edges_size, r_vertex_align(edge_flags));
    {
        void* p = edges + r_vertex_offset(edge_flags, R_VertexFlag_P);
        void* c = edges + r_vertex_offset(edge_flags, R_VertexFlag_C);
        u64 p_stride = r_vertex_stride(edge_flags, R_VertexFlag_P);
        u64 c_stride = r_vertex_stride(edge_flags, R_VertexFlag_C);
        for EachList(n, DBGDRAW_BatchNode, dbgdraw_thread_ctx->edges.first) {
            for EachIndex(i, n->count) {
                *(R_VertexType_P*)p = n->points[i];
                *(R_VertexType_P*)c = n->colors[i];

                p+=p_stride;
                c+=c_stride;
            }
        }
    }

    // resize render buffer if necessary, otherwise just load
    if (dbgdraw_thread_ctx->edge_buffer_size < edges_size) {
        if (!r_is_zero_handle(dbgdraw_thread_ctx->edge_buffer)) {
            r_buffer_release(dbgdraw_thread_ctx->edge_buffer); // @todo resize?
        }
        dbgdraw_thread_ctx->edge_buffer = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, edges_size, edges);
        dbgdraw_thread_ctx->edge_buffer_size=edges_size;
    } else {
        r_buffer_load(dbgdraw_thread_ctx->edge_buffer, 0, edges_size, edges);
    }

    d_mesh(dbgdraw_thread_ctx->edge_buffer, edge_flags, r_zero_handle(), R_VertexTopology_Lines, R_Mesh3DMaterial_Debug, make_diagonal_4x4f32(1.f), make_3f32(1,1,1));
}

void dbgdraw_edge_batch(vec3_f32* points, vec3_f32* colors, u32 count) {
    DBGDRAW_BatchNode* n = dbgdraw_thread_ctx->edges.last;

    if (n == NULL || n->count + count > ArrayLength(n->points)) {
        n = push_array(dbgdraw_thread_ctx->arena, DBGDRAW_BatchNode, 1);
        sllist_push(dbgdraw_thread_ctx->edges.first, dbgdraw_thread_ctx->edges.last, n);
    }

    Assert(count <= ArrayLength(n->points));
    memcpy(&n->points[n->count], points, count*sizeof(*points));
    memcpy(&n->colors[n->count], colors, count*sizeof(*colors));
    n->count+=count;
    dbgdraw_thread_ctx->edges.total_count+=count; 
}
void dbgdraw_point_batch(vec3_f32* points, vec4_f32* colors, f32* radii, u32 count) {
    NotImplemented;
}