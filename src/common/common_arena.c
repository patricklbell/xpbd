Arena* arena_alloc_(ArenaParams params) {
  u64 page_size = params.page_size;
  
  // allocate initial block
  void* base = params.optional_backing_buffer;
  if (base == NULL) {
    base = os_allocate(page_size);
  }
  
  // extract arena header & fill
  Arena* arena = (Arena*)base;
  arena->prev = NULL;
  arena->current = arena;
  arena->base_offset = 0;
  arena->page_offset = ARENA_HEADER_SIZE;
  arena->page_size = params.page_size;
  arena->allocation_site_file = params.allocation_site_file;
  arena->allocation_site_line = params.allocation_site_line;
  return arena;
}

void arena_release(Arena* arena) {
    // @note assumes free pages are only marked on bottom
    for (Arena *x = arena->free_stack, *prev = NULL; x != NULL && x != arena; x = prev) {
        prev = x->prev;
        os_deallocate(x);
    }

    for (Arena *x = arena, *prev = NULL; x != NULL; x = prev) {
        prev = x->prev;
        os_deallocate(x);
    }
}

void* arena_push(Arena* arena, u64 size, u64 align) {
    Arena* current = arena->current;
    u64 page_offset_before = AlignPow2(current->page_offset, align);
    u64 page_offset_after = page_offset_before + size;

    // add a new page if needed
    if (page_offset_after > current->page_size) {
        Arena* new_page;

        // if there is a large enough page on top of the free list use it
        if (arena->free_stack != NULL && arena->free_stack->page_size > size) {
            new_page = arena->free_stack;
            arena->free_stack = (new_page->prev == arena) ? NULL : new_page->prev;

            new_page->current = new_page;
            new_page->page_offset = ARENA_HEADER_SIZE;
            new_page->allocation_site_file = current->allocation_site_file;
            new_page->allocation_site_line = current->allocation_site_line;
        } else {
            u64 new_page_size = current->page_size;
            if(size + ARENA_HEADER_SIZE > new_page_size) {
                new_page_size = AlignPow2(size + ARENA_HEADER_SIZE, align);
            }
    
             new_page = arena_alloc_((ArenaParams){
                .page_size = new_page_size,
                .allocation_site_file = current->allocation_site_file,
                .allocation_site_line = current->allocation_site_line
            });
        }

        new_page->base_offset = current->base_offset + current->page_size;
        stack_push_n(arena->current, new_page, prev);

        current = new_page;
        page_offset_before = AlignPow2(current->page_offset, align);
        page_offset_after = page_offset_before + size;
        Assert(page_offset_after <= current->page_size); // @todo reserve across pages
    }

    // push onto current page
    current->page_offset = page_offset_after;
    return (u8*)current + page_offset_before;
}

u64 arena_offset(Arena *arena) {
    return arena->current->base_offset + arena->current->page_offset;
}

void arena_pop_to(Arena* arena, u64 offset) {
    Arena* current = arena->current;

    // free pages if needed @note assumes free pages are only marked on bottom
    while (offset < current->base_offset) {
        stack_pop_n(arena->current, prev);
        os_deallocate(current);
        current = arena->current;
    }

    current->page_offset = Max(offset - current->base_offset, ARENA_HEADER_SIZE);
}

void arena_pop(Arena* arena, u64 size) {
    arena_pop_to(arena, arena_offset(arena) - size);
}

void arena_clear(Arena* arena) {
    arena->page_offset = ARENA_HEADER_SIZE;
    memset((u8*)arena + ARENA_HEADER_SIZE, 0, arena->page_size - ARENA_HEADER_SIZE);

    // add pages to free list for use
    if (arena->current != arena) {
        arena->free_stack = arena->current;
        arena->current = arena;
    }
}

// temporary arena scopes
Temp temp_begin(Arena* arena) {
    return (Temp) {
        .arena = arena,
        .offset = arena_offset(arena),
    };
}

void temp_end(Temp temp) {
    arena_pop_to(temp.arena, temp.offset);
}