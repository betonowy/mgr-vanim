#pragma once

namespace utils
{
    enum class cpu_available_feature_level_e
    {
        SSE2,
        SSE3,
        SSE41,
        SSE42,
        AVX,
        AVX2,
    };

    cpu_available_feature_level_e get_cpu_available_feature_level();

    const char* cpu_available_feature_level_str(cpu_available_feature_level_e);
}
