#pragma once

#include <gl/shader.hpp>
#include <gl/shader_storage.hpp>
#include <gl/vertex_array.hpp>
#include <gl/vertex_buffer.hpp>
#include <objects/misc/world_data.hpp>
#include <scene/object.hpp>

#include <filesystem>
#include <future>
#include <vector>

namespace objects::ui
{
class popup;
}

namespace objects::vdb
{
class volume_resource_base : public scene::object
{
public:
    // virtual std::vector<std::filesystem::path> discover_data(std::filesystem::path) = 0;

    // virtual std::future<void> set_frame(float frame) = 0;
    // virtual void set_preload_hint(size_t frames_ahead) = 0;

    // virtual size_t get_cpu_memory_usage() = 0;
    // virtual size_t get_gpu_memory_usage() = 0;
    // virtual size_t get_numbef_of_frames() = 0;
    // virtual std::string get_description() = 0;

    enum class shading_algorithm
    {
        NONE,
        WATER_CLOUD,
        SMOKE,
        FIRE,
        FIRE_SMOKE,
    };

    void set_shading_algorithm(shading_algorithm);

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;
    void signal(scene::object_context &, scene::signal_e) override;

private:
    std::unique_ptr<gl::shader> _render_shader;
    std::unique_ptr<gl::vertex_array> _vertex_array;
    std::shared_ptr<gl::vertex_buffer> _vertex_buffer;

    std::shared_ptr<gl::shader_storage> _volume_data_inactive;
    std::shared_ptr<gl::shader_storage> _volume_data_active;

    std::shared_ptr<objects::ui::popup> _exception_popup;
    std::shared_ptr<objects::misc::world_data> _world_data;

    std::string _last_error;
};
} // namespace objects::vdb
