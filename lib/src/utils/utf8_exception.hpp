#pragma once

#include <exception>
#include <string>

namespace utils
{
class utf8_exception : public std::exception
{
public:
    utf8_exception(std::u8string str) : message(str)
    {
    }

    const std::u8string &utf8_what()
    {
        return message;
    }

private:
    std::u8string message;
};
} // namespace utils
