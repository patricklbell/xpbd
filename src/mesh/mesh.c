// helpers
static b32 ms_hash_is_eq(MS_VertexMapHash a, MS_VertexMapHash b) {
    return (a.normal == b.normal && a.position == b.position && a.uv == b.uv);
}

static MS_VertexMap make_vertex_map(Arena* arena, u64 slots_count) {
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
        return (MS_MeshResult) { .error = ntstr8_lit_init("Failed to open file") };
    }

    MS_Mesh mesh;
    {
        Temp scratch = scratch_begin_a(arena);

        // determine buffer sizes
        u32 positions_count = 0, normals_count = 0, uvs_count = 0, indices_count = 0;
        while (!os_is_eof(file)) {
            Temp temp = temp_begin(scratch.arena);
            NTString8 line = os_read_line(scratch.arena, file);
    
            if (ntstr8_begins_with(line, "v ")) {
                positions_count++;
            } else if (ntstr8_begins_with(line, "vn ")) {
                normals_count++;
            } else if (ntstr8_begins_with(line, "vt ")) {
                uvs_count++;
            } else if (ntstr8_begins_with(line, "f ")) {
                indices_count+=3;
            }
            temp_end(temp);
        }
        Assert(positions_count < ((u32)-1) && normals_count < ((u32)-1) && uvs_count < ((u32)-1));
        
        mesh.indices_count = indices_count;
        mesh.indices = push_array(arena, u32, mesh.indices_count);
        
        {
            // map for deduplicating vertex data
            MS_VertexMap vertex_map = make_vertex_map(scratch.arena, Max(Max(positions_count, normals_count), uvs_count));
    
            // allocate buffers
            vec3_f32* positions = push_array(scratch.arena, vec3_f32, positions_count);
            vec3_f32* normals   = push_array(scratch.arena, vec3_f32, normals_count);
            vec2_f32* uvs       = push_array(scratch.arena, vec2_f32, uvs_count);
            
            u32 off_positions = 0, off_normals = 0, off_uvs = 0, off_indices = 0;
            os_set_file_offset(file, 0);
            while (!os_is_eof(file)) {
                Temp temp = temp_begin(scratch.arena);
                NTString8 line = os_read_line(scratch.arena, file);

                if (ntstr8_begins_with(line, "f ")) {
                    // @todo other polygons and formats
                    static const int FACE_VERTICES_COUNT = 3;

                    // 1-based index
                    u32 p[FACE_VERTICES_COUNT], uv[FACE_VERTICES_COUNT], n[FACE_VERTICES_COUNT];
                    sscanf(line.cstr, "f %u/%u/%u %u/%u/%u %u/%u/%u", 
                        &p[0], &uv[0], &n[0],
                        &p[1], &uv[1], &n[1],
                        &p[2], &uv[2], &n[2]
                    );
                    temp_end(temp); // @note needs to end temp to persist allocation
    
                    // deduplicate indices so that vertex data is shared and store indice
                    for EachIndex(i, FACE_VERTICES_COUNT) {
                        MS_VertexMapHash hash = {.position = p[i] - 1, .normal = n[i] - 1, .uv = uv[i] - 1};
                        u32 indice = ms_add_to_vertex_map(scratch.arena, &vertex_map, hash);
                        mesh.indices[off_indices++] = indice;
                    }

                    continue;
                }
        
                // @todo replace sscanf
                if (ntstr8_begins_with(line, "v ")) {
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

                temp_end(temp);
            }
    
            mesh.vertices_count = vertex_map.vertices_count;
            mesh.vertices = ms_vertex_map_data(arena, &vertex_map, positions, normals, uvs);
        }
    
        scratch_end(scratch);
    }
    
    return (MS_MeshResult) { .v = mesh, .error = ntstr8_lit_init("") };;
}