#include "nano_vdb_resource.hpp"

#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/gl_resource_thread.hpp>

#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>

#include <algorithm>
#include <iostream>
#include <regex>

namespace objects::vdb
{
nano_vdb_resource::nano_vdb_resource(std::filesystem::path path) : _resource_directory(path)
{
    std::vector<std::pair<int, std::filesystem::path>> nvdb_files;

    auto path_to_frame_number = [r = std::regex("^.*[\\/\\\\].+_(\\d+)\\.nvdb$")](const std::filesystem::path &path) -> int {
        std::cmatch cm;

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

void nano_vdb_resource::init(scene::object_context &ctx)
{
    set_frame_rate(30.f);
    set_frame_range(0, _nvdb_frames.size() / get_frame_rate());
    set_preload_hint(0.035);

    volume_resource_base::init(ctx);
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
    {
        auto is_invalid = [](const std::future<frame> &future) -> bool { return !future.valid(); };
        auto begin = _incoming_frames.begin();
        auto end = _incoming_frames.end();

        _incoming_frames.erase(std::remove_if(begin, end, is_invalid), end);
    }

    for (auto &incoming_frame : _incoming_frames)
    {
        if (utils::is_ready(incoming_frame))
        {
            auto data = incoming_frame.get();
            _loaded_frames.push_back(std::move(data));
            _scheduled_frames.erase(std::find(_scheduled_frames.begin(), _scheduled_frames.end(), data.frame_number));
        }
    }

    {
        auto is_invalid = [](const std::future<frame> &future) -> bool { return !future.valid(); };
        auto begin = _incoming_frames.begin();
        auto end = _incoming_frames.end();

        _incoming_frames.erase(std::remove_if(begin, end, is_invalid), end);
    }

    int frame_rate = get_frame_rate();
    int frames_ahead = get_preload_hint() * frame_rate;
    int current_frame = get_current_time() * frame_rate;
    int range_min = get_min_time() * frame_rate;
    int range_max = get_max_time() * frame_rate;

    if (!get_no_unload())
    {
        auto unload_condition = [&](const frame &frame) -> bool {
            return is_unload_safe(range_min, range_max, current_frame, frames_ahead, frame.frame_number);
        };

        auto begin = _loaded_frames.begin();
        auto end = _loaded_frames.end();

        _loaded_frames.erase(std::remove_if(begin, end, unload_condition), end);
    }

    int load_range_start = current_frame;
    int load_range_end = current_frame + frames_ahead;

    auto &load_range = _temp_int_vector;
    load_range.clear();

    for (int i = load_range_start; i <= load_range_end; ++i)
    {
        load_range.push_back(i % range_max);
    }

    auto no_load_needed = [&](const int frame) -> bool {
        for (const auto &loaded_frame : _loaded_frames)
        {
            if (loaded_frame.frame_number == frame)
            {
                return true;
            }
        }

        for (const auto &scheduled_frame : _scheduled_frames)
        {
            if (scheduled_frame == frame)
            {
                return true;
            }
        }

        return false;
    };

    load_range.erase(std::remove_if(load_range.begin(), load_range.end(), no_load_needed), load_range.end());

    auto find_current_frame = [current_frame](const frame &frame) -> bool { return frame.frame_number == current_frame; };

    auto found = std::find_if(_loaded_frames.begin(), _loaded_frames.end(), find_current_frame);

    if (found != _loaded_frames.end())
    {
        set_next_volume_data(found->ssbo, found->offsets);
    }
    else if (!_incoming_frames.empty() &&
             std::find(_scheduled_frames.begin(), _scheduled_frames.end(), current_frame) != _scheduled_frames.end())
    {
        signal_force_load();

        bool frame_found = false;

        do
        {
            assert(!_incoming_frames.empty());

            auto data = _incoming_frames.front().get();
            _incoming_frames.erase(_incoming_frames.begin());

            if (current_frame == data.frame_number)
            {
                frame_found = true;
                set_next_volume_data(data.ssbo, data.offsets);
            }

            _loaded_frames.push_back(std::move(data));
        } while (!frame_found && !_incoming_frames.empty());
    }
    else
    {
        signal_ad_hoc_load();

        frame data{
            .ssbo = std::make_shared<gl::shader_storage>(),
            .offsets = glm::uvec4(~0),
            .frame_number = current_frame,
        };

        auto grids = nanovdb::io::readGrids(_nvdb_frames[current_frame].second);
        size_t grids_size = 0;

        for (size_t i = 0; i < grids.size(); ++i)
        {
            data.offsets[i] = grids_size;
            grids_size += grids[i].size();
            assert(grids_size % 4 == 0);
        }

        data.ssbo->buffer_data(nullptr, grids_size, GL_STREAM_DRAW);

        for (size_t i = 0; i < grids.size(); ++i)
        {
            data.ssbo->buffer_sub_data(grids[i].data(), grids[i].size(), data.offsets[i]);
        }

        set_next_volume_data(data.ssbo, data.offsets);

        auto remove_cond = [current_frame](int i) -> bool { return i == current_frame; };

        load_range.erase(std::remove_if(load_range.begin(), load_range.end(), remove_cond), load_range.end());

        _loaded_frames.push_back(std::move(data));
    }

    if (get_play_animation())
    {
        for (const auto &n : load_range)
        {
            std::function load_task = [path = _nvdb_frames[n].second, n = n]() -> frame {
                frame data{
                    .ssbo = std::make_shared<gl::shader_storage>(),
                    .offsets = glm::uvec4(~0),
                    .frame_number = n,
                };

                auto grids = nanovdb::io::readGrids(path);
                size_t grids_size = 0;

                for (size_t i = 0; i < grids.size(); ++i)
                {
                    data.offsets[i] = grids_size;
                    grids_size += grids[i].size();
                    assert(grids_size % 4 == 0);
                }

                data.ssbo->buffer_data(nullptr, grids_size, GL_STREAM_DRAW);

                for (size_t i = 0; i < grids.size(); ++i)
                {
                    data.ssbo->buffer_sub_data(grids[i].data(), grids[i].size(), data.offsets[i]);
                }

                glFinish();

                return data;
            };

            _incoming_frames.push_back(ctx.gl_thread().enqueue(load_task));
            _scheduled_frames.push_back(n);
        }
    }

    volume_resource_base::update(ctx, delta_time);
}
} // namespace objects::vdb
