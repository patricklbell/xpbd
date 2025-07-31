// helpers
static VTK_PointCount vtk_cell_type_to_point_count(VTK_CellType type) {
    switch (type) {
        case VTK_CellType_Triangle:    return VTK_PointCount_Triangle;
        case VTK_CellType_Tetrahedron: return VTK_PointCount_Tetrahedron;
        default: Assert(0);
    }
}

static void vtk_load_points(OS_Handle file, NTString8 line, vec3_f32* points, u32 points_count) {
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)){
        u32 point_i;
        for (point_i = 0; !os_is_eof(file) && point_i < points_count; point_i++) {
            os_read_line_to_buffer(file, &line);

            int matched = sscanf(line.cstr, "%f %f %f",
                &points[point_i].x,
                &points[point_i].y,
                &points[point_i].z
            );
            Assert(matched == 3);
        }
        Assert(point_i == points_count); // @todo logging
    }}
}

static void vtk_load_cells(OS_Handle file, NTString8 line, u32 cells_count, u32* cells_data, u32 cells_data_count) {
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)){
        u32 cell_i, cell_data_i;
        for (cell_i = 0, cell_data_i = 0; !os_is_eof(file) && cell_i < cells_count; cell_i++) {
            os_read_line_to_buffer(file, &line);
    
            int offset = 0, size, count;
            while (sscanf(&line.cstr[offset], "%u%n", &cells_data[cell_data_i], &size) > 0) {
                cell_data_i++;
                offset+=size;
            }
        }
        Assert(cell_data_i == cells_data_count);
        Assert(cell_i == cells_count); // @todo logging
    }}
}

static void vtk_load_cell_types(OS_Handle file, NTString8 line, VTK_CellType* cells_type, u32 cells_count) {
    {DeferResource(Temp scratch = scratch_begin(NULL, 0), scratch_end(scratch)){
        u32 cell_i;
        for (cell_i = 0; !os_is_eof(file) && cell_i < cells_count; cell_i++) {
            os_read_line_to_buffer(file, &line);

            int matched = sscanf(line.cstr, "%u", &cells_type[cell_i]);
            Assert(matched == 1);
        }
        Assert(cell_i == cells_count); // @todo logging
    }}
}

// edge map
// @todo common hash map with mesh library
static b32 vtk_hash_is_eq(VTK_EdgeMapHash a, VTK_EdgeMapHash b) {
    return (a.i == b.i) && (a.j == b.j);
}

static VTK_EdgeMap vtk_make_edge_map(Arena* arena, u64 slots_count) {
    VTK_EdgeMap result;
    result.slots_count = slots_count;
    result.slots = push_array(arena, VTK_EdgeMapNode*, result.slots_count);
    result.edge_count = 0;
    return result;
}

static void vtk_add_to_edge_map(Arena* arena, VTK_EdgeMap* map, VTK_EdgeMapHash hash) {
    u64 slot = hash_u64((u8*)&hash, sizeof(hash)) % map->slots_count;
    VTK_EdgeMapNode* list = map->slots[slot];

    // try to find matching vertex
    for EachList(n_edge, VTK_EdgeMapNode, list) {
        if (vtk_hash_is_eq(n_edge->hash, hash)) {
            return;
        }
    }

    // otherwise, create one
    VTK_EdgeMapNode* vn = push_array(arena, VTK_EdgeMapNode, 1);
    vn->hash = hash;
    stack_push(map->slots[slot], vn);
    map->edge_count++;
}

static void vtk_extract_edges_from_edge_map(Arena* arena, VTK_EdgeMap* map, u32** edge_indices, u32* edge_indices_count) {
    *edge_indices_count = map->edge_count*2;
    *edge_indices = push_array(arena, u32, *edge_indices_count);

    u32 edge_offset = 0;
    for EachIndex(slot, map->slots_count) {
        for EachList(n_edge, VTK_EdgeMapNode, map->slots[slot]) {
            (*edge_indices)[edge_offset] = n_edge->hash.i; edge_offset++;
            (*edge_indices)[edge_offset] = n_edge->hash.j; edge_offset++;
        }
    }
}

