#include <catch2/catch_test_macros.hpp>

#include <coord_transform.hpp>
#include <cstring>

TEST_CASE("cube888_dct_coord_test_f32")
{
    dvdb::coord_transform_init();

    dvdb::cube_888_f32 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    dvdb::coord_transform_carthesian_to_zigzag(&src, &dst);
    dvdb::coord_transform_zigzag_to_carthesian(&dst, &src_bis);

    REQUIRE(std::memcmp(&src, &dst, sizeof(src)) != 0);
    REQUIRE(std::memcmp(&src, &src_bis, sizeof(src)) == 0);
}

TEST_CASE("cube888_dct_coord_test_i8")
{
    dvdb::coord_transform_init();

    dvdb::cube_888_i8 src, dst, src_bis;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = rand();
    }

    dvdb::coord_transform_carthesian_to_zigzag(&src, &dst);
    dvdb::coord_transform_zigzag_to_carthesian(&dst, &src_bis);

    REQUIRE(std::memcmp(&src, &dst, sizeof(src)) != 0);
    REQUIRE(std::memcmp(&src, &src_bis, sizeof(src)) == 0);
}
