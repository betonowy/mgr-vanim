#pragma once

#include <scene/object.hpp>

#include <filesystem>
#include <functional>
#include <vector>

namespace objects::ui
{
class file_dialog : public scene::object
{
public:
    file_dialog(std::function<bool(std::filesystem::path)>, std::string = {});

    ~file_dialog() override;

    void init(scene::object_context &) override;
    void update(scene::object_context &, float) override;

private:
    void change_directory(std::string);

    std::string _current_path;
    std::function<bool(std::filesystem::path)> _callback;
    std::vector<std::string> _directoryListing;
};
} // namespace objects::ui
