#include "error_calculator.hpp"

#include <dvdb/statistics.hpp>

namespace converter
{
error_result calculate_error(nvdb_reader lhs, nvdb_reader rhs)
{
    const auto count = lhs.leaf_count();

    error_result res{
        .min_error = std::numeric_limits<float>::max(),
        .max_error = -std::numeric_limits<float>::max(),
    };

    for (int i = 0; i < count; ++i)
    {
        const auto lhs_v = lhs.leaf_table_ptr(i);
        const auto rhs_v = rhs.leaf_table_ptr(i);

        const auto lhs_m = lhs.leaf_bitmask_ptr(i);
        const auto lhs_fm = lhs_m->as_values<float, 1, 0>();

        const float error = std::sqrt(dvdb::mean_squared_error_with_mask(lhs_v, rhs_v, &lhs_fm));

        if (res.min_error > error)
        {
            res.min_error = error;
        }

        if (res.max_error < error)
        {
            res.max_error = error;
        }

        res.error += error / count;
    }

    return res;
}
} // namespace converter
