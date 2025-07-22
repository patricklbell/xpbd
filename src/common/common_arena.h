#pragma once

#define ARENA_HEADER_SIZE 128

typedef struct ArenaParams ArenaParams;
struct ArenaParams
{
    u64 page_size;
    void *optional_backing_buffer;
    char *allocation_site_file;
    int allocation_site_line;
};

typedef struct Arena Arena;
struct Arena
{
    Arena* prev;
    Arena* current;

    Arena* free_stack;

    u64 page_offset;
    u64 page_size;
    u64 base_offset;

    char *allocation_site_file;
    int allocation_site_line;
};
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct Temp Temp;
struct Temp
{
    Arena *arena;
    u64 offset;
};

static u64 arena_default_page_size = KB(1);

Arena *arena_alloc_(ArenaParams params);
#define arena_alloc() arena_alloc_((ArenaParams){.page_size = arena_default_page_size, .optional_backing_buffer = NULL, .allocation_site_file = (char*)__FILE__, .allocation_site_line = __LINE__})
void arena_release(Arena *arena);

void *arena_push(Arena *arena, u64 size, u64 align);
u64   arena_offset(Arena *arena);
void  arena_pop_to(Arena *arena, u64 offset);
void  arena_pop(Arena *arena, u64 size);

void  arena_clear(Arena *arena);

Temp temp_begin(Arena *arena);
void temp_end(Temp temp);

// push helper macros
#define push_array_no_zero_aligned(a, T, c, align) (T*)arena_push((a), sizeof(T)*(c), (align))
#define push_array_aligned(a, T, c, align) (T*)memset(push_array_no_zero_aligned(a, T, c, align), 0, sizeof(T)*(c))
#define push_array_no_zero(a, T, c) push_array_no_zero_aligned(a, T, c, Max(8, AlignOf(T)))
#define push_array(a, T, c) push_array_aligned(a, T, c, Max(8, AlignOf(T)))