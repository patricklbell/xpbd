R_BatchList r_batch_list_make(u64 instance_size) {
    R_BatchList list = {0};
    list.bytes_per_inst = instance_size;
    return list;
}

void* r_batch_list_push_inst(Arena *arena, R_BatchList *list, u64 batch_inst_cap) {
    R_BatchNode *n = list->last;
    if(n == 0 || n->v.num_bytes + list->bytes_per_inst > n->v.byte_cap)
    {
        n = push_array(arena, R_BatchNode, 1);
        n->v.byte_cap = batch_inst_cap*list->bytes_per_inst;
        n->v.v = push_array(arena, u8, n->v.byte_cap); 
        sllist_push(list->first, list->last, n);
        list->num_batches += 1;
    }

    void* inst = n->v.v + n->v.num_bytes;
    n->v.num_bytes += list->bytes_per_inst;
    list->num_bytes += list->bytes_per_inst;
    
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