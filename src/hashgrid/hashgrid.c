// helpers
internal HG_3s32 hg_3f32_3u32(vec3_f32 x, f32 scale) {
    return (HG_3s32){
        .x = floor_f32(x.x / scale),
        .y = floor_f32(x.y / scale),
        .z = floor_f32(x.z / scale),
    };
}
internal u32 hg_hash_3u32(HG_3s32 x, u32 mod) {
    // fantasy function
    u32 h = (u32)((x.x * 92837111) ^ (x.y * 689287499) ^ (x.z * 283923481));
	return h % mod; 
}
internal u32 hg_hash_3f32(vec3_f32 x, f32 scale, u32 mod) {
    return hg_hash_3u32(hg_3f32_3u32(x, scale), mod);
}

// hashgrid
internal HG_Hashgrid hg_build_hashgrid(
    Arena* arena, f32 scale,
    vec3_f32* positions, u64 positions_stride, u32 positions_count
) {
    HG_Hashgrid grid = {
        .arena = arena,
        .cell_length = scale,
        .objects_count = positions_count,
        .cells_count = positions_count*HG_OBJECT_TO_TABLE_RATIO,
    };
    grid.objects = push_array(arena, u32, grid.objects_count);
    // @note padding to allow length=[n+1].start - [n].start
    grid.cells = push_array(arena, HG_Cell, grid.cells_count);

    // count occupancy of each cell
    vec3_f32* end_p = OffsetPtr(positions, positions_stride*positions_count, vec3_f32);
    for (vec3_f32* p = positions; p <= end_p; p = OffsetPtr(p, positions_stride, vec3_f32)) {
        u32 hash = hg_hash_3f32(*p, grid.cell_length, grid.cells_count);
        grid.cells[hash].object_index++;
    }

    // use occupancy to store end index
    u32 count = 0;
    for EachIndex(cell_i, grid.cells_count) {
        count += grid.cells[cell_i].object_index;
        grid.cells[cell_i].object_index = count;
    }
    grid.cells[grid.cells_count].object_index = count;

    // fill in object entries and change end to start index
    for EachIndex(id, positions_count) {
        vec3_f32* p = OffsetPtr(positions, positions_stride*id, vec3_f32);
        u32 hash = hg_hash_3f32(*p, grid.cell_length, grid.cells_count);

        // @note avoids floating point errors in hashing causing underflow
        if (grid.cells[hash].object_index != 0)
            grid.cells[hash].object_index--;

        u32 object_index = grid.cells[hash].object_index;
        grid.objects[object_index] = id;
    }

    return grid;
}

// queries
internal HG_QueryResult hg_hashgrid_query(
    HG_Hashgrid* grid, Arena* arena,
    f32 radius, vec3_f32 position
) {
    HG_QueryResult result = zero_struct;

    HG_3s32 imin = hg_3f32_3u32(addscl_3f32(position,-radius), grid->cell_length);
    HG_3s32 imax = hg_3f32_3u32(addscl_3f32(position,+radius), grid->cell_length);
    
    // u64 q_acc = 0.f;
    // u64 q_count = 0;

    for (s32 ix = imin.x; ix <= imax.x; ix++) {
        for (s32 iy = imin.y; iy <= imax.y; iy++) {
            for (s32 iz = imin.z; iz <= imax.z; iz++) {
                u32 hash = hg_hash_3u32((HG_3s32){.x=ix,.y=iy,.z=iz}, grid->cells_count);
                u32 beg_index = grid->cells[hash  ].object_index;
                u32 end_index = grid->cells[hash+1].object_index;

                // q_acc += Max((s64)end_index - beg_index, 0); q_count++;
                for (u32 object_i = beg_index; object_i < end_index; object_i++) {
                    HG_QueryResultNode* n = push_array(arena, HG_QueryResultNode, 1);
                    n->id = grid->objects[object_i];
                    stack_push(result.first, n);
                    result.length++;
                }
            }
        }
    }

    // s32 area = (imax.x - imin.x)*(imax.y - imin.y)*(imax.z - imin.z);
    // printf("%dx%dx%d\n", imax.x - imin.x, imax.y - imin.y, imax.z - imin.z);
    // printf("\tavg len: %f\n", (f64)q_acc/(f64)q_count);
    // printf("\ttotal nodes: %f\n", (f64)area * (f64)q_acc/(f64)q_count);

    return result;
}

internal HG_BatchQueryResult hg_hashgrid_batch_query(
    HG_Hashgrid* grid, Arena* arena, u32 expected_hits,
    f32 radius, vec3_f32* positions, u64 positions_stride, u64* data, u64 data_stride, u32 object_count
) {
    f32 radius2 = radius*radius;

    Assert(expected_hits > 0);
    HG_BatchQueryResult result = {
        .object_hits_start = push_array(arena, u32, object_count+1),
        .object_count = object_count,
        .hits_capacity = expected_hits,
        .hits_count = 0,
    };

    result.hits_data = push_array(arena, u64, result.hits_capacity);

    for EachIndex(id0, object_count) {
        result.object_hits_start[id0] = result.hits_count;

        vec3_f32* p0 = OffsetPtr(positions, positions_stride*id0, vec3_f32);

        {DeferResource(Temp scratch = scratch_begin_a(arena), scratch_end(scratch)) {
            HG_QueryResult query = hg_hashgrid_query(grid, scratch.arena, radius, *p0);

            for EachList(n, HG_QueryResultNode, query.first) {
                u32 id1 = n->id;

                // conditions on query
                if (id1 >= id0) continue;

                vec3_f32* p1 = OffsetPtr(positions, positions_stride*id1, vec3_f32);

                vec3_f32 d = sub_3f32(*p1, *p0);
                if (dot_3f32(d, d) > radius2) continue;

                // store query
                // resize dynamic array if needed
                if (result.hits_count >= result.hits_capacity) {
                    result.hits_capacity *= HG_BATCH_QUERY_DYNAMIC_ARRAY_GROW_RATE;
                    u64* tmp = push_array(arena, u64, result.hits_capacity);
                    memcpy(tmp, result.hits_data, sizeof(*tmp)*result.hits_count);
                    result.hits_data = tmp;
                }

                u64* d1 = OffsetPtr(data, data_stride*id1, u64);
                result.hits_data[result.hits_count] = *d1;
                result.hits_count++;
            }
        }}
    }
    // guard
    result.object_hits_start[result.object_count] = result.hits_count;

    return result;
}