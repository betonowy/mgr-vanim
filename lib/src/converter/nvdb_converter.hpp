#pragma once

#include <filesystem>
#include "common.hpp"

namespace converter
{
    conversion_result convert_to_nvdb(std::filesystem::path);
}
