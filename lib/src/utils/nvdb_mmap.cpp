#include "nvdb_mmap.hpp"

#include <nanovdb/util/IO.h>

#include <array>

namespace utils
{
nvdb_mmap::nvdb_mmap(std::string path) : _source(path)
{
    using header_t = nanovdb::io::Header;
    using meta_t = nanovdb::io::MetaData;

    static constexpr size_t MAX_GRIDS = 2;

    struct grid_meta_t
    {
    public:
        const meta_t *metadata;
        std::string_view grid_name;
    };

    size_t read_offset = 0;

    const auto header = reinterpret_cast<const header_t *>(_source.data());
    read_offset += sizeof(header_t);

    if (!_source.is_open() || header->magic != NANOVDB_MAGIC_NUMBER)
    {
        // first check for byte-swapped header magic.
        if (header->magic == nanovdb::io::reverseEndianness(NANOVDB_MAGIC_NUMBER))
            throw std::runtime_error("This nvdb file has reversed endianness");
        throw std::runtime_error("Magic number error: This is not a valid nvdb file");
    }
    else if (header->version.getMajor() != NANOVDB_MAJOR_VERSION_NUMBER)
    {
        std::stringstream ss;
        if (header->version.getMajor() < NANOVDB_MAJOR_VERSION_NUMBER)
        {
            ss << "The file contains an older version of NanoVDB: " << std::string(header->version.c_str()) << "!\n\t"
               << "Recommendation: Re-generate this NanoVDB file with this version: " << NANOVDB_MAJOR_VERSION_NUMBER << ".X of NanoVDB";
        }
        else
        {
            ss << "This tool was compiled against an older version of NanoVDB: " << NANOVDB_MAJOR_VERSION_NUMBER << ".X!\n\t"
               << "Recommendation: Re-compile this tool against the newer version: " << header->version.getMajor() << ".X of NanoVDB";
        }
        throw std::runtime_error("An unrecoverable error in nanovdb::Segment::read:\n\tIncompatible file format: " + ss.str());
    }

    std::array<grid_meta_t, MAX_GRIDS> metadata;

    for (size_t i = 0; i < header->gridCount; ++i)
    {
        metadata[i].metadata = reinterpret_cast<const meta_t *>(_source.data() + read_offset);
        read_offset += sizeof(meta_t);

        metadata[i].grid_name = std::string_view(_source.data() + read_offset, metadata[i].metadata->nameSize);
        read_offset += metadata[i].metadata->nameSize;
    }

    _grids.resize(header->gridCount);

    for (size_t i = 0; i < header->gridCount; ++i)
    {
        _grids[i].ptr = _source.data() + read_offset;
        _grids[i].size = metadata[i].metadata->gridSize;
        read_offset += metadata[i].metadata->fileSize;
    }

    if (read_offset != _source.size())
    {
        throw std::runtime_error("Parsed block size does not equal to mapped file size: (read_offset == " + std::to_string(read_offset) +
                                 ") != (_source.size() == " + std::to_string(_source.size()) + ')');
    }
}
} // namespace utils