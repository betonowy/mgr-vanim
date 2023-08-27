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
convert_vdb_nvdb::convert_vdb_nvdb(std::filesystem::path path, converter::nvdb_format format, converter::nvdb_error_method error_method, float error)
    : _working_path(std::move(path)), _format(format), _error_method(error_method), _error(error)
{
}

convert_vdb_nvdb::~convert_vdb_nvdb() = default;

namespace
{
convert_vdb_nvdb::job_result convert_vdb_nvdb_job(std::vector<std::filesystem::path> files, size_t initial_count, std::shared_ptr<utils::thread_pool> thread_pool, converter::nvdb_format format, converter::nvdb_error_method error_method, float error)
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

    std::function next_job = [files = std::move(files), initial_count, thread_pool, format, error_method, error]() -> convert_vdb_nvdb::job_result {
        return convert_vdb_nvdb_job(std::move(files), initial_count, thread_pool, format, error_method, error);
    };

    res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(next_job)));

    const auto converter_status = converter::convert_to_nvdb(file, format, error, error_method);

    res.e = converter_status.e;
    res.em = converter_status.em;
    res.ex = converter_status.ex;

    if (!converter_status.success)
    {
        res.error = converter_status.message;
    }
    else
    {
        std::stringstream ss;
        ss << '[' << current_processed_count << '/' << initial_count << "] Processed " + file.filename().string();
        res.description = ss.str();
    }

    return res;
}
} // namespace

void convert_vdb_nvdb::init(scene::object_context &ctx)
{
    conv_debug << "frame;error;min_error;max_error\n";

    std::function directory_job = [path = _working_path, thread_pool = ctx.generic_thread_pool_sptr(), format = _format, error_method = _error_method, error = _error]() mutable -> job_result {
        job_result res;

        auto files = converter::find_files_with_extension(path, ".vdb");

        std::ranges::sort(files, std::greater{});

        if (files.empty())
        {
            res.error = "No files with specified extension found.";
            return res;
        }

        size_t initial_count = files.size();

        std::function converter_job = [files = std::move(files), initial_count, thread_pool, format, error_method, error]() mutable -> job_result {
            return convert_vdb_nvdb_job(std::move(files), initial_count, thread_pool, format, error_method, error);
        };

        res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(converter_job)));
        res.description = "Found " + std::to_string(initial_count) + " files to convert.";

        return res;
    };

    _current_status.next_job =
        std::make_unique<decltype(_current_status.next_job)::element_type>(ctx.generic_thread_pool().enqueue(std::move(directory_job)));
}

void convert_vdb_nvdb::update(scene::object_context &ctx, float)
{
    if (_current_status.finished)
    {
        ctx.add_object(std::make_shared<popup>(u8"Success", std::u8string(reinterpret_cast<const char8_t *>(_current_status.description.c_str()))));
        return destroy();
    }

    if (utils::is_ready(*_current_status.next_job))
    {
        auto job_result = _current_status.next_job->get();

        if (!job_result.error.empty())
        {
            ctx.add_object(std::make_shared<popup>(u8"Error", reinterpret_cast<const char8_t *>(job_result.error.c_str())));
            return destroy();
        }

        bool skip = !skipped_first;

        if (!job_result.next_job && !job_result.finished)
        {
            ctx.add_object(std::make_shared<popup>(u8"Error", u8"No more jobs in progress, but process didn't finish "
                                                              u8"nor error. This shouldn't ever happen"));
            return destroy();
        }
        else if (!skip && !job_result.finished)
        {
            conv_debug << frames_converted++ << ';'
                       << job_result.e << ';'
                       << job_result.em << ';'
                       << job_result.ex << '\n';
        }

        skipped_first = true;

        _current_status = std::move(job_result);
    }

    auto half_size = ImGui::GetIO().DisplaySize;
    half_size.x /= 2;
    half_size.y /= 2;

    ImGui::SetNextWindowSize(half_size);
    ImGui::SetNextWindowPos(half_size, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Converting OpenVDB to NanoVDB", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
#ifdef VANIM_WINDOWS
        ImGui::Text("Directory: %ls", _working_path.c_str());
#else
        ImGui::Text("Directory: %s", _working_path.c_str());
#endif

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, 0xffa06040);

        ImGui::ProgressBar(_current_status.progress, ImVec2(-FLT_MIN, 0),
                           _current_status.description.empty() ? nullptr : _current_status.description.c_str());

        ImGui::PopStyleColor();
    }
    ImGui::End();
}
} // namespace objects::ui
