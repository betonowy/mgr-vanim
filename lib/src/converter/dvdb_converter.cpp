#include "dvdb_converter.hpp"
#include <dvdb/dvdb_header.hpp>
#include <utils/nvdb_mmap.hpp>

#include <nanovdb/PNanoVDB.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <numeric>

#include <immintrin.h>

namespace
{
size_t vdb_determine_leafless_copy_size_direct_ptr(const void *data)
{
    // Removing constness is absolutely OK, because data will not be modified there
    const pnanovdb_buf_t buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(data))};
    const pnanovdb_grid_handle_t grid{}; // Zero initialized is ok, because buffers are aligned that way
    const pnanovdb_tree_handle_t tree = pnanovdb_grid_get_tree(buf, grid);

    return pnanovdb_tree_get_node_offset_leaf(buf, tree);
}

struct rle_diff_desc
{
    uint32_t src_leaf, dst_leaf;
    float min, max;
};

struct rle_diff_code
{
    union {
        struct
        {
            uint16_t length, value;
        };

        uint32_t bits;
    };

    float float_value(const rle_diff_desc &desc)
    {
        return desc.min + (desc.max - desc.min) * (value * (1.f / std::numeric_limits<decltype(value)>::max()));
    }

    void set_value(const rle_diff_desc &desc, float value)
    {
        assert(value <= desc.max && value >= desc.min);
    }
};

std::vector<uint8_t> vdb_create_rle_diff(const void *src_state, const void *dst_state, void *final_state)
{
    // Removing constness is absolutely OK, because data will not be modified there
    const pnanovdb_buf_t src_buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(src_state))};
    const pnanovdb_buf_t dst_buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(dst_state))};
    const pnanovdb_buf_t final_buf{.data = reinterpret_cast<uint32_t *>(final_state)};

    pnanovdb_grid_handle_t src_grid{}, dst_grid{}, final_grid{};
    pnanovdb_tree_handle_t src_tree, dst_tree, final_tree;

    dst_tree = pnanovdb_grid_get_tree(dst_buf, dst_grid);
    const auto dst_leaf_count = pnanovdb_tree_get_node_count_leaf(dst_buf, dst_tree);

    std::vector<uint8_t> output_data;

    auto push_bytes = [&output_data](void *data, size_t size) {
        uint8_t *bytes = static_cast<uint8_t *>(data);

        for (size_t i = 0; i < size; ++i)
        {
            output_data.push_back(bytes[i]);
        }
    };

    for (uint32_t i = 0; i < dst_leaf_count; ++i)
    {
        rle_diff_desc desc = {
            .src_leaf = 0,
            .dst_leaf = i,
            .min = 0,
            .max = 1,
        };

        rle_diff_code code = {
            .length = PNANOVDB_LEAF_TABLE_COUNT,
            .value = 0,
        };

        push_bytes(&desc, sizeof(desc));
        push_bytes(&code, sizeof(code));
    }

    return output_data;
} // namespace

} // namespace

