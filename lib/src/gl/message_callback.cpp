#include "message_callback.hpp"

#include <iostream>
#include <mutex>

namespace gl
{
std::mutex messageMtx;

void message_callback([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id, [[maybe_unused]] GLenum severity, [[maybe_unused]] GLsizei length, const GLchar *message, [[maybe_unused]] const void *userParam)
{
    std::string info(message);
    std::string context_name = reinterpret_cast<const char *>(userParam);
    std::lock_guard lock(messageMtx);

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        std::cout << context_name + " Error: " + info;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        std::cout << context_name + " Deprecated: " + info;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        std::cout << context_name + " Undefined Behavior: " + info;
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        std::cout << context_name + " Performance: " + info;
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        std::cout << context_name + " Portability: " + info;
        break;
    case GL_DEBUG_TYPE_MARKER:
        std::cout << context_name + " Marker: " + info;
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        std::cout << context_name + " Push Group: " + info;
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        std::cout << context_name + " PoP Group: " + info;
        break;
    // default: std::cout << context_name + ": " + info; break;
    default:
        return;
    }
    std::cout << std::endl;
}
} // namespace gl
