#include "volume_resource_base.hpp"

#include <gl/buffer_indices.hpp>
#include <objects/ui/popup.hpp>
#include <scene/object_context.hpp>
#include <utils/memory_counter.hpp>
#include <utils/resource_path.hpp>

#include <iostream>

namespace objects::vdb
{
int volume_resource_base::get_frames_ahead()
{
    return _frames_ahead;
}

void volume_resource_base::set_frame_rate(float value)
{
    _frame_rate = value;
}

float volume_resource_base::get_frame_rate()
{
    return _frame_rate;
}

float volume_resource_base::get_current_time()
{
    return _current_time;
}

const std::vector<std::string> &volume_resource_base::get_grid_names()
{
    return _grid_names;
}

void volume_resource_base::set_grid_names(std::vector<std::string> grid_names)
{
    _grid_names = std::move(grid_names);
}

size_t volume_resource_base::get_force_loads()
{
    return _force_loads;
}

size_t volume_resource_base::get_ad_hoc_loads()
{
    return _ad_hoc_loads;
}

void volume_resource_base::set_play_animation(bool value)
{
    _play_animation = value;
}

bool volume_resource_base::get_play_animation()
{
    return _play_animation;
}

const char *volume_resource_base::shading_algorithm_str(shading_algorithm value)
{
    switch (value)
    {
    case shading_algorithm::DEBUG:
        return "Debug";
    case shading_algorithm::DUST_HDDA:
        return "Dust (HDDA)";
    case shading_algorithm::DUST_RM:
        return "Dust (raymarching)";
    }

    return nullptr;
}

void volume_resource_base::set_shading_algorithm(shading_algorithm new_shading_algorithm)
{
    _current_shading_algorithm = new_shading_algorithm;

    using source = gl::shader::source;
    using type = gl::shader::source::type_e;
    using attr = gl::vertex_array::attribute;

    switch (_current_shading_algorithm)
    {
    case shading_algorithm::DEBUG:
        _render_shader = std::make_unique<gl::shader>(std::vector<source>{
            source{.type = type::VERTEX, .path = utils::resource_path("glsl/default.vert")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_pre.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_post.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/debug.frag")},
        });
        break;
    case shading_algorithm::DUST_HDDA:
        _render_shader = std::make_unique<gl::shader>(std::vector<source>{
            source{.type = type::VERTEX, .path = utils::resource_path("glsl/default.vert")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_pre.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_post.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/dust_hdda.frag")},
        });
        break;
    case shading_algorithm::DUST_RM:
        _render_shader = std::make_unique<gl::shader>(std::vector<source>{
            source{.type = type::VERTEX, .path = utils::resource_path("glsl/default.vert")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_pre.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_post.glsl")},
            source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/dust_rm.frag")},
        });
        break;
    }
}

volume_resource_base::shading_algorithm volume_resource_base::get_shading_algorithm()
{
    return _current_shading_algorithm;
}

int volume_resource_base::get_old_unused_block_number()
{
    int block = -1;
    std::chrono::steady_clock::time_point tp = _ssbo_timestamp[0];

    for (int i = 0; i < MAX_BLOCKS; ++i)
    {
        if (_ssbo_block_frame[i] != _current_frame && tp >= _ssbo_timestamp[i])
        {
            block = i;
        }
    }

    return block;
}

int volume_resource_base::get_active_block_number()
{
    for (int i = 0; i < MAX_BLOCKS; ++i)
    {
        if (_ssbo_block_frame[i] == _current_frame)
        {
            return i;
        }
    }

    return -1;
}

void volume_resource_base::init(scene::object_context &ctx)
{
    using source = gl::shader::source;
    using type = gl::shader::source::type_e;
    using attr = gl::vertex_array::attribute;

    set_shading_algorithm(shading_algorithm::DEBUG);

    _vertex_array = std::make_unique<gl::vertex_array>();
    _vertex_buffer = std::make_shared<gl::vertex_buffer>();

    glm::vec2 vertices[3]{
        {-2.f, -2.f},
        {2.f, -2.f},
        {0.f, 4.f},
    };

    _vertex_buffer->buffer_data(vertices, sizeof(vertices), GL_STATIC_DRAW);
    _vertex_array->setup({attr{0, 2, GL_FLOAT, false, sizeof(glm::vec2), nullptr}}, _vertex_buffer);

    {
        auto objects = ctx.find_objects<objects::misc::world_data>();

        if (objects.size() != 1)
        {
            throw std::runtime_error(std::string(__func__) + ": Expected 1 and only 1 world_data object.");
        }

        _world_data = std::move(objects[0]);
    }
}

void volume_resource_base::update(scene::object_context &ctx, float delta_time)
{
    const auto &active_frame = _frames[_current_frame];

    // check frame

    int block = get_active_block_number();

    if (block != -1 && _ssbo_block_loaded[block].valid()) // fails if frame is not scheduled or already loaded
    {
        const auto update_range = _ssbo_block_loaded[block].get(); // unlocks when frame is loaded

        if (update_range.size != 0)
        {
            auto flush_t1 = std::chrono::steady_clock::now();
            glFlushMappedNamedBufferRange(_ssbo, block * _ssbo_block_size, update_range.size);
            _world_data->set_vdb_data_offsets(update_range.offsets);
            _world_data->update_buffer();
            auto flush_t2 = std::chrono::steady_clock::now();

            utils::update_flush_time(std::chrono::duration_cast<std::chrono::microseconds>(flush_t2 - flush_t1).count());
        }
    }

    try
    {
        _world_data->update(ctx, delta_time);
        glBindVertexArray(*_vertex_array);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, _ssbo, block * _ssbo_block_size, _ssbo_block_size);
        glShaderStorageBlockBinding(*_render_shader, 0, gl::buffer_base_indices::SSBO_0);
        glUniformBlockBinding(*_render_shader, 0, gl::buffer_base_indices::UBO_0);
        glUseProgram(*_render_shader);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        if (_exception_popup)
        {
            _exception_popup->destroy();
            _exception_popup.reset();
            _last_error.clear();
        }
    }
    catch (std::exception &e)
    {
        // So your PC doesn't burn on shader trying to burst-recompile post failure. I will not fix this it's dev only problem.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (e.what() != _last_error)
        {
            if (_exception_popup)
            {
                _exception_popup->destroy();
            }

            _exception_popup = ctx.share_object(std::make_shared<objects::ui::popup>(u8"OpenGL error", reinterpret_cast<const char8_t*>(e.what())));
            _last_error = e.what();
        }

        _exception_popup->set_width(ctx.window_size().x - 64);
    }

    if (_play_animation)
    {
        _frame_overshoot += delta_time;

        auto frame_time = 1.0f / _frame_rate;

        if (_frame_overshoot > frame_time)
        {
            _frame_overshoot -= frame_time;
            _current_frame = (_current_frame + 1) % _frames.size();
            _ssbo_block_fences[block].sync();

            schedule_frame(ctx, get_old_unused_block_number(), (_current_frame + MAX_BLOCKS - 2) % _frames.size());
        }

        if (_frame_overshoot > frame_time)
        {
            _frame_overshoot = frame_time;
        }
    }
}

void volume_resource_base::signal(scene::object_context &, scene::signal_e signal)
{
    switch (signal)
    {
    case scene::signal_e::RELOAD_SHADERS:
        _render_shader->make_dirty();
    default:
        break;
    }
}
} // namespace objects::vdb
