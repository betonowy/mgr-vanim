#include "main_menu.hpp"

#include <imgui.h>

#include <scene/object_context.hpp>

#include "convert_vdb_nvdb.hpp"
#include "debug_window.hpp"
#include "file_dialog.hpp"

namespace objects::ui
{
main_menu::~main_menu() = default;

void main_menu::update(scene::object_context &ctx, float)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Convert OpenVDB to NanoVDB"))
            {
                ctx.add_object(std::make_shared<file_dialog>(
                    [ctx = &ctx](std::filesystem::path path) -> bool
                    {
                        ctx->add_object(std::make_shared<convert_vdb_nvdb>(std::move(path)));
                        return true;
                    }));
            }

            ImGui::MenuItem("Convert OpenVDB to custom format");
            ImGui::MenuItem("Open NanoVDB animation");
            ImGui::MenuItem("Open custom animation");
            ImGui::Separator();
            ImGui::MenuItem("Recompile shaders");
            ImGui::Separator();
            ImGui::MenuItem("Quit application", "Alt+F4");
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
