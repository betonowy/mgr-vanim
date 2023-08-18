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
#include <fstream>
#include <chrono>

namespace objects::ui
{
class popup;
}

namespace objects::vdb
{
class volume_resource_base : public scene::object
{
public:
    virtual const char *class_name() = 0;

    void reset_csv();

    void set_buffer_size(bool preserve_contents);

    virtual void schedule_frame(scene::object_context &, int block_number, int frame_number) = 0;

    void set_next_volume_data(std::shared_ptr<gl::shader_storage>, glm::uvec4 offsets);

    void set_frames_ahead(int frames_ahead);
    int get_frames_ahead();

    size_t get_current_frame()
    {
        return _current_frame;
    }

    // void set_no_unload(bool);
    // bool get_no_unload();

    void set_frame_rate(float);
    float get_frame_rate();

    void set_play_animation(bool);
    bool get_play_animation();

    float get_current_time();
    // float get_min_time();
    // float get_max_time();

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
        DEBUG,
        DUST_HDDA,
        DUST_RM,
    };

    static constexpr auto SHADING_ALGORITHM_LIST = {
        shading_algorithm::DEBUG,
        shading_algorithm::DUST_HDDA,
        shading_algorithm::DUST_RM,
    };

    static const char *shading_algorithm_str(shading_algorithm);

    void set_shading_algorithm(shading_algorithm);
    shading_algorithm get_shading_algorithm();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;
    void signal(scene::object_context &, scene::signal_e) override;

    auto tp_diff(std::chrono::steady_clock::time_point tp1, std::chrono::steady_clock::time_point tp2)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count();
    }

    auto tp_since(std::chrono::steady_clock::time_point tp)
    {
        return tp_diff(_tp_start, tp);
    }

protected:
    static constexpr size_t MAX_BLOCKS = 3;

    struct update_range
    {
        glm::uvec4 offsets;
        size_t size;
    };

    gl::shader_storage _ssbo;
    std::byte *_ssbo_ptr;
    size_t _ssbo_block_size = 0;
    size_t _ssbo_block_count = 0;
    std::array<size_t, MAX_BLOCKS> _ssbo_block_frame;
    std::array<gl::fence, MAX_BLOCKS> _ssbo_block_fences;
    std::array<std::future<update_range>, MAX_BLOCKS> _ssbo_block_loaded;
    std::array<std::chrono::steady_clock::time_point, MAX_BLOCKS> _ssbo_timestamp;

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

    std::ofstream _csv_out;
    std::chrono::steady_clock::time_point _tp_start = std::chrono::steady_clock::now();

private:
    int get_old_unused_block_number();
    int get_active_block_number();

    std::unique_ptr<gl::shader> _render_shader;
    shading_algorithm _current_shading_algorithm = shading_algorithm::DEBUG;

    std::unique_ptr<gl::vertex_array> _vertex_array;
    std::shared_ptr<gl::vertex_buffer> _vertex_buffer;
    std::shared_ptr<objects::ui::popup> _exception_popup;
    std::vector<std::string> _grid_names;

    std::string _last_error;

    int _frames_ahead{};
    float _current_time{};
    float _frame_rate{};
    bool _play_animation{};

    size_t _ad_hoc_loads{}, _force_loads{};
};
} // namespace objects::vdb
