#include "shader.hpp"

#include <mio/mmap.hpp>
#include <stdexcept>
#include <utility>

namespace gl
{
shader::shader(std::vector<source> source_param) : _sources(std::move(source_param))
{
    if (_sources.empty())
    {
        throw std::runtime_error(std::string(__func__) + ": No sources passed.");
    }
}

shader::~shader()
{
    if (_id)
    {
        glDeleteProgram(_id);
    }
}

shader::shader(shader &&other)
    : _id(std::exchange(other._id, 0)), _dirty(std::exchange(other._id, true)), _sources(std::move(other._sources))
{
}

shader &shader::operator=(shader &&other)
{
    if (&other == this)
    {
        return *this;
    }

    _id = std::exchange(other._id, 0);
    _dirty = std::exchange(other._id, true);
    _sources = std::move(other._sources);

    return *this;
}

shader::operator GLuint()
{
    if (_dirty)
    {
        recompile_shader();
    }

    return _id;
}

void shader::make_dirty()
{
    _dirty = true;
}

namespace
{
GLuint compileShader(const std::vector<shader::source> &sources, shader::source::type_e type)
{
    std::vector<std::string> read_sources;
    std::vector<const char *> source_map;

    for (const auto &source : sources)
    {
        if (source.type == type)
        {
            mio::mmap_source file(source.path);

            if (!file.is_open() || !file.is_mapped())
            {
                std::runtime_error(std::string(__func__) + ": Couldn't map file: " + source.path);
            }

            read_sources.emplace_back(file.data(), file.size());
        }
    }

    if (read_sources.empty())
    {
        return 0;
    }

    static constexpr auto VERSION_STRING = "#version 450\n";
    static constexpr auto RESET_LINE = "#line 1\n";

    source_map.push_back(VERSION_STRING);

    for (const auto &source : read_sources)
    {
        source_map.push_back(RESET_LINE);
        source_map.push_back(source.c_str());
    }

    if (!source_map.empty())
    {
        GLenum gl_type = [type]() {
            switch (type)
            {
            case shader::source::type_e::COMPUTE:
                return GL_COMPUTE_SHADER;
            case shader::source::type_e::VERTEX:
                return GL_VERTEX_SHADER;
            case shader::source::type_e::FRAGMENT:
                return GL_FRAGMENT_SHADER;
            default:
                return GL_NONE;
            }
        }();

        GLuint shader = glCreateShader(gl_type);

        glShaderSource(shader, source_map.size(), source_map.data(), NULL);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::string buffer(length + 1, 0);
            glGetShaderInfoLog(shader, length, nullptr, buffer.data());

            glDeleteShader(shader);
            throw std::runtime_error(std::string(__func__) + ": shader compilation failed:\n\n" + buffer);
        }

        return shader;
    }

    return 0;
}

void try_append_shader(const std::vector<shader::source> &sources, std::vector<GLuint> &shaders, shader::source::type_e type)
{
    std::vector<const char *> _computeShaderSources;

    const auto shader = compileShader(sources, type);

    if (shader)
    {
        shaders.push_back(shader);
    }
}
} // namespace

void shader::recompile_shader()
{
    std::vector<GLuint> shaders;

    try
    {
        try_append_shader(_sources, shaders, shader::source::type_e::COMPUTE);
        try_append_shader(_sources, shaders, shader::source::type_e::VERTEX);
        try_append_shader(_sources, shaders, shader::source::type_e::FRAGMENT);

        if (shaders.empty())
        {
            throw std::runtime_error(std::string(__func__) + ": No shaders compiled.");
        }

        if (_id)
        {
            glDeleteProgram(_id);
        }

        _id = glCreateProgram();

        for (const auto &shader : shaders)
        {
            glAttachShader(_id, shader);
        }

        glLinkProgram(_id);

        GLint status;
        glGetProgramiv(_id, GL_LINK_STATUS, &status);

        if (status == GL_FALSE)
        {
            GLint length = 0;
            glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &length);
            std::string buffer(length + 1, 0);
            glGetProgramInfoLog(_id, length, nullptr, buffer.data());

            glDeleteProgram(_id);
            _id = 0;
            throw std::runtime_error(std::string(__func__) + ": shader compilation failed:\n\n" + buffer);
        }

        for (const auto &shader : shaders)
        {
            glDeleteShader(shader);
        }

        _dirty = false;
    }
    catch (std::runtime_error &e)
    {
        for (const auto &shader : shaders)
        {
            glDeleteShader(shader);
        }

        throw e;
    }
}
} // namespace gl
