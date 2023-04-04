#pragma once

#include <scene/object.hpp>

#include <vector>

namespace objects::ui
{
class animation_controller : public scene::object
{
public:
    virtual ~animation_controller();

    void init(scene::object_context&) override;
    void update(scene::object_context&, float) override;
private:

    std::vector<float> _buffer;
    size_t _current_buffer_index;

    bool _fps_soft_cap = false;
    bool _open = true;
};
} // namespace objects::ui
