#pragma once

// vertex
typedef enum R_VertexFlag {
    R_VertexFlag_ZERO = 0,
    R_VertexFlag_P = 1 << 0,
    R_VertexFlag_N = 1 << 1,
    R_VertexFlag_T = 1 << 2,
    R_VertexFlag_C = 1 << 3,
    R_VertexFlag_PN     = R_VertexFlag_P   | R_VertexFlag_N,
    R_VertexFlag_PT     = R_VertexFlag_P   | R_VertexFlag_T,
    R_VertexFlag_PC     = R_VertexFlag_P   | R_VertexFlag_C,
    R_VertexFlag_NT     = R_VertexFlag_N   | R_VertexFlag_T,
    R_VertexFlag_NC     = R_VertexFlag_N   | R_VertexFlag_C,
    R_VertexFlag_TC     = R_VertexFlag_T   | R_VertexFlag_C,
    R_VertexFlag_PNT    = R_VertexFlag_PN  | R_VertexFlag_T,
    R_VertexFlag_PNTC   = R_VertexFlag_PNT | R_VertexFlag_C,
    R_VertexFlag_COUNT,
} R_VertexFlag;

typedef vec3_f32 R_VertexType_P;
typedef vec3_f32 R_VertexType_N;
typedef vec2_f32 R_VertexType_T;
typedef vec3_f32 R_VertexType_C;

static force_inline u64 r_vertex_size(R_VertexFlag flags);
static force_inline u64 r_vertex_align(R_VertexFlag flags);
static force_inline u64 r_vertex_offset(R_VertexFlag flags, R_VertexFlag flag);
static force_inline u64 r_vertex_stride(R_VertexFlag flags, R_VertexFlag flag);
static force_inline u64 r_vertex_i_offset(R_VertexFlag flags, R_VertexFlag flag, u64 i);

typedef enum R_VertexTopology {
    R_VertexTopology_ZERO = 0,
    R_VertexTopology_Points,
    R_VertexTopology_Lines,
    R_VertexTopology_LineStrip,
    R_VertexTopology_Triangles,
    R_VertexTopology_TriangleStrip,
    R_VertexTopology_COUNT ENUM_CASE_UNUSED,
} R_VertexTopology;

// mesh
typedef enum R_Mesh3DMaterial {
    R_Mesh3DMaterial_Lambertian,
    R_Mesh3DMaterial_Flat,
} R_Mesh3DMaterial;

typedef struct R_Mesh3DInstance R_Mesh3DInstance;
struct R_Mesh3DInstance {
    mat4x4_f32 transform;
    vec3_f32 color;
};

// batches
typedef union R_Handle R_Handle;
union R_Handle {
    u64 v64[1];
    u32 v32[2];
};

typedef struct R_Batch R_Batch;
struct R_Batch
{
    u8 *v;
    u64 bytes_count;
    u64 byte_cap;
};

typedef struct R_BatchNode R_BatchNode;
struct R_BatchNode
{
    R_BatchNode *next;
    R_Batch v;
};

typedef struct R_BatchList R_BatchList;
struct R_BatchList
{
    R_BatchNode *first;
    R_BatchNode *last;
    u64 batches_count;
    u64 bytes_count;
    u64 bytes_per_inst;
};

typedef struct R_BatchGroup3DParams R_BatchGroup3DParams;
struct R_BatchGroup3DParams
{
    R_Handle mesh_vertices;
    R_VertexFlag mesh_flags;
    R_Handle mesh_indices;
    R_VertexTopology mesh_topology;
    R_Mesh3DMaterial mesh_material;
    mat4x4_f32 batch_transform;
};

typedef struct R_BatchGroup3DMapNode R_BatchGroup3DMapNode;
struct R_BatchGroup3DMapNode
{
    R_BatchGroup3DMapNode *next;
    u64 hash;
    R_BatchList batches;
    R_BatchGroup3DParams params;
};

typedef struct R_BatchGroup3DMap R_BatchGroup3DMap;
struct R_BatchGroup3DMap
{
    R_BatchGroup3DMapNode **slots;
    u64 slots_count;
};

R_BatchList r_batch_list_make(u64 instance_size);
void*       r_batch_list_push_inst(Arena *arena, R_BatchList *list, u64 batch_inst_cap);

// passes
typedef struct R_PassParams_3D R_PassParams_3D;
struct R_PassParams_3D
{
    rect_f32 viewport;
    rect_f32 clip;
    mat4x4_f32 view;
    mat4x4_f32 projection;
    R_BatchGroup3DMap mesh_batches;
};

typedef enum R_PassKind
{
    R_PassKind_3D,
    R_PassKind_COUNT ENUM_CASE_UNUSED,
} R_PassKind;

typedef struct R_Pass R_Pass;
struct R_Pass
{
    R_PassKind kind;
    union
    {
        void *params;
        R_PassParams_3D *params_3d;
    };
};

typedef struct R_PassNode R_PassNode;
struct R_PassNode
{
    R_PassNode *next;
    R_Pass v;
};

typedef struct R_PassList R_PassList;
struct R_PassList
{
    R_PassNode *first;
    R_PassNode *last;
    u64 length;
};

R_Pass* r_add_pass_of_kind(Arena *arena, R_PassList *list, R_PassKind kind);

// resources
typedef enum R_ResourceKind
{
    R_ResourceKind_Static,
    R_ResourceKind_Dynamic,
    R_ResourceKind_Stream,
    R_ResourceKind_COUNT,
} R_ResourceKind;

typedef enum R_ResourceHint
{
    R_ResourceHint_Array,
    R_ResourceHint_Indices,
    R_ResourceHint_COUNT,
} R_ResourceHint;

R_Handle r_buffer_alloc(R_ResourceKind kind, R_ResourceHint hint, u32 size, void *data);
void     r_buffer_load(R_Handle handle, u32 offset, u32 size, void *data);
void     r_buffer_release(R_Handle buffer);

// setup/teardown
void r_init();
void r_cleanup();

// windows
R_Handle r_os_equip_window(OS_Handle window);
void     r_os_unequip_window(OS_Handle window, R_Handle rwindow);
void     r_os_select_window(OS_Handle window, R_Handle rwindow);

// draw
void r_window_begin_frame(OS_Handle window, R_Handle rwindow);
void r_window_end_frame(OS_Handle window, R_Handle rwindow);
void r_submit(OS_Handle window, R_PassList *passes);