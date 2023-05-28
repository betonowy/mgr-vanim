#include "diff_vdb_resource.hpp"

#include <dvdb/dvdb_header.hpp>
#include <dvdb/dvdb_rle.hpp>
#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/memory_counter.hpp>
#include <utils/nvdb_mmap.hpp>
#include <utils/scope_guard.hpp>
#include <utils/thread_pool.hpp>
#include <utils/utf8_exception.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>

#include <nanovdb/PNanoVDB.h>

namespace objects::vdb
{
const char *diff_vdb_resource::class_name()
{
    return "DiffVDB Animation";
}

diff_vdb_resource::diff_vdb_resource(std::filesystem::path path)
    : _resource_directory(path)
{
    std::vector<std::pair<int, std::filesystem::path>> dvdb_files;

    using regex_type = std::basic_regex<std::filesystem::path::value_type>;

#if VANIM_WINDOWS
    static constexpr auto regex_pattern = L"^.*[\\/\\\\].+_(\\d+)\\.dvdb$";
    using match_type = std::wcmatch;
#else
    static constexpr auto regex_pattern = "^.*[\\/\\\\].+_(\\d+)\\.dvdb$";
    using match_type = std::cmatch;
#endif

    auto r = regex_type(regex_pattern);
    // MSVC will experience internal error if I limit scope of "r" to this lambda. This is BS.
    auto path_to_frame_number = [&](const std::filesystem::path &path) -> int {
        match_type cm;

        if (std::regex_search(path.c_str(), cm, r))
        {
            return std::stoi(cm[1]);
        }

        throw utils::utf8_exception(u8"Can't decode frame number from path: " + path.u8string());
    };

    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().has_extension() && entry.path().extension() == ".dvdb")
        {
            dvdb_files.emplace_back(path_to_frame_number(entry), entry);
        }
    }

    if (dvdb_files.empty())
    {
        throw utils::utf8_exception(u8"No DiffVDB frames found at: " + path.u8string());
    }

    std::sort(dvdb_files.begin(), dvdb_files.end());

    _dvdb_frames = std::move(dvdb_files);
}

diff_vdb_resource::~diff_vdb_resource()
{
    utils::gpu_buffer_memory_deallocated(_ssbo_block_size * _ssbo_block_count);
    glUnmapNamedBuffer(_ssbo);
}

void diff_vdb_resource::init(scene::object_context &ctx)
{
    volume_resource_base::init(ctx);

    set_frame_rate(30.f);

    size_t max_buffer_size = 0;

    for (const auto &[n, path] : _dvdb_frames)
    {
        mio::mmap_source dvdb_mmap(path.string());

        const auto header = reinterpret_cast<const dvdb::header *>(dvdb_mmap.data());

        if (header->vdb_required_size > max_buffer_size)
        {
            max_buffer_size = header->vdb_required_size;
        }

        _frames.emplace_back(frame{
            .path = path.string(),
            .block_number = 0,
            .number = _frames.size(),
        });
    }

    _current_state.resize(max_buffer_size);
    _created_state.resize(max_buffer_size);

    _ssbo_block_size = max_buffer_size;
    _ssbo_block_count = MAX_BLOCKS;

    utils::gpu_buffer_memory_allocated(_ssbo_block_size * _ssbo_block_count);

    glNamedBufferStorage(_ssbo, _ssbo_block_size * _ssbo_block_count, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    _ssbo_ptr = reinterpret_cast<std::byte *>(glMapNamedBufferRange(_ssbo, 0, _ssbo_block_size * MAX_BLOCKS, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));

    for (int i = 0; i < MAX_BLOCKS && i < _dvdb_frames.size(); ++i)
    {
        schedule_frame(ctx, i, i);
    }
}

void grid_reconstruction_rle(void *diff_ptr, void *dst_ptr, void *src_ptr)
{
    const pnanovdb_buf_t src_buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(src_ptr))};
    const pnanovdb_buf_t dst_buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(dst_ptr))};

    pnanovdb_grid_handle_t src_grid{}, dst_grid{};
    pnanovdb_tree_handle_t src_tree, dst_tree;

    uint32_t src_leaf_offset, dst_leaf_offset;
    uint32_t src_leaf_count, dst_leaf_count;

    src_tree = pnanovdb_grid_get_tree(src_buf, src_grid);
    dst_tree = pnanovdb_grid_get_tree(dst_buf, dst_grid);

    src_leaf_offset = pnanovdb_tree_get_node_offset_leaf(src_buf, src_tree) + src_tree.address.byte_offset;
    dst_leaf_offset = pnanovdb_tree_get_node_offset_leaf(dst_buf, dst_tree) + dst_tree.address.byte_offset;

    src_leaf_count = pnanovdb_tree_get_node_count_leaf(src_buf, src_tree);
    dst_leaf_count = pnanovdb_tree_get_node_count_leaf(dst_buf, dst_tree);

    uint32_t grid_type = pnanovdb_grid_get_grid_type(src_buf, src_grid);

    const auto *diff_current_ptr = reinterpret_cast<uint8_t *>(diff_ptr);

    for (size_t i = 0; i < dst_leaf_count; ++i)
    {
        const auto *desc = (dvdb::rle_diff_desc *)(diff_current_ptr);

        diff_current_ptr += sizeof(dvdb::rle_diff_desc);

        const auto *code = (dvdb::rle_diff_code *)(diff_current_ptr);

        const auto *constants = &pnanovdb_grid_type_constants[pnanovdb_grid_get_grid_type(src_buf, src_grid)];

        const auto src_floats = (float *)(src_buf.data + (src_leaf_offset + constants->leaf_size * desc->src_leaf + constants->leaf_off_table) / sizeof(float));
        const auto dst_floats = (float *)(dst_buf.data + (dst_leaf_offset + constants->leaf_size * desc->dst_leaf + constants->leaf_off_table) / sizeof(float));
        const auto dst_leaf = (pnanovdb_leaf_t *)(dst_buf.data + (dst_leaf_offset + constants->leaf_size * desc->dst_leaf) / sizeof(float));

        for (size_t i = 0; i < 512; ++i)
        {
            dst_floats[i] = code->values[i];
        }

        for (size_t i = 0; i < 16; ++i)
        {
            dst_leaf->value_mask[i] = ~0;
        }

        diff_current_ptr += sizeof(dvdb::rle_diff_code);
    }
}

