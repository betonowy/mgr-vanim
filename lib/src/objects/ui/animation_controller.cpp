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
            _volume_resource->set_preload_hint(preload);
        }

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

        ImGui::Text("Ad hoc loads: %llu", _volume_resource->get_ad_hoc_loads());

        ImGui::Text("Force loads: %llu", _volume_resource->get_force_loads());

        if (ImGui::Button("Reset GPU statistics"))
        {
            utils::gpu_buffer_memory_peak_reset();
        }

        {
            auto gpu_mem = utils::gpu_buffer_memory_peak_high();
            auto [gpu_mem_kb_all, gpu_mem_b] = std::div(gpu_mem, 1024l);
            auto [gpu_mem_mb_all, gpu_mem_kb] = std::div(gpu_mem_kb_all, 1024l);

            if (gpu_mem_mb_all > 0)
            {
                ImGui::Text("GPU buffer memory peak high: %ld MiB %ld KiB %ld bytes", gpu_mem_mb_all, gpu_mem_kb, gpu_mem_b);
            }
            else if (gpu_mem_kb_all > 0)
            {
                ImGui::Text("GPU buffer memory peak high: %ld KiB %ld bytes", gpu_mem_kb, gpu_mem_b);
            }
            else
            {
                ImGui::Text("GPU buffer memory peak high: %ld bytes", gpu_mem_b);
            }
        }

        {
            auto gpu_mem = utils::gpu_buffer_memory_peak_low();
            auto [gpu_mem_kb_all, gpu_mem_b] = std::div(gpu_mem, 1024l);
            auto [gpu_mem_mb_all, gpu_mem_kb] = std::div(gpu_mem_kb_all, 1024l);

            if (gpu_mem_mb_all > 0)
            {
                ImGui::Text("GPU buffer memory peak low: %ld MiB %ld KiB %ld bytes", gpu_mem_mb_all, gpu_mem_kb, gpu_mem_b);
            }
            else if (gpu_mem_kb_all > 0)
            {
                ImGui::Text("GPU buffer memory peak low: %ld KiB %ld bytes", gpu_mem_kb, gpu_mem_b);
            }
            else
            {
                ImGui::Text("GPU buffer memory peak low: %ld bytes", gpu_mem_b);
            }
        }
    }
    ImGui::End();
}

void animation_controller::on_destroy(scene::object_context &)
{
    _volume_resource->destroy();
}
} // namespace objects::ui
