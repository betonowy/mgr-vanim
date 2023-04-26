#pragma once

#include <scene/object.hpp>

#include <vector>

#include <functional>

namespace objects::ui
{
class popup : public scene::object
{
public:
    popup(std::u8string title, std::u8string description, std::function<void()> callback = {})
        : _callback(callback), _title(std::move(title)), _description(std::move(description))
    {
        if (_title.empty())
        {
            _title = u8"Notification";
        }
    }

    void set_width(int value)
    {
        _width = value;
    }

    virtual ~popup();

    void update(scene::object_context &, float) override;

private:
    std::function<void()> _callback;
    std::u8string _title;
    std::u8string _description;
    int _width = 250;
};
} // namespace objects::ui
