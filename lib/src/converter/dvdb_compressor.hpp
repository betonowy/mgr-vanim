#pragma once

#include <cstdint>
#include <vector>

namespace converter
{
int pack_dvdb_file(const char *filename);
std::vector<char> unpack_dvdb_file(const char *filename);
}
