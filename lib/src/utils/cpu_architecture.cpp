#include "cpu_architecture.hpp"

#include <SDL2/SDL_cpuinfo.h>

namespace utils
{
cpu_available_feature_level_e get_cpu_available_feature_level()
{
    if (!SDL_HasSSE3())
    {
        return cpu_available_feature_level_e::SSE2;
    }

    if (!SDL_HasSSE41())
    {
        return cpu_available_feature_level_e::SSE3;
    }

    if (!SDL_HasSSE42())
    {
        return cpu_available_feature_level_e::SSE41;
    }

    if (!SDL_HasAVX())
    {
        return cpu_available_feature_level_e::SSE42;
    }

    if (!SDL_HasAVX2())
    {
        return cpu_available_feature_level_e::AVX;
    }

    return cpu_available_feature_level_e::AVX2;
}

const char *cpu_available_feature_level_str(cpu_available_feature_level_e value)
{
#define CASE(x)                            \
    case cpu_available_feature_level_e::x: \
        return #x

    switch (value)
    {
        CASE(SSE2);
        CASE(SSE3);
        CASE(SSE41);
        CASE(SSE42);
        CASE(AVX);
        CASE(AVX2);
    }

#undef CASE

    return "UNDEFINED";
}
} // namespace utils
