#include "nvdb_converter.hpp"

#include <nanovdb/NanoVDB.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/util/OpenToNanoVDB.h>
#include <openvdb/openvdb.h>

#include <thread>

namespace converter
{
conversion_result convert_to_nvdb(std::filesystem::path path)
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
            nano_grids.push_back(nanovdb::openToNanoVDB(grid, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full));
        }

        file.close();

        nanovdb::io::writeGrids(nvdb_path, nano_grids);
    }

    return res.message = nvdb_path.string(), res.success = true, res;
}
} // namespace converter
