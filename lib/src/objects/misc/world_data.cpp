#include "world_data.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

#include <gl/buffer_indices.hpp>
#include <scene/object_context.hpp>

#include <numbers>

namespace
{
struct buffer_layout
{
    glm::vec3 forward;
    uint32_t _padding_1;

    glm::vec3 right;
    uint32_t _padding_2;

    glm::vec3 up;
    uint32_t _padding_3;

    glm::vec3 origin;
    uint32_t _padding_4;

    glm::uvec4 grid_offsets;
};
} // namespace

namespace objects::misc
{
void world_data::init(scene::object_context &ctx)
{
    _camera.set_origin(glm::vec3(-30, -60, 60));
    _camera.set_dir(glm::vec3(1, 0, 0));
    _camera.set_fov(70);
    _camera.set_aspect(static_cast<float>(ctx.window_size().x) / static_cast<float>(ctx.window_size().y));

    _buffer.set_buffer_base(gl::buffer_base_indices::UBO_0);
    _buffer.buffer_data(nullptr, sizeof(buffer_layout), GL_STREAM_DRAW);
}

void world_data::update_buffer()
{
    buffer_layout layout{
        .forward = _camera.forward(),
        .right = _camera.right(),
        .up = _camera.up(),
        .origin = _camera.origin(),
        .grid_offsets = _vdb_data_offsets,
    };

    _buffer.buffer_sub_data(&layout, sizeof(layout), 0);
}

void world_data::update(scene::object_context &ctx, float delta_frame)
{
    _camera.set_aspect(static_cast<float>(ctx.window_size().x) / static_cast<float>(ctx.window_size().y));

    if (_grab_state != grab_state::NO_GRAB)
    {
        static constexpr float PI = std::numbers::pi;
        static constexpr float ANGLE_EPSILON = 0.001f;
        static constexpr float SENSITIVITY = 0.003f;

        const auto dir = glm::normalize(_camera.dir());

        auto horizontal = glm::vec2(dir.x, dir.z);
        auto vertical = glm::vec2(dir.y, glm::length(horizontal));

        float angle_flat = glm::atan(horizontal.x, horizontal.y) - _io_state.x_rel * SENSITIVITY;
        float angle_up = glm::atan(dir.y / glm::length(horizontal));

        horizontal = glm::vec2(glm::sin(angle_flat), glm::cos(angle_flat));

        angle_up -= _io_state.y_rel * SENSITIVITY;
        angle_up = glm::clamp(angle_up, -PI / 2.f + ANGLE_EPSILON, PI / 2.f - ANGLE_EPSILON);

        vertical = glm::vec2(glm::sin(angle_up), glm::cos(angle_up));

        _camera.set_dir(glm::vec3(vertical.y * horizontal.x, vertical.x, vertical.y * horizontal.y));
    }

    static constexpr float DRAG = 10.f;

    float acceleration = delta_frame * _acceleration_base;

    _velocity += _io_state.d * acceleration * _camera.right();
    _velocity -= _io_state.a * acceleration * _camera.right();
    _velocity += _io_state.e * acceleration * _camera.up();
    _velocity -= _io_state.q * acceleration * _camera.up();
    _velocity += _io_state.w * acceleration * _camera.forward();
    _velocity -= _io_state.s * acceleration * _camera.forward();

    _velocity *= 1.f - glm::clamp(DRAG * delta_frame, 0.f, 1.f);

    _camera.set_origin(_camera.origin() + _velocity * delta_frame);

    // consume mouse delta
    _io_state.y_rel = 0;
    _io_state.x_rel = 0;

    update_buffer();
}

void world_data::key(scene::object_context &, key_event event)
{
    switch (event.keycode)
    {
    case SDLK_w:
        _io_state.w = event.down;
        break;
    case SDLK_s:
        _io_state.s = event.down;
        break;
    case SDLK_a:
        _io_state.a = event.down;
        break;
    case SDLK_d:
        _io_state.d = event.down;
        break;
    case SDLK_q:
        _io_state.q = event.down;
        break;
    case SDLK_e:
        _io_state.e = event.down;
        break;
    }
}

void world_data::button(scene::object_context &, button_event event)
{
    _io_state.x = event.x;
    _io_state.y = event.y;

    switch (event.button)
    {
    case SDL_BUTTON_LEFT:
        if (event.down)
        {
            _grab_state = grab_state::TEMP_GRAB;
            _grab_warp.x = event.x, _grab_warp.y = event.y;
        }
        else
        {
            _grab_state = grab_state::NO_GRAB;
        }
        break;
    }
}

void world_data::motion(scene::object_context &, motion_event event)
{
    _io_state.x = event.x;
    _io_state.y = event.y;
    _io_state.x_rel += event.x_rel;
    _io_state.y_rel += event.y_rel;
}

void world_data::wheel(scene::object_context &, wheel_event event)
{
    static constexpr float SENSITIVITY = 0.1f;
    _acceleration_base *= 1.f + event.y * SENSITIVITY;
    _acceleration_base = glm::clamp(_acceleration_base, 10.f, 1000.f);
}

void world_data::set_vdb_data_offsets(glm::uvec4 offsets)
{
    _vdb_data_offsets = offsets;
}
} // namespace objects::misc
