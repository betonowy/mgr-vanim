#pragma once

#include <functional>

namespace utils
{
template <typename T> class scope_guard
{
public:
    scope_guard(T callback) : _callback(std::move(callback)) {}
    ~scope_guard() { _callback(); }

    scope_guard(scope_guard &&) = delete;
    scope_guard &operator=(scope_guard &&) = delete;

private:
    T _callback;
};
} // namespace utils
