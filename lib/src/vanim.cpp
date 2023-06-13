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

#include "dvdb/transform.hpp"
#include "dvdb/dct.hpp"
#include "gl/message_callback.hpp"
#include "objects/misc/world_data.hpp"
#include "objects/ui/debug_window.hpp"
#include "objects/ui/main_menu.hpp"
#include "scene/object_context.hpp"
#include "scene/scene.hpp"
#include "utils/cpu_architecture.hpp"

#ifdef VANIM_WINDOWS
extern "C"
{
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C"
{
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#include <converter/dvdb_converter.hpp>

namespace vanim
{
void run()
{
    {
        const auto arch = utils::get_cpu_available_feature_level();
        std::cout << "Detected available CPU feature level: " << utils::cpu_available_feature_level_str(arch) << '\n';

        if (arch != utils::cpu_available_feature_level_e::AVX2)
        {
            std::cerr << "This program makes use of AVX2 instruction set. This processor doesn't support this instruction set. Sorry.\n";
            std::abort();
        }
    }

    dvdb::dct_init();
    std::cout << "DCT tables initialized.\n";

    dvdb::transform_init();
    std::cout << "Coord transform tables initialized.";

    openvdb::initialize();
    std::cout << "OpenVDB initialized.\n";

    SDL_Init(SDL_INIT_EVERYTHING);

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

    if (SDL_GL_MakeCurrent(window, gl_window_context))
    {
        throw std::runtime_error(SDL_GetError());
    }

    gl3wInit();

    {
        std::cout << "Vendor   : " << glGetString(GL_VENDOR) << '\n';
        std::cout << "Renderer : " << glGetString(GL_RENDERER) << '\n';
        std::cout << "Version  : " << glGetString(GL_VERSION) << '\n';
        std::cout << "GLSL     : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
    }

    SDL_GL_SetSwapInterval(0);

    if (!gl3wIsSupported(4, 5))
    {
        throw std::runtime_error("OpenGL 4.5 is not supported");
    }

#ifndef NDEBUG
    glDebugMessageCallback(gl::message_callback, "Window GL Context");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &imgui_io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_window_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    {
        scene::scene scene;
        bool loop = true;

        {
            auto scene_initializer = [](scene::object_context &ctx) {
                ctx.add_object(std::make_shared<objects::ui::debug_window>());
                ctx.add_object(std::make_shared<objects::ui::main_menu>());
                ctx.add_object(std::make_shared<objects::misc::world_data>());
            };

            scene.init(std::move(scene_initializer));
        }

        auto t1 = std::chrono::steady_clock::now();
        auto t2 = t1 + std::chrono::milliseconds(1);

        while (loop)
        {
            {
                SDL_Event e;

                while (SDL_PollEvent(&e))
                {
                    ImGui_ImplSDL2_ProcessEvent(&e);

                    switch (e.type)
                    {
                    case SDL_QUIT:
                        loop = false;
                        break;
                    case SDL_MOUSEMOTION:
                        if (!imgui_io.WantCaptureMouse)
                        {
                            scene.push_motion_event(e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel);
                        }
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        if (!imgui_io.WantCaptureMouse)
                        {
                            scene.push_button_event(e.button.x, e.button.y, e.button.button, true);
                        }
                        break;
                    case SDL_MOUSEBUTTONUP:
                        scene.push_button_event(e.button.x, e.button.y, e.button.button, false);
                        break;
                    case SDL_KEYDOWN:
                        if (!imgui_io.WantCaptureKeyboard)
                        {
                            scene.push_key_event(e.key.keysym.sym, true);
                        }
                        break;
                    case SDL_KEYUP:
                        scene.push_key_event(e.key.keysym.sym, false);
                        break;
                    case SDL_MOUSEWHEEL:
                        scene.push_wheel_event(e.wheel.x, e.wheel.y);
                        break;
                    }
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            glViewport(0, 0, imgui_io.DisplaySize.x, imgui_io.DisplaySize.y);
            glClearColor(0.1, 0.15, 0.2, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            scene.set_window_size({imgui_io.DisplaySize.x, imgui_io.DisplaySize.y});

            loop &= scene.loop(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t2).count() * 1e-9f);

            ImGui::Render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(window);

            t2 = t1;
            t1 = std::chrono::steady_clock::now();
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_window_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    openvdb::uninitialize();
}
} // namespace vanim
