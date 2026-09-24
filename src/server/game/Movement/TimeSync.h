#ifndef TRINITY_TIME_SYNC_H
#define TRINITY_TIME_SYNC_H

#include <cstdint>
#include <queue>

namespace TimeSync
{
// Sequence numbers share the movement counter and may wrap. Outstanding
// requests are always less than half the sequence space apart.
inline bool IsBefore(std::uint32_t sequence, std::uint32_t next)
{
    std::uint32_t distance = next - sequence;
    return distance != 0 && distance < 0x80000000u;
}

inline void DiscardThrough(std::queue<std::uint32_t>& pending, std::uint32_t last, std::uint32_t next)
{
    // A notification from the previous map can arrive after the counter reset.
    // It must not discard requests in the new map or rewind the movement counter.
    if (!IsBefore(last, next))
        return;

    while (!pending.empty() && (pending.front() == last || IsBefore(pending.front(), last)))
        pending.pop();
}

inline bool ConsumeResponse(std::queue<std::uint32_t>& pending, std::uint32_t sequence)
{
    // Unknown/duplicate responses must not consume a different request. A known
    // newer response retires any earlier requests whose replies were dropped.
    auto remaining = pending;
    while (!remaining.empty() && remaining.front() != sequence)
        remaining.pop();
    if (remaining.empty())
        return false;

    remaining.pop();
    pending.swap(remaining);
    return true;
}
}

#endif
