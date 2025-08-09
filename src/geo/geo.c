
// edge map
static b32 geo_hash_is_eq(GEO_EdgeMapHash a, GEO_EdgeMapHash b) {
    return (
        ((a.i == b.i) && (a.j == b.j)) ||
        ((a.i == b.j) && (a.j == b.i))
    );
}

GEO_EdgeMap geo_make_edge_map(Arena* arena, u64 slots_count) {
    return (GEO_EdgeMap) {
        .arena = arena,
        .slots_count = slots_count,
        .slots = push_array(arena, GEO_EdgeMapNode*, slots_count),
        .edge_count = 0,
    };
}

GEO_EdgeMapNode* geo_edge_map_add_edge(GEO_EdgeMap* map, GEO_EdgeMapHash hash) {
    // order independent hashing so i,j and j,i overlap
    u64 slot = (
        hash_u64((u8*)&hash.i, sizeof(hash.i)) + 
        hash_u64((u8*)&hash.j, sizeof(hash.j))
     ) % map->slots_count;

    // try to find matching vertex
    for EachList(n_edge, GEO_EdgeMapNode, map->slots[slot]) {
        if (geo_hash_is_eq(n_edge->hash, hash)) {
            return n_edge;
        }
    }

    // otherwise, create one
    GEO_EdgeMapNode* n_edge = push_array(map->arena, GEO_EdgeMapNode, 1);
    n_edge->hash = hash;
    stack_push(map->slots[slot], n_edge);
    map->edge_count++;
    return n_edge;
}

void geo_edge_map_extract_edges(Arena* arena, GEO_EdgeMap* map, u32** edge_indices, u32* edge_indices_count) {
    *edge_indices_count = map->edge_count*2;
    *edge_indices = push_array(arena, u32, *edge_indices_count);

    u32 edge_offset = 0;
    for EachIndex(slot, map->slots_count) {
        for EachList(n_edge, GEO_EdgeMapNode, map->slots[slot]) {
            (*edge_indices)[edge_offset] = n_edge->hash.i; edge_offset++;
            (*edge_indices)[edge_offset] = n_edge->hash.j; edge_offset++;
        }
    }
}

// neighbor map
GEO_NeighborMap geo_make_neighbor_map(Arena* arena, u32 points_count) {
    return (GEO_NeighborMap) {
        .arena = arena,
        .points = push_array(arena, GEO_NeighborMapNode*, points_count),
        .points_count = points_count,
    };
}

void geo_neighbor_map_add_directed_edge(GEO_NeighborMap* map, u32 src, u32 dst) {
    GEO_NeighborMapNode* n = push_array(map->arena, GEO_NeighborMapNode, 1);
    n->v = dst;
    stack_push(map->points[src], n);
    map->directed_edge_count++;
}

void geo_neighbor_map_add_edge(GEO_NeighborMap* map, u32 i, u32 j) {
    geo_neighbor_map_add_directed_edge(map, i, j);
    geo_neighbor_map_add_directed_edge(map, j, i);
}

void geo_neighbor_map_add_indices(GEO_NeighborMap* map, GEO_Topology topology, const GEO_Connected connected, u32* indices, u32 indices_count) {
    Assert(connected == GEO_Connected_Strongly || connected == GEO_Connected_Ring);
    Assert(!(connected == GEO_Connected_Ring && topology <= GEO_Topology_Line));

    for (u32 indices_off = 0; indices_off < indices_count; indices_off+=topology) {
        for (u32 i = 0; i < topology; i++) {
            if (connected == GEO_Connected_Strongly) {
                for (u32 j = i+1; j < topology; j++) {
                    u32 indice_i = indices[indices_off + i];
                    u32 indice_j = indices[indices_off + j];
    
                    geo_neighbor_map_add_edge(map, indice_i, indice_j);
                }
            } else {
                u32 indice_i = indices[indices_off + i];
                u32 indice_j = indices[indices_off + (i+1)%topology];
    
                geo_neighbor_map_add_edge(map, indice_i, indice_j);
            }
        }
    }
}

// processing
void geo_calculate_edges(
    Arena* arena, u32 approx_edges, GEO_Topology topology, const GEO_Connected connected,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_count
) {
    Assert(connected == GEO_Connected_Strongly || connected == GEO_Connected_Ring);
    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
        GEO_EdgeMap edge_map = geo_make_edge_map(scratch.arena, approx_edges);

        for (int in_indice_i = 0; in_indice_i < in_indices_count; in_indice_i+=topology) {
            for (int point_i = 0; point_i < topology; point_i++) {
                if (connected == GEO_Connected_Strongly) {
                    for (int point_j = point_i+1; point_j < topology; point_j++) {
                        GEO_EdgeMapHash hash = {
                            .i = in_indices[in_indice_i + point_i],
                            .j = in_indices[in_indice_i + point_j],
                        };
                        geo_edge_map_add_edge(&edge_map, hash);
                    }
                } else {
                    GEO_EdgeMapHash hash = {
                        .i = in_indices[in_indice_i + point_i],
                        .j = in_indices[in_indice_i + (point_i+1)%topology],
                    };
                    geo_edge_map_add_edge(&edge_map, hash);
                }
            }
        }

        // extract deduplicated edges
        geo_edge_map_extract_edges(arena, &edge_map, out_indices, out_count);
    }}
}

