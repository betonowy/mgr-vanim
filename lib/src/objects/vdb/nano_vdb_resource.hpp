#pragma once

#include "volume_resource_base.hpp"

namespace objects::vdb
{
class nano_vdb_resource : public volume_resource_base
{
public:
    explicit nano_vdb_resource(std::filesystem::path);
    ~nano_vdb_resource();

    void schedule_frame(scene::object_context &, int block_number, int frame_number) override;

    void init(scene::object_context &) override;

    const char* class_name() override;

private:
    std::filesystem::path _resource_directory;
    std::vector<std::pair<int, std::filesystem::path>> _nvdb_frames;
};
} // namespace objects::vdb
