// files
static emscripten_fetch_t* os_handle_to_fetch(OS_Handle file) {
    return (emscripten_fetch_t*)file.v64[0];
}

static void os_open_readonly_file_thread_on_success(emscripten_fetch_t* fetch) {
    sem_t* is_fetch_done = (sem_t*)fetch->userData;
    sem_post(is_fetch_done);
}

static void os_open_readonly_file_thread_on_error(emscripten_fetch_t* fetch) {
    sem_t* is_fetch_done = (sem_t*)fetch->userData;
    sem_post(is_fetch_done);
}

typedef struct OS_WASM_OpenReadonlyFileThreadCtx OS_WASM_OpenReadonlyFileThreadCtx;
struct OS_WASM_OpenReadonlyFileThreadCtx {
    emscripten_fetch_t* fetch;
    const char* path;
};

static void* os_open_readonly_file_thread(void* args) {
    OS_WASM_OpenReadonlyFileThreadCtx* ctx = (OS_WASM_OpenReadonlyFileThreadCtx*)args;

    // create a semaphore to track fetch
    sem_t is_fetch_done;
    int err = sem_init(&is_fetch_done, /* pshared */ 0, /* value */ 0);
    Assert(err == 0);

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    
    attr.userData = (void*)&is_fetch_done;
    attr.onsuccess = os_open_readonly_file_thread_on_success;
    attr.onerror = os_open_readonly_file_thread_on_error;
    ctx->fetch = emscripten_fetch(&attr, ctx->path);

    sem_wait(&is_fetch_done);

    return NULL;
}

OS_Handle os_open_readonly_file(NTString8 path) {
    // Both Asyncify and JSPI cause page to crash with no error after
    // calling emscripten_wget_data.
    // void* buffer;
    // int num, error;
    // emscripten_wget_data(path.cstr, &buffer, &num, &error);
    // buffer = NULL;
    // num = 0;
    // error = 0;
    // if (error != 0) {
    //     return os_zero_handle();
    // }
    

    // fetch needs to be performed on a thread if the code is not running
    // in a worker, so dodgy solution is to just spawn a thread here for loading
    // the file
    OS_WASM_OpenReadonlyFileThreadCtx ctx = {
        .path = path.cstr,
    };
    pthread_t tid;
    pthread_create(&tid, NULL, os_open_readonly_file_thread, (void*)&ctx);
    pthread_join(tid, NULL);
    
    if (ctx.fetch->status != 200) {
        emscripten_fetch_close(ctx.fetch);
        return os_zero_handle();
    }
    
    OS_Handle file;
    file.v64[0] = (u64)ctx.fetch;
    file.v64[1] = 0;
    return file;
}

void os_close_file(OS_Handle file) {
    emscripten_fetch_close(os_handle_to_fetch(file));
}

void os_set_file_offset(OS_Handle file, u64 offset) {
    emscripten_fetch_t* fetch = os_handle_to_fetch(file);
    Assert(offset >= 0 && offset < fetch->numBytes);
    file.v64[1] = offset;
}

b8 os_is_eof(OS_Handle file) {
    emscripten_fetch_t* fetch = os_handle_to_fetch(file);
    u64 offset = file.v64[1];
    return offset >= fetch->numBytes;
}

NTString8 os_read_line_ml(Arena* arena, OS_Handle file, u64 max_line_length) {
    emscripten_fetch_t* fetch = os_handle_to_fetch(file);

    const char* data = fetch->data;
    u64* offset = &file.v64[1];
    u64 length = fetch->numBytes;

    NTString8 result;
    result.data = push_array(arena, u8, max_line_length + 1);
    char c = 0;
    u64 i;
    for (i = 0; *offset < length && c != '\n' && i < max_line_length; ++i) {
        c = data[*offset];
        result.data[i] = (u8)c;
        (*offset)++;
    }
    result.data[i] = '\0';
    result.length = i;

    // consume new line if needed
    if (c == '\n') {
        (*offset)++;
    }

    return result;
}