#pragma once

#include "../vdb/volume_resource_base.hpp"
#include <scene/object.hpp>

#include <vector>

namespace objects::ui
{
class animation_controller : public scene::object
{
public:
    animation_controller(std::shared_ptr<vdb::volume_resource_base>);
    virtual ~animation_controller();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;
    void on_destroy(scene::object_context &) override;

private:
    std::shared_ptr<vdb::volume_resource_base> _volume_resource;
};
} // namespace objects::ui
