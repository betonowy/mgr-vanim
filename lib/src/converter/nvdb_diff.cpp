#include "nvdb_diff.hpp"
#include <utils/nvdb_mmap.hpp>

#define PNANOVDB_C
#include <nanovdb/PNanoVDB.h>

namespace
{
using namespace converter;

void create_grid_diff(pnanovdb_buf_t src_buf, pnanovdb_buf_t dst_buf)
{
    pnanovdb_grid_handle_t src_grid{}, dst_grid{};

    if (pnanovdb_grid_get_magic(src_buf, src_grid) != PNANOVDB_MAGIC_NUMBER)
    {
        throw std::runtime_error("Corrupted source grid.");
    }

    if (pnanovdb_grid_get_magic(dst_buf, dst_grid) != PNANOVDB_MAGIC_NUMBER)
    {
        throw std::runtime_error("Corrupted destination grid.");
    }

    int _ = 0;
}
} // namespace

namespace converter
{
conversion_result create_nvdb_diff(std::filesystem::path src, std::filesystem::path dst)
{
    utils::nvdb_mmap src_mmap(src.string()), dst_mmap(dst.string());

    if (src_mmap.grids().size() != dst_mmap.grids().size())
    {
        throw std::runtime_error("NanoVDB files have different number of grids.");
    }

    std::vector<uint32_t> src_buf, dst_buf;
    static constexpr auto buf_value_type_sizeof = sizeof(decltype(src_buf)::value_type);

    for (size_t i = 0; i < src_mmap.grids().size(); ++i)
    {
        const auto &src_grid = src_mmap.grids()[i];
        const auto &dst_grid = dst_mmap.grids()[i];

        src_buf.clear(), dst_buf.clear();

        src_buf.resize(src_grid.size / buf_value_type_sizeof);
        dst_buf.resize(dst_grid.size / buf_value_type_sizeof);

        std::memcpy(src_buf.data(), src_grid.ptr, src_grid.size);
        std::memcpy(dst_buf.data(), dst_grid.ptr, dst_grid.size);

        pnanovdb_buf_t src_buf_handle, dst_buf_handle;

        src_buf_handle.data = src_buf.data();
        dst_buf_handle.data = dst_buf.data();

        create_grid_diff(src_buf_handle, dst_buf_handle);
    }

    return {};
}
} // namespace converter