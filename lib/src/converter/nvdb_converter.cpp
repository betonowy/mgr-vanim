#include "nvdb_converter.hpp"

#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/util/OpenToNanoVDB.h>
#include <openvdb/openvdb.h>

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
                throw std::runtime_error("Unsupported OpenVDB grid type. Only float grid is supported.");
            }
        }

        file.close();

        nanovdb::io::writeGrids(nvdb_path.string(), nano_grids);
    }

    return res.message = nvdb_path.string(), res.success = true, res;
}
} // namespace converter
