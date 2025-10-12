f64 os_now_seconds() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return (f64)tval.tv_sec + (f64)tval.tv_usec / Million(1.f);
}

// memory management
void* os_allocate(u64 size) {
    void* ptr = malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

void os_deallocate(void* ptr) {
    TracyFree(ptr);
    free(ptr);
}