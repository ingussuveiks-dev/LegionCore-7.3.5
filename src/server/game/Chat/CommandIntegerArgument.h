#ifndef LEGION_COMMAND_INTEGER_ARGUMENT_H
#define LEGION_COMMAND_INTEGER_ARGUMENT_H

#include <charconv>
#include <cstring>
#include <system_error>
#include <type_traits>

namespace Trinity
{
// Parse one complete decimal argument, leaving the receiver unchanged on error.
template<class T>
bool ParseCommandInteger(char const* text, T& value)
{
    static_assert(std::is_integral<T>::value && !std::is_same<T, bool>::value, "Integer receiver required");
    if (!text)
        return false;
    auto whitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; };
    while (whitespace(*text))
        ++text;
    char const* end = text + std::strlen(text);
    while (end != text && whitespace(end[-1]))
        --end;
    if (text == end)
        return false;
    if (*text == '+')
    {
        ++text;
        if (text != end && (*text == '+' || *text == '-'))
            return false;
    }
    if (text == end)
        return false;
    T parsed{};
    auto result = std::from_chars(text, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end)
        return false;
    value = parsed;
    return true;
}
}

#endif
