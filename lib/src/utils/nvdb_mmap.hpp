#include <mio/mmap.hpp>

#include <vector>

namespace utils
{
class nvdb_mmap
{
public:
    struct grid
    {
        enum class type_size
        {
            unsupported,
            f32,
            f16,
            f8,
            f4,
            fn,
        };

        const void *ptr;
        size_t size;
        std::string_view name;
        type_size type;
    };

    explicit nvdb_mmap(const void *);

    explicit nvdb_mmap(std::string path);

    const std::vector<grid> &grids() const
    {
        return _grids;
    }

    std::vector<std::string> grid_names() const;

    size_t mem_size() const
    {
        return _in_memory ? _in_memory_size : _source.size();
    }

    size_t base_offset()
    {
        return reinterpret_cast<const char *>(_grids.front().ptr) - _source.data();
    }

private:
    size_t _in_memory_size = 0;

    std::vector<grid> _grids;
    mio::mmap_source _source;

    bool _in_memory = false;
};
} // namespace utils
