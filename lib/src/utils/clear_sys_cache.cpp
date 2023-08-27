#include "clear_sys_cache.hpp"

#include <cstdio>
#include <memory>

namespace utils
{
void clear_sys_cache()
{
    std::system("sync");
    std::system("sudo -A sys_drop_caches");
}
} // namespace utils
