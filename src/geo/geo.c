
// edge map
internal b32 geo_hash_is_eq(GEO_EdgeMapHash a, GEO_EdgeMapHash b) {
    return (
        ((a.i == b.i) && (a.j == b.j)) ||
        ((a.i == b.j) && (a.j == b.i))
    );
}

internal GEO_EdgeMap geo_make_edge_map(Arena* arena, u64 slots_count) {
    return (GEO_EdgeMap) {
        .arena = arena,
        .slots = push_array(arena, GEO_EdgeMapNode*, slots_count),
        .slots_count = slots_count,
        .edge_count = 0,
    };
}

internal GEO_EdgeMapNode* geo_edge_map_add_edge(GEO_EdgeMap* map, GEO_EdgeMapHash hash) {
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

internal void geo_edge_map_extract_edges(Arena* arena, GEO_EdgeMap* map, u32** edge_indices, u32* edge_indices_count) {
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
internal GEO_NeighborMap geo_make_neighbor_map(Arena* arena, u32 points_count) {
    return (GEO_NeighborMap) {
        .arena = arena,
        .points = push_array(arena, GEO_NeighborMapNode*, points_count),
        .points_count = points_count,
    };
}

internal void geo_neighbor_map_add_directed_edge(GEO_NeighborMap* map, u32 src, u32 dst) {
    GEO_NeighborMapNode* n = push_array(map->arena, GEO_NeighborMapNode, 1);
    n->v = dst;
    stack_push(map->points[src], n);
    map->directed_edge_count++;
}

internal void geo_neighbor_map_add_edge(GEO_NeighborMap* map, u32 i, u32 j) {
    geo_neighbor_map_add_directed_edge(map, i, j);
    geo_neighbor_map_add_directed_edge(map, j, i);
}

internal void geo_neighbor_map_add_indices(GEO_NeighborMap* map, GEO_Topology topology, const GEO_Connected connected, u32* indices, u32 indices_count) {
    Assert(connected == GEO_Connected_Strongly || connected == GEO_Connected_Ring);
    Assert(!(connected == GEO_Connected_Ring && topology <= GEO_Topology_Line));

    switch (connected) {
        case GEO_Connected_Strongly:{
            for GEO_EachEdge_Strongly_Open(indice_i, indice_j, u32, indices, indices_count, topology) {
                geo_neighbor_map_add_edge(map, indice_i, indice_j);
            } GEO_EachEdge_Strongly_Close;
        }break;
        case GEO_Connected_Ring:{
            for GEO_EachEdge_Ring_Open(indice_i, indice_j, u32, indices, indices_count, topology) {
                geo_neighbor_map_add_edge(map, indice_i, indice_j);
            } GEO_EachEdge_Ring_Close;
        }break;
    }
}

// processing
internal void geo_calculate_edges(
    Arena* arena, u32 approx_edges, GEO_Topology topology, const GEO_Connected connected,
    u32* in_indices, u32 in_indices_count,
    u32** out_indices, u32* out_count
) {
    Assert(connected == GEO_Connected_Strongly || connected == GEO_Connected_Ring);
    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
        GEO_EdgeMap edge_map = geo_make_edge_map(scratch.arena, approx_edges);

        switch (connected) {
            case GEO_Connected_Strongly: {
                for GEO_EachEdge_Strongly_Open(indice_i, indice_j, u32, in_indices, in_indices_count, topology) {
                    GEO_EdgeMapHash hash = {.i = indice_i, .j = indice_j,};
                    geo_edge_map_add_edge(&edge_map, hash);
                } GEO_EachEdge_Strongly_Close;
            }break;
            case GEO_Connected_Ring: {
                for GEO_EachEdge_Ring_Open(indice_i, indice_j, u32, in_indices, in_indices_count, topology) {
                    GEO_EdgeMapHash hash = {.i = indice_i, .j = indice_j,};
                    geo_edge_map_add_edge(&edge_map, hash);
                } GEO_EachEdge_Ring_Close;
            }break;
        }

        // extract deduplicated edges
        geo_edge_map_extract_edges(arena, &edge_map, out_indices, out_count);
    }}
}

