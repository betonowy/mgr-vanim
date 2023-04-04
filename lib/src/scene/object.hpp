#pragma once

#include <string>

namespace scene
{
class object_context;
enum class signal_e;

class object
{
public:
    virtual ~object() = default;

    virtual void init(object_context &);
    virtual void update(object_context &, float delta_frame);
    virtual void signal(signal_e);

    const std::string &name() const { return _name; }
    bool is_destroyed() const { return _destroyed; }

    void destroy() { _destroyed = true; }

private:
    std::string _name;
    bool _destroyed = false;
};
} // namespace scene
