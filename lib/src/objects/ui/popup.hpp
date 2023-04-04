#pragma once

#include <scene/object.hpp>

#include <vector>

#include <functional>

namespace objects::ui
{
class popup : public scene::object
{
public:
    popup(std::string title, std::string description, std::function<void()> callback = {})
        : _callback(callback), _title(std::move(title)), _description(std::move(description))
    {
        if (_title.empty())
        {
            _title = "Notification";
        }
    }

    virtual ~popup();

    void update(scene::object_context &, float) override;

private:
    std::function<void()> _callback;
    std::string _title;
    std::string _description;
};
} // namespace objects::ui
