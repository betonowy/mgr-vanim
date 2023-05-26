#include "nano_vdb_resource.hpp"

#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/memory_counter.hpp>
#include <utils/nvdb_mmap.hpp>
#include <utils/thread_pool.hpp>
#include <utils/utf8_exception.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>

namespace objects::vdb
{
const char* nano_vdb_resource::class_name()
{
    return "NanoVDB Animation";
}

nano_vdb_resource::nano_vdb_resource(std::filesystem::path path)
    : _resource_directory(path)
{
    std::vector<std::pair<int, std::filesystem::path>> nvdb_files;

    using regex_type = std::basic_regex<std::filesystem::path::value_type>;

#if VANIM_WINDOWS
    static constexpr auto regex_pattern = L"^.*[\\/\\\\].+_(\\d+)\\.nvdb$";
    using match_type = std::wcmatch;
#else
    static constexpr auto regex_pattern = "^.*[\\/\\\\].+_(\\d+)\\.nvdb$";
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

        throw std::runtime_error("Can't decode frame number from path: " + path.string());
    };

    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().has_extension() && entry.path().extension() == ".nvdb")
        {
            nvdb_files.emplace_back(path_to_frame_number(entry), entry);
        }
    }

    if (nvdb_files.empty())
    {
        throw utils::utf8_exception(u8"No NanoVDB frames found at: " + path.u8string());
    }

    std::sort(nvdb_files.begin(), nvdb_files.end());

    _nvdb_frames = std::move(nvdb_files);
}

nano_vdb_resource::~nano_vdb_resource()
{
    utils::gpu_buffer_memory_deallocated(_ssbo_block_size * _ssbo_block_count);
    glUnmapNamedBuffer(_ssbo);
}

void nano_vdb_resource::init(scene::object_context &ctx)
{
    volume_resource_base::init(ctx);

    set_frame_rate(30.f);

    size_t max_buffer_size = 0;

    for (const auto &[n, path] : _nvdb_frames)
    {
        utils::nvdb_mmap nvdb_mmap(path.string());
        const auto &grids = nvdb_mmap.grids();

        size_t buffer_size = 0;

        std::vector<std::string> converted_names;

        for (const auto &[_, size, name, type] : grids)
        {
            buffer_size += size;

            std::string converted_name;

            switch (type)
            {
            case utils::nvdb_mmap::grid::type_size::unsupported:
                converted_name = "unsupported format";
                break;
            case utils::nvdb_mmap::grid::type_size::f32:
                converted_name = "float32";
                break;
            case utils::nvdb_mmap::grid::type_size::f16:
                converted_name = "float16";
                break;
            case utils::nvdb_mmap::grid::type_size::f8:
                converted_name = "float8";
                break;
            case utils::nvdb_mmap::grid::type_size::f4:
                converted_name = "float4";
                break;
            case utils::nvdb_mmap::grid::type_size::fn:
                converted_name = "variable bit float";
                break;
            }

            converted_name += ": ";
            converted_name += name;

            converted_names.emplace_back(std::move(converted_name));

            set_grid_names(std::move(converted_names));
        }

        if (buffer_size > max_buffer_size)
        {
            max_buffer_size = buffer_size;
        }

        _frames.emplace_back(frame{
            .path = path.string(),
            .block_number = 0,
            .number = _frames.size(),
        });
    }

    _ssbo_block_size = max_buffer_size;
    _ssbo_block_count = MAX_BLOCKS;

    utils::gpu_buffer_memory_allocated(_ssbo_block_size * _ssbo_block_count);

    glNamedBufferStorage(_ssbo, _ssbo_block_size * _ssbo_block_count, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    _ssbo_ptr = reinterpret_cast<std::byte *>(glMapNamedBufferRange(_ssbo, 0, _ssbo_block_size * MAX_BLOCKS, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));

    for (int i = 0; i < MAX_BLOCKS && i < _nvdb_frames.size(); ++i)
    {
        schedule_frame(ctx, i, i);
    }
}

void nano_vdb_resource::schedule_frame(scene::object_context &ctx, int block_number, int frame_number)
{
    auto wait_t1 = std::chrono::steady_clock::now();
    _ssbo_block_fences[block_number].client_wait(true);
    auto wait_t2 = std::chrono::steady_clock::now();

    utils::update_wait_time(std::chrono::duration_cast<std::chrono::microseconds>(wait_t2 - wait_t1).count());

    _ssbo_block_frame[block_number] = frame_number;
    _ssbo_timestamp[block_number] = std::chrono::steady_clock::now();

    std::function task = [this, sptr = shared_from_this(), block_number, frame_number]() -> update_range {
        glm::uvec4 offsets(~0);
        size_t copy_size = 0;

        auto map_t1 = std::chrono::steady_clock::now();
        utils::nvdb_mmap nvdb_file(_nvdb_frames[frame_number].second.string());
        const auto &grids = nvdb_file.grids();
        auto map_t2 = std::chrono::steady_clock::now();

        utils::update_map_time(std::chrono::duration_cast<std::chrono::microseconds>(map_t2 - map_t1).count());

        auto copy_t1 = std::chrono::steady_clock::now();
        for (size_t i = 0; i < grids.size(); ++i)
        {
            offsets[i] = copy_size;
            std::memcpy(_ssbo_ptr + copy_size + block_number * _ssbo_block_size, grids[i].ptr, grids[i].size);
            copy_size += grids[i].size;
        }
        auto copy_t2 = std::chrono::steady_clock::now();

        utils::update_copy_time(std::chrono::duration_cast<std::chrono::microseconds>(copy_t2 - copy_t1).count());

        return {
            .offsets = offsets,
            .size = copy_size,
        };
    };

    _ssbo_block_loaded[block_number] = ctx.generic_thread_pool().enqueue(std::move(task));
}
} // namespace objects::vdb
