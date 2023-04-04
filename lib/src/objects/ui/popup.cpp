#include "popup.hpp"

#include <imgui.h>

namespace objects::ui
{
popup::~popup() = default;

void popup::update(scene::object_context &, float)
{
    auto center = ImGui::GetIO().DisplaySize;
    center.x /= 2;
    center.y /= 2;

    ImGui::SetNextWindowSizeConstraints(ImVec2(250.f, 10.f), ImVec2(250.f, 1000.f));
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::OpenPopup(_title.c_str(), ImGuiPopupFlags_NoOpenOverExistingPopup);

    if (ImGui::BeginPopupModal(_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!_description.empty())
        {
            ImGui::TextWrapped("%s", _description.c_str());
        }

        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2()))
        {
            if (_callback)
            {
                _callback();
            }
            destroy();

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
} // namespace objects::ui
