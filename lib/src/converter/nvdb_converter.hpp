#pragma once

#include "common.hpp"
#include <filesystem>

namespace converter
{
conversion_result convert_to_nvdb(std::filesystem::path);
}