namespace converter
{
struct dvdb_state
{
    std::vector<uint8_t> _vdb_buffer;
};

dvdb_converter::dvdb_converter(std::shared_ptr<utils::thread_pool> pool)
    : _state(std::make_unique<dvdb_state>()), _thread_pool(std::move(pool))
{
}

dvdb_converter::~dvdb_converter() = default;

void dvdb_converter::create_keyframe(std::filesystem::path path)
{
    utils::nvdb_mmap nvdb_mmap(path.string());

    auto dvdb_path = path;
    dvdb_path.replace_extension(".dvdb");

    {
        dvdb::header header = {
            .magic = dvdb::MAGIC_NUMBER,
            .frame_type = dvdb::frame_type_e::KEY_FRAME,
            .vdb_grid_count = nvdb_mmap.grids().size(),
            .vdb_required_size = sizeof(header),
        };

        for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
        {
            const auto &grid = nvdb_mmap.grids()[i];

            header.frames[i].base_tree_offset_start = header.vdb_required_size;
            header.frames[i].base_tree_copy_size = grid.size;
            header.frames[i].base_tree_final_size = grid.size;
            header.vdb_required_size += grid.size;
        }

        std::ofstream output(dvdb_path.string(), std::ios::binary);

        output.write(reinterpret_cast<const char *>(&header), sizeof(header));

        for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
        {
            const auto &grid = nvdb_mmap.grids()[i];

            output.write(reinterpret_cast<const char *>(grid.ptr), grid.size);
        }
    }

    // Briefly verify contents and update current state
    {
        mio::mmap_source input(dvdb_path.string());

        const auto header = reinterpret_cast<const dvdb::header *>(input.data());

        if (header->magic != dvdb::MAGIC_NUMBER)
        {
            throw std::runtime_error("DVDB header magic mismatch!");
        }

        uint64_t current_offset = sizeof(*header);

        for (size_t i = 0; i < header->vdb_grid_count; ++i)
        {
            auto &frame = header->frames[i];

            if (frame.base_tree_offset_start != current_offset)
            {
                throw std::runtime_error("Grid offset mismatch!");
            }

            if (frame.base_tree_copy_size != frame.base_tree_final_size)
            {
                throw std::runtime_error("Grid size mismatch!");
            }

            const auto magic_address = reinterpret_cast<const uint64_t *>(input.data() + current_offset);

            if (*magic_address != PNANOVDB_MAGIC_NUMBER)
            {
                throw std::runtime_error("NanoVDB magic mismatch!");
            }

            current_offset += frame.base_tree_final_size;
        }

        _state->_vdb_buffer.resize(input.size());

        std::memcpy(_state->_vdb_buffer.data(), input.data(), input.size());
    }
}

void dvdb_converter::add_diff_frame(std::filesystem::path path)
{
    utils::nvdb_mmap nvdb_mmap(path.string());

    auto dvdb_path = path;
    dvdb_path.replace_extension(".dvdb");

    const auto current_state_header = reinterpret_cast<const dvdb::header *>(_state->_vdb_buffer.data());

    dvdb::header next_state_header = {
        .magic = dvdb::MAGIC_NUMBER,
        .frame_type = dvdb::frame_type_e::DIFF_FRAME,
        .vdb_grid_count = current_state_header->vdb_grid_count,
        .vdb_required_size = sizeof(next_state_header),
    };

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        const auto &grid = nvdb_mmap.grids()[i];
        const auto leafless_size = vdb_determine_leafless_copy_size_direct_ptr(grid.ptr);

        next_state_header.frames[i].base_tree_offset_start = next_state_header.vdb_required_size;
        next_state_header.frames[i].base_tree_copy_size = leafless_size;
        next_state_header.frames[i].base_tree_final_size = grid.size;
        next_state_header.vdb_required_size += grid.size;
    }

    std::vector<uint8_t> next_buffer(next_state_header.vdb_required_size);
    std::memcpy(next_buffer.data(), &next_state_header, sizeof(next_state_header));

    std::vector<std::vector<uint8_t>> diff_data_chunks;

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        const auto &grid = nvdb_mmap.grids()[i];

        const auto dst = next_state_header.frames[i].base_tree_offset_start + next_buffer.data();
        const auto size = next_state_header.frames[i].base_tree_copy_size;

        std::memcpy(dst, grid.ptr, size);

        const auto source_state_ptr = _state->_vdb_buffer.data() + current_state_header->frames[i].base_tree_offset_start;
        const auto destination_state_ptr = grid.ptr;
        const auto diff_state_ptr = next_buffer.data() + next_state_header.frames[i].base_tree_offset_start;

