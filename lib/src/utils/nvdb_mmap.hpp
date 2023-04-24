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

    nvdb_mmap(std::string path);

    const std::vector<grid> &grids() const
    {
        return _grids;
    }

    std::vector<std::string> grid_names() const;

private:
    std::vector<grid> _grids;
    mio::mmap_source _source;
};
} // namespace utils