// loader
VTK_LoadResult vtk_load(Arena* arena, NTString8 path, VTK_LoadSettings settings) {
    OS_Handle file = os_open_readonly_file(path);

    if (os_is_handle_zero(file)) {
        return (VTK_LoadResult) { .error = ntstr8_lit_init("Failed to open file") };
    }

    VTK_Data data;

    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
        NTString8 line;
        line.data = push_array(scratch.arena, u8, OS_DEFAULT_MAX_LINE_LENGTH);

        u32 cells_count, cells_data_count, cells_type_count;
        u32* cells_data = NULL;
        VTK_CellType* cells_type = NULL;
        data.points = NULL;

        while (!os_is_eof(file)) {
            os_read_line_to_buffer(file, &line);
    
            if (ntstr8_begins_with(line, "POINTS ")) {
                int matched = sscanf(line.cstr, "POINTS %u", &data.points_count);
                Assert(matched == 1); // @todo logging

                data.points = push_array(arena, vec3_f32, data.points_count);
                vtk_load_points(file, line, data.points, data.points_count);
            } else if (ntstr8_begins_with(line, "CELLS ")) {
                int matched = sscanf(line.cstr, "CELLS %u %u", &cells_count, &cells_data_count);
                Assert(matched == 2); // @todo logging

                cells_data = push_array(scratch.arena, u32, cells_data_count);
                vtk_load_cells(file, line, cells_count, cells_data, cells_data_count);
            } else if (ntstr8_begins_with(line, "CELL_TYPES ")) {
                int matched = sscanf(line.cstr, "CELL_TYPES %u", &cells_type_count);
                Assert(matched == 1); // @todo logging

                cells_type = push_array(scratch.arena, VTK_CellType, cells_type_count);
                vtk_load_cell_types(file, line, cells_type, cells_type_count);
            }
        }
        os_close_file(file);
        Assert(cells_count == cells_type_count);

        if (data.points == NULL || cells_data == NULL || cells_type == NULL) {
            scratch_end(scratch);
            return (VTK_LoadResult) { .error = ntstr8_lit_init("Missing required sections from .vtk") };
        }

        // determine how many of each cell type there is
        data.volume_indices_count = 0;
        data.surface_indices_count = 0;
        for EachIndex(cell_i, cells_count) {
            VTK_CellType type = cells_type[cell_i];
            switch (type) {
                case VTK_CellType_Triangle: {
                    data.surface_indices_count+=vtk_cell_type_to_point_count(type);
                }break;
                case VTK_CellType_Tetrahedron: {
                    data.volume_indices_count+=vtk_cell_type_to_point_count(type);
                }break;
            }
        }

        // fill in data from cells
        data.volume_indices = push_array(arena, u32, data.volume_indices_count);
        data.surface_indices = push_array(arena, u32, data.surface_indices_count);
        u32 data_offset = 0, volume_offset = 0, surface_offset = 0;
        for EachIndex(cell_i, cells_count) {
            VTK_CellType type = cells_type[cell_i];

            VTK_PointCount point_count = cells_data[data_offset];
            data_offset++;

            for (int point_i = 0; point_i < point_count; point_i++, data_offset++) {
                switch (type) {
                    case VTK_CellType_Triangle: {
                        data.surface_indices[surface_offset]=cells_data[data_offset];
                        surface_offset++;
                    }break;
                    case VTK_CellType_Tetrahedron: {
                        data.volume_indices[volume_offset]=cells_data[data_offset];
                        volume_offset++;
                    }break;
                }
            }
        }

        // calculate edges
        if (settings.calculate_surface_edges) {
            {DeferResource(Temp temp = temp_begin(scratch.arena), temp_end(temp)) {
                VTK_EdgeMap surface_edge_map = vtk_make_edge_map(temp.arena, data.surface_indices_count/VTK_PointCount_Triangle);
    
                // add each edge of each triangle
                for (int surface_i = 0; surface_i < data.surface_indices_count; surface_i+=VTK_PointCount_Triangle) {
                    for (int point_i = 0; point_i < VTK_PointCount_Triangle; point_i++) {
                        for (int point_j = point_i+1; point_j < VTK_PointCount_Triangle; point_j++) {
                            vtk_add_to_edge_map(temp.arena, &surface_edge_map, (VTK_EdgeMapHash){
                                .i = data.surface_indices[surface_i + point_i],
                                .j = data.surface_indices[surface_i + point_j],
                            });
                        }
                    }
                }
    
                // extract deduplicated edges
                vtk_extract_edges_from_edge_map(arena, &surface_edge_map, &data.surface_edge_indices, &data.surface_edge_indices_count);
            }}
        }
        if (settings.calculate_volume_edges) {
            {DeferResource(Temp temp = temp_begin(scratch.arena), temp_end(temp)) {
                VTK_EdgeMap volume_edge_map = vtk_make_edge_map(temp.arena, data.volume_indices_count/VTK_PointCount_Tetrahedron);

                // add each edge of each triangle
                for (int volume_i = 0; volume_i < data.volume_indices_count; volume_i+=VTK_PointCount_Tetrahedron) {
                    for (int point_i = 0; point_i < VTK_PointCount_Tetrahedron; point_i++) {
                        for (int point_j = point_i+1; point_j < VTK_PointCount_Tetrahedron; point_j++) {
                            vtk_add_to_edge_map(temp.arena, &volume_edge_map, (VTK_EdgeMapHash){
                                .i = data.volume_indices[volume_i + point_i],
                                .j = data.volume_indices[volume_i + point_j],
                            });
                        }
                    }
                }

                // extract deduplicated edges
                vtk_extract_edges_from_edge_map(arena, &volume_edge_map, &data.volume_edge_indices, &data.volume_edge_indices_count);
            }}
        }
    }}
    
    return (VTK_LoadResult) { .v = data, .error = ntstr8_lit_init("") };
}