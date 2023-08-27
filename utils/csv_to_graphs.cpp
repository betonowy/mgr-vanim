#include <sciplot/sciplot.hpp>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

static constexpr auto MS_TO_S = 1e-3f;

struct csv_data_format
{
    size_t frame_number, compressed_size, data_size, wait_t1, wait_t2, map_t1, copy_t2, tdiff_mc;
};

std::vector<csv_data_format> load_csv_data(std::string_view path)
{
    std::ifstream file(std::string(path), std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Couldn't open file: " << path << '\n';
        std::exit(EXIT_FAILURE);
    }

    char buf[256];
    file.getline(buf, sizeof(buf));

    if (0 != std::strcmp(buf, "frame;comp_size;data_size;ts_pre_fence;ts_post_fence;ts_start;ts_end;td;"))
    {
        std::cerr << "Unexpected header: " << buf << '\n';
        std::exit(EXIT_FAILURE);
    }

    std::vector<csv_data_format> data;

    while (!file.getline(buf, sizeof(buf)).eof())
    {
        csv_data_format row_data;
        auto ptr = buf;

        auto read = [&](auto &in) -> void {
            const auto new_ptr = std::find(ptr, buf + sizeof(buf), ';');

            if (!new_ptr)
            {
                std::cerr << "Ran out of buffer\n";
                std::exit(EXIT_FAILURE);
            }

            std::stringstream ss(std::string(ptr, new_ptr));
            ss >> in;
            ptr = new_ptr + 1;
        };

        read(row_data.frame_number);
        read(row_data.compressed_size);
        read(row_data.data_size);
        read(row_data.wait_t1);
        read(row_data.wait_t2);
        read(row_data.map_t1);
        read(row_data.copy_t2);
        read(row_data.tdiff_mc);

        data.push_back(std::move(row_data));
    }

    return data;
}

struct converted_data_format
{
    size_t frame_number, compressed_size, data_size;
    float wt1_ms, wt2_ms, mt1_ms, ct2_ms, tdmc_ms;
};

std::vector<converted_data_format> convert(std::vector<csv_data_format> input)
{
    static constexpr auto US_TO_MS = 1e-3f;
    std::vector<converted_data_format> output;

    std::transform(std::begin(input), std::end(input), std::back_inserter(output),
                   [](const csv_data_format &in) -> converted_data_format {
                       return {
                           .frame_number = in.frame_number,
                           .compressed_size = in.compressed_size,
                           .data_size = in.data_size,
                           .wt1_ms = in.wait_t1 * US_TO_MS,
                           .wt2_ms = in.wait_t2 * US_TO_MS,
                           .mt1_ms = in.map_t1 * US_TO_MS,
                           .ct2_ms = in.copy_t2 * US_TO_MS,
                           .tdmc_ms = in.tdiff_mc * US_TO_MS,
                       };
                   });

    return output;
}

std::vector<converted_data_format> get_uncached(const std::vector<converted_data_format> &input)
{
    if (input.size() < 3)
    {
        std::cerr << "Not enough data (uncached)\n";
        std::exit(EXIT_FAILURE);
    }

    const auto frame_is_zero = [](const converted_data_format &row) {
        return row.frame_number == 0;
    };

    const auto it_begin_uncached = std::find_if(std::begin(input), std::end(input), frame_is_zero);
    const auto it_end_uncached = std::find_if(it_begin_uncached + 1, std::end(input), frame_is_zero);

    return {it_begin_uncached, it_end_uncached};
}

std::vector<converted_data_format> get_cached(const std::vector<converted_data_format> &input)
{
    if (input.size() < 3)
    {
        std::cerr << "Not enough data (uncached)\n";
        std::exit(EXIT_FAILURE);
    }

    const auto frame_is_zero = [](const converted_data_format &row) {
        return row.frame_number == 0;
    };

    const auto it_begin_uncached = std::find_if(std::begin(input), std::end(input), frame_is_zero);
    const auto it_end_uncached = std::find_if(it_begin_uncached + 1, std::end(input), frame_is_zero);
    const auto it_end_cached = std::find_if(it_end_uncached + 1, std::end(input), frame_is_zero);

    return {it_end_uncached, it_end_cached};
}

auto draw_plot(const std::vector<converted_data_format> &nvdb, const std::vector<converted_data_format> &dvdb, const char *xlabel, const char *ylabel, auto functor_x, auto functor_y)
{
    sciplot::Plot2D plot;

    plot.xlabel(xlabel);
    plot.ylabel(ylabel);
    plot.fontSize(10);

    auto plot_points = [&](const char *name, int point_type, const std::vector<converted_data_format> &source) {
        std::vector<double> x;
        std::vector<double> y;

        for (const auto &row : source)
        {
            x.push_back(functor_x(row));
            y.push_back(functor_y(row));
        }

        auto &plot_spec = plot.drawPoints(x, y);
        plot_spec.label(name);
        plot_spec.pointType(point_type);
    };

    plot_points("NanoVDB", 1, nvdb);
    plot_points("DiffVDB", 2, dvdb);

    return plot;
}

auto draw_time_plot(const std::vector<converted_data_format> &nvdb, const std::vector<converted_data_format> &dvdb)
{
    return draw_plot(
        nvdb, dvdb, "Frame number", "Frame time",
        [](const converted_data_format &row) {
            return row.frame_number;
        },
        [](const converted_data_format &row) {
            return row.tdmc_ms;
        });
}

auto draw_decomp_speed(const std::vector<converted_data_format> &nvdb, const std::vector<converted_data_format> &dvdb)
{
    return draw_plot(
        nvdb, dvdb, "Frame number", "Decompression speed [MB/s]",
        [](const converted_data_format &row) {
            return row.frame_number;
        },
        [](const converted_data_format &row) {
            return row.data_size / row.tdmc_ms * MS_TO_S;
        });
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Bad number of arguments :(\n";
        return EXIT_FAILURE;
    }

    const auto data_nvdb = convert(load_csv_data(argv[1]));
    const auto data_dvdb = convert(load_csv_data(argv[2]));

    const auto uncached_nvdb = get_uncached(data_nvdb);
    const auto cached_nvdb = get_cached(data_nvdb);
    const auto uncached_dvdb = get_uncached(data_dvdb);
    const auto cached_dvdb = get_cached(data_dvdb);

    // {
    //     const auto uncached_frame_time = draw_time_plot(uncached_nvdb, uncached_dvdb);
    //     const auto cached_frame_time = draw_time_plot(cached_nvdb, cached_dvdb);

    //     sciplot::Figure figure{{uncached_frame_time, cached_frame_time}};
    //     figure.title("Uncached and caches frame load times (lower is better)");

    //     sciplot::Canvas canvas{{figure}};
    //     canvas.size(2000, 1200);
    //     canvas.fontSize(10);
    //     canvas.show();
    // }
    // {
    //     const auto uncached_ds = draw_decomp_speed(uncached_nvdb, uncached_dvdb);
    //     const auto cached_ds = draw_decomp_speed(cached_nvdb, cached_dvdb);

    //     sciplot::Figure figure{{uncached_ds, cached_ds}};
    //     figure.title("Uncached and cached decompression speed [MB/s] (higher is better)");

    //     sciplot::Canvas canvas{{figure}};
    //     canvas.size(2000, 1200);
    //     canvas.fontSize(10);
    //     canvas.show();
    // }

    std::ofstream file("./out.txt");

    file << "frame nvdb_td dvdb_td dvdb_2_td";

    

    return EXIT_SUCCESS;
}
