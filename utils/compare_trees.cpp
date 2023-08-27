#include "converter/dvdb_compressor.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>

#include <converter/dvdb_converter_nvdb.hpp>
#include <converter/nvdb_compressor.hpp>
#include <converter/nvdb_converter.hpp>
#include <nanovdb/PNanoVDB.h>
#include <nanovdb/util/IO.h>
#include <nanovdb/util/NanoToOpenVDB.h>
#include <nanovdb/util/OpenToNanoVDB.h>
#include <openvdb/openvdb.h>
#include <utils/nvdb_mmap.hpp>

struct TestFiles
{
    std::vector<std::filesystem::path> vdb;
    std::vector<std::filesystem::path> nvdb;
    std::vector<std::filesystem::path> dvdb;
};

TestFiles get_test_files(std::filesystem::path path)
{
    TestFiles output;

    for (const auto file : std::filesystem::directory_iterator(path))
    {
        if (!file.is_regular_file())
        {
            continue;
        }

        if (file.path().extension() == ".vdb")
        {
            output.vdb.emplace_back(file.path().string());
        }
        else if (file.path().extension() == ".nvdb")
        {
            output.nvdb.emplace_back(file.path().string());
        }
        else if (file.path().extension() == ".dvdb")
        {
            output.dvdb.emplace_back(file.path().string());
        }
    }

    std::ranges::sort(output.vdb);
    std::ranges::sort(output.nvdb);
    std::ranges::sort(output.dvdb);

    return output;
}

std::vector<char> open_to_nano_f32(std::filesystem::path path)
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
            nanovdb::OpenToNanoVDB<float, float> converter;
            converter.enableDithering();
            nano_grids.push_back(converter(*ovdb, nanovdb::StatsMode::All, nanovdb::ChecksumMode::Full, 0));
        }
        else
        {
            // throw std::runtime_error("Unsupported OpenVDB grid type. Only float grid is supported.");
            std::cout << "Skipping unsupported grid\n";
        }
    }

    file.close();

    std::stringstream ss;
    nanovdb::io::writeGrids(ss, nano_grids);
    const auto string = ss.str();
    std::vector<char> buffer;
    std::copy(string.begin(), string.end(), std::back_inserter(buffer));

    return buffer;
}

std::vector<char> nano_to_nano_f32(std::filesystem::path path)
{
    int alignment_correction{};
    const auto buffer = converter::unpack_nvdb_file(path.c_str(), &alignment_correction);
    const auto f32_nano_buffer = converter::nvdb_to_nvdb_float(const_cast<char *>(buffer.data() + alignment_correction));
    return f32_nano_buffer;
}

std::vector<char> dvdb_to_nano_f32(std::filesystem::path path)
{
    return converter::unpack_dvdb_file(path.c_str());
}

int main(int argc, char **argv)
{
    const auto dir = "/home/araara/Documents/vdb-animations/IndustrialChimneySmokeVDB/industrial_chimney_smoke/short";
    const auto files = get_test_files(dir);

    openvdb::initialize();

    auto vdb_buffer = open_to_nano_f32(files.vdb.front());
    auto nvdb_buffer = nano_to_nano_f32(files.nvdb.front());
    auto dvdb_buffer = dvdb_to_nano_f32(files.dvdb.front());

    converter::nvdb_reader vdb_reader, nvdb_reader, diff_reader;

    vdb_reader.initialize(vdb_buffer.data());
    nvdb_reader.initialize(nvdb_buffer.data());
    diff_reader.initialize(dvdb_buffer.data());

    return 0;
}
