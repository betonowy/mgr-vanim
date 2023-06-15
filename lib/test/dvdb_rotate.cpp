#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "dump.hpp"

#include <common.hpp>
#include <compression.hpp>
#include <dct.hpp>
#include <derivative.hpp>
#include <nvdb_mmap.hpp>
#include <resource_path.hpp>
#include <rotate.hpp>
#include <statistics.hpp>

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

    void append_lower_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_lower_handle_t lower, std::vector<std::pair<glm::ivec3, pnanovdb_leaf_handle_t>> *dest, glm::ivec3 coord)
    {
        for (int i = 0; i < PNANOVDB_LOWER_TABLE_COUNT; ++i)
        {
            if (!pnanovdb_lower_get_child_mask(buf, lower, i))
            {
                continue;
            }

            glm::ivec3 subcoord{(i >> 0) & 0xf, (i >> 4) & 0xf, (i >> 8) & 0xf};

            const auto leaf = pnanovdb_lower_get_child(PNANOVDB_GRID_TYPE_FLOAT, buf, lower, i);

            dest->emplace_back(subcoord + coord, leaf);
        }
    }

    template <typename T>
    void append_upper_leaves_from_nvdb(pnanovdb_buf_t buf, pnanovdb_upper_handle_t upper, T *dest, glm::ivec3 coord)
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

    template <typename T>
    void append_grid_leaves_from_nvdb(pnanovdb_buf_t buf, T *dest)
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
        : src_nvdb_mmap(utils::resource_path("test/src_big.nvdb")), dst_nvdb_mmap(utils::resource_path("test/dst_big.nvdb"))
    {
        static bool initialized = false;

        if (!initialized)
        {
            dvdb::dct_init(), initialized = true;
        }

        {
            const auto &grid = src_nvdb_mmap.grids()[0];
            src_buf = {reinterpret_cast<uint32_t *>(const_cast<void *>(grid.ptr))};
            append_grid_leaves_from_nvdb(src_buf, &src_pos_cubes);
            append_grid_leaves_from_nvdb(src_buf, &src_pos_cubes_vector);
        }

        {
            const auto &grid = dst_nvdb_mmap.grids()[0];
            dst_buf = {reinterpret_cast<uint32_t *>(const_cast<void *>(grid.ptr))};
            append_grid_leaves_from_nvdb(dst_buf, &dst_pos_cubes);
            append_grid_leaves_from_nvdb(dst_buf, &dst_pos_cubes_vector);
        }
    }

    pnanovdb_buf_t src_buf, dst_buf;
    utils::nvdb_mmap src_nvdb_mmap, dst_nvdb_mmap;
    std::unordered_map<glm::ivec3, pnanovdb_leaf_handle_t> src_pos_cubes, dst_pos_cubes;
    std::vector<std::pair<glm::ivec3, pnanovdb_leaf_handle_t>> src_pos_cubes_vector, dst_pos_cubes_vector;
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
//     static constexpr glm::vec3 high_coord{24.f, 25.f, 26.f};
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

//                             cubes[index].values[cube_index] = glm::length(vcoord) / glm::length(high_coord);
//                         }
//                     }
//                 }
//             }
//         }
//     }

//     for (int i = 0; i <= 16; ++i)
//     {
//         for (int j = 0; j <= 16; ++j)
//         {
//             for (int k = 0; k <= 16; ++k)
//             {
//                 const auto index = coord_to_neighbor_index(i, j, k);

//                 for (int x = 0; x < 8; ++x)
//                 {
//                     for (int y = 0; y < 8; ++y)
//                     {
//                         for (int z = 0; z < 8; ++z)
//                         {
//                             const auto cube_index = x + (y << 3) + (z << 6);

//                             glm::vec3 vcoord = glm::vec3(i, j, k) + glm::vec3(x, y, z);
//                             float expected = glm::length(vcoord) / glm::length(high_coord);

//                             dvdb::rotate_refill(&rotated, cubes, -(i - 8), -(j - 8), -(k - 8));

//                             REQUIRE(rotated.values[cube_index] == expected);
//                         }
//                     }
//                 }
//             }
//         }
//     }
// }

// TEST_CASE_METHOD(dvdb_init, "find_similars_astar_and_brute_force")
// {
//     dvdb::cube_888_f32 cubes[27], dst;

//     size_t count = 0, exact_count = 0;
//     double accumulated_error = 0;

//     auto nullnans = [](dvdb::cube_888_f32 *cube) {
//         for (int i = 0; i < std::size(cube->values); ++i)
//         {
//             if (std::isnan(cube->values[i]))
//             {
//                 cube->values[i] = 0;
//             }
//         }
//     };

//     for (const auto &[dpos, dleaf] : dst_pos_cubes)
//     {
//         {
//             auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, dst_buf, dleaf, 0);
//             std::memcpy(&dst, vdb_deref(table_address, dst_buf), sizeof(dst));
//         }

