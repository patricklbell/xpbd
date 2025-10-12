// helpers for constraint maps
internal PHYS_constraint_id phys_constraint_map_add_value(PHYS_ConstraintMap* map, Arena* arena, PHYS_ConstraintNodeValue v) {
    PHYS_ConstraintNode* new_node;
    if (map->free_chain != NULL) {
        new_node = map->free_chain;
        stack_pop(map->free_chain);
    } else {
        new_node = push_array(arena, PHYS_ConstraintNode, 1);
        new_node->id.idx = map->max_idx++;
        new_node->id.version = 1; // @note makes sure valid id is non-zero
    }
    Assert(new_node != NULL);

    PHYS_ConstraintList* list = &map->slots[new_node->id.idx % map->slots_count];
    dllist_push_back(list->first, list->last, new_node);

    new_node->v = v;
    return new_node->id;
}
internal PHYS_ConstraintNode* phys_constraint_map_get_node(PHYS_ConstraintMap* map, u32 idx) {
    u32 slot = idx % map->slots_count;
    for EachList(n, PHYS_ConstraintNode, map->slots[slot].first) {
        if (n->id.idx == idx) {
            return n;
        }
    }
    return NULL;
}
internal PHYS_ConstraintNodeValue* phys_constraint_map_get_value(PHYS_ConstraintMap* map, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_constraint_map_get_node(map, id.idx);
    if (n == NULL || n->id.version != id.version) {
        return NULL;
    }
    return &n->v;
}
internal void phys_constraint_map_delete(PHYS_ConstraintMap* map, PHYS_constraint_id id) {
    PHYS_ConstraintNode* n = phys_constraint_map_get_node(map, id.idx);
    Assert(n != NULL && n->id.version == id.version);
    
    // remove from slot's list
    u32 slot = id.idx % map->slots_count;
    PHYS_ConstraintList* list = &map->slots[slot];
    dllist_remove(list->first, list->last, n);

    // invalidate old id by increasing version
    n->id.version++;
    // add node to free chain for reuse
    stack_push(map->free_chain, n);

    map->length--;
}