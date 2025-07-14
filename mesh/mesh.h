#pragma once

typedef struct MS_Mesh MS_Mesh;
struct MS_Mesh {
    R_VertexLayout* vertices;
    u32 num_vertices;
    u32* indices;
    u32 num_indices;
};

typedef struct MS_MeshResult MS_MeshResult;
struct MS_MeshResult {
    MS_Mesh v;
    NTString8 error;
};

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
    u64 num_slots;
    u64 num_vertices;
};

MS_MeshResult ms_load_obj(Arena* arena, NTString8 path);