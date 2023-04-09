#pragma once

#include "signal.hpp"

#include <string>

namespace scene
{
class object_context;

class object
{
public:
    virtual ~object() = default;

    struct key_event
    {
        int keycode;
        bool down;
    };

    struct motion_event
    {
        int x, y, x_rel, y_rel;
    };

    struct button_event
    {
        int x, y, button;
        bool down;
    };

    struct wheel_event
    {
        int x, y;
    };

    virtual void init(object_context &);
    virtual void update(object_context &, float delta_frame);
    virtual void signal(object_context &, signal_e);
    virtual void key(object_context &, key_event);
    virtual void motion(object_context &, motion_event);
    virtual void button(object_context &, button_event);
    virtual void wheel(object_context &, wheel_event);

    const std::string &name() const
    {
        return _name;
    }
    bool is_destroyed() const
    {
        return _destroyed;
    }

    void destroy()
    {
        _destroyed = true;
    }

private:
    std::string _name;
    bool _destroyed = false;
};
} // namespace scene
