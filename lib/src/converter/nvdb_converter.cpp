#include "nvdb_converter.hpp"

#include <mio/mmap.hpp>
#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/util/OpenToNanoVDB.h>
#include <nanovdb/util/NanoToOpenVDB.h>
#include <openvdb/openvdb.h>

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

    converter::pack_nvdb_file(nvdb_path.c_str());

    return res.message = nvdb_path.string(), res.success = true, res;
}

std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> nvdb_to_nvdb_fp8(const char* in)
{
    const auto in_grids = nanovdb::io::readGrids(in);
    std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> out_grids;
    out_grids.reserve(in_grids.size());

    for (const auto& in_grid : in_grids)
    {
        const auto ovdb = nanovdb::nanoToOpenVDB(in_grid);
        nanovdb::OpenToNanoVDB<float, nanovdb::Fp8> converter;
        auto ovdb_float = openvdb::GridBase::grid<openvdb::FloatGrid>(ovdb);
        out_grids.push_back(converter(*ovdb_float, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
    }

    for (const auto& out_grid : out_grids)
    {
        const auto lol = out_grid.grid<nanovdb::Fp8>();
        const auto a = lol->gridType();
    }

    return out_grids;
}
} // namespace converter
