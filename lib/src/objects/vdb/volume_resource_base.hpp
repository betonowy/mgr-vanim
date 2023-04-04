#pragma once

#include <scene/object.hpp>

#include <filesystem>
#include <future>
#include <vector>

namespace objects
{
class volume_resource_base : public scene::object
{
public:
    virtual std::vector<std::filesystem::path> discover_data(std::filesystem::path) = 0;

    virtual std::future<void> set_frame(float frame) = 0;
    virtual void set_preload_hint(size_t frames_ahead) = 0;

    virtual size_t get_cpu_memory_usage() = 0;
    virtual size_t get_gpu_memory_usage() = 0;
    virtual size_t get_numbef_of_frames() = 0;
    virtual std::string get_description() = 0;

    enum class shading_algorithm
    {
        NONE,
        WATER_CLOUD,
        SMOKE,
        FIRE,
        FIRE_SMOKE,
    };

    void set_shading_algorithm(shading_algorithm);

private:
};
} // namespace objects
