#pragma once

#include "dvdb_converter_nvdb.hpp"

namespace converter
{
    struct error_result
    {
        double error;
        double min_error;
        double max_error;
    };

    error_result calculate_error(nvdb_reader lhs, nvdb_reader rhs);
}
