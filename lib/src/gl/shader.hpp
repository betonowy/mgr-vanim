#pragma once

#include <GL/gl3w.h>

#include <string>
#include <vector>

namespace gl
{
class shader
{
public:
    struct source
    {
        enum class type_e
        {
            VERTEX,
            FRAGMENT,
            COMPUTE,
        };

        type_e type;
        std::string path;
    };

    explicit shader(std::vector<source>);
    ~shader();

    shader(const shader &) = delete;
    shader &operator=(const shader &) = delete;
    shader(shader &&);
    shader &operator=(shader &&);

    operator GLuint();

    void make_dirty();

private:
    void recompile_shader();

    std::vector<source> _sources;
    GLuint _id = 0;
    bool _dirty = true;
};
} // namespace gl
