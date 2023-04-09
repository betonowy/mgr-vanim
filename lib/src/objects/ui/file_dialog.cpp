#include "file_dialog.hpp"

#include <imgui.h>

#include <utils/scope_guard.hpp>

#include <algorithm>

namespace objects::ui
{
file_dialog::file_dialog(std::function<bool(std::filesystem::path)> callback, std::string starting_path)
    : _current_path(std::move(starting_path)), _callback(std::move(callback))
{
}

file_dialog::~file_dialog() = default;

void file_dialog::init(scene::object_context &)
{
    std::filesystem::path path = _current_path;
    std::filesystem::directory_iterator dir_it(path);

    for (auto entry : dir_it)
    {
        if (entry.is_directory())
        {
            _directoryListing.push_back(dir_it->path().filename());
        }
    }

    std::sort(_directoryListing.begin(), _directoryListing.end());
}

void file_dialog::update(scene::object_context &, float)
{
    std::filesystem::path path = _current_path;

    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2());

    ImGui::Begin("File Dialog", nullptr, ImGuiWindowFlags_NoTitleBar);
    {

        ImGui::Text("Directory: %s", _current_path.c_str());

        ImGui::Separator();

        if (ImGui::Button("Go up"))
        {
            _current_path = std::filesystem::path(_current_path).parent_path();
            std::filesystem::directory_iterator dir_it(_current_path);
            _directoryListing.clear();

            for (auto entry : dir_it)
            {
                if (entry.is_directory())
                {
                    _directoryListing.push_back(dir_it->path().filename());
                }
            }

            std::sort(_directoryListing.begin(), _directoryListing.end());
        }

        ImGui::SameLine();

        if (ImGui::Button("Load this directory"))
        {
            if (_callback(_current_path))
            {
                destroy();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            destroy();
        }

        ImGui::Separator();

        for (auto entry : _directoryListing)
        {
            auto newPath = std::filesystem::path(_current_path) / entry;

            if (std::filesystem::is_directory(newPath))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, 0xffffd0c0);

                utils::scope_guard popStyle([]() { ImGui::PopStyleColor(); });

                if (ImGui::Selectable(entry.c_str()))
                {
                    _current_path = std::filesystem::weakly_canonical(newPath).make_preferred();
                    std::filesystem::directory_iterator dir_it(_current_path);
                    _directoryListing.clear();

                    for (auto entry : dir_it)
                    {
                        _directoryListing.push_back(dir_it->path().filename());
                    }

                    std::sort(_directoryListing.begin(), _directoryListing.end());
                    break;
                }
            }
            else
            {
                ImGui::TextUnformatted(entry.c_str());
            }
        }
    }
    ImGui::End();
}
} // namespace objects::ui
