#include "vanim.hpp"

#include <GL/gl3w.h>

#include <SDL2/SDL.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>
#include <openvdb/openvdb.h>

#include <chrono>
#include <exception>
#include <thread>

#include "objects/ui/main_menu.hpp"
#include "scene/object_context.hpp"
#include "scene/scene.hpp"

namespace
{
std::mutex messageMtx;

void MessageCallback([[maybe_unused]] GLenum source, GLenum type, [[maybe_unused]] GLuint id, [[maybe_unused]] GLenum severity,
                     [[maybe_unused]] GLsizei length, const GLchar *message, [[maybe_unused]] const void *userParam)
{
    std::string info(message);
    std::string context_name = reinterpret_cast<const char *>(userParam);
    std::lock_guard lock(messageMtx);

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: std::cout << context_name + " Error: " + info; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << context_name + " Deprecated: " + info; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cout << context_name + " Undefined Behavior: " + info; break;
    case GL_DEBUG_TYPE_PERFORMANCE: std::cout << context_name + " Performance: " + info; break;
    case GL_DEBUG_TYPE_PORTABILITY: std::cout << context_name + " Portability: " + info; break;
    case GL_DEBUG_TYPE_MARKER: std::cout << context_name + " Marker: " + info; break;
    case GL_DEBUG_TYPE_PUSH_GROUP: std::cout << context_name + " Push Group: " + info; break;
    case GL_DEBUG_TYPE_POP_GROUP: std::cout << context_name + " PoP Group: " + info; break;
    // default: std::cout << context_name + ": " + info; break;
    default: return;
    }
    std::cout << std::endl;
}
} // namespace

namespace vanim
{
void run()
{
    openvdb::initialize();
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        int gl_context_flags;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &gl_context_flags);
#ifndef NDEBUG
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, gl_context_flags | GL_CONTEXT_FLAG_DEBUG_BIT);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, gl_context_flags | GL_CONTEXT_FLAG_NO_ERROR_BIT);
#endif
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
    }
    const auto window = SDL_CreateWindow("Vanim", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, //
                                         800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    SDL_SetWindowMinimumSize(window, 800, 600);

    const auto gl_window_context = SDL_GL_CreateContext(window);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    const auto gl_resource_context = SDL_GL_CreateContext(window);

    if (SDL_GL_MakeCurrent(window, gl_window_context))
    {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_SetSwapInterval(0);

    gl3wInit();

    if (!gl3wIsSupported(4, 5))
    {
        throw std::runtime_error("OpenGL 4.5 is not supported");
    }

#ifndef NDEBUG
    glDebugMessageCallback(MessageCallback, "Window GL Context");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    std::thread t(
        [&]()
        {
            if (SDL_GL_MakeCurrent(window, gl_resource_context))
            {
                throw std::runtime_error(SDL_GetError());
            }

#ifndef NDEBUG
            glDebugMessageCallback(MessageCallback, "Resource GL Context");
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
        });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &imgui_io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_window_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    {
        scene::scene scene;
        bool loop = true;

        {
            auto scene_initializer = [](scene::object_context &ctx)
            {
                // ctx.add_object(std::make_shared<objects::ui::file_dialog>());
                // ctx.add_object(std::make_shared<objects::ui::debug_window>());
                ctx.add_object(std::make_shared<objects::ui::main_menu>());
            };

            scene.init(std::move(scene_initializer));
        }

        auto t1 = std::chrono::steady_clock::now();
        auto t2 = t1 + std::chrono::milliseconds(1);

        while (loop)
        {
            SDL_Event e;

            while (SDL_PollEvent(&e))
            {
                ImGui_ImplSDL2_ProcessEvent(&e);

                switch (e.type)
                {
                case SDL_QUIT: loop = false; break;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            scene.set_window_size({imgui_io.DisplaySize.x, imgui_io.DisplaySize.y});

            scene.loop(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t2).count() * 1e-9f);

            ImGui::Render();

            glViewport(0, 0, imgui_io.DisplaySize.x, imgui_io.DisplaySize.y);
            glClearColor(0.1, 0.15, 0.2, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(window);

            t2 = t1;
            t1 = std::chrono::steady_clock::now();
        }
    }

    t.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_resource_context);
    SDL_GL_DeleteContext(gl_window_context);
    SDL_DestroyWindow(window);
}
} // namespace vanim
