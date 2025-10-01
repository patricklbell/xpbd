internal void thread_equip(ThreadCtx* ctx) {
    for EachElement(i, ctx->arenas) {
        ctx->arenas[i] = arena_alloc();
    }

    thread_local_ctx = ctx;
}

internal void thread_release() {
    ThreadCtx* ctx = thread_get_context();
    if (ctx == NULL) {
        return;
    }

    for EachElement(i, ctx->arenas) {
        ctx->arenas[i] = arena_alloc();
    }
    thread_local_ctx = NULL;
}

internal ThreadCtx* thread_get_context() {
    return thread_local_ctx;
}

internal Arena* thread_get_scratch(Arena** conflicts, u64 count) {
    ThreadCtx* ctx = thread_get_context();

    for EachElement(i, ctx->arenas) {
        b32 is_conflicting = false;
        for EachIndex(j, count) {
            if (ctx->arenas[i] == conflicts[j]) {
                is_conflicting = true;
                break;
            }
        }
        if (!is_conflicting) {
            return ctx->arenas[i];
        }
    }

    return NULL; // @todo logging
}