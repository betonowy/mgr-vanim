#pragma once

#include "common.hpp"

namespace converter {
conversion_result create_nvdb_diff(std::filesystem::path src, std::filesystem::path dst);
}
