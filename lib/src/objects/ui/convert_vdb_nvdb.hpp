#pragma once

#include <scene/object.hpp>

#include <vector>
#include <filesystem>
#include <functional>
#include <future>
#include <variant>

namespace objects::ui
{
class convert_vdb_nvdb : public scene::object
{
public:
    convert_vdb_nvdb(std::filesystem::path);
    ~convert_vdb_nvdb() override;

    void init(scene::object_context&) override;
    void update(scene::object_context&, float) override;

    struct job_data
    {
        std::filesystem::path path;
    };

    struct job_result
    {
        std::unique_ptr<std::future<job_result>> next_job;
        std::string description;
        std::string error;
        float progress = 0;
        bool finished = false;
    };

private:
    std::filesystem::path _working_path;
    job_result _current_status;
};
} // namespace objects::ui
