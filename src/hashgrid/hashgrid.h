#pragma once

typedef union HG_3s32 HG_3s32;
union HG_3s32 {
    struct {
        s32 x;
        s32 y;
        s32 z;
    };
    s32 v[3];
};

// helpers
HG_3s32 hg_3f32_3u32(vec3_f32 x, f32 scale);
u32     hg_hash_3u32(HG_3s32 x, u32 mod);
u32     hg_hash_3f32(vec3_f32 x, f32 scale, u32 mod);

// hashgrid
typedef struct HG_Cell HG_Cell;
struct HG_Cell {
    u32 object_index;
};

typedef struct HG_Hashgrid HG_Hashgrid;
struct HG_Hashgrid {
    Arena* arena;

    f32 cell_length;

    u32 objects_count;
    u32* objects;

    u32 cells_count;
    HG_Cell* cells;
};

#define HG_OBJECT_TO_TABLE_RATIO 5

HG_Hashgrid hg_build_hashgrid(
    Arena* arena, f32 scale,
    vec3_f32* positions, u64 positions_stride, u32 positions_count
);

// queries
typedef struct HG_QueryResultNode HG_QueryResultNode;
struct HG_QueryResultNode {
    HG_QueryResultNode* next;
    u32 id;
};

typedef struct HG_QueryResult HG_QueryResult;
struct HG_QueryResult {
    HG_QueryResultNode* first;
    u32 length;
};

HG_QueryResult hg_hashgrid_query(
    HG_Hashgrid* grid, Arena* arena,
    f32 radius, vec3_f32 position
);

typedef struct HG_BatchQueryResult HG_BatchQueryResult;
struct HG_BatchQueryResult {
    u32* object_hits_start;
    u32 object_count;

    u64* hits_data;
    u32 hits_capacity;
    u32 hits_count;
};

#define HG_BATCH_QUERY_DYNAMIC_ARRAY_GROW_RATE 2

HG_BatchQueryResult hg_hashgrid_batch_query(
    HG_Hashgrid* grid, Arena* arena, u32 expected_hits,
    f32 radius, vec3_f32* positions, u64 positions_stride, u64* data, u64 data_stride, u32 object_count
);