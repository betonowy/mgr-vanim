#include "object.hpp"
#include "signal.hpp"

namespace scene
{
void object::init(object_context &)
{
}

void object::update(object_context &, float)
{
}

void object::signal(object_context &, signal_e)
{
}

void object::key(object_context &, key_event)
{
}

void object::motion(object_context &, motion_event)
{
}

void object::button(object_context &, button_event)
{
}

void object::wheel(object_context &, wheel_event)
{
}
} // namespace scene
