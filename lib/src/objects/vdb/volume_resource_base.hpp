#pragma once

#include <gl/fence.hpp>
#include <gl/shader.hpp>
#include <gl/shader_storage.hpp>
#include <gl/vertex_array.hpp>
#include <gl/vertex_buffer.hpp>
#include <objects/misc/world_data.hpp>
#include <scene/object.hpp>

#include <array>
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

    void set_buffer_size(bool preserve_contents);

    void set_load_callback(std::function<std::future<void>(int)>);

    // old

    void set_next_volume_data(std::shared_ptr<gl::shader_storage>, glm::uvec4 offsets);

    // void set_preload_hint(float frames_ahead);
    float get_preload_hint();

    void set_no_unload(bool);
    bool get_no_unload();

    void set_frame_rate(float);
    float get_frame_rate();

    void set_play_animation(bool);
    bool get_play_animation();

    float get_current_time();
    float get_min_time();
    float get_max_time();

    const std::vector<std::string> &get_grid_names();
    void set_grid_names(std::vector<std::string>);

    size_t get_force_loads();
    size_t get_ad_hoc_loads();

    void set_frame_range(float min, float max);

    // virtual size_t get_cpu_memory_usage() = 0;
    // virtual size_t get_gpu_memory_usage() = 0;
    // virtual size_t get_number_of_frames() = 0;
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
    shading_algorithm get_shading_algorithm();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;
    void signal(scene::object_context &, scene::signal_e) override;

protected:
    static constexpr size_t MAX_BLOCKS = 1;

    gl::shader_storage _ssbo;
    std::byte *_ssbo_ptr;
    size_t _ssbo_block_size = 0;
    size_t _ssbo_block_count = 0;
    std::array<size_t, MAX_BLOCKS> _ssbo_block_frame;
    std::array<gl::fence, MAX_BLOCKS> _ssbo_block_fences;

    struct frame
    {
        std::string path;
        size_t block_number;
        gl::fence drawFence;
        gl::fence loadFence;
        size_t number;
    };

    std::vector<frame> _frames;

    size_t _current_frame = 0;
    float _frame_overshoot = 0;

    std::shared_ptr<objects::misc::world_data> _world_data;

private:
    std::unique_ptr<gl::shader> _render_shader;
    std::unique_ptr<gl::vertex_array> _vertex_array;
    std::shared_ptr<gl::vertex_buffer> _vertex_buffer;
    std::shared_ptr<objects::ui::popup> _exception_popup;
    std::vector<std::string> _grid_names;

    std::string _last_error;

    float _preload_hint{};
    float _min_time{}, _max_time{}, _current_time{};
    float _frame_rate{};
    bool _play_animation{}, _no_unload{};

    size_t _ad_hoc_loads{}, _force_loads{};
};
} // namespace objects::vdb
