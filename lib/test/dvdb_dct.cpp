#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <dct.hpp>
#include <derivative.hpp>

class dvdb_init
{
public:
    dvdb_init()
    {
        static bool initialized = false;

        if (!initialized)
        {
            dvdb::dct_init(), initialized = true;
        }
    }
};

TEST_CASE_METHOD(dvdb_init, "encode_f32_constant_component")
{
    dvdb::cube_888_f32 src, dst;

    static constexpr auto TEST_VALUE = 1;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        src.values[i] = TEST_VALUE;
    }

    dvdb::dct_3d_encode(&src, &dst);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        if (i == 0)
        {
            REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(TEST_VALUE, 1e-6));
        }
        else
        {
            REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(0, 1e-6));
        }
    }
}

TEST_CASE_METHOD(dvdb_init, "decode_f32_constant_component")
{
    dvdb::cube_888_f32 src{}, dst{};

    src.values[0] = 1;

    dvdb::dct_3d_decode(&src, &dst);

    for (int i = 0; i < std::size(dst.values); ++i)
    {
        REQUIRE_THAT(dst.values[i], Catch::Matchers::WithinAbsMatcher(1, 1e-6));
    }
}

TEST_CASE_METHOD(dvdb_init, "encode_decode_f32_smooth_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 2 * 3.141593 / 16.f);
    }

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::dct_3d_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dvdb_init, "encode_decode_f32_smooth_values_2")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x) * std::cos(y * 1.23) * std::cos(z * 1.11);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::dct_3d_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dvdb_init, "encode_decode_f32_random_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = rand() / static_cast<float>(RAND_MAX);
    }

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::dct_3d_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1e-5));
    }
}

TEST_CASE_METHOD(dvdb_init, "encode_decode_f32_smooth_values_optimized_max_128_values")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x) * std::cos(y * 1.23) * std::cos(z * 1.11);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_3d_encode(&src, &dct);
    {
        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count > 128);
    }
    {
        dvdb::dct_3d_optimize(&dct, 0.f, 128);

        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count == 128);
    }
    dvdb::dct_3d_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 8e-2));
    }
}

TEST_CASE_METHOD(dvdb_init, "encode_decode_f32_smooth_values_optimized_max_128_values_and_mse")
{
    dvdb::cube_888_f32 src{}, dct{}, res{};

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
        src.values[i] *= src.values[i];
    }

    dvdb::dct_3d_encode(&src, &dct);
    {
        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE(count > 128);
    }
    dvdb::dct_3d_decode(&dct, &res);
    {
        dvdb::dct_3d_optimize(&dct, 0.01f, 512);

        int count = 0;

        for (int i = 0; i < std::size(dct.values); ++i)
        {
            if (dct.values[i] != 0.f)
            {
                ++count;
            }
        }

        REQUIRE((count <= 512 && count > 0));
    }
    dvdb::dct_3d_decode(&dct, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.5f));
    }
}

TEST_CASE_METHOD(dvdb_init, "heavy_derivative_compression_255")
{
    dvdb::cube_888_f32 src{}, dct{}, res{}, dctder{}, dctderdct{}, dctderres{}, dctres{};
    dvdb::cube_888_i8 der8;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
        src.values[i] *= src.values[i];
    }

    float min, max;
    static constexpr uint8_t quantization_limit = 0xff;

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::encode_derivative_to_i8(&dct, &der8, &min, &max, quantization_limit);
    dvdb::decode_derivative_from_i8(&der8, &dctres, min, max, quantization_limit);
    dvdb::dct_3d_decode(&dctres, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.11f));
    }
}

TEST_CASE_METHOD(dvdb_init, "heavy_derivative_compression_127")
{
    dvdb::cube_888_f32 src{}, dct{}, res{}, dctder{}, dctderdct{}, dctderres{}, dctres{};
    dvdb::cube_888_i8 der8;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
        src.values[i] *= src.values[i];
    }

    float min, max;
    static constexpr uint8_t quantization_limit = 0x7f;

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::encode_derivative_to_i8(&dct, &der8, &min, &max, quantization_limit);
    dvdb::decode_derivative_from_i8(&der8, &dctres, min, max, quantization_limit);
    dvdb::dct_3d_decode(&dctres, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.19f));
    }
}

TEST_CASE_METHOD(dvdb_init, "heavy_derivative_compression_63")
{
    dvdb::cube_888_f32 src{}, dct{}, res{}, dctder{}, dctderdct{}, dctderres{}, dctres{};
    dvdb::cube_888_i8 der8;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
        src.values[i] *= src.values[i];
    }

    float min, max;
    static constexpr uint8_t quantization_limit = 0x3f;

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::encode_derivative_to_i8(&dct, &der8, &min, &max, quantization_limit);
    dvdb::decode_derivative_from_i8(&der8, &dctres, min, max, quantization_limit);
    dvdb::dct_3d_decode(&dctres, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        REQUIRE_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 0.43f));
    }
}

TEST_CASE_METHOD(dvdb_init, "heavy_derivative_compression_31")
{
    dvdb::cube_888_f32 src{}, dct{}, res{}, dctder{}, dctderdct{}, dctderres{}, dctres{};
    dvdb::cube_888_i8 der8;

    for (int i = 0; i < std::size(src.values); ++i)
    {
        int x = (i & 0b000000111) >> 0;
        int y = (i & 0b000111000) >> 3;
        int z = (i & 0b111000000) >> 6;

        src.values[i] = std::cos(x * 0.33 + y * 0.51 + z * 0.13 + 3) * std::cos(y * 0.63 + x * 0.37 + z * 0.44 + 0.3) + std::cos(z * 0.461 + x * 0.1 + y * 0.22 + 2);
        src.values[i] *= src.values[i];
    }

    float min, max;
    static constexpr uint8_t quantization_limit = 0x1f;

    dvdb::dct_3d_encode(&src, &dct);
    dvdb::encode_derivative_to_i8(&dct, &der8, &min, &max, quantization_limit);
    dvdb::decode_derivative_from_i8(&der8, &dctres, min, max, quantization_limit);
    dvdb::dct_3d_decode(&dctres, &res);

    for (int i = 0; i < std::size(src.values); ++i)
    {
        CHECK_THAT(src.values[i], Catch::Matchers::WithinAbsMatcher(res.values[i], 1.93f));
    }
}
