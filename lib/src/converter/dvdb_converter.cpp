#include "dvdb_converter.hpp"
#include "dvdb_compressor.hpp"
#include "dvdb_converter_nvdb.hpp"
#include "nvdb_compressor.hpp"
#include "nvdb_converter.hpp"

#include <dvdb/common.hpp>
#include <dvdb/compression.hpp>
#include <dvdb/dct.hpp>
#include <dvdb/derivative.hpp>
#include <dvdb/rotate.hpp>
#include <dvdb/statistics.hpp>
#include <dvdb/transform.hpp>
#include <dvdb/types.hpp>

#include <utils/nvdb_mmap.hpp>
#include <utils/thread_pool.hpp>

#include <nanovdb/PNanoVDB.h>

#include <cassert>
#include <cstring>
#include <fstream>
#include <numeric>

#include "../test/dump.hpp"

namespace converter
{
struct dvdb_state
{
    std::mutex status_mtx;
    std::string status_string;
    std::string compression_string;

    size_t read_size = 0;
    std::atomic<size_t> written_size = 0;

    size_t leaves_total = 0;
    size_t leaves_processed = 0;

    size_t pending_compressions = 0;

    std::vector<uint8_t> _vdb_buffer;
    bool previous_was_empty = true;
};
} // namespace converter

namespace
{
static constexpr auto FORCE_KEYFRAME_SIZE = 4096;

size_t vdb_determine_leafless_copy_size_direct_ptr(const void *data)
{
    // Removing constness is absolutely OK, because data will not be modified there
    const pnanovdb_buf_t buf{.data = reinterpret_cast<uint32_t *>(const_cast<void *>(data))};
    const pnanovdb_grid_handle_t grid{}; // Zero initialized is ok, because buffers are aligned that way
    const pnanovdb_tree_handle_t tree = pnanovdb_grid_get_tree(buf, grid);

    return pnanovdb_tree_get_node_offset_leaf(buf, tree) + tree.address.byte_offset;
}

namespace details
{
size_t vdb_get_lower_leaf_count(pnanovdb_buf_t buf, pnanovdb_lower_handle_t lower)
{
    size_t count = 0;

    for (size_t i = 0; i < PNANOVDB_LOWER_TABLE_COUNT; ++i)
    {
        if (pnanovdb_lower_get_child_mask(buf, lower, i))
        {
            ++count;
        }
    }

    return count;
}

size_t vdb_get_upper_leaf_count(pnanovdb_buf_t buf, pnanovdb_upper_handle_t upper)
{
    size_t count = 0;

    for (size_t i = 0; i < PNANOVDB_UPPER_TABLE_COUNT; ++i)
    {
        if (pnanovdb_upper_get_child_mask(buf, upper, i))
        {
            count += vdb_get_lower_leaf_count(buf, pnanovdb_upper_get_child(PNANOVDB_GRID_TYPE_FP8, buf, upper, i));
        }
    }

    return count;
}

size_t vdb_get_tile_leaf_count(pnanovdb_buf_t buf, pnanovdb_root_handle_t root, pnanovdb_root_tile_handle_t tile)
{
    if (pnanovdb_int64_is_zero(pnanovdb_root_tile_get_child(buf, tile)))
    {
        return 0;
    }

    return vdb_get_upper_leaf_count(buf, pnanovdb_root_get_child(PNANOVDB_GRID_TYPE_FP8, buf, root, tile));
}
} // namespace details

size_t vdb_get_actual_leaf_count(pnanovdb_buf_t buf, pnanovdb_tree_handle_t tree)
{
    const auto root = pnanovdb_tree_get_root(buf, tree);
    const auto tile_count = pnanovdb_root_get_tile_count(buf, root);

    size_t count = 0;

    for (size_t i = 0; i < tile_count; ++i)
    {
        count += details::vdb_get_tile_leaf_count(buf, root, pnanovdb_root_get_tile(PNANOVDB_GRID_TYPE_FP8, root, i));
    }

    return count;
}

struct encoder_context
{
    static constexpr size_t src_center_index = 13;

