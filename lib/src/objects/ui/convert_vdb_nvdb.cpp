#include "convert_vdb_nvdb.hpp"

#include <converter/common.hpp>
#include <converter/nvdb_converter.hpp>
#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/scope_guard.hpp>
#include <utils/thread_pool.hpp>

#include "popup.hpp"

#include <imgui.h>

#include <algorithm>

namespace objects::ui
{
convert_vdb_nvdb::convert_vdb_nvdb(std::filesystem::path path) : _working_path(std::move(path))
{
}

convert_vdb_nvdb::~convert_vdb_nvdb() = default;

namespace
{
convert_vdb_nvdb::job_result convert_vdb_nvdb_job(std::vector<std::filesystem::path> files, size_t initial_count,
                                                  std::shared_ptr<utils::thread_pool> thread_pool)
{
    convert_vdb_nvdb::job_result res;

    if (files.empty())
    {
        res.finished = true;
        res.description = "All conversions have been finished.";
        res.progress = 1.f;
        return res;
    }

    auto file = std::move(files.back());
    files.pop_back();

    size_t current_processed_count = initial_count - files.size();

    res.progress = 1.f - (static_cast<float>(files.size()) / initial_count);

    std::function next_job = [files = std::move(files), initial_count, thread_pool]() -> convert_vdb_nvdb::job_result {
        return convert_vdb_nvdb_job(std::move(files), initial_count, thread_pool);
    };

    res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(next_job)));

    const auto converter_status = converter::convert_to_nvdb(file);

    if (!converter_status.success)
    {
        res.error = converter_status.message;
    }
    else
    {
        std::stringstream ss;
        ss << '[' << current_processed_count << '/' << initial_count << "] Processed " + file.filename().string();
        res.description =  ss.str();
    }

    return res;
}
} // namespace

void convert_vdb_nvdb::init(scene::object_context &ctx)
{
    std::function directory_job = [path = _working_path, thread_pool = ctx.generic_thread_pool_sptr()]() mutable -> job_result {
        job_result res;

        auto files = converter::find_files_with_extension(path, ".vdb");

        if (files.empty())
        {
            res.error = "No files with specified extension found.";
            return res;
        }

        size_t initial_count = files.size();

        std::function converter_job = [files = std::move(files), initial_count, thread_pool]() mutable -> job_result {
            return convert_vdb_nvdb_job(std::move(files), initial_count, thread_pool);
        };

        res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(converter_job)));
        res.description = "Found " + std::to_string(initial_count) + " files to convert.";

        return res;
    };

    _current_status.next_job = std::make_unique<decltype(_current_status.next_job)::element_type>(
        ctx.generic_thread_pool().enqueue(std::move(directory_job)));
}

void convert_vdb_nvdb::update(scene::object_context &ctx, float delta_time)
{
    if (_current_status.finished)
    {
        ctx.add_object(std::make_shared<popup>("Success", _current_status.description));
        return destroy();
    }

    if (utils::is_ready(*_current_status.next_job))
    {
        auto job_result = _current_status.next_job->get();

        if (!job_result.error.empty())
        {
            ctx.add_object(std::make_shared<popup>("Error", job_result.error));
            return destroy();
        }

        if (!job_result.next_job && !job_result.finished)
        {
            ctx.add_object(std::make_shared<popup>("Error", "No more jobs in progress, but process didn't finish "
                                                            "nor error. This shouldn't ever happen"));
            return destroy();
        }

        _current_status = std::move(job_result);
    }

    auto half_size = ImGui::GetIO().DisplaySize;
    half_size.x /= 2;
    half_size.y /= 2;

    ImGui::SetNextWindowSize(half_size);
    ImGui::SetNextWindowPos(half_size, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::Begin("Converting OpenVDB to NanoVDB", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    {
        ImGui::Text("Directory: %s", _working_path.c_str());

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xffa06040);

        ImGui::ProgressBar(_current_status.progress, ImVec2(-FLT_MIN, 0),
                           _current_status.description.empty() ? nullptr : _current_status.description.c_str());

        ImGui::PopStyleColor();
    }
    ImGui::End();
}
} // namespace objects::ui
