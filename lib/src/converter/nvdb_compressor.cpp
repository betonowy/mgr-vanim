#include "nvdb_compressor.hpp"

#include <dvdb/compression.hpp>
#include <dvdb/types.hpp>
#include <utils/nvdb_mmap.hpp>

#include <mio/mmap.hpp>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace converter
{
static constexpr auto ALIGNMENT = 32; // must be a multiply of 2

int pack_nvdb_file(const char *filename)
{
    glm::uvec4 offsets(~0);
    int alignment_correction = 0;

    {
        utils::nvdb_mmap nvdb_mmap((std::string(filename)));

        offsets[0] = nvdb_mmap.base_offset();
        alignment_correction = ALIGNMENT - (offsets[0] & ALIGNMENT - 1);
        offsets[0] += alignment_correction;

        for (int i = 1; i < nvdb_mmap.grids().size(); ++i)
        {
            offsets[i] = offsets[i - 1] + nvdb_mmap.grids()[i - 1].size;
        }
    }

    mio::mmap_source mmap(filename);

    std::vector<char> output_buffer(mmap.size() + alignment_correction);
    std::vector<char> input_buffer(mmap.size() + alignment_correction);

    std::memcpy(input_buffer.data() + alignment_correction, mmap.data(), mmap.size());

    int compressed = dvdb::compress_stream(input_buffer.data(), input_buffer.size(), output_buffer.data(), output_buffer.size());

    if (compressed < 0)
    {
        throw std::runtime_error("Failed to compress stream!");
    }

    dvdb::headers::nvdb_block_description header{
        .compressed_size = static_cast<uint64_t>(compressed),
        .uncompressed_size = mmap.size() + alignment_correction,
        .offsets = offsets,
        .alignment_correction = alignment_correction,
    };

    mmap.unmap();

    std::ofstream file(filename, std::ios::binary);
    
    file.write(reinterpret_cast<char *>(&header), sizeof(header));
    file.write(output_buffer.data(), compressed);

    return compressed;
}

glm::uvec4 unpack_nvdb_file(const char *filename, void *dest, size_t size, size_t *copied)
{
    mio::mmap_source mmap(filename);

    const auto *header = reinterpret_cast<const dvdb::headers::nvdb_block_description *>(mmap.data());
    const char *data_begin = mmap.data() + sizeof(*header);

    int decompressed = dvdb::decompress_stream(data_begin, header->compressed_size, reinterpret_cast<char *>(dest), size);

    if (decompressed < 0)
    {
        throw std::runtime_error("Failed to decompress stream!");
    }

    *copied = decompressed;

    return header->offsets;
}

std::vector<char> unpack_nvdb_file(const char *filename, int *alignment_correction)
{
    mio::mmap_source mmap(filename);

    const auto *header = reinterpret_cast<const dvdb::headers::nvdb_block_description *>(mmap.data());
    const char *data_begin = mmap.data() + sizeof(*header);

    std::vector<char> buffer(header->uncompressed_size);
    int decompressed = dvdb::decompress_stream(data_begin, header->compressed_size, buffer.data(), buffer.size());

    if (decompressed < 0)
    {
        throw std::runtime_error("Failed to decompress stream!");
    }

    *alignment_correction = header->alignment_correction;

    return buffer;
}
} // namespace converter
