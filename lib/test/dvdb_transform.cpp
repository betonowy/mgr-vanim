#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>
#include <transform.hpp>

TEST_CASE("cube888_dct_coord_test_f32")
{
    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    dvdb::transform_carthesian_to_hilbert(&src, &dst);
    dvdb::transform_hilbert_to_carthesian(&dst, &src_bis);

    REQUIRE(std::memcmp(&src, &dst, sizeof(src)) != 0);
    REQUIRE(std::memcmp(&src, &src_bis, sizeof(src)) == 0);
}

TEST_CASE("cube888_dct_coord_test_i8")
{
    dvdb::cube_888_i8 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand();
    }

    dvdb::transform_carthesian_to_hilbert(&src, &dst);
    dvdb::transform_hilbert_to_carthesian(&dst, &src_bis);

    REQUIRE(std::memcmp(&src, &dst, sizeof(src)) != 0);
    REQUIRE(std::memcmp(&src, &src_bis, sizeof(src)) == 0);
}

TEST_CASE("cube888_check_hilbert_properties")
{
    dvdb::cube_888_f32 src, dst;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i;
    }

    dvdb::transform_carthesian_to_hilbert(&src, &dst);

    int px = -1, py = 0, pz = 0;

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        int x = (static_cast<int>(dst.values[i]) & 0b000000111) >> 0;
        int y = (static_cast<int>(dst.values[i]) & 0b000111000) >> 3;
        int z = (static_cast<int>(dst.values[i]) & 0b111000000) >> 6;

        int coord_move = std::abs(x - px) + std::abs(y - py) + std::abs(z - pz);

        REQUIRE(coord_move == 1);

        px = x;
        py = y;
        pz = z;
    }
}

TEST_CASE("alignment")
{
    dvdb::cube_888_f32 src{};

    const auto fake = reinterpret_cast<dvdb::cube_888_f32 *>(321214211267);

    CHECK(src.is_avx_aligned());
    CHECK_FALSE(fake->is_avx_aligned());
}
