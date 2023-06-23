#include "dvdb_compressor.hpp"

#include <dvdb/compression.hpp>
#include <dvdb/types.hpp>
#include <mio/mmap.hpp>

#include <fstream>

namespace converter
{
int pack_dvdb_file(const char *filename)
{
    mio::mmap_source mmap(filename);

    const auto dvdb_header = reinterpret_cast<const dvdb::headers::main *>(mmap.data());

    std::vector<char> output_buffer(mmap.size());

    int compressed = dvdb::compress_stream(mmap.data(), mmap.size(), output_buffer.data(), output_buffer.size());

    if (compressed < 0)
    {
        throw std::runtime_error("Failed to compress stream!");
    }

    dvdb::headers::block_description header{
        .compressed_size = static_cast<uint64_t>(compressed),
        .uncompressed_size = dvdb_header->vdb_required_size,
    };

    mmap.unmap();

    std::ofstream file(filename, std::ios::binary);
    
    file.write(reinterpret_cast<char *>(&header), sizeof(header));
    file.write(output_buffer.data(), compressed);

    return compressed;
}

std::vector<char> unpack_dvdb_file(const char *filename)
{
    mio::mmap_source mmap(filename);

    const auto *header = reinterpret_cast<const dvdb::headers::block_description *>(mmap.data());
    const char *data_begin = mmap.data() + sizeof(*header);

    std::vector<char> output(header->uncompressed_size);
    int decompressed = dvdb::decompress_stream(data_begin, header->compressed_size, output.data(), output.size());

    if (decompressed < 0)
    {
        throw std::runtime_error("Failed to decompress stream!");
    }

    return output;
}
} // namespace converter
