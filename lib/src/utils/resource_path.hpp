#pragma once

#include <filesystem>

namespace utils
{
std::string resource_path(std::filesystem::path relative);
}