void diff_vdb_resource::schedule_frame(scene::object_context &ctx, int block_number, int frame_number)
{
    auto wait_t1 = std::chrono::steady_clock::now();
    _ssbo_block_fences[block_number].client_wait(true);
    auto wait_t2 = std::chrono::steady_clock::now();

    utils::update_wait_time(std::chrono::duration_cast<std::chrono::microseconds>(wait_t2 - wait_t1).count());

    _ssbo_block_frame[block_number] = frame_number;
    _ssbo_timestamp[block_number] = std::chrono::steady_clock::now();

    std::swap(_created_state, _current_state);

    std::atomic_bool resource_locked = false;

    std::function task = [this, sptr = shared_from_this(), frame_number, block_number, &resource_locked]() -> update_range {
        glm::uvec4 offsets(~0);
        size_t copy_size = 0;

        std::lock_guard lock(_state_modification_mtx);
        resource_locked = true;

        auto map_t1 = std::chrono::steady_clock::now();
        mio::mmap_source dvdb_mmap(_dvdb_frames[frame_number].second.string());
        const auto header = reinterpret_cast<const dvdb::header *>(dvdb_mmap.data());
        copy_size = header->vdb_required_size;
        auto map_t2 = std::chrono::steady_clock::now();

        utils::update_map_time(std::chrono::duration_cast<std::chrono::microseconds>(map_t2 - map_t1).count());

        auto copy_t1 = std::chrono::steady_clock::now();

        switch (header->frame_type)
        {
        case dvdb::frame_type_e::KEY_FRAME: {
            for (size_t i = 0; i < header->vdb_grid_count; ++i)
            {
                offsets[i] = header->frames[i].base_tree_offset_start;
            }

            assert(_created_state.size() >= dvdb_mmap.size());

            std::memcpy(_created_state.data(), dvdb_mmap.data(), dvdb_mmap.size());
            std::memcpy(_ssbo_ptr + block_number * _ssbo_block_size, dvdb_mmap.data(), dvdb_mmap.size());
        }
        break;
        case dvdb::frame_type_e::DIFF_FRAME: {
            offsets[0] = header->frames[0].base_tree_offset_start;

            for (size_t i = 1; i < header->vdb_grid_count; ++i)
            {
                offsets[i] = offsets[i - 1] + header->frames[i - 1].base_tree_final_size;
            }

            std::memcpy(_created_state.data(), dvdb_mmap.data(), sizeof(*header));

            for (size_t i = 0; i < header->vdb_grid_count; ++i)
            {
                std::memcpy(_created_state.data() + offsets[i], dvdb_mmap.data() + header->frames[i].base_tree_offset_start, header->frames[i].base_tree_copy_size);
            }

            const auto src_header = reinterpret_cast<const dvdb::header *>(_current_state.data());

            uint64_t src_offsets[4];

            src_offsets[0] = src_header->frames[0].base_tree_offset_start;
            src_offsets[1] = src_offsets[0] + src_header->frames[0].base_tree_final_size;
            src_offsets[2] = src_offsets[1] + src_header->frames[1].base_tree_final_size;
            src_offsets[3] = src_offsets[2] + src_header->frames[2].base_tree_final_size;

            for (size_t i = 0; i < header->vdb_grid_count; ++i)
            {
                void *src_grid = _current_state.data() + src_offsets[i];
                void *dst_grid = _created_state.data() + offsets[i];
                // removing constness is ok here
                void *diff_data = const_cast<char *>(dvdb_mmap.data()) + header->frames[i].diff_data_offset_start;

                grid_reconstruction_rle(diff_data, dst_grid, src_grid);
            }

            std::memcpy(_ssbo_ptr + block_number * _ssbo_block_size, _created_state.data(), header->vdb_required_size);
        }
        break;
        }

        auto copy_t2 = std::chrono::steady_clock::now();

        utils::update_copy_time(std::chrono::duration_cast<std::chrono::microseconds>(copy_t2 - copy_t1).count());

        return {
            .offsets = offsets,
            .size = copy_size,
        };
    };

    _ssbo_block_loaded[block_number] = ctx.generic_thread_pool().enqueue(std::move(task));

    while (!resource_locked)
    {
        std::this_thread::yield();
    }
}
} // namespace objects::vdb
