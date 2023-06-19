#pragma once

#include "volume_resource_base.hpp"

namespace objects::vdb
{
class diff_vdb_resource : public volume_resource_base
{
public:
    explicit diff_vdb_resource(std::filesystem::path);
    ~diff_vdb_resource();

    void schedule_frame(scene::object_context &, int block_number, int frame_number) override;

    void init(scene::object_context &) override;

    const char *class_name() override;

    void on_destroy(scene::object_context &) override;

private:
    std::filesystem::path _resource_directory;
    std::vector<std::pair<int, std::filesystem::path>> _dvdb_frames;

    std::mutex _state_modification_mtx;

    std::vector<char> _current_state;
    std::vector<char> _created_state;
};
} // namespace objects::vdb