    size_t written = 0;
    uint8_t buffer[1024];
    uint64_t key;

    dvdb::cube_888_mask *dst_mask, *final_mask;
    const dvdb::cube_888_mask *src_neighborhood_masks[27]{};
    dvdb::cube_888_i8 *dst, *final, dst_fmask;
    const dvdb::cube_888_i8 *src_neighborhood[27]{};
    dvdb::cube_888_f32 fdst, ffinal, fdst_fmask, fsrc_neighborhood[27];
    const dvdb::cube_888_f32 *fsrc_neighborhood_ptr[27]{};
    float *dst_min, *dst_quantum, *final_min, *final_quantum, src_min[27], src_quantum[27];
};

void vdb_encode(encoder_context *ctx, float max_error)
{
    {
        float min = 1e12, max = -1e12;

        for (int j = 0; j < std::size(ctx->dst->values); ++j)
        {
            const auto value = ctx->dst->values[j];

            if (value < min)
            {
                min = value;
            }

            if (value > max)
            {
                max = value;
            }
        }

        max_error = max_error * max_error * (max - min);
    }

    static constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
    static constexpr dvdb::cube_888_f32 empty_values_f32{};

    ctx->written = 0;

    const auto write = [&](const auto &object) {
        auto output = ctx->buffer + ctx->written;
        std::copy_n(reinterpret_cast<const uint8_t *>(&object), sizeof(object), output);
        ctx->written += sizeof(object);
    };

    {
        dvdb::decode_from_i8(ctx->dst, &ctx->fdst, *ctx->dst_quantum, *ctx->dst_min, 0xff);

        for (int i = 0; i < std::size(ctx->src_neighborhood); ++i)
        {
            dvdb::decode_from_i8(ctx->src_neighborhood[i], ctx->fsrc_neighborhood + i, ctx->src_quantum[i], ctx->src_min[i], 0xff);
            ctx->fsrc_neighborhood_ptr[i] = ctx->fsrc_neighborhood + i;
        }

        dvdb::from_i8(&ctx->dst_fmask, &ctx->fdst_fmask);
    }

    // determine if copy is even needed
    // float error_empty = dvdb::mean_squared_error_with_mask(&empty_values_f32, &ctx->fdst, &ctx->fdst_fmask);

    glm::ivec3 rotation{};

    // rotated source copy
    float error_rotation_only_float = dvdb::rotate_refill_find_astar(&ctx->fdst, {}, ctx->fsrc_neighborhood_ptr, &rotation.x, &rotation.y, &rotation.z);

    dvdb::cube_888_i8 processed_i8;
    dvdb::cube_888_f32 processed_f32;
    dvdb::cube_888_mask processed_mask;
    dvdb::cube_888_f32 processed_fmask;

    dvdb::rotate_refill(&processed_i8, ctx->src_neighborhood, rotation.x, rotation.y, rotation.z);
    dvdb::rotate_refill(&processed_mask, ctx->src_neighborhood_masks, rotation.x, rotation.y, rotation.z);
    dvdb::decode_from_i8(&processed_i8, &processed_f32, ctx->src_min[13], ctx->src_quantum[13], 0xff);
    processed_fmask = processed_mask.as_values<float, 1, 0>();

    float error_rotation_only = dvdb::mean_squared_error_with_mask(&ctx->fdst, &processed_f32, &ctx->fdst_fmask);

    if (true)
    {
        write(dvdb::code_points::setup{
            .has_source = true,
            .has_rotation = rotation != glm::ivec3(0),
        });

        write(dvdb::code_points::source_key{
            ctx->key,
        });

        if (rotation != glm::ivec3(0))
        {
            write(dvdb::code_points::rotation_offset{
                .x = static_cast<int16_t>(rotation.x),
                .y = static_cast<int16_t>(rotation.y),
                .z = static_cast<int16_t>(rotation.z),
            });
        }

        dvdb::encode_to_i8(&processed_f32, ctx->final, ctx->final_quantum, ctx->final_min, 0xff);
        *ctx->final_mask = processed_mask;

        return;
    }

    // BEWARE that past this point bitmask must be encoded if rotation isn't enough
    float fadd, fmul;

    dvdb::linear_regression_with_mask(&processed_f32, &ctx->fdst, &fadd, &fmul, &ctx->fdst_fmask);

    // Until I fix linear regression sometimes going wild
    if (std::abs(fadd) >= dvdb::code_points::fma::range || std::abs(fmul) >= dvdb::code_points::fma::range)
    {
        fadd = 0, fmul = 1;
    }

    // simulate precision loss
    {
        const auto [add, mul] = dvdb::code_points::fma::to_float(dvdb::code_points::fma::from_float(fadd, fmul));
        dvdb::fma(&processed_f32, &processed_f32, add, mul);
    }

    float error_rotation_fma_only = dvdb::mean_squared_error_with_mask(&processed_f32, &ctx->fdst, &ctx->fdst_fmask);

    if (false)
    {
        const bool encodes_rotation = rotation != glm::ivec3(0);

        write(dvdb::code_points::setup{
            .has_source = true,
            .has_rotation = encodes_rotation,
            .has_fma_and_new_mask = true,
        });

        write(dvdb::code_points::source_key{
            ctx->key,
        });

        if (encodes_rotation)
        {
            write(dvdb::code_points::rotation_offset{
                .x = static_cast<int16_t>(rotation.x),
                .y = static_cast<int16_t>(rotation.y),
                .z = static_cast<int16_t>(rotation.z),
            });
        }

        write(*ctx->dst_mask);

        write(dvdb::code_points::fma::from_float(fadd, fmul));

        dvdb::encode_to_i8(&processed_f32, ctx->final, ctx->final_quantum, ctx->final_min, 0xff);
        *ctx->final_mask = processed_mask;

        return;
    }

    dvdb::cube_888_i8 diff8;
    dvdb::cube_888_f32 diff, diff_decoded;
    float min, quantum, error_diff;
    uint8_t quantization;

    dvdb::sub(&ctx->fdst, &processed_f32, &diff);

    dvdb::cube_888_f32 post_diff;

    for (int i = 0x02; i <= 0xfe; ++i)
    {
        quantization = i;

        dvdb::encode_to_i8(&diff, &diff8, &quantum, &min, quantization);
        dvdb::decode_from_i8(&diff8, &diff_decoded, quantum, min, quantization);

        dvdb::add(&diff_decoded, &processed_f32, &post_diff);

        error_diff = dvdb::mean_squared_error_with_mask(&post_diff, &ctx->fdst, &ctx->fdst_fmask);

        if (error_diff <= max_error)
        {
            break;
        }
    }

    // Most elaborate encoding
    {
        write(dvdb::code_points::setup{
            .has_source = true,
            .has_rotation = rotation != glm::ivec3(0),
            .has_fma_and_new_mask = true,
            .has_values = true,
            .has_diff = true,
            .has_derivative = false,
            .has_dct = false,
            .has_map = true,
        });

        write(dvdb::code_points::source_key{
            ctx->key,
        });

        if (rotation != glm::ivec3(0))
        {
            write(dvdb::code_points::rotation_offset{
                .x = static_cast<int16_t>(rotation.x),
                .y = static_cast<int16_t>(rotation.y),
                .z = static_cast<int16_t>(rotation.z),
            });
        }

        write(*ctx->dst_mask);

        write(dvdb::code_points::fma::from_float(fadd, fmul));

        write(dvdb::code_points::quantization{
            quantization,
        });

        write(dvdb::code_points::map{
            .min = min,
            .quantum = quantum,
        });

        write(diff8);

        dvdb::encode_to_i8(&post_diff, ctx->final, ctx->final_quantum, ctx->final_min, 0xff);
        *ctx->final_mask = *ctx->dst_mask;
    }
}

std::vector<uint8_t> vdb_create_rle_diff(const void *src_state, const void *dst_state, void *final_state, converter::dvdb_state *state, std::shared_ptr<utils::thread_pool> thread_pool)
{
    static constexpr float max_error_base = 0.01f;

    converter::nvdb_reader src_reader{}, dst_reader{}, final_reader{};

    src_reader.initialize(const_cast<void *>(src_state));
    dst_reader.initialize(const_cast<void *>(dst_state));
    final_reader.initialize(final_state);

    state->leaves_total = dst_reader.leaf_count();
    state->leaves_processed = 0;

    std::vector<uint8_t> output_data;

    float empty_float{};
    const dvdb::cube_888_i8 empty_values{};
    const dvdb::cube_888_mask empty_mask{};

    float stat_min = 1e12, stat_max = -1e12;

    {
        std::lock_guard lock(state->status_mtx);
        state->status_string = "Adjusting expected error...";
    }

    for (int i = 0; i < dst_reader.leaf_count(); ++i)
    {
        const auto leaf_data = dst_reader.leaf_min_quantum(i);

        if (leaf_data.min < stat_min)
        {
            stat_min = leaf_data.min;
        }

        const auto calculated_max = leaf_data.min + leaf_data.quantum * 0xff;

        if (calculated_max > stat_max)
        {
            stat_max = calculated_max;
        }

        ++state->leaves_processed;
    }

    state->leaves_processed = 0;

    {
        std::lock_guard lock(state->status_mtx);
        state->status_string = "Dispatching leaf data processing...";
    }

    std::vector<encoder_context> thread_contexts(dst_reader.leaf_count());
    std::vector<std::future<void>> work_finished(dst_reader.leaf_count());

    for (int i = 0; i < dst_reader.leaf_count(); ++i)
    {
        auto &ctx = thread_contexts[i];

        ctx.dst_mask = dst_reader.leaf_bitmask_ptr(i);
        ctx.dst = dst_reader.leaf_table_ptr(i);
        ctx.fdst_fmask = ctx.dst_mask->as_values<float, 1, 0>();

        {
            const auto [min, quantum] = dst_reader.leaf_min_quantum_ptrs(i);
            ctx.dst_min = min;
            ctx.dst_quantum = quantum;
        }

        ctx.final_mask = final_reader.leaf_bitmask_ptr(i);
        ctx.final = final_reader.leaf_table_ptr(i);
        ctx.key = dst_reader.leaf_key(i);
        ctx.written = 0;

        {
            const auto [min, quantum] = final_reader.leaf_min_quantum_ptrs(i);
            ctx.final_min = min;
            ctx.final_quantum = quantum;
        }

        src_reader.leaf_neighbors(dst_reader.leaf_coord(i), ctx.src_neighborhood, ctx.src_neighborhood_masks, &empty_values, &empty_mask);

        for (int i = 0; i < std::size(ctx.src_neighborhood); ++i)
        {
            if (ctx.src_neighborhood[i] != &empty_values)
            {
                const auto [min, quantum] = src_reader.leaf_min_quantum_from_table(ctx.src_neighborhood[i]);
                ctx.src_min[i] = min;
                ctx.src_quantum[i] = quantum;
            }
            else
            {
                ctx.src_min[i] = {};
                ctx.src_quantum[i] = {};
            }
        }

        work_finished[i] = thread_pool->enqueue([ctx_ptr = &ctx]() { vdb_encode(ctx_ptr, max_error_base); });

        ++state->leaves_processed;
    }

    {
        std::lock_guard lock(state->status_mtx);
        state->status_string = "Main thread joined data processing workers...";
    }

    thread_pool->work_together();

    {
        std::lock_guard lock(state->status_mtx);
        state->status_string = "Finished! Acquiring processed data...";
    }

    state->leaves_processed = 0;

    for (int i = 0; i < dst_reader.leaf_count(); ++i)
    {
        work_finished[i].get();
        const auto &ctx = thread_contexts[i];

        if (ctx.written > sizeof(ctx.buffer))
        {
            throw std::runtime_error("Buffer overrun when writing encoded data!");
        }

        std::copy_n(ctx.buffer, ctx.written, std::back_inserter(output_data));
        ++state->leaves_processed;
    }

    state->leaves_total = 0;

    return output_data;
}
} // namespace

