#pragma once

#include <GL/gl3w.h>

namespace gl
{
void message_callback([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id, [[maybe_unused]] GLenum severity, [[maybe_unused]] GLsizei length, const GLchar *message, [[maybe_unused]] const void *userParam);
}