//         for (int i = 0; i < MAX_INDEX; ++i)
//         {
//             const auto spos = index_to_coord(i) + dpos;
//             const auto sleaf_it = src_pos_cubes.find(spos);

//             if (sleaf_it == src_pos_cubes.end())
//             {
//                 std::memset(cubes + i, 0, sizeof(*cubes));
//                 continue;
//             }

//             auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, src_buf, sleaf_it->second, 0);

//             std::memcpy(cubes + i, vdb_deref(table_address, src_buf), sizeof(*cubes));

//             nullnans(cubes + i);
//         }

//         glm::ivec3 rot_astar, rot_brute_force;
//         dvdb::cube_888_f32 converted;

//         dvdb::rotate_refill_find_astar(&dst, cubes, &rot_astar.x, &rot_astar.y, &rot_astar.z);
//         dvdb::rotate_refill_find_brute_force(&dst, cubes, &rot_brute_force.x, &rot_brute_force.y, &rot_brute_force.z);

//         if (rot_brute_force == rot_astar || (std::abs(rot_brute_force.x) > 6 || std::abs(rot_brute_force.y) > 6 || std::abs(rot_brute_force.z) > 6))
//         {
//             ++exact_count;
//         }

//         dvdb::rotate_refill(&converted, cubes, rot_brute_force.x, rot_brute_force.y, rot_brute_force.z);
//         float add, mul;

//         dvdb::linear_regression(&converted, &dst, &add, &mul);
//         dvdb::fma(&converted, &converted, add, mul);

//         if (count == 29)
//         {
//             int _ = 0;
//         }

//         accumulated_error += dvdb::mean_squared_error(&converted, &dst);
//         ++count;
//     }

//     accumulated_error /= count;

//     std::printf("final error == %f\n", accumulated_error);
//     std::printf("astar results in the exact same rotation %f%% of times.\n", exact_count * 100.f / count);
//     CHECK(accumulated_error < 0.005);
// }

