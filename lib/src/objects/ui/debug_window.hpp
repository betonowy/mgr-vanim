#pragma once

#include <scene/object.hpp>

#include <vector>

namespace objects::ui
{
class debug_window : public scene::object
{
public:
    virtual ~debug_window();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

private:
    std::vector<float> _buffer;
    size_t _current_buffer_index;

    bool _auto_recompile = false;
    float _recompile_counter = 0;
    bool _fps_soft_cap = false;
    bool _open = true;

    float _wait_time = 0.01f;
};
} // namespace objects::ui
