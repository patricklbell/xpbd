internal OS_Event* os_gfx_add_event(Arena* arena, OS_Events* list, OS_Event event) {
    OS_EventNode* n = push_array(arena, OS_EventNode, 1);
    dllist_push_back(list->first, list->last, n);
    list->length++;
    n->v = event;
    return &n->v;
}