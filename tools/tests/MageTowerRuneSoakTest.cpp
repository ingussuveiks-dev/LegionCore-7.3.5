#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;

struct Position { float x, y, z; };
struct Player;
struct Unit
{
    Position pos { 0, 0, 0 };
    Player* owner = nullptr;
    bool alive = true, inWorld = true, sameMap = true, samePhase = true, pet = false, guardian = false;
    unsigned summons = 0;
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return inWorld; }
    bool isPet() const { return pet; }
    bool isGuardian() const { return guardian; }
    float GetPositionZ() const { return pos.z; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return owner; }
    void CastSpell(Position point, uint32 spell, bool triggered)
    {
        assert(point.x == 0 && point.y == 0 && point.z == 0 && spell == 236461 && triggered);
        ++summons;
    }
};

using ObjectGuid = Unit*;
struct Player : Unit { std::vector<ObjectGuid> m_Controlled; };

namespace ObjectAccessor
{
Unit* GetUnit(Player const&, ObjectGuid guid) { return guid; }
}

struct AreaTrigger
{
    Unit* caster = nullptr;
    uint32 elapsed = 0;
    bool removed = false;
    Unit* GetCaster() const { return caster; }
    int32 GetDuration() const { return 8000; }
    uint32 GetElapsedTime() const { return elapsed; }
    float GetRadius() const { return 3.0f; }
    float GetPositionZ() const { return 0; }
    Position GetPosition() const { return { 0, 0, 0 }; }
    bool IsInMap(Unit const* target) const { return target->sameMap; }
    bool InSamePhase(Unit const* target) const { return target->samePhase; }
    float GetExactDist2d(Unit const* target) const { return std::hypot(target->pos.x, target->pos.y); }
    void Remove(bool duration) { assert(!duration); removed = true; }
};

struct AreaTriggerAI
{
    explicit AreaTriggerAI(AreaTrigger* rune) : at(rune) {}
    virtual void OnCreate() {}
    virtual void OnUpdate(uint32) {}
    virtual void OnRemove() {}
    virtual ~AreaTriggerAI() = default;
    AreaTrigger* at;
};

namespace Twins
{
constexpr uint32 NightmareSummon = 236461;
#include "RuneSoakDuration.inc"
struct RaestAI
{
    Player* player = nullptr;
    uint64 epoch = 1;
    Player* GetRunePlayer() const { return player; }
    uint64 ActiveGeneration() const { return player ? epoch : 0; }
};
RaestAI* actor = nullptr;
RaestAI* GetRaest(Unit* caster) { return caster ? actor : nullptr; }
}
#include "TwinsRuneAreaAI.inc"

struct Fixture
{
    Fixture()
    {
        area.caster = &caster;
        owner.pos.x = 20.0f;
        controller.player = &owner;
        Twins::actor = &controller;
        rune.OnCreate();
    }
    void At(uint32 elapsed)
    {
        area.elapsed = elapsed;
        rune.OnUpdate(0);
    }
    void Expire()
    {
        area.elapsed = 8000;
        rune.OnRemove();
    }
    Unit caster;
    Player owner;
    Twins::RaestAI controller;
    AreaTrigger area;
    at_mage_tower_summoning_rune rune { &area };
};

int main()
{
    {
        Fixture f;
        f.owner.pos.x = 3.0f;
        f.At(100);
        f.At(3099);
        assert(!f.area.removed);
        f.At(3100);
        assert(f.area.removed);
        f.rune.OnRemove();
        assert(!f.caster.summons);
    }
    for (bool guardian : { false, true })
    {
        Fixture f;
        Unit pet;
        pet.owner = &f.owner;
        pet.pet = !guardian;
        pet.guardian = guardian;
        f.owner.m_Controlled = { &pet };
        f.At(5000);
        f.Expire();
        assert(!f.caster.summons);
    }
    {
        Fixture f;
        Unit pet;
        Player other;
        pet.pet = true;
        pet.owner = &other;
        f.owner.m_Controlled = { &pet };
        f.At(100);
        f.At(7999);
        assert(!f.area.removed);
        f.Expire();
        f.Expire();
        assert(f.caster.summons == 1);
    }
    {
        Fixture f;
        f.owner.pos = { 0, 0, 0 };
        f.At(1000);
        f.owner.pos.z = 3.0f;
        f.At(2500);
        f.owner.pos.z = 0;
        f.At(5500);
        f.Expire();
        assert(f.caster.summons == 1);
    }
    {
        Fixture f;
        f.At(2000);
        f.rune.OnRemove();
        assert(!f.caster.summons);
        ++f.controller.epoch;
        f.At(2100);
        assert(f.area.removed);
        f.Expire();
        assert(!f.caster.summons);
    }
    {
        Fixture f;
        f.controller.player = nullptr;
        f.At(200);
        f.Expire();
        assert(f.area.removed && !f.caster.summons);
    }
    std::cout << "PASS Twins rune player, pet, guardian, expiry, wipe, old attempt and exactly-once nightmare.\n";
}
