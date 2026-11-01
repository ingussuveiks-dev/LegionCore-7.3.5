#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>

using uint8 = uint8_t;
using uint32 = uint32_t;
using ObjectGuid = uint32;
using GuidSet = std::set<ObjectGuid>;
enum { UNIT_FIELD_CREATED_BY_SPELL, AURA_EFFECT_HANDLE_REAL = 1 };
struct Creature;
struct TempSummon;
struct Unit
{
    using ControlList = GuidSet;
    ObjectGuid identity = 1;
    uint32 createdBy = 217304;
    bool inWorld = true;
    ControlList m_Controlled;
    std::map<uint32, std::list<ObjectGuid>> tempSummonList;
    unsigned casts = 0, gameObjectRemovals = 0;
    virtual ~Unit() = default;
    bool IsInWorld() const { return inWorld; }
    ObjectGuid GetGUID() const { return identity; }
    uint32 GetUInt32Value(int) const { return createdBy; }
    virtual Creature* ToCreature() { return nullptr; }
    TempSummon* ToTempSummon();
    void RemoveAllMinionsByFilter(uint32, uint8 = 0);
    void CastSpell(Unit* target, uint32 spell, bool triggered)
    { assert(target == this && spell == 217304 && triggered); ++casts; }
    void RemoveGameObject(uint32 spell, bool all)
    { assert(spell == 217304 && all); ++gameObjectRemovals; }
};
struct Creature : Unit
{
    bool summon = false;
    unsigned delayedRemovals = 0;
    Creature* ToCreature() override { return this; }
    bool isSummon() const { return summon; }
    void DespawnOrUnsummon(uint32 delay) { assert(delay == 500); ++delayedRemovals; }
};
struct TempSummon : Creature
{
    ObjectGuid owner = 1;
    unsigned removals = 0;
    std::function<void()> callback;
    TempSummon() { summon = true; }
    ObjectGuid GetSummonerGUID() const { return owner; }
    void UnSummon()
    {
        ++removals;
        inWorld = false;
        if (callback) callback();
    }
};
TempSummon* Unit::ToTempSummon() { return static_cast<TempSummon*>(this); }
static std::unordered_map<ObjectGuid, Unit*> objects;
namespace ObjectAccessor
{
    Unit* GetUnit(Unit&, ObjectGuid guid)
    { auto found = objects.find(guid); return found == objects.end() ? nullptr : found->second; }
    Creature* GetCreature(Unit& owner, ObjectGuid guid)
    { auto unit = GetUnit(owner, guid); return unit ? unit->ToCreature() : nullptr; }
}
struct Effect { uint32 TriggerSpell = 217304; };
struct Info
{
    Effect effect;
    Effect const* GetEffect(int, int) const { return &effect; }
};
struct AuraApplication { Unit* target; Unit* GetTarget() const { return target; } };
struct AuraEffect
{
    Info info;
    int m_diffMode = 0;
    Info const* GetSpellInfo() const { return &info; }
    int GetEffIndex() const { return 0; }
    void HandleSummonController(AuraApplication const*, uint8, bool) const;
};
#include "SummonControllerCleanup.inc"
#include "SummonControllerAura.inc"
static unsigned cases = 0;
static void Passed(char const* label) { ++cases; std::cout << "CASE " << label << " PASS\n"; }
static void Attach(Unit& owner, Unit& actor, bool controlled, uint32 entry = 109224)
{
    objects[actor.identity] = &actor;
    owner.tempSummonList[entry].push_back(actor.identity);
    if (controlled) owner.m_Controlled.insert(actor.identity);
}
int main(int argc, char**)
{
    if (argc > 1)
    {
        Unit owner; TempSummon actor; actor.identity = 10; Attach(owner, actor, false);
        AuraEffect effect; AuraApplication application{&owner};
        effect.HandleSummonController(&application, AURA_EFFECT_HANDLE_REAL, false);
        assert(actor.removals == 0 && owner.gameObjectRemovals == 1);
        std::cout << "EXPECTED_BROKEN native controller leaves an ordinary owned summon\n";
        return 0;
    }
    for (bool controlled : {false, true})
        for (bool own : {false, true})
            for (bool inWorld : {false, true})
                for (uint32 spell : {0u, 217304u, 217305u})
                {
                    objects.clear(); Unit owner; TempSummon actor; actor.identity = 10;
                    actor.owner = own ? 1 : 2; actor.inWorld = inWorld; actor.createdBy = spell;
                    Attach(owner, actor, controlled);
                    owner.RemoveAllMinionsByFilter(217304, 1);
                    bool expected = inWorld && spell == 217304 && (controlled || own);
                    assert(actor.removals == unsigned(expected));
                    Passed("owner/control/spell/world admission");
                }
    {
        objects.clear(); Unit owner, player; Creature ordinary; player.identity = 10; ordinary.identity = 11;
        Attach(owner, player, true); Attach(owner, ordinary, true);
        owner.tempSummonList[109224].push_back(999);
        owner.RemoveAllMinionsByFilter(217304, 1);
        assert(player.inWorld && ordinary.inWorld && !ordinary.delayedRemovals);
        Passed("missing/player/non-summon candidates stay intact");
    }
    {
        objects.clear(); Unit owner; TempSummon actor; actor.identity = 10;
        Attach(owner, actor, true); Attach(owner, actor, false, 109225);
        owner.RemoveAllMinionsByFilter(217304, 1); owner.RemoveAllMinionsByFilter(217304, 1);
        assert(actor.removals == 1); Passed("duplicate entry/control GUID and repeat cleanup");
    }
    {
        objects.clear(); Unit owner; TempSummon first, second, added;
        first.identity = 10; second.identity = 11; added.identity = 12;
        Attach(owner, first, true); Attach(owner, second, true);
        first.callback = [&] {
            owner.m_Controlled.clear(); owner.tempSummonList.clear();
            objects.erase(second.identity); Attach(owner, added, true);
        };
        owner.RemoveAllMinionsByFilter(217304, 1);
        assert(first.removals == 1 && !second.removals && !added.removals);
        Passed("callbacks invalidate other candidates and create a new summon safely");
    }
    {
        objects.clear(); Unit owner; TempSummon first, second;
        first.identity = 10; second.identity = 11; Attach(owner, first, false); Attach(owner, second, false);
        first.callback = [&] { owner.tempSummonList.clear(); };
        owner.RemoveAllMinionsByFilter(217304, 1);
        assert(first.removals == 1 && second.removals == 1);
        Passed("entry-list removal does not skip a live matching summon");
    }
    {
        objects.clear(); Unit owner; TempSummon raw, controlled; raw.identity = 10; controlled.identity = 11;
        raw.createdBy = controlled.createdBy = 0; Attach(owner, raw, false); Attach(owner, controlled, true);
        owner.RemoveAllMinionsByFilter(0, 1);
        assert(!raw.removals && controlled.removals == 1);
        Passed("spell zero preserves ordinary raw summons and old controlled semantics");
    }
    {
        objects.clear(); Unit owner; TempSummon first, other; first.identity = 10; other.identity = 11;
        Attach(owner, first, false); Attach(owner, other, false, 109225);
        owner.RemoveAllMinionsByFilter(109224);
        assert(first.delayedRemovals == 1 && !first.removals && !other.delayedRemovals);
        Passed("native entry filter keeps its 500ms behavior");
    }
    {
        objects.clear(); Unit owner; TempSummon actor; actor.identity = 10; Attach(owner, actor, false);
        AuraEffect effect; AuraApplication application{&owner}, missing{nullptr};
        effect.HandleSummonController(&application, 0, false);
        effect.HandleSummonController(&missing, AURA_EFFECT_HANDLE_REAL, false);
        effect.HandleSummonController(&application, AURA_EFFECT_HANDLE_REAL, true);
        assert(!actor.removals && owner.casts == 1 && !owner.gameObjectRemovals);
        effect.HandleSummonController(&application, AURA_EFFECT_HANDLE_REAL, false);
        assert(actor.removals == 1 && owner.gameObjectRemovals == 1);
        Passed("actual aura apply/remove mode and independent GO cleanup");
    }
    assert(cases == 31);
    std::cout << "PASS summon controller cleanup31\n";
}