        diff_data_chunks.emplace_back(vdb_create_rle_diff(source_state_ptr, destination_state_ptr, diff_state_ptr));
    }

    // reallign data for storage

    dvdb::header compressed_header = next_state_header;

    uint64_t compressed_base_tree_offset = sizeof(compressed_header);

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        const auto &grid = nvdb_mmap.grids()[i];
        const auto leafless_size = vdb_determine_leafless_copy_size_direct_ptr(grid.ptr);

        compressed_header.frames[i].base_tree_offset_start = compressed_base_tree_offset;
        compressed_header.frames[i].base_tree_copy_size = leafless_size;
        compressed_header.frames[i].base_tree_final_size = grid.size;
        compressed_header.vdb_required_size += grid.size;

        compressed_base_tree_offset += leafless_size;
    }

    {
        std::ofstream output(dvdb_path.string(), std::ios::binary);

        output.write(reinterpret_cast<const char *>(&compressed_header), sizeof(compressed_header));

        for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
        {
            const auto &grid = nvdb_mmap.grids()[i];

            output.write(reinterpret_cast<const char *>(grid.ptr), compressed_header.frames[i].base_tree_copy_size);
        }

        for (size_t i = 0; i < diff_data_chunks.size(); ++i)
        {
            output.write(reinterpret_cast<const char *>(diff_data_chunks[i].data()), diff_data_chunks[i].size());
        }
    }

    _state->_vdb_buffer = std::move(next_buffer);
}

//
//
//

namespace
{
using namespace converter;

static constexpr auto PNANOVDB_LEAF_TABLE_COUNT_AVX2 = PNANOVDB_LEAF_TABLE_COUNT / 8;

struct pnanovdb_grid_diff
{
    uint32_t src_leaf, dst_leaf;
    __m256 values[PNANOVDB_LEAF_TABLE_COUNT_AVX2];
};

int calculate_diff_avx2(const __m256 *src, const __m256 *dst, __m256 *diff)
{
    const __m256 zeros = _mm256_setzero_ps();
    int bits_set_count = 0;

    for (size_t i = 0; i < PNANOVDB_LEAF_TABLE_COUNT_AVX2; ++i)
    {
        diff[i] = dst[i] - src[i];

        const auto cmp = _mm256_cmp_ps(diff[i], zeros, _CMP_EQ_UQ);

        for (int i = 0; i < 8; ++i)
        {
            bits_set_count += cmp[i] != 0;
        }
    }

    return bits_set_count;
}

void create_grid_diff(pnanovdb_buf_t src_buf, pnanovdb_buf_t dst_buf)
{
    pnanovdb_grid_handle_t src_grid{}, dst_grid{}; // Zero initialized is ok, because buffers are aligned that way
    pnanovdb_grid_type_t grid_type = pnanovdb_grid_get_grid_type(src_buf, src_grid);

    if (pnanovdb_grid_get_magic(src_buf, src_grid) != PNANOVDB_MAGIC_NUMBER)
    {
        throw std::runtime_error("Corrupted source grid.");
    }

    if (pnanovdb_grid_get_magic(dst_buf, dst_grid) != PNANOVDB_MAGIC_NUMBER)
    {
        throw std::runtime_error("Corrupted destination grid.");
    }

    if (grid_type != pnanovdb_grid_get_grid_type(dst_buf, dst_grid) || grid_type == PNANOVDB_GRID_TYPE_FPN)
    {
        throw std::runtime_error("Unsupported or incompatible grid types");
    }

    pnanovdb_tree_handle_t src_tree, dst_tree;

    src_tree = pnanovdb_grid_get_tree(src_buf, src_grid);
    dst_tree = pnanovdb_grid_get_tree(dst_buf, dst_grid);

    pnanovdb_root_handle_t src_root, dst_root;

    src_root = pnanovdb_tree_get_root(src_buf, src_tree);
    dst_root = pnanovdb_tree_get_root(dst_buf, dst_tree);

    pnanovdb_readaccessor_t src_acc, dst_acc;

    pnanovdb_readaccessor_init(&src_acc, src_root);
    pnanovdb_readaccessor_init(&dst_acc, dst_root);

    pnanovdb_uint64_t src_leaves_offset, dst_leaves_offset;
    pnanovdb_uint32_t src_leaves_count, dst_leaves_count;

    src_leaves_offset = pnanovdb_tree_get_node_offset_leaf(src_buf, src_tree);
    dst_leaves_offset = pnanovdb_tree_get_node_offset_leaf(dst_buf, dst_tree);

    src_leaves_count = pnanovdb_tree_get_node_count_leaf(src_buf, src_tree);
    dst_leaves_count = pnanovdb_tree_get_node_count_leaf(dst_buf, dst_tree);

    std::vector<pnanovdb_leaf_handle_t> src_leaf_handles;
    std::vector<pnanovdb_leaf_handle_t> dst_leaf_handles;

    src_leaf_handles.reserve(src_leaves_count);
    dst_leaf_handles.reserve(dst_leaves_count);

    pnanovdb_uint32_t leaf_size = PNANOVDB_GRID_TYPE_GET(grid_type, leaf_size);

    for (size_t i = 0; i < src_leaves_count; ++i)
    {
        src_leaf_handles.emplace_back(pnanovdb_leaf_handle_t{leaf_size * i + src_leaves_offset});
    }

    for (size_t i = 0; i < dst_leaves_count; ++i)
    {
        dst_leaf_handles.emplace_back(pnanovdb_leaf_handle_t{leaf_size * i + dst_leaves_offset});
    }

    for (size_t i = 0; i < dst_leaf_handles.size(); ++i)
    {
        const auto dst_table_offset = pnanovdb_leaf_get_table_address(grid_type, dst_buf, dst_leaf_handles[i], 0);
        const auto dst_table_u32_ptr = dst_buf.data + (dst_table_offset.byte_offset >> 2);

        __m256 dst_values[PNANOVDB_LEAF_TABLE_COUNT_AVX2];
        std::memcpy(dst_values, dst_table_u32_ptr, sizeof(dst_values));

        int best_src_leaf = -1;
        int best_src_leaf_zeros = -1;

        for (size_t j = 0; j < src_leaf_handles.size(); ++j)
        {
            const auto src_table_offset = pnanovdb_leaf_get_table_address(grid_type, src_buf, src_leaf_handles[j], 0);
            const auto src_table_u32_ptr = src_buf.data + (src_table_offset.byte_offset >> 2);

            __m256 src_values[PNANOVDB_LEAF_TABLE_COUNT_AVX2];
            std::memcpy(src_values, src_table_u32_ptr, sizeof(src_values));

            __m256 diff_values[PNANOVDB_LEAF_TABLE_COUNT_AVX2];

            int zeros = calculate_diff_avx2(src_values, dst_values, diff_values);

            if (zeros > best_src_leaf_zeros)
            {
                best_src_leaf = j;
                best_src_leaf_zeros = zeros;
            }
        }
    }
}
} // namespace

