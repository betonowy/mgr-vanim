#include "debug_window.hpp"

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

namespace objects::ui
{
debug_window::~debug_window() = default;

void debug_window::init(scene::object_context &) { _buffer.resize(256, 1.f); }

void debug_window::update(scene::object_context &, float delta_time)
{
    if (!_open)
    {
        destroy();
        return;
    }

    _buffer[_current_buffer_index] = delta_time;

    const auto average = std::accumulate(_buffer.begin(), _buffer.end(), 0.f) / _buffer.size();
    const auto average_fps = 1.f / average;
    const auto sample_fps = 1.f / delta_time;

    if (++_current_buffer_index > _buffer.size())
    {
        _current_buffer_index = 0;
    }

    ImGui::Begin("Debug Window", &_open);
    ImGui::Text("FPS Sample: %.1f", sample_fps);
    ImGui::Text("Sample ms: %.2f", delta_time * 1e3f);
    ImGui::Text("FPS Average: %.1f", average_fps);
    ImGui::Text("Average ms: %.2f", average * 1e3f);
    ImGui::Checkbox("FPS cap", &_fps_soft_cap);
    ImGui::End();

    if (_fps_soft_cap)
    {
        if (delta_time < 0.01)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(static_cast<size_t>((0.01f - delta_time) * 1e6f)));
        }
    }
}
} // namespace objects::ui