TEST_CASE_METHOD(dvdb_init, "find_similars_brute_force")
{
    dvdb::cube_888_f32 cubes[27], dst;
    dvdb::cube_888_mask mask;

    int w0 = 0, w1 = 0, w2 = 0;

    std::puts("my, n8, n4, compression_ratio");

    for (int qi = 1; qi < 255; ++qi)
    {
        size_t count = 0;

        double acc_errorh = 0;
        double acc_error = 0;
        double acc_error4 = 0;
        double acc_error8 = 0;
        double acc_unmasked_error = 0;
        double acc_unmasked_error4 = 0;
        double acc_unmasked_error8 = 0;

        std::vector<dvdb::cube_888_i8> dct_cubes;

        auto nullnans = [](dvdb::cube_888_f32 *cube) {
            for (int i = 0; i < std::size(cube->values); ++i)
            {
                if (std::isnan(cube->values[i]))
                {
                    cube->values[i] = 0;
                }
            }
        };

        for (size_t di = 0; di < dst_pos_cubes_vector.size(); ++di)
        {
            const auto &dpos = dst_pos_cubes_vector[di].first;
            const auto &dleaf = dst_pos_cubes_vector[di].second;

            // if (di > 1000)
            // {
            //     continue;
            // }

            float sum = 0;

            {
                auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, dst_buf, dleaf, 0);
                std::memcpy(&dst, vdb_deref(table_address, dst_buf), sizeof(dst));
            }

            for (int i = 0; i < 512; ++i)
            {
                sum += dst.values[i];
            }

            // if (sum < 20.0)
            // {
            //     continue;
            // }

            {
                auto leafptr = reinterpret_cast<pnanovdb_leaf_t *>((dleaf.address.byte_offset >> 2) + dst_buf.data);
                std::memcpy(&mask, leafptr->value_mask, sizeof(mask));
            }

            dvdb::cube_888_f32 fmask = mask.as_values<float, 1, 0>();

            for (int i = 0; i < MAX_INDEX; ++i)
            {
                const auto spos = index_to_coord(i) + dpos;
                const auto sleaf_it = src_pos_cubes.find(spos);

                if (sleaf_it == src_pos_cubes.end())
                {
                    std::memset(cubes + i, 0, sizeof(*cubes));
                    continue;
                }

                auto table_address = pnanovdb_leaf_get_table_address(PNANOVDB_GRID_TYPE_FLOAT, src_buf, sleaf_it->second, 0);

                std::memcpy(cubes + i, vdb_deref(table_address, src_buf), sizeof(*cubes));

                nullnans(cubes + i);
            }

            glm::ivec3 rot;
            dvdb::cube_888_f32 converted;
            dvdb::cube_888_f32 reference8;
            dvdb::cube_888_f32 reference4;
            dvdb::cube_888_i8 h0, h1, h2;

            // make reference aka NanoVDB
            {
                dvdb::cube_888_i8 mid;
                float min, max;

                dvdb::encode_derivative_to_i8(&dst, &mid, &max, &min, 0xff);
                dvdb::decode_derivative_from_i8(&mid, &reference8, max, min, 0xff);
                h1 = mid;

                dvdb::encode_derivative_to_i8(&dst, &mid, &max, &min, 0x0f);
                dvdb::decode_derivative_from_i8(&mid, &reference4, max, min, 0x0f);
                h2 = mid;

                dct_cubes.push_back(mid);
            }

            dvdb::rotate_refill_find_astar(&dst, cubes, &rot.x, &rot.y, &rot.z);
            dvdb::rotate_refill(&converted, cubes, rot.x, rot.y, rot.z);

            // std::printf("best rotation: %d %d %d\n", rot.x, rot.y, rot.z);

            // dump("/tmp/sleaf.data", cubes[coord_to_neighbor_index(0, 0, 0)]);
            // dump("/tmp/dleaf.data", dst);

            float add, mul;

            dvdb::linear_regression_with_mask(&converted, &dst, &add, &mul, &fmask);
            dvdb::fma(&converted, &converted, add, mul);

            dvdb::cube_888_f32 diff;

            dvdb::sub(&dst, &converted, &diff);
            dvdb::mul(&diff, &fmask, &diff);

            {
                dvdb::cube_888_f32 dct;
                dvdb::cube_888_i8 mid;
                float min, max;

                uint8_t quant = 0x0f;

                // dvdb::dct_3d_encode(&diff, &dct);
                // dvdb::encode_derivative_to_i8(&dct, &mid, &max, &min, quant);

                // dct_cubes.push_back(mid);

                // dvdb::decode_derivative_from_i8(&mid, &dct, max, min, quant);
                // dvdb::dct_3d_decode(&dct, &diff);

                dvdb::encode_derivative_to_i8(&diff, &mid, &max, &min, quant);
                // h0 = mid;
                dct_cubes.push_back(mid);
                dvdb::decode_derivative_from_i8(&mid, &diff, max, min, quant);
            }

            dvdb::add(&converted, &diff, &converted);
            dvdb::mul(&diff, &fmask, &diff);

            if (count == 29)
            {
                int _ = 0;
                // continue;
            }

            float e0 = dvdb::mean_squared_error_with_mask(&converted, &dst, &fmask);
            float e1 = dvdb::mean_squared_error_with_mask(&reference8, &dst, &fmask);
            float e2 = dvdb::mean_squared_error_with_mask(&reference4, &dst, &fmask);

            acc_error += e0;
            acc_error8 += e1;
            acc_error4 += e2;

            // if (e0 < e1 && e0 < e2)
            // {
            //     ++w0;
            //     acc_errorh += e0;
            //     dct_cubes.push_back(h0);
            // }
            // else
            // {
            //     if (e1 < e2)
            //     {
            //         ++w1;
            //         acc_errorh += e1;
            //         dct_cubes.push_back(h1);
            //     }
            //     else
            //     {
            //         ++w2;
            //         acc_errorh += e2;
            //         dct_cubes.push_back(h2);
            //     }
            // }

            acc_unmasked_error += dvdb::mean_squared_error(&converted, &dst);
            acc_unmasked_error8 += dvdb::mean_squared_error(&reference8, &dst);
            acc_unmasked_error4 += dvdb::mean_squared_error(&reference4, &dst);

            ++count;
        }

        acc_error /= count;
        acc_error8 /= count;
        acc_error4 /= count;

        acc_unmasked_error /= count;
        acc_unmasked_error8 /= count;
        acc_unmasked_error4 /= count;

        std::printf("%f, ", sqrt(acc_error));
        std::printf("%f, ", sqrt(acc_error8));
        std::printf("%f, ", sqrt(acc_error4));

        // std::printf("final mine error == %f\n", sqrt(acc_error));
        // std::printf("final 8bit error == %f\n", sqrt(acc_error8));
        // std::printf("final 4bit error == %f\n", sqrt(acc_error4));

        // std::printf("unmasked mine error == %f\n", sqrt(acc_unmasked_error));
        // std::printf("unmasked 8bit error == %f\n", sqrt(acc_unmasked_error8));
        // std::printf("unmasked 4bit error == %f\n", sqrt(acc_unmasked_error4));

        // compression test
        std::vector<char> buffer(dct_cubes.size() * 512);

        int compressed_size = dvdb::compress_stream(reinterpret_cast<char *>(dct_cubes.data()), dct_cubes.size() * 512, buffer.data(), buffer.size());

        std::printf("%f\n", dct_cubes.size() * 512.f / compressed_size);
        // std::printf("w0: %d, w1: %d, w2: %d\n", w0, w1, w2);

        break;
    }
}
