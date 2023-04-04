#pragma once

#include "object_context.hpp"

#include <functional>

#include <glm/vec2.hpp>

namespace scene
{
class scene
{
public:
    void init(std::function<void(object_context&)>);

    void loop(float delta_time);

    void set_window_size(glm::ivec2);

private:
    object_context _context;
};
} // namespace scene
