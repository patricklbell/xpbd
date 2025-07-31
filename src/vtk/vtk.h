#pragma once

typedef struct VTK_Data VTK_Data;
struct VTK_Data {
    u32 points_count;
    vec3_f32* points;

    // volume elements
    u32 volume_indices_count;
    u32* volume_indices;

    u32 volume_edge_indices_count;
    u32* volume_edge_indices;

    // surface elements
    u32 surface_indices_count;
    u32* surface_indices;

    u32 surface_edge_indices_count;
    u32* surface_edge_indices;
};

typedef struct VTK_LoadResult VTK_LoadResult;
struct VTK_LoadResult {
    VTK_Data v;
    NTString8 error;
};

typedef struct VTK_LoadSettings VTK_LoadSettings;
struct VTK_LoadSettings {
    b32 calculate_surface_edges;
    b32 calculate_volume_edges;
};

VTK_LoadResult vtk_load(Arena* arena, NTString8 path, VTK_LoadSettings settings);

// internal
typedef enum VTK_CellType {
    VTK_CellType_Triangle = 5,
    VTK_CellType_Tetrahedron = 10,
} VTK_CellType;

typedef enum VTK_PointCount {
    VTK_PointCount_Triangle    = 3,
    VTK_PointCount_Tetrahedron = 4,
    VTK_PointCount_COUNT       = 5,
} VTK_PointCount;

typedef struct VTK_EdgeMapHash VTK_EdgeMapHash;
struct VTK_EdgeMapHash {
    u32 i;
    u32 j;
};

typedef struct VTK_EdgeMapNode VTK_EdgeMapNode;
struct VTK_EdgeMapNode {
    VTK_EdgeMapNode* next;
    VTK_EdgeMapHash hash;
};

typedef struct VTK_EdgeMap VTK_EdgeMap;
struct VTK_EdgeMap {
    VTK_EdgeMapNode** slots;
    u64 slots_count;
    u64 edge_count;
};