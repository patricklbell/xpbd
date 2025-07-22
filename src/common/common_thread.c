void thread_equip(ThreadCtx* ctx) {
    for EachElement(i, ctx->arenas) {
        ctx->arenas[i] = arena_alloc();
    }

    thread_local_ctx = ctx;
}

void thread_release() {
    ThreadCtx* ctx = thread_get_context();
    if (ctx == NULL) {
        return;
    }

    for EachElement(i, ctx->arenas) {
        ctx->arenas[i] = arena_alloc();
    }
    thread_local_ctx = NULL;
}

ThreadCtx* thread_get_context() {
    return thread_local_ctx;
}

Arena* thread_get_scratch(Arena** conflicts, u64 count) {
    ThreadCtx* ctx = thread_get_context();

    for EachElement(i, ctx->arenas) {
        b32 is_conflicting = 0;
        for EachIndex(j, count) {
            if (ctx->arenas[i] == conflicts[j]) {
                is_conflicting = 1;
                break;
            }
        }
        if (!is_conflicting) {
            return ctx->arenas[i];
        }
    }

    return NULL; // @todo logging
}