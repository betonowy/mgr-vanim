#pragma once

#include "common.hpp"

#include <memory>

namespace utils
{
class thread_pool;
}

namespace converter
{
struct dvdb_state;

class dvdb_converter
{
public:
    dvdb_converter(std::shared_ptr<utils::thread_pool> = {});
    ~dvdb_converter();

    void create_keyframe(std::filesystem::path);
    void add_diff_frame(std::filesystem::path);
    conversion_result process_next();

    float current_compression_ratio();
    std::string current_step();

    size_t current_total_leaves();
    size_t current_processed_leaves();

private:
    void set_status(std::string);

    std::shared_ptr<utils::thread_pool> _thread_pool;
    std::unique_ptr<dvdb_state> _state;
};
} // namespace converter
