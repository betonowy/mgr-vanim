#include "nano_vdb_resource.hpp"

namespace objects::vdb
{
void nano_vdb_resource::init(scene::object_context &ctx)
{
    volume_resource_base::init(ctx);
}

void nano_vdb_resource::update(scene::object_context &ctx, float delta_time)
{
    volume_resource_base::update(ctx, delta_time);
}
} // namespace objects::vdb
