#pragma once

#include "volume_resource_base.hpp"

namespace objects::vdb
{
class nano_vdb_resource : public volume_resource_base
{
public:
    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;
};
} // namespace objects::vdb
