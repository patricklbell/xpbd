static force_inline u64 r_vertex_size(R_VertexFlag flags) {
    return count_ones_u64(flags)*sizeof(vec4_f32);
}
static force_inline u64 r_vertex_align(R_VertexFlag flags) {
    return sizeof(vec4_f32);
}
static force_inline u64 r_vertex_offset(R_VertexFlag flags, R_VertexFlag flag) {
    Assert(flags & flag);
    return first_set_bit_u64(flags & flag)*sizeof(vec4_f32);
}
static force_inline u64 r_vertex_stride(R_VertexFlag flags, R_VertexFlag flag) {
    Assert(flags & flag);
    return r_vertex_size(flags);
}
static force_inline u64 r_vertex_i_offset(R_VertexFlag flags, R_VertexFlag flag, u64 i) {
    return r_vertex_offset(flags, flag) + i*r_vertex_stride(flags, flag);
}

R_BatchList r_batch_list_make(u64 instance_size) {
    R_BatchList list = {0};
    list.bytes_per_inst = instance_size;
    return list;
}

void* r_batch_list_push_inst(Arena *arena, R_BatchList *list, u64 batch_inst_cap) {
    R_BatchNode *n = list->last;
    if(n == 0 || n->v.bytes_count + list->bytes_per_inst > n->v.byte_cap)
    {
        n = push_array(arena, R_BatchNode, 1);
        n->v.byte_cap = batch_inst_cap*list->bytes_per_inst;
        n->v.v = push_array(arena, u8, n->v.byte_cap); 
        sllist_push(list->first, list->last, n);
        list->batches_count += 1;
    }

    void* inst = n->v.v + n->v.bytes_count;
    n->v.bytes_count += list->bytes_per_inst;
    list->bytes_count += list->bytes_per_inst;
    
    return inst;
}

R_Pass* r_pass_from_kind(Arena *arena, R_PassList *list, R_PassKind kind) {
    Assert(kind == R_PassKind_3D); // @todo

    R_PassNode *n = list->last;
    if(n == NULL || n->v.kind != kind) {
        n = push_array(arena, R_PassNode, 1);
        
        sllist_push(list->first, list->last, n);
        list->length += 1;
        n->v.kind = kind;
        n->v.params = push_array(arena, u8, sizeof(R_PassParams_3D));
    }

    return &n->v;
}