#include "volume_resource_base.hpp"

#include <gl/buffer_indices.hpp>
#include <objects/ui/popup.hpp>
#include <scene/object_context.hpp>
#include <utils/resource_path.hpp>

namespace objects::vdb
{
void volume_resource_base::set_next_volume_data(std::shared_ptr<gl::shader_storage> data, glm::uvec4 offsets)
{
    _volume_data_active = std::move(data);
    _world_data->set_vdb_data_offsets(offsets);
}

void volume_resource_base::set_preload_hint(float ms_ahead)
{
    _preload_hint = glm::max(ms_ahead, 0.f);
}

float volume_resource_base::get_preload_hint()
{
    return _preload_hint;
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

float volume_resource_base::get_min_time()
{
    return _min_time;
}

float volume_resource_base::get_max_time()
{
    return _max_time;
}

void volume_resource_base::set_frame_range(float min, float max)
{
    _min_time = min, _max_time = max;
}

void volume_resource_base::signal_force_load()
{
    ++_force_loads;
}

size_t volume_resource_base::get_force_loads()
{
    return _force_loads;
}

void volume_resource_base::signal_ad_hoc_load()
{
    ++_ad_hoc_loads;
}

size_t volume_resource_base::get_ad_hoc_loads()
{
    return _ad_hoc_loads;
}

void volume_resource_base::set_play_animation(bool value)
{
    _play_animation = value;
}

void volume_resource_base::set_no_unload(bool value)
{
    _no_unload = value;
}

bool volume_resource_base::get_no_unload()
{
    return _no_unload;
}

bool volume_resource_base::get_play_animation()
{
    return _play_animation;
}

void volume_resource_base::init(scene::object_context &ctx)
{
    using source = gl::shader::source;
    using type = gl::shader::source::type_e;
    using attr = gl::vertex_array::attribute;

    _render_shader = std::make_unique<gl::shader>(std::vector<source>{
        source{.type = type::VERTEX, .path = utils::resource_path("glsl/default.vert")},
        source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_pre.glsl")},
        source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb.glsl")},
        source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/pnanovdb_post.glsl")},
        source{.type = type::FRAGMENT, .path = utils::resource_path("glsl/default.frag")},
    });

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
    if (_play_animation)
    {
        _current_time += delta_time;

        if (_current_time >= _max_time)
        {
            _current_time = _min_time;
        }
    }

    if (!_volume_data_active)
    {
        return;
    }

    _world_data->update_buffer();

    try
    {
        glBindVertexArray(*_vertex_array);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *_volume_data_active);
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
        if (e.what() != _last_error)
        {
            if (_exception_popup)
            {
                _exception_popup->destroy();
            }

            _exception_popup = ctx.share_object(std::make_shared<objects::ui::popup>("OpenGL error", e.what()));
            _last_error = e.what();
        }

        _exception_popup->set_width(ctx.window_size().x - 64);
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
