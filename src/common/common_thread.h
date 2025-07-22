#pragma once

typedef struct ThreadCtx ThreadCtx;
struct ThreadCtx {
    Arena *arenas[2];
};

thread_static ThreadCtx* thread_local_ctx;

void        thread_equip(ThreadCtx* ctx);
void        thread_release();
ThreadCtx*  thread_get_context();
Arena*      thread_get_scratch(Arena** conflicts, u64 count);

#define scratch_begin(conflicts, count)             temp_begin(thread_get_scratch(conflicts, count))
#define scratch_begin_a(arena)                      scratch_begin(&arena, 1)
#define scratch_end(scratch)                        temp_end(scratch)