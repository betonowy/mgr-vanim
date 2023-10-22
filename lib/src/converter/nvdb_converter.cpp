#include "nvdb_converter.hpp"

#include <filesystem>
#include <mio/mmap.hpp>
#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/util/NanoToOpenVDB.h>
#include <nanovdb/util/OpenToNanoVDB.h>
#include <openvdb/openvdb.h>
#include <utils/nvdb_mmap.hpp>

#include "dvdb_converter_nvdb.hpp"
#include "error_calculator.hpp"
#include "nvdb_compressor.hpp"

#include <thread>

namespace converter
{
conversion_result convert_to_nvdb(std::filesystem::path path, nvdb_format format, float error, nvdb_error_method error_method)
{
    conversion_result res;

    if (!std::filesystem::exists(path))
    {
        return res.message = "File " + path.string() + " doesn't exist.", res;
    }

    if (!std::filesystem::is_regular_file(path))
    {
        return res.message = "File " + path.string() + " is not a regular file.", res;
    }

    if (!path.has_extension() || path.extension() != ".vdb")
    {
        return res.message = "File " + path.string() + " is not an OpenVDB file.", res;
    }

    auto nvdb_path = path;
    nvdb_path.replace_extension(".nvdb");

    std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> original_grids;

    {
        openvdb::io::File file(path.string());

        file.open();

        std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> nano_grids;

        for (auto name_it = file.beginName(); name_it != file.endName(); ++name_it)
        {
            if (name_it.gridName() != "density")
            {
                continue;
            }

            auto grid = file.readGrid(name_it.gridName());

            if (auto ovdb = openvdb::GridBase::grid<openvdb::FloatGrid>(grid))
            {
                switch (format)
                {
                case nvdb_format::F32: {
                    nanovdb::OpenToNanoVDB<float, float> converter;
                    converter.enableDithering();
                    nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    break;
                }
                case nvdb_format::F16: {
                    nanovdb::OpenToNanoVDB<float, nanovdb::Fp16> converter;
                    converter.enableDithering();
                    nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    break;
                }
                case nvdb_format::F8: {
                    nanovdb::OpenToNanoVDB<float, nanovdb::Fp8> converter;
                    converter.enableDithering();
                    nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    break;
                }
                case nvdb_format::F4: {
                    nanovdb::OpenToNanoVDB<float, nanovdb::Fp4> converter;
                    converter.enableDithering();
                    nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    break;
                }
                case nvdb_format::FN:
                    if (error_method == nvdb_error_method::absolute)
                    {
                        auto converter = nanovdb::OpenToNanoVDB<openvdb::FloatTree::BuildType, nanovdb::FpN, nanovdb::AbsDiff>();
                        converter.enableDithering();
                        converter.oracle() = nanovdb::AbsDiff(error);
                        nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    }
                    else
                    {
                        auto converter = nanovdb::OpenToNanoVDB<openvdb::FloatTree::BuildType, nanovdb::FpN, nanovdb::RelDiff>();
                        converter.enableDithering();
                        converter.oracle() = nanovdb::RelDiff(error);
                        nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                    }
                }

                {
                    nanovdb::OpenToNanoVDB<float, float> converter;
                    converter.enableDithering();
                    original_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
                }
            }
            else
            {
                // throw std::runtime_error("Unsupported OpenVDB grid type. Only float grid is supported.");
                std::cout << "Skipping unsupported grid: " << int(format) << '\n';
            }
        }

        file.close();

        nanovdb::io::writeGrids<nanovdb::HostBuffer, std::vector>(nvdb_path.string(), nano_grids);
    }

    const auto str8 = nvdb_path.string();
    const auto size = converter::pack_nvdb_file(str8.c_str());

    int correction{};
    auto compressed_buffer = converter::unpack_nvdb_file(str8.c_str(), &correction);
    compressed_buffer.erase(compressed_buffer.begin(), compressed_buffer.begin() + correction);
    auto f32_compressed_buffer = nvdb_to_nvdb_float(compressed_buffer);

    nvdb_reader org_rdr, new_rdr;
    utils::nvdb_mmap header_reader(f32_compressed_buffer.data());

    new_rdr.initialize(const_cast<void *>(header_reader.grids().front().ptr));
    org_rdr.initialize(original_grids.front().data());

    const auto error_result = calculate_error(std::move(org_rdr), std::move(new_rdr));

    std::cout << "[nvdb_converter] finished frame: " << path << '\n';

    res.e = error_result.error;
    res.em = error_result.min_error;
    res.ex = error_result.max_error;

    return res.message = nvdb_path.string(), res.success = true, res;
}

std::vector<char> nvdb_to_nvdb_float(const char *in)
{
    const auto in_grids = nanovdb::io::readGrids(in);
    std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> out_grids;
    out_grids.reserve(in_grids.size());

    for (const auto &in_grid : in_grids)
    {
        const auto ovdb = nanovdb::nanoToOpenVDB(in_grid);
        nanovdb::OpenToNanoVDB<float, float> converter;
        auto ovdb_float = openvdb::GridBase::grid<openvdb::FloatGrid>(ovdb);
        out_grids.push_back(converter(*ovdb_float, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
    }

    for (const auto &out_grid : out_grids)
    {
        const auto lol = out_grid.grid<float>();
        const auto a = lol->gridType();
    }

    std::stringstream ss;
    nanovdb::io::writeGrids(ss, out_grids);
    const auto string = ss.str();
    std::vector<char> buffer;
    std::copy(string.begin(), string.end(), std::back_inserter(buffer));

    return buffer;
}

std::vector<char> nvdb_to_nvdb_float(const std::vector<char> &in)
{
    std::stringstream ssin;
    ssin.write(in.data(), in.size());
    const auto in_grids = nanovdb::io::readGrids(ssin);
    std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> out_grids;
    out_grids.reserve(in_grids.size());

    for (const auto &in_grid : in_grids)
    {
        const auto ovdb = nanovdb::nanoToOpenVDB(in_grid);
        nanovdb::OpenToNanoVDB<float, float> converter;
        auto ovdb_float = openvdb::GridBase::grid<openvdb::FloatGrid>(ovdb);
        out_grids.push_back(converter(*ovdb_float, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
    }

    for (const auto &out_grid : out_grids)
    {
        const auto lol = out_grid.grid<float>();
        const auto a = lol->gridType();
    }

    static std::atomic<int> counter = 0;
    std::stringstream ss;
    std::vector<char> buffer;

    {
        ss << std::filesystem::temp_directory_path().string() + "/tempnano_to_nano" << counter++;
        nanovdb::io::writeGrids(ss.str(), out_grids);
        const auto file = mio::mmap_source(ss.str());
        std::copy(file.begin(), file.end(), std::back_inserter(buffer));
    }

    std::filesystem::remove(ss.str());

    return buffer;
}
} // namespace converter
