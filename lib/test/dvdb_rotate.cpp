#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "dump.hpp"

#include <dct.hpp>
#include <fma.hpp>
#include <nvdb_mmap.hpp>
#include <resource_path.hpp>
#include <rotate.hpp>

#include <glm/glm.hpp>
#include <nanovdb/PNanoVDB.h>

#include <unordered_map>

template <>
struct std::hash<glm::ivec3>
{
    std::size_t operator()(const glm::ivec3 &s) const noexcept
    {
        std::size_t h1 = std::hash<int>()(s.x);
        std::size_t h2 = std::hash<int>()(s.y);
        std::size_t h3 = std::hash<int>()(s.z);

        return h1 ^ (h2 << 11) ^ (h3 << 23);
    }
};

class dvdb_init
{
    void append_lower_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_lower_handle_t lower, std::unordered_map<glm::ivec3, pnanovdb_leaf_handle_t> *dest, glm::ivec3 coord)
    {
        for (int i = 0; i < PNANOVDB_LOWER_TABLE_COUNT; ++i)
        {
            if (!pnanovdb_lower_get_child_mask(buf, lower, i))
            {
                continue;
            }

            glm::ivec3 subcoord{(i >> 0) & 0xf, (i >> 4) & 0xf, (i >> 8) & 0xf};

            const auto leaf = pnanovdb_lower_get_child(PNANOVDB_GRID_TYPE_FLOAT, buf, lower, i);

            dest->emplace(subcoord + coord, leaf);
        }
    }

    void append_upper_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_upper_handle_t upper, std::unordered_map<glm::ivec3, pnanovdb_leaf_handle_t> *dest, glm::ivec3 coord)
    {
        for (int i = 0; i < PNANOVDB_UPPER_TABLE_COUNT; ++i)
        {
            if (!pnanovdb_upper_get_child_mask(buf, upper, i))
            {
                continue;
            }

            glm::ivec3 subcoord{(i >> 0) & 0x1f, (i >> 5) & 0x1f, (i >> 10) & 0x1f};

            append_lower_leaves_from_nvdb(buf, pnanovdb_upper_get_child(PNANOVDB_GRID_TYPE_FLOAT, buf, upper, i), dest, coord + (subcoord << 4));
        }
    }

    void append_grid_leaves_from_nvdb(pnanovdb_buf_t buf, std::unordered_map<glm::ivec3, pnanovdb_leaf_handle_t> *dest)
    {
        const pnanovdb_grid_handle_t grid{};
        const pnanovdb_grid_type_t type = pnanovdb_grid_get_grid_type(buf, grid);
        const auto tree = pnanovdb_grid_get_tree(buf, grid);

        REQUIRE(type == PNANOVDB_GRID_TYPE_FLOAT);

        const auto root = pnanovdb_tree_get_root(buf, tree);
        const auto tile_count = pnanovdb_root_get_tile_count(buf, root);

        for (int i = 0; i < tile_count; ++i)
        {
            const auto tile = pnanovdb_root_get_tile(type, root, i);
            const auto upper = pnanovdb_root_get_child(PNANOVDB_GRID_TYPE_FLOAT, buf, root, tile);
            const auto key = pnanovdb_root_tile_get_key(buf, tile);

            glm::ivec3 coord{((key >> 42) & 0x1fffff), ((key >> 21) & 0x1fffff), ((key >> 0) & 0x1fffff)};

            append_upper_leaves_from_nvdb(buf, upper, dest, coord << (4 + 5));
        }
    }

public:
    dvdb_init()
        : src_nvdb_mmap(utils::resource_path("test/src.nvdb")), dst_nvdb_mmap(utils::resource_path("test/dst.nvdb"))
    {
        static bool initialized = false;

        if (!initialized)
        {
            dvdb::dct_init(), initialized = true;
        }

        {
            const auto &grid = src_nvdb_mmap.grids()[1];
            src_buf = {reinterpret_cast<uint32_t *>(const_cast<void *>(grid.ptr))};
            append_grid_leaves_from_nvdb(src_buf, &src_pos_cubes);
        }

        {
            const auto &grid = dst_nvdb_mmap.grids()[1];
            dst_buf = {reinterpret_cast<uint32_t *>(const_cast<void *>(grid.ptr))};
            append_grid_leaves_from_nvdb(dst_buf, &dst_pos_cubes);
        }
    }

    pnanovdb_buf_t src_buf, dst_buf;
    utils::nvdb_mmap src_nvdb_mmap, dst_nvdb_mmap;
    std::unordered_map<glm::ivec3, pnanovdb_leaf_handle_t> src_pos_cubes, dst_pos_cubes;
};

void make_cubes(dvdb::cube_888_f32 *dst, int n)
{
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < std::size(dst->values); ++j)
        {
            dst[i].values[j] = j + i * 1000;
        }
    }
}

constexpr int index_to_x(int i)
{
    return i & 0b111;
}

constexpr int index_to_y(int i)
{
    return (i >> 3) & 0b111;
}

constexpr int index_to_z(int i)
{
    return (i >> 6) & 0b111;
}

TEST_CASE("no_rotation")
{
    dvdb::cube_888_f32 cubes[27], dst;
    make_cubes(cubes, std::size(cubes));
    dvdb::rotate_refill(&dst, cubes, 0, 0, 0);

    for (int i = 0; i < std::size(cubes->values); ++i)
    {
        REQUIRE(cubes[1 + 3 + 9].values[i] == dst.values[i]);
    }
}

