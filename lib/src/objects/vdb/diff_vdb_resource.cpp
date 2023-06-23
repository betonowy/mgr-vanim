#include "diff_vdb_resource.hpp"

#include <converter/dvdb_compressor.hpp>
#include <converter/dvdb_converter_nvdb.hpp>

#include <dvdb/common.hpp>
#include <dvdb/compression.hpp>
#include <dvdb/dct.hpp>
#include <dvdb/derivative.hpp>
#include <dvdb/rotate.hpp>
#include <dvdb/statistics.hpp>
#include <dvdb/types.hpp>
#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/gpu_memcpy.hpp>
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

#include "../test/dump.hpp"

namespace objects::vdb
{
const char *diff_vdb_resource::class_name()
{
    return "DiffVDB Animation";
}

void diff_vdb_resource::on_destroy(scene::object_context &ctx)
{
    ctx.generic_thread_pool().finish();
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
        mio::mmap_source mmap(path.c_str());

        const auto header = reinterpret_cast<const dvdb::headers::block_description *>(mmap.data());

        if (header->uncompressed_size > max_buffer_size)
        {
            max_buffer_size = header->uncompressed_size;
        }

        _frames.emplace_back(frame{
            .path = path.string(),
            .block_number = 0,
            .number = _frames.size(),
        });
    }

    static constexpr auto ALIGNMENT = 64;

