#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <set>
#include <vector>

using uint32 = uint32_t;
constexpr int SPELL_CAST_OK = 0;
struct Position
{
    float x = 0, y = 0, z = 0, orientation = 0;
    void Relocate(float px, float py, float pz, float o) { x = px; y = py; z = pz; orientation = o; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
};
struct Unit
{
    Position position;
    Position landing;
    unsigned inst = 4;
    std::vector<Position> spawn;
    std::vector<uint32> casts;
    std::function<void()> onSummon;
    bool rejectCharge = false;
    uint32 despawns = 0, cleanup = 0;
    float GetPositionX() const { return position.x; }
    float GetPositionY() const { return position.y; }
    uint32 GetInstanceId() const { return inst; }
    void CastSpell(float x, float y, float z, uint32 spell, bool triggered)
    {
        assert(spell == 237730 && triggered);
        spawn.push_back({ x, y, z });
        onSummon();
    }
    void CastSpell(Unit* target, uint32 spell, bool triggered)
    {
        assert(target == this && triggered);
        casts.push_back(spell);
    }
    int CastSpell(Position const& p, uint32 spell, bool triggered)
    {
        assert(triggered);
        casts.push_back(spell);
        landing = p;
        return (spell == 237759 && rejectCharge) ? 1 : 0;
    }
    void DespawnOrUnsummon(uint32 delay = 0)
    {
        if (delay)
            cleanup = delay;
        else
            ++despawns;
    }
};
struct Wings { unsigned despawns = 0; void DespawnAll() { ++despawns; } };
struct SpellInfo { uint32 Id; };
uint32 selectedDirection, selectedGap;
uint32 urand(uint32 min, uint32 max)
{
    assert((min == 0 && max == 5) || (min == 1 && max == 7));
    return min ? selectedGap : selectedDirection;
}
unsigned warnings = 0;
#define TC_LOG_WARN(...) (++warnings)

namespace GodQueen
{
constexpr uint32 SummonValkyr = 237730;
constexpr uint32 DarkWingsPrepare = 237769;
constexpr uint32 DarkWingsCharge = 237759;
constexpr uint32 DarkWingsDamage = 237761;

struct SigrynAI
{
    explicit SigrynAI(Unit* actor) : me(actor) {}
    Unit* me;
    uint32 _wave = 0, _published = 0;
    bool _arming = false;
    Position _landing;
    Wings _wings;
    std::set<int> _hits;
    std::vector<uint32> talk;
    void Talk(uint32 group) { talk.push_back(group); }
#include "DarkWingsFormation.inc"
};

struct ValkyrAI
{
    Unit* me;
    SigrynAI* owner;
    Position _destination;
    bool _released = false;
    SigrynAI* ActiveSigryn() const { return owner; }
    void SpellFinishCast(SpellInfo const* spell);
    bool Covers(Unit const& target) const;
};
#include "DarkWingsRelease.inc"
}

int main()
{
    using namespace GodQueen;
    for (selectedDirection = 0; selectedDirection < 6; ++selectedDirection)
        for (selectedGap = 1; selectedGap <= 7; ++selectedGap)
        {
            Unit boss;
            SigrynAI ai(&boss);
            std::vector<Position> land;
            boss.onSummon = [&]
            {
                assert(ai._arming);
                land.push_back(ai._landing);
                ++ai._published;
            };
            ai.StartWave();
            assert(ai._wave == 1 && ai._published == 8 && !ai._arming && ai._wings.despawns == 1);
            assert(boss.spawn.size() == 8 && land.size() == 8 && boss.casts == std::vector<uint32>{ 237772 });
            assert(ai.talk.size() == 2);
            for (size_t i = 0; i < land.size(); ++i)
            {
                assert(std::abs(std::hypot(land[i].x - boss.spawn[i].x, land[i].y - boss.spawn[i].y) - 56.0f) < 0.01f);
                if (i)
                    assert(std::abs(std::hypot(land[i].x - land[i-1].x, land[i].y - land[i-1].y) -
                        (i == selectedGap ? 18.0f : 6.0f)) < 0.01f);
            }
        }
    selectedDirection = 0;
    selectedGap = 1;
    Unit boss;
    SigrynAI sigryn(&boss);
    boss.onSummon = [&] { if (boss.spawn.size() < 8) ++sigryn._published; };
    sigryn.StartWave();
    assert(sigryn._wings.despawns == 2 && sigryn._published == 0 && warnings == 1 && boss.casts.empty());

    SpellInfo prepare { DarkWingsPrepare };
    Unit actor;
    ValkyrAI valkyr { &actor, &sigryn, { 56.0f, 0, 0 } };
    valkyr.SpellFinishCast(&prepare);
    valkyr.SpellFinishCast(&prepare);
    assert((actor.casts == std::vector<uint32>{ DarkWingsCharge, DarkWingsDamage }));
    assert(actor.cleanup == 2000);
    Unit target;
    target.position.x = 56.0f;
    assert(valkyr.Covers(target));
    target.position.x = 56.01f;
    assert(!valkyr.Covers(target));
    target.position.x = -1.0f;
    assert(!valkyr.Covers(target));

    Unit failed;
    failed.rejectCharge = true;
    ValkyrAI blocked { &failed, &sigryn, { 56.0f, 0, 0 } };
    blocked.SpellFinishCast(&prepare);
    assert(failed.casts == std::vector<uint32>{ DarkWingsCharge });

    Unit stale;
    ValkyrAI expired { &stale, nullptr, { 56.0f, 0, 0 } };
    expired.SpellFinishCast(&prepare);
    assert(stale.casts.empty() && stale.despawns == 1);
    std::cout << "PASS Dark Wings formation in six directions, every gap, cancelled partial formations and one native charge per Val'kyr.\n";
}
