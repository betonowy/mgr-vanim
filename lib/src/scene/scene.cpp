#include "scene.hpp"

#include <algorithm>

namespace scene
{
void scene::init(std::function<void(object_context &)> func)
{
    func(_context);
}

void scene::loop(float delta_time)
{
    std::move(_context._objects_to_add.begin(), _context._objects_to_add.end(), std::back_inserter(_context._objects_to_init));

    _context._objects_to_add.clear();

    for (const auto &object : _context._objects_to_init)
    {
        object->init(_context);
        _context._objects.push_back(std::move(object));
    }

    _context._objects_to_init.clear();

    for (const auto &object : _context._objects)
    {
        if (object->is_destroyed())
        {
            _context._objects_to_delete.push_back(std::move(object));
        }
        else
        {
            object->update(_context, delta_time);
        }
    }

    for (const auto &signal : _context._pending_signals)
    {
        for (const auto &object : _context._objects)
        {
            if (!object->is_destroyed())
            {
                object->signal(signal);
            }
        }
    }

    {
        const auto cond = [](const std::shared_ptr<object> &object) { return !object || object->is_destroyed(); };
        const auto begin = _context._objects.begin();
        const auto end = _context._objects.end();

        _context._objects.erase(std::remove_if(begin, end, cond), end);
    }

    // noop for now
    // for (const auto &object : _context._objects_to_delete)
    // {
    // }

    _context._objects_to_delete.clear();
}

void scene::set_window_size(glm::ivec2 size)
{
    _context._window_size = size;
}
} // namespace scene
