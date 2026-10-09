#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>
#include <iostream>
using uint32 = uint32_t;
using SpellEffIndex = uint32;
using SpellCastResult = uint32;
constexpr uint32 QUEST_STATUS_INCOMPLETE = 1, QUEST_STATUS_COMPLETE = 2, SPELL_CAST_OK = 0, SPELL_FAILED_NOT_HERE = 1,
                 SPELL_FAILED_BAD_TARGETS = 2;
constexpr uint32 UNIT_FIELD_FLAGS = 0, UNIT_FLAG_NOT_SELECTABLE = 1;
struct Seconds
{
    uint32 value;
    explicit Seconds(uint32 n) : value(n)
    {
    }
};
struct Player;
struct Creature;
struct WorldObject
{
    virtual ~WorldObject() = default;
    float x = 0, y = 0, z = 0;
    uint32 map = 0;
    float GetDistance(WorldObject* other)
    {
        return GetDistance(other->x, other->y, other->z);
    }
    float GetDistance(float a, float b, float c)
    {
        return std::sqrt((x - a) * (x - a) + (y - b) * (y - b) + (z - c) * (z - c));
    }
    bool IsWithinDistInMap(WorldObject* other, float r)
    {
        return map == other->map && GetDistance(other) <= r;
    }
};
struct Unit : WorldObject
{
    virtual Player* ToPlayer()
    {
        return nullptr;
    }
    virtual Creature* ToCreature()
    {
        return nullptr;
    }
};
struct Creature : Unit
{
    void RemoveFlag(uint32,uint32 f){flags &= ~f;}
    void SetReactState(uint32){}
    uint32 entry = 0, guid = 1, flags = 0, despawns = 0;
    bool alive = true;
    Creature* ToCreature() override
    {
        return this;
    }
    uint32 GetEntry()
    {
        return entry;
    }
    uint32 GetGUID()
    {
        return guid;
    }
    bool IsAlive()
    {
        return alive;
    }
    bool HasFlag(uint32, uint32 f)
    {
        return (flags & f) != 0;
    }
    void SetFlag(uint32, uint32 f)
    {
        flags |= f;
    }
    void DespawnOrUnsummon(uint32 ms, Seconds seconds)
    {
        assert(ms == 100 && seconds.value == 60);
        ++despawns;
    }
};
struct Player : Unit
{
    bool alive = true;
    std::map<uint32, uint32> quests, credits;
    std::vector<Creature*> nearby;
    uint32 casts = 0;
    Player* ToPlayer() override
    {
        return this;
    }
    uint32 GetMapId()
    {
        return map;
    }
    bool IsAlive()
    {
        return alive;
    }
    uint32 GetQuestStatus(uint32 q)
    {
        return quests[q];
    }
    uint32 GetQuestObjectiveData(uint32, uint32 id)
    {
        return credits[id];
    }
    Creature* FindNearestCreature(uint32 id, float range)
    {
        Creature* result = nullptr;
        for (auto c : nearby)
            if (c->entry == id && IsWithinDistInMap(c, range) && (!result || GetDistance(c) < GetDistance(result)))
                result = c;
        return result;
    }
    void KilledMonsterCredit(uint32 id, uint32)
    {
        ++credits[id];
    }
    void CastSpell(Player*, uint32 id, bool)
    {
        assert(id == 230205);
        ++casts;
        ++credits[114322];
    }
};
struct SpellInfo
{
    uint32 Id = 0;
};
#define SpellCheckCastFn(...) 1
#define SpellObjectTargetSelectFn(...) 1
#define SpellEffectFn(...) 1
struct Hook
{
    uint32 count = 0;
    void operator+=(int)
    {
        ++count;
    }
};
struct ScriptBase
{
    uint32 m_scriptSpellId = 0;
    Hook OnCheckCast, OnObjectTargetSelect, OnEffectHitTarget;
    Unit* caster = nullptr;
    Unit* target = nullptr;
    Creature* hit = nullptr;
    SpellInfo spell;
    uint32 prevented = 0;
    Unit* GetCaster()
    {
        return caster;
    }
    Unit* GetExplTargetUnit()
    {
        return target;
    }
    Creature* GetHitCreature()
    {
        return hit;
    }
    SpellInfo* GetSpellInfo()
    {
        assert(caster);
        return &spell;
    }
    void PreventHitDefaultEffect(uint32 index)
    {
        assert(index == 1);
        ++prevented;
    }
};
#include "helpers.inc"
struct Collect : ScriptBase
{
#include "collect.inc"
};
struct Node : ScriptBase
{
#include "node.inc"
};
constexpr uint32 REACT_PASSIVE=0;
struct NodeAI {Creature* me;
#include "node-reset.inc"
};
struct Arrival
{
#include "arrival.inc"
};
int main()
{
    Node portalRegistration;
    portalRegistration.m_scriptSpellId = 229466;
    portalRegistration.Register();
    assert(portalRegistration.OnObjectTargetSelect.count == 0 && portalRegistration.OnEffectHitTarget.count == 1);
    Node leyRegistration;
    leyRegistration.m_scriptSpellId = 228208;
    leyRegistration.Register();
    assert(leyRegistration.OnObjectTargetSelect.count == 1);
    Collect sampleRegistration;
    sampleRegistration.Register();
    assert(sampleRegistration.OnObjectTargetSelect.count == 1);
    Player p;
    Creature soil, water;
    soil.entry = 114641;
    soil.x = 3;
    water.entry = 114645;
    water.x = 8;
    p.nearby = {&soil, &water};
    Collect collect;
    collect.caster = &p;
    assert(collect.Check() == SPELL_FAILED_NOT_HERE);
    p.quests[44684] = QUEST_STATUS_INCOMPLETE;
    assert(collect.Check() == SPELL_CAST_OK);
    WorldObject* selected = nullptr;
    collect.Select(selected);
    assert(selected == &soil);
    collect.hit = &soil;
    collect.Hit(0);
    assert(p.credits[114641] == 1);
    collect.Select(selected);
    assert(selected == &water);
    water.x = 11;
    assert(collect.Check() == SPELL_FAILED_NOT_HERE);
    collect.hit = &water;
    collect.Hit(0);
    assert(!p.credits[114645]);
    water.x = 8;
    p.map = 1220;
    collect.Hit(0);
    assert(!p.credits[114645]);
    p.map = 0;
    collect.Hit(0);
    assert(p.credits[114645] == 1 && collect.Check() == SPELL_FAILED_NOT_HERE);
    p.credits.clear();
    p.alive = false;
    assert(collect.Check() == SPELL_FAILED_NOT_HERE);
    p.alive = true;
    Creature portal;
    portal.entry = 115414;
    portal.x = 5;
    Node node;
    node.caster = &p;
    node.target = &portal;
    node.spell.Id = 229466;
    assert(node.Check() == SPELL_FAILED_BAD_TARGETS);
    p.quests[44557] = 1;
    assert(node.Check() == SPELL_CAST_OK);
    assert(!portal.despawns); // starting or interrupting the cast grants nothing
    node.Credit(1);
    assert(portal.despawns == 1 && !node.prevented);
    node.Credit(1);
    assert(node.prevented == 1 && portal.despawns == 1);
    NodeAI{&portal}.Reset();
    portal.x = 11;
    node.Credit(1);
    assert(node.prevented == 2 && portal.despawns == 1);
    portal.x = 5;
    node.spell.Id = 228208;
    assert(node.Check() == SPELL_FAILED_BAD_TARGETS);
    portal.entry = 115027;
    p.quests[44683] = 1;
    assert(node.Check() == SPELL_CAST_OK);
    node.Credit(1);
    assert(portal.despawns == 2);
    NodeAI{&portal}.Reset();
    node.spell.Id = 231458;
    assert(node.Check() == SPELL_FAILED_BAD_TARGETS);
    portal.entry = 115037;
    assert(node.Check() == SPELL_CAST_OK);
    p.quests[44683] = QUEST_STATUS_COMPLETE;
    node.Credit(1);
    assert(portal.despawns == 2);
    Arrival arrival;
    p.quests[44556] = 1;
    p.credits[115871] = 1;
    p.map = 1220;
    arrival.OnMapChanged(&p);
    assert(!p.casts);
    p.map = 0;
    arrival.OnMapChanged(&p);
    assert(!p.casts);
    p.x = -11144.5f;
    p.y = -2108.11f;
    p.z = 49.8f;
    arrival.OnMapChanged(&p);
    arrival.OnMapChanged(&p);
    assert(p.casts == 1);
    std::cout << "Karazhan sample, node completion/duplicate/interrupt and arrival checks passed\n";
}
