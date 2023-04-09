#include "resource_path.hpp"

#include <iostream>

namespace
{
std::filesystem::path find_resource_path()
{
    auto current_directory = std::filesystem::current_path();

    for (size_t i = 0; i < 32; ++i)
    {
        for (auto &entry : std::filesystem::directory_iterator(current_directory))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            if (entry.path().filename() == "res")
            {
                std::cout << __func__ << ": Discovered resource directory: " << entry << '\n';

                return entry;
            }
        }

        current_directory = current_directory.parent_path();
    }

    throw std::runtime_error(std::string(__func__) + ": Failed to discover resource directory.");
}
} // namespace

namespace utils
{
std::filesystem::path resource_path(std::filesystem::path relative)
{
    static const auto path = find_resource_path();
    return path / relative;
}
} // namespace utils
