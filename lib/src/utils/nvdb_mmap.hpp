#include <mio/mmap.hpp>

#include <vector>

namespace utils
{
class nvdb_mmap
{
public:
    struct grid
    {
        const void *ptr;
        size_t size;
    };

    nvdb_mmap(std::string path);

    const std::vector<grid> &grids() const
    {
        return _grids;
    }

private:
    std::vector<grid> _grids;
    mio::mmap_source _source;
};
} // namespace utils
