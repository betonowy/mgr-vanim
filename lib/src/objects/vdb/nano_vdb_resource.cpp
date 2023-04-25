#include "nano_vdb_resource.hpp"

#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/memory_counter.hpp>
#include <utils/nvdb_mmap.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <regex>

namespace objects::vdb
{
nano_vdb_resource::nano_vdb_resource(std::filesystem::path path) : _resource_directory(path)
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

    auto path_to_frame_number = [r = regex_type(regex_pattern)](const std::filesystem::path &path) -> int {
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
        throw std::runtime_error("No NanoVDB frames found at: " + path.string());
    }

    std::sort(nvdb_files.begin(), nvdb_files.end());

    _nvdb_frames = std::move(nvdb_files);
}

nano_vdb_resource::~nano_vdb_resource()
{
    glUnmapNamedBuffer(_ssbo);
}

void nano_vdb_resource::init(scene::object_context &ctx)
{
    volume_resource_base::init(ctx);

    set_frame_rate(30.f);
    // set_preload_hint(0.035);

    max_buffer_size = 0;

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
            case utils::nvdb_mmap::grid::type_size::f32:
                converted_name = "float32";
            case utils::nvdb_mmap::grid::type_size::f16:
                converted_name = "float16";
            case utils::nvdb_mmap::grid::type_size::f8:
                converted_name = "float8";
            case utils::nvdb_mmap::grid::type_size::f4:
                converted_name = "float4";
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
            .path = path,
            .block_number = 0,
            .number = _frames.size(),
        });
    }

    _ssbo_block_size = max_buffer_size;
    _ssbo_block_count = _MAX_BLOCKS;

    utils::gpu_buffer_memory_allocated(_ssbo_block_size * _ssbo_block_count);

    glNamedBufferStorage(_ssbo, _ssbo_block_size * _ssbo_block_count, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    _ssbo_ptr = reinterpret_cast<std::byte *>(glMapNamedBufferRange(_ssbo, 0, _ssbo_block_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));

    {
        utils::nvdb_mmap nvdb_file(_nvdb_frames[0].second.string());

        glm::uvec4 offsets(~0);

        const auto &grids = nvdb_file.grids();

        size_t offset = 0;

        for (size_t i = 0; i < grids.size(); ++i)
        {
            offsets[i] = offset;
            std::memcpy(_ssbo_ptr + offset, grids[i].ptr, grids[i].size);
            offset += grids[i].size;
        }

        _world_data->set_vdb_data_offsets(offsets);
    }

    glFlushMappedNamedBufferRange(_ssbo, 0, _ssbo_block_size);
}

namespace
{
bool is_unload_safe(int range_min, int range_max, int current_frame, int frames_ahead, int frame)
{
    int load_range = current_frame + frames_ahead;

    if (frame >= current_frame && frame <= load_range)
    {
        return false;
    }

    if (load_range >= range_max)
    {
        load_range %= range_max;

        if (frame <= load_range)
        {
            return false;
        }
    }

    return true;
}
} // namespace

