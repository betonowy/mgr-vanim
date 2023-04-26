#include "file_dialog.hpp"

#include <imgui.h>

#include <stdlib.h>
#include <utils/scope_guard.hpp>

#include <algorithm>
#include <codecvt>

namespace objects::ui
{
file_dialog::file_dialog(std::function<bool(std::filesystem::path)> callback, std::filesystem::path starting_path)
    : _callback(std::move(callback))
{
    if (!starting_path.empty())
    {
        _current_path = std::move(starting_path);
    }
    else
    {
#ifdef VANIM_WINDOWS
        char *buffer;
        size_t size;
        _dupenv_s(&buffer, &size, "UserProfile");
        _current_path = buffer;
#else
        _current_path = std::getenv("HOME");
#endif
    }
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
            _directoryListing.push_back(dir_it->path().filename().u8string());
        }
    }

    std::sort(_directoryListing.begin(), _directoryListing.end());
}

void file_dialog::update(scene::object_context &, float)
{
    std::u8string work_utf8_string;

    std::filesystem::path path = _current_path;

    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2());

    if (ImGui::Begin("File Dialog", nullptr, ImGuiWindowFlags_NoTitleBar))
    {
        {
            work_utf8_string = _current_path.u8string();
            ImGui::Text("Directory: %s", reinterpret_cast<const char*>(work_utf8_string.c_str()));
        }

        ImGui::Separator();

        if (ImGui::Button("Go up"))
        {
            _current_path = std::filesystem::path(_current_path).parent_path().u8string();
            std::filesystem::directory_iterator dir_it(_current_path);
            _directoryListing.clear();

            for (auto entry : dir_it)
            {
                if (entry.is_directory())
                {
                    _directoryListing.push_back(dir_it->path().filename().u8string());
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

                work_utf8_string = entry.u8string();

                if (ImGui::Selectable(reinterpret_cast<const char*>(work_utf8_string.c_str())))
                {
                    _current_path = std::filesystem::weakly_canonical(newPath).make_preferred();
                    std::filesystem::directory_iterator dir_it(_current_path);
                    _directoryListing.clear();

                    for (auto entry : dir_it)
                    {
                        if (entry.is_regular_file() || entry.is_directory())
                        {
                            _directoryListing.push_back(entry.path().filename());
                        }
                    }

                    std::sort(_directoryListing.begin(), _directoryListing.end());
                    break;
                }
            }
            else
            {
                work_utf8_string = entry.u8string();
                ImGui::TextUnformatted(reinterpret_cast<const char*>(work_utf8_string.c_str()));
            }
        }
    }
    ImGui::End();
}
} // namespace objects::ui
