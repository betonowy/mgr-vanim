#pragma once

#include "common.hpp"
#include <filesystem>
#include <optional>

namespace converter
{
enum class nvdb_format
{
    F32, // 32-bit floats
    F16, // 16-bit floats
    F8,  // 8-bit floats
    F4,  // 4-bit floats
    FN,  // variable bit length floats
};

enum class nvdb_error_method
{
    absolute,
    relative,
};

conversion_result convert_to_nvdb(std::filesystem::path, nvdb_format, float error = 0.01, nvdb_error_method = nvdb_error_method::relative);
} // namespace converter