void nano_vdb_resource::update(scene::object_context &ctx, float delta_time)
{
    if (_ssbo_block_frame[0] != _current_frame)
    {
        utils::nvdb_mmap nvdb_file(_nvdb_frames[_current_frame].second.string());

        glm::uvec4 offsets(~0);

        const auto &grids = nvdb_file.grids();

        size_t copy_size = 0;

        _ssbo_block_fences[0].client_wait(true);

        for (size_t i = 0; i < grids.size(); ++i)
        {
            offsets[i] = copy_size;
            std::memcpy(_ssbo_ptr + copy_size, grids[i].ptr, grids[i].size);
            copy_size += grids[i].size;
        }

        _world_data->set_vdb_data_offsets(offsets);

        glFlushMappedNamedBufferRange(_ssbo, 0, copy_size);

        _ssbo_block_frame[0] = _current_frame;
    }

    // int frame_rate = get_frame_rate();
    // int frames_ahead = get_preload_hint() * frame_rate + 1;
    // int current_frame = get_current_time() * frame_rate;
    // int range_min = get_min_time() * frame_rate;
    // int range_max = get_max_time() * frame_rate;

    // {
    //     auto is_invalid = [](const std::future<frame> &future) -> bool { return !future.valid(); };
    //     auto begin = _incoming_frames.begin();
    //     auto end = _incoming_frames.end();

    //     _incoming_frames.erase(std::remove_if(begin, end, is_invalid), end);
    // }

    // for (auto &incoming_frame : _incoming_frames)
    // {
    //     if (utils::is_ready(incoming_frame))
    //     {
    //         auto data = incoming_frame.get();
    //         _loaded_frames.push_back(std::move(data));
    //         _scheduled_frames.erase(std::find(_scheduled_frames.begin(), _scheduled_frames.end(), data.frame_number));
    //     }
    // }

    // {
    //     auto is_invalid = [](const std::future<frame> &future) -> bool { return !future.valid(); };
    //     auto begin = _incoming_frames.begin();
    //     auto end = _incoming_frames.end();

    //     _incoming_frames.erase(std::remove_if(begin, end, is_invalid), end);
    // }

    // if (!get_no_unload())
    // {
    //     auto begin = _loaded_frames.begin();
    //     auto end = _loaded_frames.end();

    //     for (auto &frame : _loaded_frames)
    //     {
    //         if (is_unload_safe(range_min, range_max, current_frame, frames_ahead, frame.frame_number))
    //         {
    //             _free_frames.push_back(std::move(frame));
    //         }
    //     }

    //     auto is_empty = [](const frame &frame) { return !frame.ssbo; };

    //     _loaded_frames.erase(std::remove_if(begin, end, is_empty), end);
    // }

    // while (frame_count < frames_ahead)
    // {
    //     _free_frames.push_back(frame{.ssbo = std::make_shared<gl::shader_storage>()});
    //     ++frame_count;
    // }

    // while (frame_count > frames_ahead && !_free_frames.empty())
    // {
    //     _free_frames.pop_back();
    //     --frame_count;
    // }

    // int load_range_start = current_frame;
    // int load_range_end = current_frame + frames_ahead;

    // auto &load_range = _temp_int_vector;
    // load_range.clear();

    // for (int i = load_range_start; i <= load_range_end; ++i)
    // {
    //     load_range.push_back(i % range_max);
    // }

    // auto no_load_needed = [&](const int frame) -> bool {
    //     for (const auto &loaded_frame : _loaded_frames)
    //     {
    //         if (loaded_frame.frame_number == frame)
    //         {
    //             return true;
    //         }
    //     }

    //     for (const auto &scheduled_frame : _scheduled_frames)
    //     {
    //         if (scheduled_frame == frame)
    //         {
    //             return true;
    //         }
    //     }

    //     return false;
    // };

    // load_range.erase(std::remove_if(load_range.begin(), load_range.end(), no_load_needed), load_range.end());

    // auto find_current_frame = [current_frame](const frame &frame) -> bool { return frame.frame_number == current_frame; };

    // auto found = std::find_if(_loaded_frames.begin(), _loaded_frames.end(), find_current_frame);

    // if (found != _loaded_frames.end())
    // {
    //     set_next_volume_data(found->ssbo, found->offsets);
    // }
    // else if (!_incoming_frames.empty() &&
    //          std::find(_scheduled_frames.begin(), _scheduled_frames.end(), current_frame) != _scheduled_frames.end())
    // {
    //     signal_force_load();

    //     bool frame_found = false;

    //     do
    //     {
    //         assert(!_incoming_frames.empty());

    //         auto data = _incoming_frames.front().get();
    //         _incoming_frames.erase(_incoming_frames.begin());
    //         _scheduled_frames.erase(std::find(_scheduled_frames.begin(), _scheduled_frames.end(), data.frame_number));

    //         if (current_frame == data.frame_number)
    //         {
    //             frame_found = true;
    //             set_next_volume_data(data.ssbo, data.offsets);
    //         }

    //         _loaded_frames.push_back(std::move(data));
    //     } while (!frame_found && !_incoming_frames.empty());
    // }
    // else
    // {
    //     signal_ad_hoc_load();

    //     if (get_no_unload())
    //     {
    //         _free_frames.push_back(frame{.ssbo = std::make_shared<gl::shader_storage>()});
    //         ++frame_count;
    //     }

    //     if (!_free_frames.empty())
    //     {
    //         auto data = std::move(_free_frames.back());
    //         data.offsets = glm::uvec4(~0);
    //         data.frame_number = current_frame;

    //         const auto nvdb_mmap = utils::nvdb_mmap(_nvdb_frames[current_frame].second.string());
    //         const auto &grids = nvdb_mmap.grids();
    //         size_t grids_size = 0;

    //         for (size_t i = 0; i < nvdb_mmap.grids().size(); ++i)
    //         {
    //             data.offsets[i] = grids_size;
    //             grids_size += grids[i].size;
    //             assert(grids_size % 4 == 0);
    //         }

    //         data.ssbo->buffer_data_grow(nullptr, grids_size, GL_STREAM_DRAW);

    //         for (size_t i = 0; i < grids.size(); ++i)
    //         {
    //             data.ssbo->buffer_sub_data(grids[i].ptr, grids[i].size, data.offsets[i]);
    //         }

    //         glFlush();

    //         set_next_volume_data(data.ssbo, data.offsets);

    //         auto remove_cond = [current_frame](int i) -> bool { return i == current_frame; };

    //         load_range.erase(std::remove_if(load_range.begin(), load_range.end(), remove_cond), load_range.end());

    //         _loaded_frames.push_back(std::move(data));

    //         _free_frames.pop_back();
    //     }
    // }

    // if (get_play_animation())
    // {
    //     for (const auto &n : load_range)
    //     {
    //         if (get_no_unload())
    //         {
    //             _free_frames.push_back(frame{.ssbo = std::make_shared<gl::shader_storage>()});
    //             ++frame_count;
    //         }

    //         if (_free_frames.empty())
    //         {
    //             break;
    //         }

    //         std::function load_task = [path = _nvdb_frames[n].second, n = n, data = std::move(_free_frames.back())]() mutable -> frame {
    //             data.frame_number = n;
    //             data.offsets = glm::uvec4(~0);

    //             const auto nvdb_mmap = utils::nvdb_mmap(path.string());
    //             const auto &grids = nvdb_mmap.grids();
    //             size_t grids_size = 0;

    //             for (size_t i = 0; i < grids.size(); ++i)
    //             {
    //                 data.offsets[i] = grids_size;
    //                 grids_size += grids[i].size;
    //                 assert(grids_size % 4 == 0);
    //             }

    //             data.ssbo->buffer_data_grow(nullptr, grids_size, GL_STREAM_DRAW);

    //             for (size_t i = 0; i < grids.size(); ++i)
    //             {
    //                 data.ssbo->buffer_sub_data(grids[i].ptr, grids[i].size, data.offsets[i]);
    //             }

    //             glFlush();

    //             return data;
    //         };

    //         _free_frames.pop_back();
    //         _incoming_frames.push_back(ctx.gl_thread().enqueue(load_task));
    //         _scheduled_frames.push_back(n);
    //     }
    // }

    volume_resource_base::update(ctx, delta_time);
}
} // namespace objects::vdb
