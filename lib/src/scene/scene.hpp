#pragma once

#include "object_context.hpp"

#include <functional>

#include <glm/vec2.hpp>

namespace scene
{
class scene
{
public:
    scene(SDL_GLContext, SDL_Window *);

    void init(std::function<void(object_context &)>);

    bool loop(float delta_time);

    void set_window_size(glm::ivec2);

    void push_key_event(int keycode, bool down);
    void push_motion_event(int x, int y, int x_rel, int y_rel);
    void push_button_event(int x, int y, int keycode, bool down);
    void push_wheel_event(int x, int y);

private:
    object_context _context;

    std::vector<object::key_event> _pending_key_events;
    std::vector<object::motion_event> _pending_motion_events;
    std::vector<object::button_event> _pending_button_events;
    std::vector<object::wheel_event> _pending_wheel_events;
};
} // namespace scene
