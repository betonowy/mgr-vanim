#pragma once

#include "volume_resource_base.hpp"

namespace objects::vdb
{
class nano_vdb_resource : public volume_resource_base
{
public:
    explicit nano_vdb_resource(std::filesystem::path);

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

private:
    std::filesystem::path _resource_directory;
    std::vector<std::pair<int, std::filesystem::path>> _nvdb_frames;

    struct frame
    {
        std::shared_ptr<gl::shader_storage> ssbo;
        glm::uvec4 offsets;
        int frame_number;
    };

    std::vector<std::future<frame>> _incoming_frames;
    std::vector<int> _scheduled_frames;
    std::vector<frame> _loaded_frames;
    std::vector<frame> _free_frames;
    std::vector<int> _temp_int_vector;

    int frame_count = 0;

    std::shared_ptr<frame> bkg_frame;
    std::shared_ptr<frame> view_frame;

    size_t max_buffer_size;
};
} // namespace objects::vdb
