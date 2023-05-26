#include "convert_nvdb_dvdb.hpp"

#include <converter/common.hpp>
#include <scene/object_context.hpp>
#include <utils/future_helpers.hpp>
#include <utils/scope_guard.hpp>
#include <utils/thread_pool.hpp>

#include "popup.hpp"

#include <imgui.h>

#include <algorithm>
#include <regex>

namespace objects::ui
{
convert_nvdb_dvdb::convert_nvdb_dvdb(std::filesystem::path path)
    : _working_path(std::move(path))
{
}

convert_nvdb_dvdb::~convert_nvdb_dvdb() = default;

namespace
{
convert_nvdb_dvdb::job_result convert_nvdb_dvdb_job(std::vector<std::filesystem::path> files, size_t initial_count, std::shared_ptr<utils::thread_pool> thread_pool, std::shared_ptr<converter::dvdb_converter> converter)
{
    convert_nvdb_dvdb::job_result res;

    if (files.empty())
    {
        res.finished = true;
        res.description = "All conversions have been finished.";
        res.progress = 1.f;
        return res;
    }

    auto file = std::move(files.back());
    files.pop_back();

    bool make_keyframe = files.size() % 5 == 0 || files.size() > 97;

    size_t current_processed_count = initial_count - files.size();

    res.progress = 1.f - (static_cast<float>(files.size()) / initial_count);

    std::function next_job = [files = std::move(files), initial_count, thread_pool, converter]() -> convert_nvdb_dvdb::job_result {
        return convert_nvdb_dvdb_job(std::move(files), initial_count, thread_pool, converter);
    };

    if (make_keyframe)
    {
        converter->create_keyframe(file);
    }
    else
    {
        converter->add_diff_frame(file);
    }

    res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(next_job)));

    // if (!converter_status.success)
    // {
    //     res.error = converter_status.message;
    // }
    // else
    // {
    std::stringstream ss;
    ss << '[' << current_processed_count << '/' << initial_count << "] Processed " + file.filename().string();
    res.description = ss.str();
    // }

    return res;
}
} // namespace

void convert_nvdb_dvdb::init(scene::object_context &ctx)
{
    std::function directory_job = [path = _working_path, thread_pool = ctx.generic_thread_pool_sptr(), converter = dvdb_converter]() mutable -> job_result {
        job_result res;

        auto files = converter::find_files_with_extension(path, ".nvdb");

        if (files.empty())
        {
            res.error = "No files with specified extension found.";
            return res;
        }

        size_t initial_count = files.size();

        using regex_type = std::basic_regex<std::filesystem::path::value_type>;

#if VANIM_WINDOWS
        static constexpr auto regex_pattern = L"^.*[\\/\\\\].+_(\\d+)\\.nvdb$";
        using match_type = std::wcmatch;
#else
        static constexpr auto regex_pattern = "^.*[\\/\\\\].+_(\\d+)\\.nvdb$";
        using match_type = std::cmatch;
#endif

        regex_type regex(regex_pattern);

        std::sort(files.begin(), files.end(), [&](const std::filesystem::path &lhs, const std::filesystem::path &rhs) -> bool {
            match_type cm;

            int lhs_index{}, rhs_index{};

            if (std::regex_search(lhs.c_str(), cm, regex))
            {
                lhs_index = std::stoi(cm[1]);
            }

            if (std::regex_search(rhs.c_str(), cm, regex))
            {
                rhs_index = std::stoi(cm[1]);
            }

            return lhs_index > rhs_index;
        });

        std::function converter_job = [files = std::move(files), initial_count, thread_pool, converter]() mutable -> job_result {
            return convert_nvdb_dvdb_job(std::move(files), initial_count, thread_pool, converter);
        };

        res.next_job = std::make_unique<decltype(res.next_job)::element_type>(thread_pool->enqueue(std::move(converter_job)));
        res.description = "Found " + std::to_string(initial_count) + " files to convert.";

        return res;
    };

    _current_status.next_job =
        std::make_unique<decltype(_current_status.next_job)::element_type>(ctx.generic_thread_pool().enqueue(std::move(directory_job)));
}

void convert_nvdb_dvdb::update(scene::object_context &ctx, float)
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

        if (!job_result.next_job && !job_result.finished)
        {
            ctx.add_object(std::make_shared<popup>(u8"Error", u8"No more jobs in progress, but process didn't finish "
                                                              u8"nor error. This shouldn't ever happen"));
            return destroy();
        }

        _current_status = std::move(job_result);
    }

    auto half_size = ImGui::GetIO().DisplaySize;
    half_size.x /= 2;
    half_size.y /= 2;

    ImGui::SetNextWindowSize(half_size);
    ImGui::SetNextWindowPos(half_size, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Converting NanoVDB to DiffVDB", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
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
