#pragma once

#include <scene/object.hpp>

#include <filesystem>
#include <functional>
#include <future>
#include <variant>
#include <vector>

#include <converter/dvdb_converter.hpp>

namespace objects::ui
{
class convert_nvdb_dvdb : public scene::object
{
public:
    convert_nvdb_dvdb(std::filesystem::path, float max_error);
    ~convert_nvdb_dvdb() override;

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

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
    std::shared_ptr<converter::dvdb_converter> dvdb_converter;
    std::filesystem::path _working_path;
    job_result _current_status;
    float _error;
    float _max_error;
};
} // namespace objects::ui
