#pragma once

typedef struct MS_Mesh MS_Mesh;
struct MS_Mesh {
    void* vertices;
    u32 vertices_count;
    u32* indices;
    u32 indices_count;
    R_VertexTopology topology;
    R_VertexFlag flags;
};

typedef struct MS_LoadResult MS_LoadResult;
struct MS_LoadResult {
    MS_Mesh v;
    NTString8 error;
};

typedef struct MS_LoadSettings MS_LoadSettings;
struct MS_LoadSettings {
    R_VertexTopology topology;
    R_VertexFlag flags;
};

MS_LoadResult ms_load_obj(Arena* arena, NTString8 path, MS_LoadSettings settings);

// helpers
void ms_calculate_flat_normals(MS_Mesh* mesh, R_VertexTopology topology);

// internal
typedef struct MS_VertexMapHash MS_VertexMapHash;
struct MS_VertexMapHash {
    u32 indices[3];
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