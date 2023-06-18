#include "debug_window.hpp"

#include <glm/glm.hpp>
#include <imgui.h>

#include <scene/object_context.hpp>
#include <utils/memory_counter.hpp>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

namespace objects::ui
{
debug_window::~debug_window() = default;

void debug_window::init(scene::object_context &)
{
    _buffer.resize(256, 10.f);
}

void debug_window::update(scene::object_context &ctx, float delta_time)
{
    if (!_open)
    {
        destroy();
        return;
    }

    _buffer[_current_buffer_index] = delta_time;

    const auto average = std::accumulate(_buffer.begin(), _buffer.end(), 0.f) / _buffer.size();
    const auto max_element = *std::max_element(_buffer.begin(), _buffer.end());
    const auto min_element = *std::min_element(_buffer.begin(), _buffer.end());
    const auto average_fps = 1.f / average;
    const auto sample_fps = 1.f / delta_time;

    if (++_current_buffer_index >= _buffer.size())
    {
        _current_buffer_index = 0;
    }

    if (ImGui::Begin("Debug Window", &_open))
    {
        {
            ImGui::Text("FPS Sample: %.1f", sample_fps);
            ImGui::Text("FPS Average: %.1f", average_fps);
            ImGui::Text("Sample ms: %.2f", delta_time * 1e3f);
            ImGui::Text("Average ms: %.2f", average * 1e3f);
            ImGui::Text("Minimum ms: %.2f", min_element * 1e3f);
            ImGui::Text("Maximum ms: %.2f", max_element * 1e3f);
        }

        ImGui::Separator();

        {
            float map_time_ms = utils::get_map_time() * 1e-3;
            float fence_time_ms = utils::get_wait_time() * 1e-3;
            float copy_time_ms = utils::get_copy_time() * 1e-3;
            float flush_time_ms = utils::get_flush_time() * 1e-3;

            ImGui::Text("Map/Decompression time: %.3f ms", map_time_ms);
            ImGui::Text("Fence time: %.3f ms", fence_time_ms);
            ImGui::Text("Copy/Process time: %.3f ms", copy_time_ms);
            ImGui::Text("Flush time: %.3f ms", flush_time_ms);
        }

        ImGui::Separator();

        {
            auto gpu_mem = utils::gpu_buffer_memory_used();
            auto [gpu_mem_kb_all, gpu_mem_b] = std::div(gpu_mem, 1024l);
            auto [gpu_mem_mb_all, gpu_mem_kb] = std::div(gpu_mem_kb_all, 1024l);

            if (gpu_mem_mb_all > 0)
            {
                ImGui::Text("GPU buffer memory: %ld MiB %ld KiB %ld bytes", gpu_mem_mb_all, gpu_mem_kb, gpu_mem_b);
            }
            else if (gpu_mem_kb_all > 0)
            {
                ImGui::Text("GPU buffer memory: %ld KiB %ld bytes", gpu_mem_kb, gpu_mem_b);
            }
            else
            {
                ImGui::Text("GPU buffer memory: %ld bytes", gpu_mem_b);
            }
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

        if (ImGui::Button("Reset GPU statistics"))
        {
            utils::gpu_buffer_memory_peak_reset();
        }

        ImGui::Checkbox("FPS cap", &_fps_soft_cap);
        ImGui::SameLine();
        ImGui::Checkbox("Auto-recompile shaders", &_auto_recompile);
    }
    ImGui::End();

    if (_fps_soft_cap)
    {
        std::this_thread::sleep_until(tp);
        tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(5);
    }

    if (_auto_recompile)
    {
        _recompile_counter += delta_time;

        if (_recompile_counter > 0.5)
        {
            ctx.broadcast_signal(scene::signal_e::RELOAD_SHADERS);
            _recompile_counter = 0;
        }
    }
}
} // namespace objects::ui
