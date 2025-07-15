// helpers
static b32 ms_hash_is_eq(MS_VertexMapHash a, MS_VertexMapHash b) {
    return (a.normal == b.normal && a.position == b.position && a.uv == b.uv);
}

static MS_VertexMap make_vertex_map(Arena* arena, u64 num_slots) {
    MS_VertexMap result;
    result.num_slots = num_slots;
    result.slots = push_array(arena, MS_VertexMapNode*, result.num_slots);
    result.num_vertices = 0;
    return result;
}

static u32 ms_add_to_vertex_map(Arena* arena, MS_VertexMap* map, MS_VertexMapHash hash) {
    u64 slot = hash_u64((u8*)&hash, sizeof(hash)) % map->num_slots;
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
    vn->index = map->num_vertices++;
    stack_push(map->slots[slot], vn);
    return vn->index;
}

static R_VertexLayout* ms_vertex_map_data(Arena* arena, MS_VertexMap* map, vec3_f32* positions, vec3_f32* normals, vec2_f32* uvs) {
    R_VertexLayout* result = push_array(arena, R_VertexLayout, map->num_vertices);
    for EachIndex(slot, map->num_slots) {
        for EachList(n_vertex, MS_VertexMapNode, map->slots[slot]) {
            result[n_vertex->index].position  = positions [n_vertex->hash.position];
            result[n_vertex->index].normal    = normals   [n_vertex->hash.normal];
            result[n_vertex->index].uv        = uvs       [n_vertex->hash.uv];
        }
    }
    return result;
}

// loaders
MS_MeshResult ms_load_obj(Arena* arena, NTString8 path) {
    OS_Handle file = os_open_readonly_file(path);

    if (os_is_handle_zero(file)) {
        return (MS_MeshResult) { .error = str_8("Failed to open file") };
    }

    MS_Mesh mesh;
    {
        Temp scratch = scratch_begin_a(arena);

        // determine buffer sizes
        u32 num_positions = 0, num_normals = 0, num_uvs = 0, num_indices = 0;
        while (!os_is_eof(file)) {
            Temp temp = temp_begin(scratch.arena);
            NTString8 line = os_read_line(scratch.arena, file);
    
            if (str_begins_with(line, "v ")) {
                num_positions++;
            } else if (str_begins_with(line, "vn ")) {
                num_normals++;
            } else if (str_begins_with(line, "vt ")) {
                num_uvs++;
            } else if (str_begins_with(line, "f ")) {
                num_indices+=3;
            }
            temp_end(temp);
        }
        Assert(num_positions < ((u32)-1) && num_normals < ((u32)-1) && num_uvs < ((u32)-1));
        
        mesh.num_indices = num_indices;
        mesh.indices = push_array(arena, u32, mesh.num_indices);
        
        {
            // map for deduplicating vertex data
            MS_VertexMap vertex_map = make_vertex_map(scratch.arena, Max(Max(num_positions, num_normals), num_uvs));
    
            // allocate buffers
            vec3_f32* positions = push_array(scratch.arena, vec3_f32, num_positions);
            vec3_f32* normals   = push_array(scratch.arena, vec3_f32, num_normals);
            vec2_f32* uvs       = push_array(scratch.arena, vec2_f32, num_uvs);
            
            u32 off_positions = 0, off_normals = 0, off_uvs = 0, off_indices = 0;
            os_set_file_offset(file, 0);
            while (!os_is_eof(file)) {
                Temp temp = temp_begin(scratch.arena);
                NTString8 line = os_read_line(scratch.arena, file);

                if (str_begins_with(line, "f ")) {
                    // @todo other polygons and formats
                    static const int NUM_INDICES = 3;

                    // 1-based index
                    u32 p[NUM_INDICES], uv[NUM_INDICES], n[NUM_INDICES];
                    sscanf(line.data, "f %u/%u/%u %u/%u/%u %u/%u/%u", 
                        &p[0], &uv[0], &n[0],
                        &p[1], &uv[1], &n[1],
                        &p[2], &uv[2], &n[2]
                    );
                    temp_end(temp); // @note needs to end temp to persist allocation
    
                    // deduplicate indices so that vertex data is shared and store indice
                    for EachIndex(i, NUM_INDICES) {
                        MS_VertexMapHash hash = {.position = p[i] - 1, .normal = n[i] - 1, .uv = uv[i] - 1};
                        u32 indice = ms_add_to_vertex_map(scratch.arena, &vertex_map, hash);
                        mesh.indices[off_indices++] = indice;
                    }

                    continue;
                }
        
                // @todo replace sscanf
                if (str_begins_with(line, "v ")) {
                    sscanf(line.data, "v %f %f %f", 
                        &positions[off_positions].x,
                        &positions[off_positions].y,
                        &positions[off_positions].z
                    );
                    off_positions++;
                } else if (str_begins_with(line, "vn ")) {
                    sscanf(line.data, "vn %f %f %f", 
                        &normals[off_normals].x,
                        &normals[off_normals].y,
                        &normals[off_normals].z
                    );
                    off_normals++;
                } else if (str_begins_with(line, "vt ")) {
                    sscanf(line.data, "vt %f %f", 
                        &uvs[off_uvs].x,
                        &uvs[off_uvs].y
                    );
                    off_uvs++;
                }

                temp_end(temp);
            }
    
            mesh.num_vertices = vertex_map.num_vertices;
            mesh.vertices = ms_vertex_map_data(arena, &vertex_map, positions, normals, uvs);
        }
    
        scratch_end(scratch);
    }
    
    return (MS_MeshResult) { .v = mesh, .error = str_8("") };;
}