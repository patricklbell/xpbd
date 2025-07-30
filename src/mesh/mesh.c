// helpers
static b32 ms_hash_is_eq(MS_VertexMapHash a, MS_VertexMapHash b) {
    for EachElement(i, a.indices) {
        if (a.indices[i] != b.indices[i]) {
            return 0;
        }
    }
    return 1;
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
    vn->index = map->vertices_count;
    map->vertices_count++;
    stack_push(map->slots[slot], vn);
    return vn->index;
}

static void* ms_vertex_map_data(Arena* arena, MS_VertexMap* map, vec3_f32* positions, vec3_f32* normals, vec2_f32* uvs, R_VertexFlag flags) {
    void* result = arena_push(arena, r_vertex_size(flags)*map->vertices_count, r_vertex_align(flags));
    for EachIndex(slot, map->slots_count) {
        for EachList(vn, MS_VertexMapNode, map->slots[slot]) {
            for EachElement(indice_i, vn->hash.indices) {
                R_VertexFlag flag = (1 << indice_i);
                u32 index = vn->hash.indices[indice_i];

                // @note change from 1 -> 0 indexing
                if (index == 0 || !(flags & flag)) {
                    continue;
                }
                index--;
                
                u64 offset = r_vertex_i_offset(flags, flag, vn->index);
                switch (flag) {
                    case R_VertexFlag_P: {
                        *(R_VertexType_P*)(result + offset) = positions[index];
                    }break;
                    case R_VertexFlag_N: {
                        *(R_VertexType_N*)(result + offset) = normals[index];
                    }break;
                    case R_VertexFlag_T: {
                        *(R_VertexType_T*)(result + offset) = uvs[index];
                    }break;
                    default: NotImplemented;
                }
            }
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

    // solve settings
    int vertices_per_face;
    if (settings.topology == R_VertexTopology_ZERO) {
        settings.topology = R_VertexTopology_Triangles;
    }
    if (settings.flags == R_VertexFlag_ZERO) {
        settings.flags = R_VertexFlag_P | R_VertexFlag_T | R_VertexFlag_N;
    }
    switch (settings.topology) {
        case R_VertexTopology_Lines:         vertices_per_face = 2; break;
        case R_VertexTopology_LineStrip:     NotImplemented;        break;
        case R_VertexTopology_Triangles:     vertices_per_face = 3; break;
        case R_VertexTopology_TriangleStrip: NotImplemented;        break;
        default:                             Assert(0);
    }

    // load mesh
    MS_Mesh mesh;
    mesh.flags = settings.flags;
    mesh.topology = settings.topology;

    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) { 

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
                indices_count+=vertices_per_face;
            }
        }
        
        // map for deduplicating vertex data
        MS_VertexMap vertex_map = ms_make_vertex_map(scratch.arena, Max(Max(positions_count, normals_count), uvs_count));
        
        // allocate buffers
        mesh.indices_count = indices_count;
        mesh.indices = push_array(arena, u32, mesh.indices_count);

        vec3_f32* positions = push_array(scratch.arena, vec3_f32, positions_count);
        vec3_f32* normals   = push_array(scratch.arena, vec3_f32, normals_count);
        vec2_f32* uvs       = push_array(scratch.arena, vec2_f32, uvs_count);

        // @note 1-based index of each attribute
        u32* p = push_array(scratch.arena, u32, vertices_per_face);
        u32* t = push_array(scratch.arena, u32, vertices_per_face);
        u32* n = push_array(scratch.arena, u32, vertices_per_face);
        
        u32 off_positions = 0, off_normals = 0, off_uvs = 0, off_indices = 0;
        os_set_file_offset(file, 0);
        while (!os_is_eof(file)) {
            os_read_line_to_buffer(file, &line);

            // @todo replace sscanf
            if (ntstr8_begins_with(line, "f ")) {
                for (int line_offset = 1, vertex_index = 0; vertex_index < vertices_per_face; vertex_index++) {
                    int consumed = 0, success = 0;

                    p[vertex_index] = 0;
                    t[vertex_index] = 0;
                    n[vertex_index] = 0;
                    switch (settings.flags) {
                        case R_VertexFlag_P: {
                            success = sscanf(&line.cstr[line_offset], " %u%n", 
                                &p[vertex_index], &consumed
                            ) == 1;
                        }break;
                        case R_VertexFlag_PT: {
                            success = sscanf(&line.cstr[line_offset], " %u/%u%n", 
                                &p[vertex_index], &t[vertex_index], &consumed
                            ) == 2;
                        }break;
                        case R_VertexFlag_PN: {
                            success = sscanf(&line.cstr[line_offset], " %u//%u%n", 
                                &p[vertex_index], &n[vertex_index], &consumed
                            ) == 2;
                        }break;
                        case R_VertexFlag_PNT: {
                            success = sscanf(&line.cstr[line_offset], " %u/%u/%u%n", 
                                &p[vertex_index], &t[vertex_index], &n[vertex_index], &consumed
                            ) == 3;
                        }break;
                        default: NotImplemented;
                    }

                    line_offset += consumed;
                    if (!success) {
                        // @todo logging
                        fprintf(stderr, "Failed to parse face in obj: %s", line.cstr);
                        break;
                    }
                }

                // deduplicate indices so that vertex data is shared and store indice
                for EachIndex(i, vertices_per_face) {
                    // @note order has to match order of R_VertexFlag
                    MS_VertexMapHash hash = {.indices = {p[i], n[i], t[i]}};
                    u32 indice = ms_add_to_vertex_map(scratch.arena, &vertex_map, hash);
                    mesh.indices[off_indices] = indice;
                    off_indices++;
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
        mesh.vertices = ms_vertex_map_data(arena, &vertex_map, positions, normals, uvs, settings.flags);
    }}
    
    os_close_file(file);
    return (MS_LoadResult) { .v = mesh, .error = ntstr8_lit_init("") };;
}

// helpers
// @note assumes CCW winding order
void ms_calculate_flat_normals(MS_Mesh* mesh, R_VertexTopology topology) {
    AssertAlways(topology == R_VertexTopology_Triangles); // @todo
    Assert(mesh->vertices_count == mesh->indices_count);
    
    void* p = mesh->vertices + r_vertex_offset(mesh->flags, R_VertexFlag_P);
    void* n = mesh->vertices + r_vertex_offset(mesh->flags, R_VertexFlag_N);
    u64 p_stride = r_vertex_stride(mesh->flags, R_VertexFlag_P);
    u64 n_stride = r_vertex_stride(mesh->flags, R_VertexFlag_N);

    for (u32 i = 0; i < mesh->vertices_count;) {
        vec3_f32 u = sub_3f32(*(R_VertexType_P*)(p),              *(R_VertexType_P*)(p + p_stride));
        vec3_f32 v = sub_3f32(*(R_VertexType_P*)(p + 2*p_stride), *(R_VertexType_P*)(p));

        vec3_f32 tri_n = normalize_3f32(cross_3f32(v, u));

        for (int tri_i = 0; tri_i < 3; tri_i++, i++, p+=p_stride, n+=n_stride) {
            (*(R_VertexType_N*)n) = tri_n;
        }
    }
}