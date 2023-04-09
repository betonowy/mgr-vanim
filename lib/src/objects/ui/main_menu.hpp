#pragma once

#include <scene/object.hpp>

#include <memory>
#include <vector>

namespace objects::ui
{
class main_menu : public scene::object
{
public:
    virtual ~main_menu();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

private:
    std::shared_ptr<scene::object> _debug_window;
};
} // namespace objects::ui
