#pragma once

#include <fstream>
#include <iostream>
#include <iterator>

#include <filesystem>
#include <types.hpp>

inline void dump(const char *path, dvdb::cube_888_f32 in)
{
    {
        namespace fs = std::filesystem;

        std::filesystem::path dir(path);
        dir = dir.parent_path();

        if (!fs::is_directory(dir) || !fs::exists(dir))
        {
            fs::create_directory(dir);
        }
    }

    std::ofstream binary(path, std::ios::binary);

    static constexpr float max = 1.0;
    static constexpr float min = 0.0;

    uint16_t values[512];

    for (auto &value : in.values)
    {
        value = value > max ? max : (value < min ? min : value);
    }

    size_t count = 0;
    size_t changes = 0;
    bool state = false;

    for (int i = 0; i < std::size(values); ++i)
    {
        values[i] = in.values[i] * std::numeric_limits<uint16_t>::max();

        if (values[i] != 0)
        {
            ++count;
        }

        bool new_state = values[i] != 0;

        if (i == 0)
        {
            state = new_state;
            continue;
        }

        if (state == new_state)
        {
            continue;
        }

        state = new_state;
        ++changes;
    }

    std::cout << path << ": " << count << " non zeros and " << changes << " state changes.\n";

    binary.write(reinterpret_cast<const char *>(values), sizeof(values));
}

inline void dump(const char *path, dvdb::cube_888_i8 in)
{
    {
        namespace fs = std::filesystem;

        std::filesystem::path dir(path);
        dir = dir.parent_path();

        if (!fs::is_directory(dir) || !fs::exists(dir))
        {
            fs::create_directory(dir);
        }
    }

    std::ofstream binary(path, std::ios::binary);

    uint16_t values[512];

    size_t count = 0;
    size_t changes = 0;
    bool state = false;

    for (int i = 0; i < std::size(values); ++i)
    {
        values[i] = static_cast<uint16_t>(in.values[i]) << 8;

        if (values[i] != 0)
        {
            ++count;
        }

        bool new_state = values[i] != 0;

        if (i == 0)
        {
            state = new_state;
            continue;
        }

        if (state == new_state)
        {
            continue;
        }

        state = new_state;
        ++changes;
    }

    std::cout << path << ": " << count << " non zeros and " << changes << " state changes.\n";

    binary.write(reinterpret_cast<const char *>(values), sizeof(values));
}
