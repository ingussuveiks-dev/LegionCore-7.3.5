#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

struct Position
{
    float x, y, z;
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float GetExactDist2dSq(float otherX, float otherY) const
    {
        return (x - otherX) * (x - otherX) + (y - otherY) * (y - otherY);
    }
};

struct ObjectGuid
{
    unsigned value = 0;
    void Clear() { value = 0; }
};

struct AreaTrigger
{
    Position position;
    bool removed = false;
    Position const& GetPosition() const { return position; }
    void Remove() { assert(!removed); removed = true; }
};

struct Creature
{
    unsigned stacks = 9;
    unsigned breakVisuals = 0;
    ObjectGuid GetGUID() const { return { 5 }; }
    void RemoveAuraFromStack(unsigned spell, ObjectGuid guid)
    {
        assert(spell == 238471 && guid.value == 5);
        if (stacks)
            --stacks;
    }
    void CastSpell(Position, unsigned spell, bool triggered)
    {
        assert(spell == 243674 && triggered);
        ++breakVisuals;
    }
};

namespace ObjectAccessor
{
AreaTrigger* GetAreaTrigger(std::array<AreaTrigger, 3>& area, ObjectGuid guid)
{
    assert(guid.value <= area.size());
    return guid.value && !area[guid.value - 1].removed ? &area[guid.value - 1] : nullptr;
}
}

struct TugarAI
{
    std::array<AreaTrigger, 3> area;
    std::array<AreaTrigger, 3>* me = &area;
    std::array<ObjectGuid, 3> _rocks{{ { 1 }, { 2 }, { 3 } }};
    void HitStalactites(Creature* worm, Position const& start, Position const& end);
};

#include "FeltotemEarthquake.inc"

int main()
{
    Position const start{ 0, 0, 0 }, end{ 30, 0, 1 };
    assert(!SegmentTouches(start, start, start, RockRadius));
    assert(SegmentTouches(start, end, { 15, RockRadius, 0 }, RockRadius));
    assert(!SegmentTouches(start, end, { 15, RockRadius + 0.01f, 0 }, RockRadius));
    assert(!SegmentTouches(start, end, { 32.8f, 0, 0 }, RockRadius));
    assert(!SegmentTouches(start, end, { 15, 0, 3.6f }, RockRadius));
    assert(SegmentTouches(start, end, { 15, 0, 3.4f }, RockRadius));
    assert(SegmentTouches(end, start, { 2, 0, 0 }, RockRadius));

    TugarAI encounter{ { AreaTrigger{{ 5, 0, 0 }}, AreaTrigger{{ 15, 0, 1 }}, AreaTrigger{{ 24, 4, 1 }} } };
    Creature worm;
    encounter.HitStalactites(&worm, start, end);
    assert(worm.stacks == 7 && worm.breakVisuals == 2);
    assert(!encounter._rocks[0].value && !encounter._rocks[1].value && encounter._rocks[2].value);
    encounter.HitStalactites(&worm, end, start);
    assert(worm.stacks == 7 && worm.breakVisuals == 2);

    // An expired rock and a real rock in another instance cannot remove another stack.
    encounter.area[2].Remove();
    TugarAI other{ { AreaTrigger{{ 5, 0, 0 }}, AreaTrigger{{ 15, 0, 1 }}, AreaTrigger{{ 24, 4, 1 }} } };
    encounter.HitStalactites(&worm, { 20, 4, 1 }, { 30, 4, 1 });
    assert(worm.stacks == 7 && worm.breakVisuals == 2 && !other.area[0].removed);
    other.HitStalactites(&worm, { 20, 4, 1 }, { 30, 4, 1 });
    assert(worm.stacks == 6 && worm.breakVisuals == 3 && other.area[2].removed);
    std::cout << "PASS swept contacts, height and radius, multiple rocks per update, consumed and expired rocks, and instance ownership.\n";
}
