#include "scene.hpp"
#include "signal.h"

#include <algorithm>
#include <iterator>

namespace scene
{
scene::scene() = default;

void scene::init(std::function<void(object_context &)> func)
{
    func(_context);
}

bool scene::loop(float delta_time)
{
    std::move(_context._objects_to_add.begin(), _context._objects_to_add.end(), std::back_inserter(_context._objects_to_init));

    _context._objects_to_add.clear();

    for (const auto &object : _context._objects_to_init)
    {
        object->init(_context);
    }

    std::move(_context._objects_to_init.begin(), _context._objects_to_init.end(), std::back_inserter(_context._objects));

    _context._objects_to_init.clear();

    for (const auto &object : _context._objects)
    {
        if (object->is_destroyed())
        {
            _context._objects_to_delete.push_back(std::move(object));
        }
        else
        {
            for (const auto &motion : _pending_motion_events)
            {
                object->motion(_context, motion);
            }

            for (const auto &button : _pending_button_events)
            {
                object->button(_context, button);
            }

            for (const auto &key : _pending_key_events)
            {
                object->key(_context, key);
            }

            for (const auto &wheel : _pending_wheel_events)
            {
                object->wheel(_context, wheel);
            }

            object->update(_context, delta_time);
        }
    }

    _pending_motion_events.clear();
    _pending_button_events.clear();
    _pending_key_events.clear();
    _pending_wheel_events.clear();

    for (const auto &signal : _context._pending_signals)
    {
        if (signal == signal_e::EXIT_APPLICATION)
        {
            return false;
        }

        for (const auto &object : _context._objects)
        {
            if (!object->is_destroyed())
            {
                object->signal(_context, signal);
            }
        }
    }

    _context._pending_signals.clear();

    {
        const auto cond = [](const std::shared_ptr<object> &object) { return !object || object->is_destroyed(); };
        const auto begin = _context._objects.begin();
        const auto end = _context._objects.end();

        _context._objects.erase(std::remove_if(begin, end, cond), end);
    }

    for (const auto &object : _context._objects_to_delete)
    {
        object->on_destroy(_context);
    }

    _context._objects_to_delete.clear();

    return true;
}

void scene::set_window_size(glm::ivec2 size)
{
    _context._window_size = size;
}

void scene::push_key_event(int keycode, bool down)
{
    _pending_key_events.emplace_back(object::key_event{keycode, down});
}

void scene::push_motion_event(int x, int y, int x_rel, int y_rel)
{
    _pending_motion_events.emplace_back(object::motion_event{x, y, x_rel, y_rel});
}

void scene::push_button_event(int x, int y, int button, bool down)
{
    _pending_button_events.emplace_back(object::button_event{x, y, button, down});
}

void scene::push_wheel_event(int x, int y)
{
    _pending_wheel_events.emplace_back(object::wheel_event{x, y});
}
} // namespace scene
