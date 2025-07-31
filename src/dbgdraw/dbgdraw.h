#define DBGDRAW_BATCH_SIZE 1024

typedef struct DBGDRAW_BatchNode DBGDRAW_BatchNode;
struct DBGDRAW_BatchNode {
    DBGDRAW_BatchNode* next;
    vec3_f32 points[DBGDRAW_BATCH_SIZE];
    vec3_f32 colors[DBGDRAW_BATCH_SIZE];
    u32 count;
};

typedef struct DBGDRAW_BatchList DBGDRAW_BatchList;
struct DBGDRAW_BatchList {
    DBGDRAW_BatchNode* first;
    DBGDRAW_BatchNode* last;
    u32 total_count;
};

typedef struct DBGDRAW_ThreadCtx DBGDRAW_ThreadCtx;
struct DBGDRAW_ThreadCtx {
    Arena* arena;

    R_Handle edge_buffer;
    u32 edge_buffer_size;
    
    DBGDRAW_BatchList edges;
};

thread_static DBGDRAW_ThreadCtx* dbgdraw_thread_ctx = NULL;

void dbgdraw_begin();
void dbgdraw_submit(OS_Handle window, R_Handle rwindow);

void dbgdraw_edge_batch(vec3_f32* points, vec3_f32* colors, u32 count);
void dbgdraw_point_batch(vec3_f32* points, vec4_f32* colors, f32* radii, u32 count);