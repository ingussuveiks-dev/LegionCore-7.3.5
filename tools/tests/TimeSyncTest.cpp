#include "../../src/server/game/Movement/TimeSync.h"
#include <cassert>

int main()
{
    std::queue<std::uint32_t> pending;
    pending.push(258);
    pending.push(263);
    pending.push(264);
    // Regression: the client discarded 263 while the next server index was 265.
    TimeSync::DiscardThrough(pending, 263, 265);
    assert(pending.size() == 1 && pending.front() == 264);
    TimeSync::DiscardThrough(pending, 263, 265); // duplicate
    assert(pending.size() == 1);
    assert(!TimeSync::ConsumeResponse(pending, 263));
    assert(!TimeSync::ConsumeResponse(pending, 266));
    assert(pending.front() == 264);
    assert(TimeSync::ConsumeResponse(pending, 264));
    assert(pending.empty());
    assert(!TimeSync::ConsumeResponse(pending, 264));

    pending.push(0); // requests after a map transition
    pending.push(1);
    TimeSync::DiscardThrough(pending, 263, 2); // old-map notification
    TimeSync::DiscardThrough(pending, 2, 2); // not issued yet
    assert(pending.size() == 2);
    assert(TimeSync::ConsumeResponse(pending, 1)); // missing older reply
    assert(pending.empty());

    pending.push(0xFFFFFFFEu);
    pending.push(0xFFFFFFFFu);
    pending.push(0);
    pending.push(1);
    TimeSync::DiscardThrough(pending, 0, 2);
    assert(pending.size() == 1 && pending.front() == 1);
    TimeSync::DiscardThrough(pending, 1, 2);
    assert(pending.empty());
    return 0;
}
