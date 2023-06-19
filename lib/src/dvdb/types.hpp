#pragma once

#include <bitset>
#include <cmath>
#include <cstdint>

// Not thoroughly tested. If this breaks, just define an empty macro.
#ifndef _WIN32
#define dvdb_restrict __restrict__
#else
#define dvdb_restrict __restrict
#endif

namespace dvdb
{
static constexpr uint64_t MAGIC_NUMBER = 0x42445666666944; // DiffVDB
static constexpr uint64_t MAX_SUPPORTED_GRID_COUNT = 4;

template <typename T>
struct cube_888
{
    T values[512];

    bool is_avx_aligned() const
    {
        const auto uptr = reinterpret_cast<uintptr_t>(this);
        return (uptr & ~static_cast<uintptr_t>(32 - 1)) == uptr;
    }
};

using cube_888_f32 = cube_888<float>;
using cube_888_i8 = cube_888<uint8_t>;

struct cube_888_mask
{
    std::bitset<512> values;

    template <typename T, auto A, auto I>
    cube_888<T> as_values() const
    {
        cube_888<T> out;

        for (int i = 0; i < 512; ++i)
        {
            out.values[i] = values[i] ? A : I;
        }

        return out;
    }
};

namespace headers
{

struct main
{
    enum class frame_type_e : uint64_t
    {
        KEY_FRAME,
        DIFF_FRAME,
    };

    uint64_t magic = MAGIC_NUMBER; // DiffVDB
    frame_type_e frame_type;       // key and diff frames
    uint64_t vdb_grid_count;       // how many grids in this file
    uint64_t vdb_required_size;    // how much data to allocate for final vsb

    struct
    {
        uint64_t base_tree_offset_start; // offset in this file
        uint64_t base_tree_copy_size;    // data to copy
        uint64_t base_tree_final_size;   // data to allocate
        uint64_t diff_data_offset_start; // diff offset in this file
    } frames[MAX_SUPPORTED_GRID_COUNT];
};

struct block_description
{
    uint64_t dct_data_compressed_size;
    uint64_t dct_data_uncompressed_size;
};
} // namespace headers

namespace code_points
{
struct setup
{
    bool has_source : 1;
    bool has_rotation : 1;
    bool has_fma_and_new_mask : 1;
    bool has_values : 1;
    bool has_diff : 1;
    bool has_derivative : 1;
    bool has_dct : 1;
    bool has_map : 1;
};

struct rotation_offset
{
    int16_t x : 5;
    int16_t y : 5;
    int16_t z : 5;
};

struct fma
{
    float add, multiply;
};

struct map
{
    float min, max;
};

struct quantization
{
    uint8_t value;
};

using source_key = uint64_t;
// using dct_index = uint32_t;
} // namespace code_points
} // namespace dvdb
