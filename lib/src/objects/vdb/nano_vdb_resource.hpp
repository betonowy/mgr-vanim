#pragma once

#include "volume_resource_base.hpp"

namespace objects::vdb
{
class nano_vdb_resource : public volume_resource_base
{
public:
    explicit nano_vdb_resource(std::filesystem::path);
    ~nano_vdb_resource();

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

private:
    std::filesystem::path _resource_directory;
    std::vector<std::pair<int, std::filesystem::path>> _nvdb_frames;

    size_t max_buffer_size;
};
} // namespace objects::vdb
