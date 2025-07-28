// helpers
static b32 ms_hash_is_eq(MS_VertexMapHash a, MS_VertexMapHash b) {
    return (a.normal == b.normal && a.position == b.position && a.uv == b.uv);
}

static MS_VertexMap ms_make_vertex_map(Arena* arena, u64 slots_count) {
    MS_VertexMap result;
    result.slots_count = slots_count;
    result.slots = push_array(arena, MS_VertexMapNode*, result.slots_count);
    result.vertices_count = 0;
    return result;
}

static u32 ms_add_to_vertex_map(Arena* arena, MS_VertexMap* map, MS_VertexMapHash hash) {
    u64 slot = hash_u64((u8*)&hash, sizeof(hash)) % map->slots_count;
    MS_VertexMapNode* list = map->slots[slot];

    // try to find matching vertex
    for EachList(n_vertex, MS_VertexMapNode, list) {
        if (ms_hash_is_eq(n_vertex->hash, hash)) {
            return n_vertex->index;
        }
    }

    // otherwise, create one
    MS_VertexMapNode* vn = push_array(arena, MS_VertexMapNode, 1);
    vn->hash = hash;
    vn->index = map->vertices_count++;
    stack_push(map->slots[slot], vn);
    return vn->index;
}

static R_VertexLayout* ms_vertex_map_data(Arena* arena, MS_VertexMap* map, vec3_f32* positions, vec3_f32* normals, vec2_f32* uvs) {
    R_VertexLayout* result = push_array(arena, R_VertexLayout, map->vertices_count);
    for EachIndex(slot, map->slots_count) {
        for EachList(n_vertex, MS_VertexMapNode, map->slots[slot]) {
            if (n_vertex->hash.position != (u32)-1)
                result[n_vertex->index].position  = positions [n_vertex->hash.position];
            if (n_vertex->hash.normal != (u32)-1)
                result[n_vertex->index].normal    = normals   [n_vertex->hash.normal];
            if (n_vertex->hash.uv != (u32)-1)
                result[n_vertex->index].uv        = uvs       [n_vertex->hash.uv];
        }
    }
    return result;
}

