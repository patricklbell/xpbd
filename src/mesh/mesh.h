#pragma once

typedef struct MS_Mesh MS_Mesh;
struct MS_Mesh {
    R_VertexLayout* vertices;
    u32 vertices_count;
    u32* indices;
    u32 indices_count;
};

typedef struct MS_LoadResult MS_LoadResult;
struct MS_LoadResult {
    MS_Mesh v;
    NTString8 error;
};

typedef enum MS_Topology {
    MS_Topology_Default     = 0,
    MS_Topology_Line        = 2,
    MS_Topology_Triangle    = 3,
    MS_Topology_Quad        = 4,
    MS_Topology_COUNT,
} MS_Topology;

typedef enum MS_VertexFlags {
    MS_VertexFlags_Default ENUM_CASE_UNUSED = 0,
    MS_VertexFlags_P,
    MS_VertexFlags_PT,
    MS_VertexFlags_PN,
    MS_VertexFlags_PTN,
    MS_VertexFlags_COUNT ENUM_CASE_UNUSED,
} MS_VertexFlags;

typedef struct MS_LoadSettings MS_LoadSettings;
struct MS_LoadSettings {
    MS_Topology topology;
    MS_VertexFlags vertex_flags;
};

MS_LoadResult ms_load_obj(Arena* arena, NTString8 path, MS_LoadSettings settings);

// helpers
void ms_calculate_flat_normals(MS_Mesh* mesh, MS_Topology topology);

// internal
typedef struct MS_VertexMapHash MS_VertexMapHash;
struct MS_VertexMapHash {
    u32 position;
    u32 normal;
    u32 uv;
};

typedef struct MS_VertexMapNode MS_VertexMapNode;
struct MS_VertexMapNode {
    MS_VertexMapNode* next;
    MS_VertexMapHash hash;
    u32 index;
};

typedef struct MS_VertexMap MS_VertexMap;
struct MS_VertexMap {
    MS_VertexMapNode** slots;
    u64 slots_count;
    u64 vertices_count;
};