namespace converter
{
dvdb_converter::dvdb_converter(std::shared_ptr<utils::thread_pool> pool)
    : _thread_pool(std::move(pool)), _state(std::make_unique<dvdb_state>())
{
}

dvdb_converter::~dvdb_converter() = default;

std::string dvdb_converter::current_processing_step()
{
    std::lock_guard lock(_state->status_mtx);
    return _state->status_string;
}

std::string dvdb_converter::current_compression_step()
{
    std::lock_guard lock(_state->status_mtx);
    return _state->compression_string;
}

void dvdb_converter::set_status(std::string str)
{
    std::lock_guard lock(_state->status_mtx);
    _state->status_string = std::move(str);
}

void dvdb_converter::change_compression_status(int diff)
{
    std::lock_guard lock(_state->status_mtx);
    _state->pending_compressions += diff;
    _state->compression_string = "Pending files to compress (LZ4): " + std::to_string(_state->pending_compressions);
}

void dvdb_converter::create_keyframe(std::filesystem::path path)
{
    set_status("Rewriting keyframe:\n  " + path.string());

    int alignment_correction = 0;
    const auto nvdb_buffer = converter::unpack_nvdb_file(path.c_str(), &alignment_correction);

    const auto fp8nvdb = converter::nvdb_to_nvdb_fp8(nvdb_buffer.data() + alignment_correction, nvdb_buffer.size() - alignment_correction);

    utils::nvdb_mmap nvdb_mmap(fp8nvdb.data());
    _state->read_size += nvdb_mmap.mem_size();

    auto dvdb_path = path;
    dvdb_path.replace_extension(".dvdb");

    {
        dvdb::headers::main header = {
            .magic = dvdb::MAGIC_NUMBER,
            .frame_type = dvdb::headers::main::frame_type_e::KEY_FRAME,
            .vdb_grid_count = nvdb_mmap.grids().size(),
            .vdb_required_size = sizeof(header),
            .frames = {},
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

        if (header.vdb_required_size < FORCE_KEYFRAME_SIZE)
        {
            _state->previous_was_empty = true;
        }
    }

    set_status("Verifying:\n  " + dvdb_path.string());

    // Briefly verify contents and update current state
    {
        mio::mmap_source input(dvdb_path.string());

        const auto header = reinterpret_cast<const dvdb::headers::main *>(input.data());

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

    change_compression_status(1);

    _thread_pool->enqueue([weak = std::weak_ptr(_state), this, dvdb_path]() {
        auto lock = weak.lock();
        _state->written_size += pack_dvdb_file(dvdb_path.c_str());
        change_compression_status(-1);
    });
}

void dvdb_converter::add_diff_frame(std::filesystem::path path)
{
    set_status("Processing interframe:\n  " + path.string());

    int alignment_correction = 0;
    const auto nvdb_buffer = converter::unpack_nvdb_file(path.c_str(), &alignment_correction);

    const auto fp8nvdb = converter::nvdb_to_nvdb_fp8(nvdb_buffer.data() + alignment_correction, nvdb_buffer.size() - alignment_correction);

    utils::nvdb_mmap nvdb_mmap(fp8nvdb.data());
    _state->read_size += nvdb_mmap.mem_size();

    auto dvdb_path = path;
    dvdb_path.replace_extension(".dvdb");

    const auto current_state_header = reinterpret_cast<const dvdb::headers::main *>(_state->_vdb_buffer.data());

    // First frame, make keyframe first
    if (!current_state_header)
    {
        return create_keyframe(path);
    }

    dvdb::headers::main next_state_header = {
        .magic = dvdb::MAGIC_NUMBER,
        .frame_type = dvdb::headers::main::frame_type_e::DIFF_FRAME,
        .vdb_grid_count = current_state_header->vdb_grid_count,
        .vdb_required_size = sizeof(next_state_header),
        .frames = {}};

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        const auto &grid = nvdb_mmap.grids()[i];
        const auto leafless_size = vdb_determine_leafless_copy_size_direct_ptr(grid.ptr);

        next_state_header.frames[i].base_tree_offset_start = next_state_header.vdb_required_size;
        next_state_header.frames[i].base_tree_copy_size = leafless_size;
        next_state_header.frames[i].base_tree_final_size = grid.size;
        next_state_header.vdb_required_size += grid.size;
    }

    // look this tree is probably leafless and has no values. Diff breaks on this so just do it the normal way...
    if (bool is_empty = next_state_header.vdb_required_size < FORCE_KEYFRAME_SIZE; is_empty || _state->previous_was_empty)
    {
        _state->previous_was_empty = is_empty;
        return create_keyframe(path);
    }
    else
    {
        _state->previous_was_empty = false;
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

        set_status("Creating diff (grid " + std::to_string(i) + ")\n  " + dvdb_path.string());

        diff_data_chunks.emplace_back(vdb_create_rle_diff(source_state_ptr, destination_state_ptr, diff_state_ptr, _state.get(), _thread_pool));
    }

    set_status("Realigning data\n  " + dvdb_path.string());

    dvdb::headers::main compressed_header = next_state_header;

    uint64_t compressed_base_tree_offset = sizeof(compressed_header);

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        const auto &grid = nvdb_mmap.grids()[i];
        const auto leafless_size = vdb_determine_leafless_copy_size_direct_ptr(grid.ptr);

        compressed_header.frames[i].base_tree_offset_start = compressed_base_tree_offset;
        compressed_header.frames[i].base_tree_copy_size = leafless_size;
        compressed_header.frames[i].base_tree_final_size = grid.size;
        // compressed_header.vdb_required_size += grid.size;

        compressed_base_tree_offset += leafless_size;
    }

    for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    {
        compressed_header.frames[i].diff_data_offset_start = compressed_base_tree_offset;
        compressed_base_tree_offset += diff_data_chunks[i].size();
    }

    {
        std::ofstream output(dvdb_path.string(), std::ios::binary);

        output.write(reinterpret_cast<const char *>(&compressed_header), sizeof(compressed_header));

        set_status("Writing grids\n  " + dvdb_path.string());

        for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
        {
            const auto &grid = nvdb_mmap.grids()[i];

            output.write(reinterpret_cast<const char *>(grid.ptr), compressed_header.frames[i].base_tree_copy_size);
        }

        set_status("Writing diffs\n  " + dvdb_path.string());

        for (size_t i = 0; i < diff_data_chunks.size(); ++i)
        {
            output.write(reinterpret_cast<const char *>(diff_data_chunks[i].data()), diff_data_chunks[i].size());
        }
    }

    _state->_vdb_buffer = std::move(next_buffer);

    change_compression_status(1);

    _thread_pool->enqueue([weak = std::weak_ptr(_state), this, dvdb_path]() {
        auto lock = weak.lock();
        _state->written_size += pack_dvdb_file(dvdb_path.c_str());
        change_compression_status(-1);
    });
}

float dvdb_converter::current_compression_ratio()
{
    if (_state->written_size == 0)
    {
        return 0;
    }

    return static_cast<float>(_state->read_size) / _state->written_size;
}

size_t dvdb_converter::current_total_leaves()
{
    return _state->leaves_total;
}

size_t dvdb_converter::current_processed_leaves()
{
    return _state->leaves_processed;
}

bool dvdb_converter::finished()
{
    return _state->pending_compressions == 0;
}
} // namespace converter
