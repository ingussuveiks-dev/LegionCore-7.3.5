#ifndef LEGION_COMMAND_FLOAT_ARGUMENT_H
#define LEGION_COMMAND_FLOAT_ARGUMENT_H

#include <cmath>
#include <cstdlib>

namespace Trinity
{
// Keep the native C numeric syntax, but require a complete finite float.
// Leave the receiver unchanged on failure, as ParseCommandInteger does.
inline bool ParseCommandFloat(char const* text, float& value)
{
    if (!text)
        return false;
    char* end = nullptr;
    float parsed = std::strtof(text, &end);
    if (end == text || !std::isfinite(parsed))
        return false;
    auto whitespace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f'; };
    while (whitespace(*end))
        ++end;
    if (*end)
        return false;
    value = parsed;
    return true;
}
}

#endif
