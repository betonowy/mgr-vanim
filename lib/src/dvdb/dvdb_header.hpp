#pragma once

#include <cstdint>

namespace dvdb
{
static constexpr uint64_t MAGIC_NUMBER = 0x42445666666944; // DiffVDB
static constexpr uint64_t MAX_SUPPORTED_GRID_COUNT = 4;

enum class frame_type_e : uint64_t
{
    KEY_FRAME,
    DIFF_FRAME,
};

struct header
{
    uint64_t magic;             // DiffVDB
    frame_type_e frame_type;    // key and diff frames
    uint64_t vdb_grid_count;    // how many grids in this file
    uint64_t vdb_required_size; // how much data to allocate for final vsb

    struct
    {
        uint64_t base_tree_offset_start; // offset in this file
        uint64_t base_tree_copy_size;    // data to copy
        uint64_t base_tree_final_size;   // data to allocate
        uint64_t diff_data_offset_start; // diff offset in this file
    } frames[MAX_SUPPORTED_GRID_COUNT];
};
} // namespace dvdb
