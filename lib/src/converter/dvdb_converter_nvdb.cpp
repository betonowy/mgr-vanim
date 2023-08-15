#include "dvdb_converter_nvdb.hpp"

#include <algorithm>
#include <stdexcept>

namespace converter
{
nvdb_reader::nvdb_reader() = default;

namespace
{
bool operator==(const std::pair<glm::ivec3, pnanovdb_leaf_handle_t> &lhs, const std::pair<glm::ivec3, pnanovdb_leaf_handle_t> &rhs)
{
    return lhs == rhs;
}

void append_lower_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_lower_handle_t lower, std::vector<std::pair<glm::ivec3, pnanovdb_leaf_handle_t>> *dest, glm::ivec3 coord, pnanovdb_grid_type_t type)
{
    for (int i = 0; i < PNANOVDB_LOWER_TABLE_COUNT; ++i)
    {
        if (!pnanovdb_lower_get_child_mask(buf, lower, i))
        {
            continue;
        }

        glm::ivec3 subcoord{(i >> 0) & 0xf, (i >> 4) & 0xf, (i >> 8) & 0xf};

        const auto leaf = pnanovdb_lower_get_child(type, buf, lower, i);

        dest->emplace_back(subcoord + coord, leaf);
    }
}

template <typename T>
void append_upper_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_upper_handle_t upper, T *dest, glm::ivec3 coord, pnanovdb_grid_type_t type)
{
    for (int i = 0; i < PNANOVDB_UPPER_TABLE_COUNT; ++i)
    {
        if (!pnanovdb_upper_get_child_mask(buf, upper, i))
        {
            continue;
        }

        glm::ivec3 subcoord{(i >> 0) & 0x1f, (i >> 5) & 0x1f, (i >> 10) & 0x1f};

        append_lower_leaves_from_nvdb(buf, pnanovdb_upper_get_child(type, buf, upper, i), dest, coord + (subcoord << 4), type);
    }
}
} // namespace

void nvdb_reader::initialize(void *grid_ptr)
{
    if (!_leaf_handles.empty())
    {
        _leaf_handles.clear();
    }

    _buf.data = static_cast<uint32_t *>(grid_ptr);

    const pnanovdb_grid_handle_t grid{};
    const auto tree = pnanovdb_grid_get_tree(_buf, grid);
    const auto root = pnanovdb_tree_get_root(_buf, tree);
    const auto type = pnanovdb_grid_get_grid_type(_buf, grid);

    _leaf_handles.reserve(pnanovdb_tree_get_node_count_leaf(_buf, tree));

    if (type != PNANOVDB_GRID_TYPE_FP8)
    {
        throw std::runtime_error("Expects only f8 grid.");
    }

    auto tile_count = pnanovdb_root_get_tile_count(_buf, root);

    for (uint32_t i = 0; i < tile_count; ++i)
    {
        const auto tile = pnanovdb_root_get_tile(type, root, i);
        const auto upper = pnanovdb_root_get_child(type, _buf, root, tile);
        const auto key = pnanovdb_root_tile_get_key(_buf, tile);
        glm::ivec3 coord = key_to_ivec3(key);

        append_upper_leaves_from_nvdb(_buf, upper, &_leaf_handles, coord, type);
    }

    using T = decltype(_leaf_handles)::value_type;

    std::sort(_leaf_handles.begin(), _leaf_handles.end(), [](const T &lhs, const T &rhs) {
        return ivec3_to_key(lhs.first) < ivec3_to_key(rhs.first);
    });

    _leaf_keys.resize(_leaf_handles.size());

    for (int i = _leaf_handles.size() - 1; i >= 0; --i)
    {
        _leaf_keys[i] = ivec3_to_key(_leaf_handles[i].first);
    }

    for (size_t i = 1; i < _leaf_keys.size(); ++i)
    {
        if (_leaf_keys[i - 1] > _leaf_keys[i])
        {
            throw std::runtime_error("Bad leaf sorting.");
        }
    }

    const auto leaf_count = pnanovdb_tree_get_node_count_leaf(_buf, tree);

    if (leaf_count != _leaf_handles.size())
    {
        throw std::runtime_error("Leaf count expected to be the same as actual.");
    }

    const auto leaf_offset = pnanovdb_tree_get_node_offset_leaf(_buf, tree);

    _main_size = leaf_offset;
    _leaf_ptr = _buf.data + (leaf_offset >> 2);
    _leaf_size = leaf_count * pnanovdb_grid_type_constants[type].leaf_size;
    _buf_size = _leaf_size + leaf_offset;
}

int nvdb_reader::get_leaf_index_from_key(uint64_t key) const
{
    const auto coord = key_to_ivec3(key);

    int64_t l = 0;
    int64_t r = _leaf_keys.size() - 1;

    // binary search (array must be sorted)
    while (l <= r)
    {
        int64_t m = (l + r) >> 1;

        uint64_t sample = _leaf_keys[m];

        if (sample == key)
        {
            return m;
        }

        if (sample < key)
        {
            l = m + 1;
        }
        else
        {
            r = m - 1;
        }
    }

    return -1;
}

namespace
{
constexpr glm::ivec3 index_to_diff_coord(int i)
{
    glm::ivec3 ret;

    ret.x = i % 3;
    ret.y = (i / 3) % 3;
    ret.z = (i / 9) % 3;

    return ret - 1;
}
} // namespace

void nvdb_reader::leaf_neighbors(glm::ivec3 coord, const dvdb::cube_888_i8 **values, const dvdb::cube_888_mask **masks, const dvdb::cube_888_i8 *empty_values, const dvdb::cube_888_mask *empty_mask) const
{
    for (int i = 0; i < 3 * 3 * 3; ++i)
    {
        int index = get_leaf_index_from_coord(index_to_diff_coord(i) + coord);

        if (index == -1)
        {
            values[i] = empty_values;
            masks[i] = empty_mask;
        }
        else
        {
            values[i] = leaf_table_ptr(index);
            masks[i] = leaf_bitmask_ptr(index);
        }
    }
}
} // namespace converter