TEST_CASE("x_only")
{
    dvdb::cube_888_f32 cubes[27], dst;
    make_cubes(cubes, std::size(cubes));

    dvdb::rotate_refill(&dst, cubes, 1, 0, 0);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int x = index_to_x(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (x == 0 ? 12 : 13));
    }

    dvdb::rotate_refill(&dst, cubes, -1, 0, 0);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int x = index_to_x(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (x == 7 ? 14 : 13));
    }
}

TEST_CASE("y_only")
{
    dvdb::cube_888_f32 cubes[27], dst;
    make_cubes(cubes, std::size(cubes));

    dvdb::rotate_refill(&dst, cubes, 0, 1, 0);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int y = index_to_y(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (y == 0 ? 10 : 13));
    }

    dvdb::rotate_refill(&dst, cubes, 0, -1, 0);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int y = index_to_y(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (y == 7 ? 16 : 13));
    }
}

TEST_CASE("z_only")
{
    dvdb::cube_888_f32 cubes[27], dst;
    make_cubes(cubes, std::size(cubes));

    dvdb::rotate_refill(&dst, cubes, 0, 0, 1);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int z = index_to_z(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (z == 0 ? 4 : 13));
    }

    dvdb::rotate_refill(&dst, cubes, 0, 0, -1);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int z = index_to_z(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == (z == 7 ? 22 : 13));
    }
}

TEST_CASE("xyz_corner")
{
    dvdb::cube_888_f32 cubes[27], dst;
    make_cubes(cubes, std::size(cubes));

    dvdb::rotate_refill(&dst, cubes, 8, 8, 8);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int z = index_to_z(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == 0);
    }

    dvdb::rotate_refill(&dst, cubes, -8, -8, -8);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int z = index_to_z(i);
        REQUIRE(static_cast<int>(dst.values[i] / 1000) == 26);
    }
}

static constexpr int MAX_INDEX = 27;

static constexpr int coord_to_neighbor_index(int x, int y, int z)
{
    return (x + 1) + (y + 1) * 3 + (z + 1) * 3 * 3;
}

static constexpr glm::ivec3 index_to_coord(int i)
{
    glm::ivec3 ret;

    ret.x = i % 3;
    ret.y = (i / 3) % 3;
    ret.z = (i / 9) % 3;

    return ret - 1;
}

static constexpr void *vdb_deref(pnanovdb_address_t address, pnanovdb_buf_t buf)
{
    return buf.data + (address.byte_offset >> 2);
}

// TEST_CASE("Gradient rotation")
// {
//     dvdb::cube_888_f32 cubes[27], rotated;

//     for (int i = -1; i <= 1; ++i)
//     {
//         for (int j = -1; j <= 1; ++j)
//         {
//             for (int k = -1; k <= 1; ++k)
//             {
//                 const auto index = coord_to_neighbor_index(i, j, k);

//                 for (int x = 0; x < 8; ++x)
//                 {
//                     for (int y = 0; y < 8; ++y)
//                     {
//                         for (int z = 0; z < 8; ++z)
//                         {
//                             const auto cube_index = x + (y << 3) + (z << 6);

//                             glm::vec3 vcoord = (glm::vec3(i, j, k) + 1.f) * 8.f + glm::vec3(x, y, z);
//                             static constexpr glm::vec3 high_coord{24.f, 24.f, 24.f};

//                             cubes[index].values[cube_index] = glm::length(vcoord) / glm::length(high_coord);
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     dvdb::rotate_refill(&rotated, cubes, -8, -8, -8);
// }

TEST_CASE_METHOD(dvdb_init, "find_similars")
{
    dvdb::cube_888_f32 cubes[27], dst;

    for (const auto &[dpos, dleaf] : dst_pos_cubes)
    {
        {
            auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, dst_buf, dleaf, 0);
            std::memcpy(&dst, vdb_deref(table_address, dst_buf), sizeof(dst));
        }

        for (int i = 0; i < MAX_INDEX; ++i)
        {
            const auto spos = index_to_coord(i) + dpos;
            const auto sleaf_it = src_pos_cubes.find(spos);

            if (sleaf_it == src_pos_cubes.end())
            {
                std::memset(cubes + 1, 0, sizeof(*cubes));
                continue;
            }

            auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, src_buf, sleaf_it->second, 0);

            std::memcpy(cubes + i, vdb_deref(table_address, src_buf), sizeof(*cubes));
        }

        glm::ivec3 rot;
        dvdb::cube_888_f32 converted;

        dvdb::rotate_refill_find(&dst, cubes, &rot.x, &rot.y, &rot.z);
        dvdb::rotate_refill(&converted, cubes, rot.x, rot.y, rot.z);

        std::printf("best rotation: %d %d %d\n", rot.x, rot.y, rot.z);

        // dump("/tmp/sleaf.data", cubes[coord_to_neighbor_index(0, 0, 0)]);
        // dump("/tmp/dleaf.data", dst);
        // dump("/tmp/conve.data", converted);

        float add, mul;

        dvdb::find_fused_multiply_add(&converted, &dst, &add, &mul);
        dvdb::fused_multiply_add(&converted, &converted, add, mul);

        // dump("/tmp/conve_fma.data", converted);

        int _ = 0;
        break;
    }
}
