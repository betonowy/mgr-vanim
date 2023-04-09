#include "volume_resource_base.hpp"

#include <gl/buffer_indices.hpp>
#include <objects/ui/popup.hpp>
#include <scene/object_context.hpp>
#include <utils/resource_path.hpp>

#include <SDL2/SDL_events.h>
#include <glm/glm.hpp>
#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>

#define PNANOVDB_C
#define PNANOVDB_ADDRESS_32
#include <nanovdb/PNanoVDB.h>

#include <iostream>

namespace objects::vdb
{
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

    _volume_data_active = std::make_shared<gl::shader_storage>();
    _volume_data_inactive = std::make_shared<gl::shader_storage>();

    {
        auto objects = ctx.find_objects<objects::misc::world_data>();

        if (objects.size() != 1)
        {
            throw std::runtime_error(std::string(__func__) + ": Expected 1 and only 1 world_data object.");
        }

        _world_data = std::move(objects[0]);
    }
    {
        auto t1 = std::chrono::steady_clock::now();

        auto grids = nanovdb::io::readGrids("/home/araara/Documents/vdb-animations/Fire_01/embergen_fire_a_69.nvdb");

        size_t grids_size = 0;
        std::vector<size_t> offsets;

        for (auto &grid : grids)
        {
            offsets.push_back(grids_size);
            grids_size += grid.size();
            assert(grids_size % 4 == 0); // GPU can support 4-byte aligned grids only
        }

        _volume_data_active->buffer_data(nullptr, grids_size, GL_STATIC_DRAW);

        glm::uvec4 uvec_offsets(~0);

        for (size_t i = 0; i < grids.size(); ++i)
        {
            _volume_data_active->buffer_sub_data(grids[i].data(), grids[i].size(), offsets[i]);
            uvec_offsets[i] = offsets[i];
        }

        _world_data->set_vdb_data_offsets(uvec_offsets);

        auto t2 = std::chrono::steady_clock::now();

        std::cout << "Loading took: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000.f << " ms\n";
    }
}

void volume_resource_base::update(scene::object_context &ctx, float)
{
    _world_data->update_buffer();
    // render
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