    max_buffer_size += ALIGNMENT - (max_buffer_size & (ALIGNMENT - 1));

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

template <typename T>
T read(uint8_t *&data)
{
    T obj = *reinterpret_cast<T *>(data);
    data += sizeof(obj);
    return obj;
}

static dvdb::cube_888_mask empty_mask{};
static dvdb::cube_888_f32 empty_values{};

void grid_reconstruction_worker(int index, void *diff_ptr, int bundle_size, const converter::nvdb_reader &dst_accessor, const converter::nvdb_reader &src_accessor)
{
    auto *diff_current_ptr = reinterpret_cast<uint8_t *>(diff_ptr);

    dvdb::cube_888_f32 *src_neighbor_values_ptrs[27];
    dvdb::cube_888_mask *src_neighbor_masks_ptrs[27];

    for (int i = 0; i < bundle_size; ++i)
    {
        const auto setup = read<dvdb::code_points::setup>(diff_current_ptr);

        const auto setup_as_byte = *reinterpret_cast<const uint8_t *>(&setup);

        auto dst_ptr = dst_accessor.leaf_table_ptr(i + index);
        auto dst_mask_ptr = dst_accessor.leaf_bitmask_ptr(i + index);

        dvdb::cube_888_f32 dst;
        dvdb::cube_888_mask dst_mask;

        if (setup.has_source && setup.has_rotation)
        {
            const auto source = read<dvdb::code_points::source_key>(diff_current_ptr);

            const auto [x, y, z] = read<dvdb::code_points::rotation_offset>(diff_current_ptr);

            src_accessor.leaf_neighbors(source, src_neighbor_values_ptrs, src_neighbor_masks_ptrs, &empty_values, &empty_mask);

            dvdb::rotate_refill(&dst, src_neighbor_values_ptrs, x, y, z);

            if (!setup.has_fma_and_new_mask)
            {
                dvdb::rotate_refill(&dst_mask, src_neighbor_masks_ptrs, x, y, z);
            }
        }
        else if (setup.has_source)
        {
            const auto source = read<dvdb::code_points::source_key>(diff_current_ptr);
            const auto index = src_accessor.get_leaf_index_from_key(source);

            if (index == -1)
            {
                dst = {};
                dst_mask = {};
            }
            else
            {
                const auto src = src_accessor.leaf_table_ptr(index);
                const auto src_mask = src_accessor.leaf_bitmask_ptr(index);

                dst = *src;

                if (!setup.has_fma_and_new_mask)
                {
                    dst_mask = *src_mask;
                }
            }
        }
        else
        {
            dst = {};

            if (!setup.has_fma_and_new_mask)
            {
                dst_mask = {};
            }
        }

        if (setup.has_fma_and_new_mask)
        {
            const auto [add, mul] = dvdb::code_points::fma::to_float(read<dvdb::code_points::fma>(diff_current_ptr));
            dvdb::fma(&dst, &dst, add, mul);

            dst_mask = read<dvdb::cube_888_mask>(diff_current_ptr);
        }

        *dst_ptr = dst;
        *dst_mask_ptr = dst_mask;

        if (!setup.has_values)
        {

            continue;
        }

        const auto [quantization] = read<dvdb::code_points::quantization>(diff_current_ptr);

        const auto [min, max] = [&]() {
            if (setup.has_map)
            {
                return read<dvdb::code_points::map>(diff_current_ptr);
            }

            return dvdb::code_points::map{.min = 0.f, .max = 1.f};
        }();

        dvdb::cube_888_i8 encoder_values = read<dvdb::cube_888_i8>(diff_current_ptr);
        dvdb::cube_888_f32 encoder_float_values{};

        if (setup.has_derivative)
        {
            dvdb::decode_derivative_from_i8(&encoder_values, &encoder_float_values, max, min, quantization);
        }
        else
        {
            dvdb::decode_from_i8(&encoder_values, &encoder_float_values, max, min, quantization);
        }

        dvdb::cube_888_f32 values;

        if (setup.has_dct)
        {
            dvdb::dct_3d_decode(&encoder_float_values, &values);
        }
        else
        {
            values = encoder_float_values;
        }

        if (setup.has_diff)
        {
            dvdb::add(&dst, &values, dst_ptr);
        }
        else
        {
            *dst_ptr = values;
        }

        *dst_mask_ptr = dst_mask;
    }
}

int get_one_diff_size(void *ptr)
{
    auto *moving_ptr = reinterpret_cast<uint8_t *>(ptr);
    int size = sizeof(dvdb::code_points::setup);

    const auto setup = read<dvdb::code_points::setup>(moving_ptr);

    if (setup.has_source)
    {
        size += sizeof(dvdb::code_points::source_key);
    }

    if (setup.has_rotation)
    {
        size += sizeof(dvdb::code_points::rotation_offset);
    }

    if (setup.has_fma_and_new_mask)
    {
        size += sizeof(dvdb::code_points::fma) + sizeof(dvdb::cube_888_mask);
    }

    if (setup.has_values)
    {
        size += sizeof(dvdb::code_points::quantization) + sizeof(dvdb::cube_888_i8);
    }

    if (setup.has_map)
    {
        size += sizeof(dvdb::code_points::map);
    }

    return size;
}

int get_next_bundle_size(void *ptr, int bundle_size)
{
    int size = 0;

    for (int i = 0; i < bundle_size; ++i)
    {
        int next_size = get_one_diff_size(ptr);
        size += next_size;
        ptr = static_cast<char *>(ptr) + next_size;
    }

    return size;
}

void grid_reconstruction(void *diff_ptr, void *dst_ptr, void *src_ptr, utils::thread_pool *thread_pool)
{
    converter::nvdb_reader dst_accessor, src_accessor;

    dst_accessor.initialize(dst_ptr);
    src_accessor.initialize(src_ptr);

    auto *diff_moving_ptr = reinterpret_cast<uint8_t *>(diff_ptr);

    dvdb::cube_888_f32 *src_neighbor_values_ptrs[27];
    dvdb::cube_888_mask *src_neighbor_masks_ptrs[27];

    int dst_leaf_count = dst_accessor.leaf_count();
    int dst_leaf_current = 0;
    int expected_bundle_size = dst_leaf_count / thread_pool->worker_count();

    std::vector<std::future<void>> signals;
    signals.reserve(thread_pool->worker_count() + 1);

    while (dst_leaf_current + expected_bundle_size < dst_leaf_count)
    {
        const auto next_bundle_size = get_next_bundle_size(diff_moving_ptr, expected_bundle_size);

        signals.emplace_back(thread_pool->enqueue([=]() {
            grid_reconstruction_worker(dst_leaf_current, diff_moving_ptr, expected_bundle_size, dst_accessor, src_accessor);
        }));

        dst_leaf_current += expected_bundle_size;
        diff_moving_ptr = diff_moving_ptr + next_bundle_size;
    }

    if (dst_leaf_current < dst_leaf_count)
    {
        signals.emplace_back(thread_pool->enqueue([=]() {
            grid_reconstruction_worker(dst_leaf_current, diff_moving_ptr, dst_leaf_count - dst_leaf_current, dst_accessor, src_accessor);
        }));
    }

    thread_pool->work_together();

    for (auto &signal : signals)
    {
        signal.get();
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

    { // lock to make sure workers finished their previous workload
        std::lock_guard lock(_state_modification_mtx);
        std::swap(_created_state, _current_state);
    }

    std::atomic_bool worker_side_locked = false;

    std::function task = [this, wptr = weak_from_this(), frame_number, block_number, &worker_side_locked, wtp = std::weak_ptr(ctx.generic_thread_pool_sptr())]() -> update_range {
        glm::uvec4 offsets(~0);
        size_t copy_size = 0;

        auto sptr = wptr.lock();
        auto thread_pool = wtp.lock();

        if (!sptr)
        {
            return {};
        }

        std::lock_guard lock(_state_modification_mtx);
        worker_side_locked = true;

        auto map_t1 = std::chrono::steady_clock::now();
        const auto source_buffer = converter::unpack_dvdb_file(_dvdb_frames[frame_number].second.c_str());

        const auto header = reinterpret_cast<const dvdb::headers::main *>(source_buffer.data());

        if (header->magic != dvdb::MAGIC_NUMBER)
        {
            throw std::runtime_error("DiffVDB magic number failed");
        }

        copy_size = header->vdb_required_size;
        auto map_t2 = std::chrono::steady_clock::now();

        utils::update_map_time(std::chrono::duration_cast<std::chrono::microseconds>(map_t2 - map_t1).count());

        auto copy_t1 = std::chrono::steady_clock::now();

        switch (header->frame_type)
        {
        case dvdb::headers::main::frame_type_e::KEY_FRAME: {
            for (size_t i = 0; i < header->vdb_grid_count; ++i)
            {
                offsets[i] = header->frames[i].base_tree_offset_start;

                if (offsets[i] % 16 != 0)
                {
                    throw std::runtime_error("Bad alignment");
                }
            }

            _created_state = std::move(source_buffer);
            utils::gpu_memcpy(_ssbo_ptr + block_number * _ssbo_block_size, _created_state.data(), _created_state.size());
        }
        break;
        case dvdb::headers::main::frame_type_e::DIFF_FRAME: {
            offsets[0] = header->frames[0].base_tree_offset_start;

            for (size_t i = 1; i < header->vdb_grid_count; ++i)
            {
                offsets[i] = offsets[i - 1] + header->frames[i - 1].base_tree_final_size;
            }

            std::memcpy(_created_state.data(), source_buffer.data(), sizeof(*header));

            for (size_t i = 0; i < header->vdb_grid_count; ++i)
            {
                std::memcpy(_created_state.data() + offsets[i], source_buffer.data() + header->frames[i].base_tree_offset_start, header->frames[i].base_tree_copy_size);
            }

            const auto src_header = reinterpret_cast<const dvdb::headers::main *>(_current_state.data());

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
                void *diff_data = const_cast<char *>(source_buffer.data()) + header->frames[i].diff_data_offset_start;

                if (frame_number == 179)
                {
                    int _ = 0;
                }

                grid_reconstruction(diff_data, dst_grid, src_grid, thread_pool.get());
            }

            utils::gpu_memcpy(_ssbo_ptr + block_number * _ssbo_block_size, _created_state.data(), header->vdb_required_size);
        }
        break;
        default:
            throw std::runtime_error("Invalid frame type! Corrupted data?");
        }

        auto copy_t2 = std::chrono::steady_clock::now();

        utils::update_copy_time(std::chrono::duration_cast<std::chrono::microseconds>(copy_t2 - copy_t1).count());

        return {
            .offsets = offsets,
            .size = copy_size,
        };
    };

    _ssbo_block_loaded[block_number] = ctx.generic_thread_pool().enqueue(std::move(task));

    while (!worker_side_locked)
    {
        std::this_thread::yield();
    }
}
} // namespace objects::vdb
