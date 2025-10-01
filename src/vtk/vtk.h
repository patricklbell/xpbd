#pragma once

typedef enum VTK_CellType {
    VTK_CellType_None               = 0,
    VTK_CellType_Vertex             = 1,
    VTK_CellType_PolyVertex         = 2,
    VTK_CellType_Line               = 3,
    VTK_CellType_PolyLine           = 4,
    VTK_CellType_Triangle           = 5,
    VTK_CellType_TriangleStrip      = 6,
    VTK_CellType_Polygon            = 7,
    VTK_CellType_Pixel              = 8,
    VTK_CellType_Quad               = 9,
    VTK_CellType_Tetrahedron        = 10,
    VTK_CellType_Voxel              = 11,
    VTK_CellType_Hexahedron         = 12,
    VTK_CellType_Wedge              = 13,
    VTK_CellType_Pyramid            = 14,
    VTK_CellType_PentagonalPrism    = 15,
    VTK_CellType_HexagonalPrism     = 16,
    VTK_CellType_COUNT ENUM_CASE_UNUSED,
} VTK_CellType;

typedef struct VTK_Data VTK_Data;
struct VTK_Data {
    u32 points_count;
    vec3_f32* points;

    u32 indices_counts[VTK_CellType_COUNT];
    u32* indices[VTK_CellType_COUNT];
};

typedef struct VTK_LoadResult VTK_LoadResult;
struct VTK_LoadResult {
    VTK_Data v;
    NTString8 error;
};

typedef struct VTK_LoadSettings VTK_LoadSettings;
struct VTK_LoadSettings {};

internal VTK_LoadResult vtk_load(Arena* arena, NTString8 path, VTK_LoadSettings settings);