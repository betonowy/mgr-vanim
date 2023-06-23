#pragma once

#include <cstdint>
#include <glm/vec4.hpp>
#include <vector>

namespace converter
{
int pack_nvdb_file(const char *filename);
glm::uvec4 unpack_nvdb_file(const char *filename, void *dest, size_t size, size_t *copied);
std::vector<char> unpack_nvdb_file(const char *filename, int *alignment_correction);
} // namespace converter