// loaders
MS_LoadResult ms_load_obj(Arena* arena, NTString8 path, MS_LoadSettings settings) {
    OS_Handle file = os_open_readonly_file(path);

    if (os_is_handle_zero(file)) {
        return (MS_LoadResult) { .error = ntstr8_lit_init("Failed to open file") };
    }

    if (settings.topology == MS_Topology_Default) {
        settings.topology = MS_Topology_Triangle;
    }
    if (settings.vertex_flags == MS_VertexFlags_Default) {
        settings.vertex_flags = MS_VertexFlags_PTN;
    }

    MS_Mesh mesh;
    {
        Temp scratch = scratch_begin_a(arena);

        NTString8 line;
        line.data = push_array(scratch.arena, u8, OS_DEFAULT_MAX_LINE_LENGTH);

        // determine buffer sizes
        u32 positions_count = 0, normals_count = 0, uvs_count = 0, indices_count = 0;
        while (!os_is_eof(file)) {
            os_read_line_to_buffer(file, &line);
    
            if (ntstr8_begins_with(line, "v ")) {
                positions_count++;
            } else if (ntstr8_begins_with(line, "vn ")) {
                normals_count++;
            } else if (ntstr8_begins_with(line, "vt ")) {
                uvs_count++;
            } else if (ntstr8_begins_with(line, "f ")) {
                indices_count+=settings.topology;
            }
        }
        Assert(positions_count < ((u32)-1) && normals_count < ((u32)-1) && uvs_count < ((u32)-1));
        
        mesh.indices_count = indices_count;
        mesh.indices = push_array(arena, u32, mesh.indices_count);
        
        {
            // map for deduplicating vertex data
            MS_VertexMap vertex_map = ms_make_vertex_map(scratch.arena, Max(Max(positions_count, normals_count), uvs_count));
    
            // allocate buffers
            vec3_f32* positions = push_array(scratch.arena, vec3_f32, positions_count);
            vec3_f32* normals   = push_array(scratch.arena, vec3_f32, normals_count);
            vec2_f32* uvs       = push_array(scratch.arena, vec2_f32, uvs_count);
            
            u32 off_positions = 0, off_normals = 0, off_uvs = 0, off_indices = 0;
            os_set_file_offset(file, 0);
            while (!os_is_eof(file)) {
                os_read_line_to_buffer(file, &line);

                // @todo replace sscanf
                if (ntstr8_begins_with(line, "f ")) {
                    // 1-based index
                    u32 p[MS_Topology_COUNT] = zero_struct;
                    u32 t[MS_Topology_COUNT] = zero_struct;
                    u32 n[MS_Topology_COUNT] = zero_struct;

                    for (int offset = 1, vertex_index = 0; vertex_index < settings.topology; vertex_index++) {
                        int consumed = 0, success = 0;

                        switch (settings.vertex_flags) {
                            case MS_VertexFlags_P: {
                                success = sscanf(&line.cstr[offset], " %u%n", 
                                    &p[vertex_index], &consumed
                                ) == 1;
                            }break;
                            case MS_VertexFlags_PT: {
                                success = sscanf(&line.cstr[offset], " %u/%u%n", 
                                    &p[vertex_index], &t[vertex_index], &consumed
                                ) == 2;
                            }break;
                            case MS_VertexFlags_PN: {
                                success = sscanf(&line.cstr[offset], " %u//%u%n", 
                                    &p[vertex_index], &n[vertex_index], &consumed
                                ) == 2;
                            }break;
                            case MS_VertexFlags_PTN: {
                                success = sscanf(&line.cstr[offset], " %u/%u/%u%n", 
                                    &p[vertex_index], &t[vertex_index], &n[vertex_index], &consumed
                                ) == 3;
                            }break;
                            default: Assert(0);
                        }

                        offset += consumed;
                        if (!success) {
                            // @todo logging
                            fprintf(stderr, "Failed to process face in obj: %s", line.cstr);
                            break;
                        }
                    }
    
                    // deduplicate indices so that vertex data is shared and store indice
                    for EachIndex(i, settings.topology) {
                        MS_VertexMapHash hash = {.position = p[i] - 1, .normal = n[i] - 1, .uv = t[i] - 1};
                        u32 indice = ms_add_to_vertex_map(scratch.arena, &vertex_map, hash);
                        mesh.indices[off_indices++] = indice;
                    }
                } else if (ntstr8_begins_with(line, "v ")) {
                    sscanf(line.cstr, "v %f %f %f", 
                        &positions[off_positions].x,
                        &positions[off_positions].y,
                        &positions[off_positions].z
                    );
                    off_positions++;
                } else if (ntstr8_begins_with(line, "vn ")) {
                    sscanf(line.cstr, "vn %f %f %f", 
                        &normals[off_normals].x,
                        &normals[off_normals].y,
                        &normals[off_normals].z
                    );
                    off_normals++;
                } else if (ntstr8_begins_with(line, "vt ")) {
                    sscanf(line.cstr, "vt %f %f", 
                        &uvs[off_uvs].x,
                        &uvs[off_uvs].y
                    );
                    off_uvs++;
                }
            }
    
            
            mesh.vertices_count = vertex_map.vertices_count;
            mesh.vertices = ms_vertex_map_data(arena, &vertex_map, positions, normals, uvs);
        }
    
        scratch_end(scratch);
    }
    
    os_close_file(file);
    return (MS_LoadResult) { .v = mesh, .error = ntstr8_lit_init("") };;
}

// helpers
// @note assumes CCW winding order
void ms_calculate_flat_normals(MS_Mesh* mesh, MS_Topology topology) {
    AssertAlways(topology == MS_Topology_Triangle); // @todo
    Assert(mesh->vertices_count == mesh->indices_count);
    
    for (u32 i = 0; i < mesh->vertices_count;) {
        u32 i1 = i + 0;
        u32 i2 = i + 1;
        u32 i3 = i + 2;

        vec3_f32 u = sub_3f32(mesh->vertices[i1].position, mesh->vertices[i2].position);
        vec3_f32 v = sub_3f32(mesh->vertices[i3].position, mesh->vertices[i2].position);

        vec3_f32 n = normalize_3f32(cross_3f32(v, u));

        for (int tri_i = 0; tri_i < MS_Topology_Triangle; tri_i++, i++) {
            mesh->vertices[i].normal = n;
        }
    }
}