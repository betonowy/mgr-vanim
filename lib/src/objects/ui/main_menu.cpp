#include "main_menu.hpp"

#include <imgui.h>

#include <scene/object_context.hpp>

#include "../vdb/volume_resource_base.hpp"
#include "convert_vdb_nvdb.hpp"
#include "debug_window.hpp"
#include "file_dialog.hpp"

namespace objects::ui
{
main_menu::~main_menu() = default;

void main_menu::init(scene::object_context &ctx)
{
    {
        auto found_debug_windows = ctx.find_objects<debug_window>();

        if (!found_debug_windows.empty())
        {
            if (found_debug_windows.size() == 1)
            {
                _debug_window = std::move(found_debug_windows[0]);
            }
            else
            {
                throw std::runtime_error(std::string(__func__) + ": Unexpected more than one instance of debug "
                                                                 "window.");
            }
        }
    }
}

void main_menu::update(scene::object_context &ctx, float)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Convert OpenVDB to NanoVDB"))
            {
                ctx.add_object(std::make_shared<file_dialog>([ctx = &ctx](std::filesystem::path path) -> bool {
                    ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path)));
                    return true;
                }));
            }

            if (ImGui::MenuItem("Convert OpenVDB to custom format"))
            {
            }

            if (ImGui::MenuItem("Open NanoVDB animation"))
            {
                ctx.add_object(std::make_shared<vdb::volume_resource_base>());
            }

            if (ImGui::MenuItem("Open custom animation"))
            {
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Recompile shaders"))
            {
                ctx.broadcast_signal(scene::signal_e::RELOAD_SHADERS);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit application", "Alt+F4"))
            {
                ctx.broadcast_signal(scene::signal_e::EXIT_APPLICATION);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            {
                bool active = _debug_window && !_debug_window.unique();

                if (_debug_window && _debug_window.unique())
                {
                    _debug_window->destroy(), _debug_window.reset();
                }

                if (ImGui::MenuItem("Debug window", nullptr, active))
                {
                    if (!active)
                    {
                        _debug_window = ctx.share_object(std::make_shared<debug_window>());
                    }
                    else
                    {
                        _debug_window->destroy(), _debug_window.reset();
                    }
                }
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
} // namespace objects::ui
