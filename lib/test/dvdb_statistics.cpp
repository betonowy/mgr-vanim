#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <statistics.hpp>

TEST_CASE("accumulate")
{
    dvdb::cube_888_f32 src;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = i + 0.5f;
    }

    float acc = dvdb::accumulate(&src);

    REQUIRE_THAT(acc, Catch::Matchers::WithinRelMatcher(std::size(src.values) * std::size(src.values) / 2.f, 0.001));
}
