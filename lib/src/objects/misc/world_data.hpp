#pragma once

#include <gl/uniform_buffer.hpp>
#include <scene/camera.hpp>
#include <scene/object.hpp>

namespace objects::misc
{
class world_data : public scene::object
{
public:
    void init(scene::object_context &) override;
    void update(scene::object_context &, float delta_frame) override;
    void key(scene::object_context &, key_event) override;
    void motion(scene::object_context &, motion_event) override;
    void button(scene::object_context &, button_event) override;
    void wheel(scene::object_context &, wheel_event) override;

    void set_vdb_data_offsets(glm::uvec4);
    void update_buffer();

private:
    struct
    {
        int x, y, x_rel, y_rel;
        bool w, s, a, d, q, e;
    } _io_state;

    enum class grab_state
    {
        NO_GRAB,
        TEMP_GRAB,
    };

    gl::uniform_buffer _buffer;
    scene::camera _camera;
    glm::vec3 _velocity;
    glm::vec2 _mouse_rotation;
    glm::ivec2 _grab_warp;
    float _acceleration_base = 100.f;
    grab_state _grab_state = grab_state::NO_GRAB;
    glm::uvec4 _vdb_data_offsets;
};
} // namespace objects::misc
