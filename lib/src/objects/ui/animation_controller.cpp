#include "animation_controller.hpp"

#include <imgui.h>

#include "popup.hpp"
#include <scene/object_context.hpp>
#include <utils/memory_counter.hpp>

namespace objects::ui
{
animation_controller::animation_controller(std::shared_ptr<vdb::volume_resource_base> resource)
    : _volume_resource(std::move(resource))
{
}

animation_controller::~animation_controller() = default;

void animation_controller::init(scene::object_context &)
{
}

void animation_controller::update(scene::object_context &ctx, float)
{
    if (_volume_resource->is_destroyed())
    {
        ctx.add_object(std::make_shared<popup>("Error", "Volume resource failed. Animation controler closed."));
        destroy();
    }

    if (ImGui::Begin("Animation Controller"))
    {
        ImGui::TextUnformatted("NanoVDB animation");

        {
            float preload = _volume_resource->get_preload_hint();
            ImGui::SliderFloat("Load ahead (seconds)", &preload, 0.f, 0.5f, "%.3f", ImGuiSliderFlags_Logarithmic);
            // _volume_resource->set_preload_hint(preload);
        }

        ImGui::Text("Buffer count: %d", static_cast<int>(_volume_resource->get_preload_hint() * _volume_resource->get_frame_rate() + 1));
        ImGui::SameLine();
        ImGui::Text("Ad hoc loads: %lu", _volume_resource->get_ad_hoc_loads());
        ImGui::SameLine();
        ImGui::Text("Force loads: %lu", _volume_resource->get_force_loads());

        {
            bool no_unload = _volume_resource->get_no_unload();
            ImGui::Checkbox("No unload", &no_unload);
            _volume_resource->set_no_unload(no_unload);
        }

        ImGui::SameLine();

        {
            bool play_animation = _volume_resource->get_play_animation();
            ImGui::Checkbox("Play animation", &play_animation);
            _volume_resource->set_play_animation(play_animation);
        }

#ifdef VANIM_WINDOWS
        ImGui::Text("Ad hoc loads: %llu", _volume_resource->get_ad_hoc_loads());
        ImGui::Text("Force loads: %llu", _volume_resource->get_force_loads());
#else
#endif
    }
    ImGui::End();
}

void animation_controller::on_destroy(scene::object_context &)
{
    _volume_resource->destroy();
}
} // namespace objects::ui
