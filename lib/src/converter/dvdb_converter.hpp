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

private:
    std::shared_ptr<utils::thread_pool> _thread_pool;
    std::unique_ptr<dvdb_state> _state;
};
} // namespace converter
