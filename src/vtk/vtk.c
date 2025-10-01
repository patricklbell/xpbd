internal void vtk_load_points(OS_Handle file, NTString8 line, vec3_f32* points, u32 points_count) {
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

internal void vtk_load_cells(OS_Handle file, NTString8 line, u32 cells_count, u32* cells_data, u32 cells_data_count) {
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

internal void vtk_load_cell_types(OS_Handle file, NTString8 line, VTK_CellType* cells_type, u32 cells_count) {
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

// loader
internal VTK_LoadResult vtk_load(Arena* arena, NTString8 path, VTK_LoadSettings settings) {
    OS_Handle file = os_open_readonly_file(path);
    VTK_LoadResult res = zero_struct;

    if (os_is_handle_zero(file)) {
        res.error = ntstr8_lit("Failed to open file");
        return res;
    }

    {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
        NTString8 line;
        line.data = push_array(scratch.arena, u8, Max(OS_DEFAULT_MAX_LINE_LENGTH, 256));

        // version
        os_read_line_to_buffer(file, &line);
        u32 version_major, version_minor;
        int matched = sscanf(line.cstr, "# vtk DataFile Version %u.%u", &version_major, &version_minor);
        if (matched != 2) {
            res.error = ntstr8_lit("Could not parse version in header.");
            continue;
        }

        // header
        os_read_line_to_buffer(file, &line);

        // file format
        os_read_line_to_buffer(file, &line);
        if (!ntstr8_begins_with(line, "ASCII")) {
            res.error = ntstr8_lit("Unsupported file format (expected ASCII).");
            continue;
        }

        // dataset structure
        os_read_line_to_buffer(file, &line);
        if (!ntstr8_begins_with(line, "DATASET UNSTRUCTURED_GRID")) {
            res.error = ntstr8_lit("Unsupported dataset structure (expected UNSTRUCTURED_GRID).");
            continue;
        }

        u32 cells_count, cells_data_count, cells_type_count;
        u32* cells_data = NULL;
        VTK_CellType* cells_type = NULL;
        res.v.points = NULL;

        while (!os_is_eof(file)) {
            os_read_line_to_buffer(file, &line);
    
            // specification says POINT_DATA, etc. not sure why exported files don't match
            if (ntstr8_begins_with(line, "POINTS ")) {
                int matched = sscanf(line.cstr, "POINTS %u", &res.v.points_count);
                Assert(matched == 1); // @todo logging

                res.v.points = push_array(arena, vec3_f32, res.v.points_count);
                vtk_load_points(file, line, res.v.points, res.v.points_count);
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

        if (cells_count != cells_type_count) {
            res.error = ntstr8_lit("Cells and cell type's did not match");
            continue;
        }
        if (res.v.points == NULL || cells_data == NULL || cells_type == NULL) {
            res.error = ntstr8_lit("Missing required section");
            continue;
        }

        // determine how many of each cell type there is
        for (u32 cell_i = 0, data_offset = 0; cell_i < cells_count; cell_i++) {
            VTK_CellType type = cells_type[cell_i];

            u32 vertex_count = cells_data[data_offset];
            data_offset++; // consume count
            
            res.v.indices_counts[type]+=vertex_count;

            data_offset+=vertex_count; // consume vertices
        }

        // allocate indices
        for EachIndex(type, VTK_CellType_COUNT) {
            u32 indices_count = res.v.indices_counts[type];
            if (indices_count > 0) {
                res.v.indices[type] = push_array(arena, u32, indices_count);
            }
        }

        // fill in data
        u32 indices_offsets[VTK_CellType_COUNT] = { };
        for (u32 cell_i = 0, data_offset = 0; cell_i < cells_count; cell_i++) {
            VTK_CellType type = cells_type[cell_i];
            u32* indice_offset = &indices_offsets[type];

            u32 vertex_count = cells_data[data_offset];
            data_offset++; // consume count
            
            for (int vertex_i = 0; vertex_i < vertex_count; vertex_i++, data_offset++, (*indice_offset)++) {
                res.v.indices[type][*indice_offset] = cells_data[data_offset];
            }
        }
    }}
    
    return res;
}