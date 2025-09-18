void dbgdraw_begin(b32 do_not_clear) {
    Arena* arena;
    if (dbgdraw_thread_ctx == NULL) {
        arena = arena_alloc();
        dbgdraw_thread_ctx = push_array(arena, DBGDRAW_ThreadCtx, 1);
        dbgdraw_thread_ctx->arena = arena;
        dbgdraw_thread_ctx->edge_buffer = r_zero_handle();
    } else if (!do_not_clear) {
        arena = dbgdraw_thread_ctx->arena;
        arena_pop_to(dbgdraw_thread_ctx->arena, ARENA_HEADER_SIZE + sizeof(*dbgdraw_thread_ctx));
        dbgdraw_thread_ctx->edges = (DBGDRAW_BatchList){};
        dbgdraw_thread_ctx->points = (DBGDRAW_BatchList){};
    }
}

static void dbgdraw_load_into_rbuffer(R_Handle* buffer, u32* buffer_size, u32 size, void* data) {
    if (*buffer_size < size) {
        if (!r_is_zero_handle(*buffer)) {
            r_buffer_release(buffer); // @todo resize?
        }
        *buffer = r_buffer_alloc(R_ResourceKind_Stream, R_ResourceHint_Array, size, data);
        *buffer_size = size;
    } else {
        r_buffer_load(buffer, 0, size, data);
    }
}

void dbgdraw_submit() {
    if (dbgdraw_thread_ctx->edges.total_count) {
        // convert edges into flat vertex array
        R_VertexFlag edge_flags = R_VertexFlag_PC;
        u32 edges_size = dbgdraw_thread_ctx->edges.total_count*r_vertex_size(edge_flags);
        void* edges = arena_push(dbgdraw_thread_ctx->arena, edges_size, r_vertex_align(edge_flags));
        {
            vec3_f32* p_start = OffsetPtr(edges, r_vertex_offset(edge_flags, R_VertexFlag_P), vec3_f32);
            vec3_f32* c_start = OffsetPtr(edges, r_vertex_offset(edge_flags, R_VertexFlag_C), vec3_f32);
            u64 p_stride = r_vertex_stride(edge_flags, R_VertexFlag_P);
            u64 c_stride = r_vertex_stride(edge_flags, R_VertexFlag_C);
    
            u32 offset_count = 0;
            for EachList(n, DBGDRAW_BatchNode, dbgdraw_thread_ctx->edges.first) {
                for EachIndex(i, n->count) {
                    *OffsetPtr(p_start, offset_count*p_stride, R_VertexType_P) = n->vertices[i];
                    *OffsetPtr(c_start, offset_count*c_stride, R_VertexType_C) = n->colors[i];
                    offset_count++;
                }
            }
        }
        dbgdraw_load_into_rbuffer(&dbgdraw_thread_ctx->edge_buffer, &dbgdraw_thread_ctx->edge_buffer_size, edges_size, edges);
        arena_pop(dbgdraw_thread_ctx->arena, edges_size);
    
        d_debug(r_buffer_view(dbgdraw_thread_ctx->edge_buffer, edges_size), edge_flags, r_zero_handle(), R_VertexTopology_Lines);
    }

    if (dbgdraw_thread_ctx->points.total_count) {
        // convert points into flat array
        // @note splat uses texcoords for radii
        // @todo depth sorting
        R_VertexFlag point_flags = R_VertexFlag_PTC;
        u32 points_size = dbgdraw_thread_ctx->points.total_count*r_vertex_size(point_flags);
        void* points = arena_push(dbgdraw_thread_ctx->arena, points_size, r_vertex_align(point_flags));
        {
            vec3_f32* p_start = OffsetPtr(points, r_vertex_offset(point_flags, R_VertexFlag_P), vec3_f32);
            vec2_f32* t_start = OffsetPtr(points, r_vertex_offset(point_flags, R_VertexFlag_T), vec2_f32);
            vec3_f32* c_start = OffsetPtr(points, r_vertex_offset(point_flags, R_VertexFlag_C), vec3_f32);
            u64 p_stride = r_vertex_stride(point_flags, R_VertexFlag_P);
            u64 t_stride = r_vertex_stride(point_flags, R_VertexFlag_T);
            u64 c_stride = r_vertex_stride(point_flags, R_VertexFlag_C);
    
            u32 offset_count = 0;
            for EachList(n, DBGDRAW_BatchNode, dbgdraw_thread_ctx->points.first) {
                for EachIndex(i, n->count) {
                    *OffsetPtr(p_start, offset_count*p_stride, R_VertexType_P) = n->vertices[i];
                    *OffsetPtr(t_start, offset_count*t_stride, R_VertexType_T) = n->radii[i];
                    *OffsetPtr(c_start, offset_count*c_stride, R_VertexType_C) = n->colors[i];
                    offset_count++;
                }
            }
        }
        dbgdraw_load_into_rbuffer(&dbgdraw_thread_ctx->point_buffer, &dbgdraw_thread_ctx->point_buffer_size, points_size, points);
        arena_pop(dbgdraw_thread_ctx->arena, points_size);

        d_splat(r_buffer_view(dbgdraw_thread_ctx->point_buffer, points_size), point_flags);
    }
}

void dbgdraw_edge_batch(vec3_f32* vertices, vec3_f32* colors, u32 count) {
    DBGDRAW_BatchNode* n = dbgdraw_thread_ctx->edges.last;

    if (n == NULL || n->count + count > ArrayLength(n->vertices)) {
        n = push_array(dbgdraw_thread_ctx->arena, DBGDRAW_BatchNode, 1);
        sllist_push(dbgdraw_thread_ctx->edges.first, dbgdraw_thread_ctx->edges.last, n);
    }

    Assert(count <= ArrayLength(n->vertices)); // @todo
    memcpy(&n->vertices[n->count], vertices, count*sizeof(*vertices));
    memcpy(&n->colors[n->count], colors, count*sizeof(*colors));
    n->count+=count;
    dbgdraw_thread_ctx->edges.total_count+=count; 
}
void dbgdraw_point_batch(vec3_f32* points, vec3_f32* colors, vec2_f32* radii, u32 count) {
    DBGDRAW_BatchNode* n = dbgdraw_thread_ctx->points.last;

    if (n == NULL || n->count + count > ArrayLength(n->vertices)) {
        n = push_array(dbgdraw_thread_ctx->arena, DBGDRAW_BatchNode, 1);
        sllist_push(dbgdraw_thread_ctx->points.first, dbgdraw_thread_ctx->points.last, n);
    }

    Assert(count <= ArrayLength(n->vertices)); // @todo
    memcpy(&n->vertices[n->count], points, count*sizeof(*points));
    memcpy(&n->colors[n->count], colors, count*sizeof(*colors));
    memcpy(&n->radii[n->count], radii, count*sizeof(*radii));
    n->count+=count;
    dbgdraw_thread_ctx->points.total_count+=count; 
}