#include "common.hpp"

namespace converter
{
std::vector<std::filesystem::path> find_files_with_extension(std::filesystem::path directory, std::string extension)
{
    auto dir_it = std::filesystem::directory_iterator(directory);

    std::vector<std::filesystem::path> files;

    for (const auto entry : dir_it)
    {
        if (entry.is_regular_file() && entry.path().extension() == extension)
        {
            files.push_back(entry);
        }
    }

    return files;
}
} // namespace converter