namespace converter
{
conversion_result create_nvdb_diff(std::filesystem::path src, std::filesystem::path dst)
{
    utils::nvdb_mmap src_mmap(src.string()), dst_mmap(dst.string());

    if (src_mmap.grids().size() != dst_mmap.grids().size())
    {
        throw std::runtime_error("NanoVDB files have different number of grids.");
    }

    std::vector<uint32_t> src_buf, dst_buf;
    static constexpr auto buf_value_type_sizeof = sizeof(decltype(src_buf)::value_type);

    for (size_t i = 0; i < src_mmap.grids().size(); ++i)
    {
        const auto &src_grid = src_mmap.grids()[i];
        const auto &dst_grid = dst_mmap.grids()[i];

        src_buf.clear(), dst_buf.clear();

        src_buf.resize(src_grid.size / buf_value_type_sizeof);
        dst_buf.resize(dst_grid.size / buf_value_type_sizeof);

        std::memcpy(src_buf.data(), src_grid.ptr, src_grid.size);
        std::memcpy(dst_buf.data(), dst_grid.ptr, dst_grid.size);

        pnanovdb_buf_t src_buf_handle, dst_buf_handle;

        src_buf_handle.data = src_buf.data();
        dst_buf_handle.data = dst_buf.data();

        create_grid_diff(src_buf_handle, dst_buf_handle);
    }

    return {};
}
} // namespace converter
} // namespace converter
