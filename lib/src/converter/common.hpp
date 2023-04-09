#pragma once

#include <filesystem>
#include <variant>
#include <vector>

namespace converter
{
struct conversion_result
{
    std::string message;
    bool success = false;
};

std::vector<std::filesystem::path> find_files_with_extension(std::filesystem::path directory, std::string extension);
} // namespace converter
