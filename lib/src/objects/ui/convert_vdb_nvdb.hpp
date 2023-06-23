#pragma once

#include <scene/object.hpp>

#include <filesystem>
#include <functional>
#include <future>
#include <variant>
#include <vector>

#include <converter/nvdb_converter.hpp>

namespace objects::ui
{
class convert_vdb_nvdb : public scene::object
{
public:
    convert_vdb_nvdb(std::filesystem::path, converter::nvdb_format, converter::nvdb_error_method, float error);
    ~convert_vdb_nvdb() override;

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

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
    converter::nvdb_format _format;
    converter::nvdb_error_method _error_method;
    float _error;
};
} // namespace objects::ui
