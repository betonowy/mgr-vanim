#pragma once

#include <vector>

#include <dvdb/types.hpp>
#include <glm/vec3.hpp>
#include <nanovdb/PNanoVDB.h>

namespace converter
{
class nvdb_reader
{
public:
    nvdb_reader();

    void initialize(void *grid_ptr);

    size_t size() const
    {
        return _buf_size;
    }

    const void *data() const
    {
        return _buf.data;
    }

    size_t main_size() const
    {
        return _main_size;
    }

    const void *main_data() const
    {
        return data();
    }

    size_t leaf_size() const
    {
        return _leaf_size;
    }

    void *leaf_data() const
    {
        return _leaf_ptr;
    }

    size_t leaf_count() const
    {
        return _leaf_handles.size();
    }

    pnanovdb_leaf_t *leaf_ptr(size_t i) const
    {
        return reinterpret_cast<pnanovdb_leaf_t *>(_buf.data + (_leaf_handles[i].second.address.byte_offset >> 2));
    }

    dvdb::cube_888_mask *leaf_bitmask_ptr(size_t i) const
    {
        return reinterpret_cast<dvdb::cube_888_mask *>(leaf_ptr(i)->value_mask);
    }

    dvdb::cube_888_i8 *leaf_table_ptr(size_t i) const
    {
        auto base = _buf.data + (_leaf_handles[i].second.address.byte_offset >> 2);
        auto table_offset = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FP8].leaf_off_table;
        return reinterpret_cast<dvdb::cube_888_i8 *>(base + (table_offset >> 2));
    }

    auto leaf_min_quantum_from_table(const dvdb::cube_888_i8* table)
    {
        auto base = reinterpret_cast<const uint32_t*>(table);
        auto min_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_table - pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_min;
        auto max_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_table - pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_max;

        const float *min = reinterpret_cast<const float *>(base - (min_off >> 2));
        const float *quantum = reinterpret_cast<const float *>(base - (max_off >> 2));

        struct minmax
        {
            float min, quantum;
        };

        return minmax{.min = *min, .quantum = *quantum};
    }

    auto leaf_min_quantum(size_t i) const
    {
        auto base = _buf.data + (_leaf_handles[i].second.address.byte_offset >> 2);
        auto min_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_min;
        auto max_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_max;

        float *min = reinterpret_cast<float *>(base + (min_off >> 2));
        float *quantum = reinterpret_cast<float *>(base + (max_off >> 2));

        struct minmax
        {
            float min, quantum;
        };

        return minmax{.min = *min, .quantum = *quantum};
    }

    auto leaf_min_quantum_ptrs(size_t i) const
    {
        auto base = _buf.data + (_leaf_handles[i].second.address.byte_offset >> 2);
        auto min_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_min;
        auto max_off = pnanovdb_grid_type_constants[PNANOVDB_GRID_TYPE_FLOAT].leaf_off_max;

        float *min = reinterpret_cast<float *>(base + (min_off >> 2));
        float *quantum = reinterpret_cast<float *>(base + (max_off >> 2));

        struct minmax_ptr
        {
            float *min, *quantum;
        };

        return minmax_ptr{.min = min, .quantum = quantum};
    }

    uint64_t leaf_key(size_t i) const
    {
        return ivec3_to_key(_leaf_handles[i].first);
    }

    glm::ivec3 leaf_coord(size_t i) const
    {
        return _leaf_handles[i].first;
    }

    int get_leaf_index_from_coord(glm::ivec3 coord) const
    {
        return get_leaf_index_from_key(ivec3_to_key(coord));
    }

    int get_leaf_index_from_key(uint64_t key) const;

    void leaf_neighbors(glm::ivec3 coord, const dvdb::cube_888_i8 **values, const dvdb::cube_888_mask **masks, const dvdb::cube_888_i8 *empty_values, const dvdb::cube_888_mask *empty_mask) const;

    void leaf_neighbors(uint64_t key, const dvdb::cube_888_i8 **values, const dvdb::cube_888_mask **masks, const dvdb::cube_888_i8 *empty_values, const dvdb::cube_888_mask *empty_mask) const
    {
        leaf_neighbors(key_to_ivec3(key), values, masks, empty_values, empty_mask);
    }

    static glm::ivec3 key_to_ivec3(uint64_t key)
    {
        return {((key >> 42) & 0x1fffff), ((key >> 21) & 0x1fffff), ((key >> 0) & 0x1fffff)};
    }

    static uint64_t ivec3_to_key(glm::ivec3 coord)
    {
        uint64_t iu = coord.x;
        uint64_t ju = coord.y;
        uint64_t ku = coord.z;

        return (ku) | (ju << 21u) | (iu << 42u);
    }

private:
    size_t _buf_size;
    pnanovdb_buf_t _buf;

    size_t _main_size;
    size_t _leaf_size;
    void *_leaf_ptr;

    std::vector<std::pair<glm::ivec3, pnanovdb_leaf_handle_t>> _leaf_handles;
    std::vector<uint64_t> _leaf_keys;
};
} // namespace converter
