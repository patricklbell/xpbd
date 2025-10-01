f64 os_now_seconds() {
    struct timeval tval;
    gettimeofday(&tval, NULL);
    return (f64)tval.tv_sec + (f64)tval.tv_usec / Million(1.f);
}

// memory management
void* os_allocate(u64 size) {
    return malloc(size);
}

void os_deallocate(void* ptr) {
    free(ptr);
}