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

    int nvdb_read_size = 0;
    int nvdb_write_size = 0;
};

std::vector<std::filesystem::path> find_files_with_extension(std::filesystem::path directory, std::string extension);
} // namespace converter
