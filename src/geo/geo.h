#pragma once

typedef enum GEO_Topology {
    GEO_Topology_Empty       = 0,
    GEO_Topology_Point       = 1,
    GEO_Topology_Edge        = 2,
    GEO_Topology_Line        = 2,
    GEO_Topology_Triangle    = 3,
    GEO_Topology_Quad        = 4,
    GEO_Topology_Tetrahedron = 4,
    GEO_Topology_COUNT,
} GEO_Topology;

#define GEO_MAX_CLIPPED_TOPOLOGY (2*GEO_Topology_COUNT)

typedef struct GEO_Polygon GEO_Polygon;
struct GEO_Polygon {
    GEO_Topology topology;
    // @note needs enough space for worst case clipping
    vec3_f32 data[GEO_MAX_CLIPPED_TOPOLOGY];
};

typedef enum GEO_Connected {
  GEO_Connected_Strongly,
  GEO_Connected_Ring,  
} GEO_Connected;

// edge map
typedef struct GEO_EdgeMapHash GEO_EdgeMapHash;
struct GEO_EdgeMapHash {
    u32 i;
    u32 j;
};

typedef struct GEO_EdgeMapNode GEO_EdgeMapNode;
struct GEO_EdgeMapNode {
    GEO_EdgeMapNode* next;
    GEO_EdgeMapHash hash;
    f32 data;
};

typedef struct GEO_EdgeMap GEO_EdgeMap;
struct GEO_EdgeMap {
    Arena* arena;
    GEO_EdgeMapNode** slots;
    u64 slots_count;
    u64 edge_count;
};

GEO_EdgeMap         geo_make_edge_map(Arena* arena, u64 slots_count);
GEO_EdgeMapNode*    geo_edge_map_add_edge(GEO_EdgeMap* map, GEO_EdgeMapHash hash);
void                geo_edge_map_extract_edges(Arena* arena, GEO_EdgeMap* map, u32** edge_indices, u32* edge_indices_count);

// neighbor map
typedef struct GEO_NeighborMapNode GEO_NeighborMapNode;
struct GEO_NeighborMapNode {
    GEO_NeighborMapNode* next;
    u32 v;
};

typedef struct GEO_NeighborMap GEO_NeighborMap;
struct GEO_NeighborMap {
    Arena* arena;
    GEO_NeighborMapNode** points;
    u32 points_count;
    u32 directed_edge_count;
};

GEO_NeighborMap geo_make_neighbor_map(Arena* arena, u32 points_count);
void geo_neighbor_map_add_directed_edge(GEO_NeighborMap* map, u32 src, u32 dst);
void geo_neighbor_map_add_edge(GEO_NeighborMap* map, u32 i, u32 j);
void geo_neighbor_map_add_indices(GEO_NeighborMap* map, GEO_Topology topology, const GEO_Connected connected, u32* indices, u32 indices_count);

// processing
void geo_calculate_edges(
    Arena* arena, u32 approx_edges, GEO_Topology topology, const GEO_Connected connected,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_count
);
void geo_calculate_points(
    Arena* arena,
    u32 in_point_count, u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_count
);

// @note assume CCW winding order
void geo_calculate_flat_normals(
    vec3_f32* in_p, u64 in_p_stride, u64 in_p_count,
    vec3_f32* out_n, u64 in_n_stride
);
// @note assumes normals are zeroed
// @note assume CCW winding order
void geo_calculate_smooth_normals(
    GEO_Topology topology,
    vec3_f32* in_p, u64 in_p_stride, u64 in_p_count, u32* in_indices, u32 in_indices_count,
    vec3_f32* out_n, u64 in_n_stride
);

void geo_triangulate(
    Arena* arena, GEO_Topology topology, b32 double_sided,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_indices_count
);

#define GEO_EachEdge_Strongly_Open(u, v, T, data, data_count, topology) (u32 off##__LINE__ = 0; off##__LINE__ < data_count; off##__LINE__+=topology) { \
        for (u32 i##__LINE__ = 0; i##__LINE__ < topology; i##__LINE__++) { \
            for (u32 j##__LINE__ = i##__LINE__+1; j##__LINE__ < topology; j##__LINE__++) { \
                T u = data[off##__LINE__ + i##__LINE__]; \
                T v = data[off##__LINE__ + j##__LINE__];
#define GEO_EachEdge_Strongly_Close } \
        } \
    }

#define GEO_EachEdge_Ring_Open(u, v, T, data, data_count, topology) (u32 off##__LINE__ = 0; off##__LINE__ < data_count; off##__LINE__+=topology) { \
        for (u32 i##__LINE__ = 0; i##__LINE__ < topology; i##__LINE__++) { \
            T u = data[off##__LINE__ + i##__LINE__]; \
            T v = data[off##__LINE__ + (i##__LINE__ + 1)%topology];
#define GEO_EachEdge_Ring_Close } \
    }

GEO_Polygon geo_clip_polygon_against_plane(GEO_Polygon* in_to_clip, vec3_f32 in_origin, vec3_f32 in_normal);
GEO_Polygon geo_clip_polygon_against_polygon(GEO_Polygon* in_to_clip, GEO_Polygon* in_clip, vec3_f32 in_normal);
GEO_Polygon geo_clip_polygon_against_edge(GEO_Polygon* in_to_clip, vec3_f32 in_clip_start, vec3_f32 in_clip_end, vec3_f32 in_normal);