#pragma once

typedef struct R_VertexLayout R_VertexLayout;
struct R_VertexLayout {
    vec3_f32 position;
    vec3_f32 normal;
    vec2_f32 uv;
};

typedef struct R_Mesh3DInst R_Mesh3DInst;
struct R_Mesh3DInst {
    mat4x4_f32 inst_transform;
};

typedef union R_Handle R_Handle;
union R_Handle {
    u64 v64;
    u32 v32[2];
};

typedef struct R_Batch R_Batch;
struct R_Batch
{
    u8 *v;
    u64 num_bytes;
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
    u64 num_batches;
    u64 num_bytes;
    u64 bytes_per_inst;
};

typedef struct R_BatchGroup3DParams R_BatchGroup3DParams;
struct R_BatchGroup3DParams
{
    R_Handle mesh_vertices;
    R_Handle mesh_indices;
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
    u64 num_slots;
};

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
    R_PassKind_COUNT,
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

typedef enum R_ResourceKind
{
    R_ResourceKind_Static,
    R_ResourceKind_Dynamic,
    R_ResourceKind_Stream,
    R_ResourceKind_COUNT,
} R_ResourceKind;

void r_init();
void r_cleanup();

R_BatchList r_batch_list_make(u64 instance_size);
void* r_batch_list_push_inst(Arena *arena, R_BatchList *list, u64 batch_inst_cap);

R_Pass* r_add_pass_of_kind(Arena *arena, R_PassList *list, R_PassKind kind);

R_Handle r_buffer_alloc(R_ResourceKind kind, u32 size, void *data);
void r_buffer_release(R_Handle buffer);

R_Handle    r_os_equip_window(OS_Handle window);
void        r_os_unequip_window(OS_Handle window, R_Handle rwindow);
void        r_os_select_window(OS_Handle window, R_Handle rwindow);

void r_window_begin_frame(OS_Handle window, R_Handle rwindow);
void r_window_end_frame(OS_Handle window, R_Handle rwindow);

void r_submit(R_PassList *passes);