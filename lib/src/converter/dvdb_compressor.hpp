#pragma once

#include <cstdint>
#include <vector>

namespace converter
{
int pack_file(const char *filename);
std::vector<char> unpack_file(const char *filename);
}