void geo_calculate_points(
    Arena* arena, u32 in_point_count,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_count
) {
    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
        b32* occupied = push_array(scratch.arena, b32, in_point_count);

        *out_count = 0;
        for (int in_indice_i = 0; in_indice_i < in_indices_count; in_indice_i++) {
            u32 indice = in_indices[in_indice_i];

            if (!occupied[indice]) {
                occupied[indice] = true;
                (*out_count)++;
            }
        }

        (*out_indices) =  push_array(arena, u32, *out_count);
        for (int point_i = 0, out_indice_i = 0; point_i < in_point_count; point_i++) {
            if (occupied[point_i]) {
                (*out_indices)[out_indice_i] = point_i;
                out_indice_i++;
            }
        }
    }}
}

// @note assume CCW winding order
void geo_calculate_flat_normals(
    vec3_f32* in_p, u64 in_p_stride, u64 in_p_count,
    vec3_f32* out_n, u64 in_n_stride
) {
    vec3_f32* p = in_p;
    vec3_f32* n = out_n;
    for (u32 i = 0; i < in_p_count;) {
        vec3_f32 u = sub_3f32(*p, *OffsetPtr(p, in_p_stride, vec3_f32));
        vec3_f32 v = sub_3f32(*OffsetPtr(p, 2*in_p_stride, vec3_f32), *p);

        vec3_f32 tri_n = normalize_3f32(cross_3f32(v, u)); // CCW

        for (
            int tri_i = 0; tri_i < 3; tri_i++, i++,
            p=OffsetPtr(p, in_p_stride, vec3_f32), n=OffsetPtr(n, in_n_stride, vec3_f32)
        ) {
            *n = tri_n;
        }
    }
}

// @note assumes normals are zeroed
// @note assume CCW winding order
void geo_calculate_smooth_normals(
    GEO_Topology topology,
    vec3_f32* in_p, u64 in_p_stride, u64 in_p_count, u32* in_indices, u32 in_indices_count,
    vec3_f32* out_n, u64 in_n_stride
) {
    Assert(topology >= GEO_Topology_Triangle);
    int tris = topology - 2;
    for (u32 s = 0; s < in_indices_count; s+=topology) {
        for (int tri_i = 0; tri_i < tris; tri_i++) {
            u32 i1 = in_indices[s], i2 = in_indices[s+tri_i+1], i3 = in_indices[s+tri_i+2];

            vec3_f32* p1 = OffsetPtr(in_p, i1*in_p_stride, vec3_f32);
            vec3_f32* p2 = OffsetPtr(in_p, i2*in_p_stride, vec3_f32);
            vec3_f32* p3 = OffsetPtr(in_p, i3*in_p_stride, vec3_f32);
    
            vec3_f32 u = normalize_3f32(sub_3f32(*p2, *p1));
            vec3_f32 v = normalize_3f32(sub_3f32(*p3, *p1));
            vec3_f32 x = normalize_3f32(sub_3f32(*p3, *p2));
    
            vec3_f32 tri_n = cross_3f32(u, v); // ccw
    
            // weight normals by angle
            // @todo check validity
            f32 a1 = acos_f32(Clamp(dot_3f32(u, v), -1.f, 1.f));
            f32 a3 = acos_f32(Clamp(dot_3f32(v, x), -1.f, 1.f));
            f32 a2 = PI - a1 - a2;
    
            vec3_f32* n1 = OffsetPtr(out_n, i1*in_n_stride, vec3_f32);
            vec3_f32* n2 = OffsetPtr(out_n, i2*in_n_stride, vec3_f32);
            vec3_f32* n3 = OffsetPtr(out_n, i3*in_n_stride, vec3_f32);
    
            *n1 = add_3f32(*n1, mul_3f32(tri_n, a1));
            *n2 = add_3f32(*n2, mul_3f32(tri_n, a2));
            *n3 = add_3f32(*n3, mul_3f32(tri_n, a3));
        }
    }

    // renormalized
    for (u32 i = 0; i < in_p_count; i++) {
        vec3_f32* ni = OffsetPtr(out_n, i*in_n_stride, vec3_f32);
        *ni = normalize_3f32(*ni);
    }
}

void geo_triangulate(
    Arena* arena, GEO_Topology topology, b32 cw,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_indices_count
) {
    int tris = topology - 2;
    *out_indices_count = (in_indices_count/topology)*tris*GEO_Topology_Triangle;
    *out_indices = push_array(arena, u32, *out_indices_count);

    u32 i = 0;
    for (u32 s = 0; s < in_indices_count; s+=topology) {
        for (int tri_i = 0; tri_i < tris; tri_i++) {
            u32 i1 = in_indices[s], i2 = in_indices[s+tri_i+1], i3 = in_indices[s+tri_i+2];

            if (cw) {
                (*out_indices)[i++] = i1;
                (*out_indices)[i++] = i3;
                (*out_indices)[i++] = i2;
            } else {
                (*out_indices)[i++] = i1;
                (*out_indices)[i++] = i2;
                (*out_indices)[i++] = i3;
            }
        }
    }
}