internal void geo_calculate_points(
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
internal void geo_calculate_flat_normals(
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
internal void geo_calculate_smooth_normals(
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
            f32 a2 = PI - a1 - a3;
    
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

internal void geo_triangulate(
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
internal void geo_triangulate_quad(
    Arena* arena, b32 cw,
    u32 x, u32 y,
    u32** out_indices, u32* out_indices_count
) {
    Assert(x > 0 && y > 0);
    *out_indices_count = 2*(x-1)*(y-1)*GEO_Topology_Triangle;
    *out_indices = push_array(arena, u32, *out_indices_count);

    u32 i = 0;
    for EachIndex(xi, x-1) {
        for EachIndex(yi, y-1) {
            u32 i0 = y*(xi  ) + yi;
            u32 i1 = y*(xi+1) + yi;
            u32 i2 = y*(xi+1) + yi+1;
            u32 i3 = y*(xi  ) + yi+1;

            if (cw) {
                (*out_indices)[i++] = i0;
                (*out_indices)[i++] = i3;
                (*out_indices)[i++] = i2;

                (*out_indices)[i++] = i2;
                (*out_indices)[i++] = i1;
                (*out_indices)[i++] = i0;
            } else {
                (*out_indices)[i++] = i0;
                (*out_indices)[i++] = i1;
                (*out_indices)[i++] = i2;

                (*out_indices)[i++] = i2;
                (*out_indices)[i++] = i3;
                (*out_indices)[i++] = i0;
            }
        }
    }
}

// clip polygon to lie inside positive halfspace of plane
internal GEO_Polygon geo_clip_polygon_against_plane(GEO_Polygon* in_to_clip, vec3_f32 in_origin, vec3_f32 in_normal) {
    Assert(in_to_clip->topology >= GEO_Topology_Edge);
    GEO_Polygon clipped = zero_struct;

    vec3_f32 prev_v = in_to_clip->data[in_to_clip->topology - 1];
    f32 prev_num = dot_3f32(sub_3f32(in_origin, prev_v), in_normal);
    b32 prev_inside = prev_num < 0.f;
    for EachIndex(i, in_to_clip->topology) {
        vec3_f32 cur_v = in_to_clip->data[i];
        f32 cur_num = dot_3f32(sub_3f32(in_origin, cur_v), in_normal);
        b32 cur_inside = cur_num < 0.f;

        // transition from clipped to unclipped side of plane, add projected point on plane
        if (cur_inside != prev_inside) {
            // intersection between segment and planar equations
            // S: X = prev_v + t*d
            // P: (X - in_origin) . in_normal = 0
            vec3_f32 d = sub_3f32(cur_v, prev_v);
            f32 denom = dot_3f32(d, in_normal);
            if (denom != 0.f) {
                f32 t = prev_num / denom;
                clipped.data[clipped.topology] = add_3f32(prev_v, mul_3f32(d, t));
                clipped.topology = IntToEnum(GEO_Topology, clipped.topology+1);
            } else {
                cur_inside = prev_inside; // segment is parallel, no transition could have occurred
            }
        }

        if (cur_inside) {
            clipped.data[clipped.topology] = cur_v;
            clipped.topology = IntToEnum(GEO_Topology, clipped.topology+1);
        }

        prev_v = cur_v; prev_num = cur_num; prev_inside = cur_inside;
    }

    Assert(clipped.topology < GEO_MAX_CLIPPED_TOPOLOGY);
    return clipped;
}

// clip a polygon against each of edge planes of another polygon, tangent to the normal
internal GEO_Polygon geo_clip_polygon_against_polygon(GEO_Polygon* in_to_clip, GEO_Polygon* in_clip, vec3_f32 in_normal) {
    GEO_Polygon ping_pong[2] = zero_struct;
    u32 src_idx = 0;
    ping_pong[src_idx] = *in_to_clip;

    for GEO_EachEdge_Ring_Open(edge_start, edge_end, vec3_f32, in_clip->data, in_clip->topology, in_clip->topology) {
        vec3_f32 edge_normal = cross_3f32(in_normal, sub_3f32(edge_end, edge_start));

        GEO_Polygon* src = &ping_pong[src_idx];
        ping_pong[src_idx^1] = geo_clip_polygon_against_plane(src, edge_start, edge_normal);
        src_idx = src_idx^1;

        if (ping_pong[src_idx].topology < GEO_Topology_Triangle) {
            GEO_Polygon empty = zero_struct;
            return empty;
        }
    } GEO_EachEdge_Ring_Close;

    Assert(ping_pong[src_idx].topology < GEO_MAX_CLIPPED_TOPOLOGY);
    return ping_pong[src_idx];
}

internal GEO_Polygon geo_clip_polygon_against_edge(GEO_Polygon* in_to_clip, vec3_f32 in_clip_start, vec3_f32 in_clip_end, vec3_f32 in_normal) {
    Assert(in_to_clip->topology >= GEO_Topology_Triangle);
    GEO_Polygon clipped = zero_struct;

    vec3_f32 clipping_edge_dir = sub_3f32(in_clip_start, in_clip_end);
    vec3_f32 clipping_edge_normal = cross_3f32(clipping_edge_dir, in_normal);

    // project edge onto the polygon we are clipping
    vec3_f32 polygon_normal = cross_3f32(sub_3f32(in_to_clip->data[2], in_to_clip->data[0]), sub_3f32(in_to_clip->data[1], in_to_clip->data[0]));
    f32 polygon_normal_l2 = length2_3f32(polygon_normal);
    vec3_f32 proj_clipping_edge_start = sub_3f32(in_clip_start,     mul_3f32(polygon_normal, dot_3f32(sub_3f32(in_clip_start, in_to_clip->data[0]), polygon_normal)/polygon_normal_l2));
    vec3_f32 proj_clipping_edge_dir   = sub_3f32(clipping_edge_dir, mul_3f32(polygon_normal, dot_3f32(clipping_edge_dir,                            polygon_normal)/polygon_normal_l2));
    f32 proj_clipping_edge_dir_l2 = length2_3f32(proj_clipping_edge_dir);    

    vec3_f32 prev_v = in_to_clip->data[in_to_clip->topology - 1];
    f32 prev_num = dot_3f32(sub_3f32(in_clip_start, prev_v), in_normal);
    b32 prev_inside = prev_num < 0.f;
    for EachIndex(i, in_to_clip->topology) {
        vec3_f32 cur_v = in_to_clip->data[i];
        f32 cur_num = dot_3f32(sub_3f32(in_clip_start, cur_v), in_normal);
        b32 cur_inside = cur_num < 0.f;

        // transition from clipped to unclipped side of plane
        if (cur_inside != prev_inside) {
            // intersection between polygon edge segment and edge's planar equations
            // S: X = prev_v + t*d
            // P: (X - in_clip_start) . clipping_edge_normal = 0
            vec3_f32 d = sub_3f32(cur_v, prev_v);
            f32 denom = dot_3f32(d, clipping_edge_normal);
            // @note if polygon face is perpendicular to clipping edge, dont clip
            vec3_f32 x = (denom == 0.f) ? prev_v : add_3f32(prev_v, mul_3f32(d, prev_num / denom));

            // now check which side of the edge x lies on
            f32 proj = dot_3f32(sub_3f32(x, proj_clipping_edge_start), proj_clipping_edge_dir);
            if (proj < 0.f)
                clipped.data[clipped.topology] = prev_v;
            else if (proj > proj_clipping_edge_dir_l2)
                clipped.data[clipped.topology] = cur_v;
            else
                clipped.data[clipped.topology] = x;
            clipped.topology = IntToEnum(GEO_Topology, clipped.topology+1);
        }

        prev_v = cur_v; prev_num = cur_num; prev_inside = cur_inside;
    }

    Assert(clipped.topology < GEO_MAX_CLIPPED_TOPOLOGY);
    return clipped;
}