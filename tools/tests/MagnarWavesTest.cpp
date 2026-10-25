#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
using int32 = int32_t;
enum { REACT_PASSIVE, REACT_DEFENSIVE, UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE = 0x2000000,
    UNIT_FLAG_IMMUNE_TO_PC = 0x100, UNIT_FLAG_IMMUNE_TO_NPC = 0x200, UNIT_FLAG_NOT_ATTACKABLE_1 = 0x80,
    UNIT_STATE_CASTING = 1, UNIT_STATE_STUNNED = 2, CRITERIA_TYPE_SCRIPT_EVENT_2 = 92,
    EVENT_1 = 1, EVENT_2, EVENT_3, EVENT_4, EVENT_5 };
struct ObjectGuid
{
    uint32 identity;
    bool operator==(ObjectGuid const& other) const { return identity == other.identity; }
    bool operator<(ObjectGuid const& other) const { return identity < other.identity; }
};
struct Position { float positionX, positionY, positionZ, orientation; };
#include "Positions.inc"
struct Creature;
struct TempSummon;
struct Unit;
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual void Reset() { }
    virtual void EnterCombat(Unit*) { }
    virtual void JustSummoned(Creature*) { }
    virtual void SummonedCreatureDies(Creature*, Unit*) { }
    virtual void DoAction(int32) { }
    virtual void UpdateAI(uint32) { }
    virtual void AttackStart(Unit*) { }
};
struct Unit
{
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
    bool IsAIEnabled = true;
    void NotifyDeath(Creature* creature);
};
struct Player : Unit
{
    uint32 credits = 0;
    std::function<void()> onCredit;
    void UpdateAchievementCriteria(uint32 type, uint32 event) { assert(type == 92 && event == 46427); ++credits; if (onCredit) onCredit(); }
};
struct MotionMaster
{
    uint32 jumps = 0;
    void MoveJump(float, float, float, float, float, float) { ++jumps; }
};
struct Map
{
    std::map<ObjectGuid, Creature*> actors;
    Creature* GetCreature(ObjectGuid guid) { auto found = actors.find(guid); return found == actors.end() ? nullptr : found->second; }
};
struct Creature : Unit
{
    ObjectGuid guid{0};
    uint32 entry = 0, flags = 0, faction = 14, health = 100, conversations = 0, casts = 0, removed = 0, unitState = 0;
    bool alive = true, victim = true;
    CreatureAI* ai = nullptr;
    Creature* ally = nullptr;
    Creature* marker = nullptr;
    Player* player = nullptr;
    Map* map = nullptr;
    MotionMaster motion;
    std::function<void(Creature*)> onActivation;
    std::function<Creature*(uint32, Position const&)> spawn;
    Creature* ToCreature() override { return this; }
    virtual TempSummon* ToTempSummon() { return nullptr; }
    CreatureAI* AI() const { return ai; }
    ObjectGuid GetGUID() const { return guid; }
    uint32 GetEntry() const { return entry; }
    bool IsAlive() const { return alive; }
    Map* GetMap() const { return map; }
    void SetReactState(uint32) { }
    void SetFlag(uint32, uint32 mask) { flags |= mask; }
    void RemoveFlag(uint32, uint32 mask) { flags &= ~mask; if (onActivation) onActivation(this); }
    void RemoveAura(uint32 spell) { assert(spell == 191821 || spell == 128749); ++removed; }
    void setFaction(uint32 value) { faction = value; }
    void StopAttack() { }
    void AttackStop() { }
    bool HasUnitState(uint32 mask) const { return (unitState & mask) != 0; }
    MotionMaster* GetMotionMaster() { return &motion; }
    Creature* FindNearestCreature(uint32 identity, float, bool) { return identity == 96468 ? ally : identity == 64367 ? marker : nullptr; }
    Player* FindNearestPlayer(float range) { assert(range == 100.f); return player; }
    float GetPositionX() const { return 1; }
    float GetPositionY() const { return 2; }
    float GetPositionZ() const { return 3; }
    float GetOrientation() const { return 0; }
    void CastSpell(Creature* target, uint32 spell) { assert(target == this && spell == 191032); ++casts; }
    void CreateConversation(uint32 identity) { assert(identity == 498); ++conversations; }
    uint32 CountPctFromMaxHealth(uint32 percent) const { return percent; }
    void SetHealth(uint32 value) { health = value; }
    Creature* SummonCreature(uint32 identity, Position const& pose) { return spawn(identity, pose); }
};
struct TempSummon : Creature
{
    Unit* summoner = nullptr;
    TempSummon* ToTempSummon() override { return this; }
    Unit* GetSummoner() { return summoner; }
};
void Unit::NotifyDeath(Creature* creature)
{
#include "NativeDeath.inc"
}
struct SummonList
{
    std::set<ObjectGuid> actors;
    void Summon(Creature* actor) { actors.insert(actor->GetGUID()); }
    void Despawn(Creature* actor) { actors.erase(actor->GetGUID()); }
};
struct EventMap
{
    uint32 clock = 0;
    std::map<uint32, uint32> events;
    void Reset() { clock = 0; events.clear(); }
    void RescheduleEvent(uint32 identity, uint32 delay) { events[identity] = clock + delay; }
    void Update(uint32 diff) { clock += diff; }
    uint32 ExecuteEvent()
    {
        auto selected = events.end();
        for (auto current = events.begin(); current != events.end(); ++current)
            if (current->second <= clock && (selected == events.end() || current->second < selected->second)) selected = current;
        if (selected == events.end()) return 0;
        uint32 identity = selected->first; events.erase(selected); return identity;
    }
};
struct Magnar : CreatureAI
{
    Creature* me;
    explicit Magnar(Creature* actor) : me(actor) { wave = 1; count_dies = 0; first_phase = true; Reset(); }
    bool UpdateVictim() const { return me->victim; }
    void DoCast(uint32) { }
    void DoMeleeAttackIfReady() { }
#include "Fields.inc"
#include "Reset.inc"
#include "EnterCombat.inc"
#include "JustSummoned.inc"
#include "Death.inc"
#include "Advance.inc"
#include "Start.inc"
#include "Update.inc"
#include "Launch.inc"
#include "Summon.inc"
};
struct Fixture
{
    Map map;
    Creature actor, ally, marker;
    CreatureAI allyAI;
    Player player;
    Unit pet, guardian;
    Magnar ai{&actor};
    std::vector<std::unique_ptr<TempSummon>> summons;
    Fixture()
    {
        actor.entry = 96034; actor.map = &map; actor.ai = &ai; actor.player = &player; actor.ally = &ally;
        ally.ai = &allyAI;
        actor.spawn = [&](uint32 identity, Position const&) {
            auto summon = std::make_unique<TempSummon>(); summon->entry = identity;
            summon->guid = {uint32(summons.size() + 1)}; summon->summoner = &actor; summon->map = &map;
            summon->ai = &allyAI;
            summon->ally = &ally; summon->marker = &marker;
            TempSummon* result = summon.get(); map.actors[result->guid] = result; summons.push_back(std::move(summon));
            ai.JustSummoned(result); return result;
        };
        ai.DoAction(1);
        assert(summons.size() == 12 && ai.trashList.size() == 12);
    }
    void BeginWaves()
    {
        ai.events.events.clear(); ai.events.RescheduleEvent(EVENT_4, 0); ai.UpdateAI(0);
        assert(ai.stop_combat && ai.wave == 2 && ai.count_dies == 2);
    }
    void Die(uint32 index, Unit* killer = nullptr)
    {
        summons[index]->alive = false; (killer ? killer : &player)->NotifyDeath(summons[index].get());
    }
};
int main()
{
#ifdef BASELINE
    { Fixture fixture; fixture.Die(11); assert(fixture.ai.count_dies == 255 && fixture.ai.wave == 1); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(0); fixture.Die(0); assert(fixture.ai.wave == 3 && fixture.summons[1]->alive); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(10); fixture.Die(11); assert(fixture.ai.wave == 3 && fixture.summons[0]->alive && fixture.summons[1]->alive); }
    std::cout << "BASELINE: pinned6e6d1715 underflows before waves and advances from a duplicate or two unselected deaths while both intended opponents can remain alive. Native death notification body, controlled actors/clock.\n";
#else
    for (uint32 source = 0; source < 3; ++source)
    {
        Fixture fixture; Unit* killer = source == 0 ? static_cast<Unit*>(&fixture.player) : source == 1 ? &fixture.pet : &fixture.guardian;
        fixture.BeginWaves();
        for (uint32 index = 0; index < 10; ++index)
        {
            uint32 previousWave = fixture.ai.wave; uint32 previousCount = fixture.ai.count_dies;
            fixture.ai.SummonedCreatureDies(fixture.summons[index].get(), killer);
            assert(fixture.ai.wave == previousWave && fixture.ai.count_dies == previousCount);
            fixture.Die(index, killer);
            previousWave = fixture.ai.wave; previousCount = fixture.ai.count_dies;
            fixture.Die(index, killer); assert(fixture.ai.wave == previousWave && fixture.ai.count_dies == previousCount);
            if (index == 1 || index == 3 || index == 4 || index == 6) assert(fixture.ai.count_dies == (index == 3 ? 1 : index == 6 ? 3 : 2));
        }
        assert(fixture.ai.wave == 7 && !fixture.ai.first_phase && !fixture.ai.stop_combat && fixture.player.credits == 1);
        assert(fixture.actor.health == 80 && fixture.summons[10]->alive && fixture.summons[11]->alive);
        assert(fixture.summons[10]->casts == 1 && fixture.summons[11]->casts == 1);
        fixture.Die(10, killer); fixture.Die(11, killer); assert(fixture.player.credits == 1 && fixture.ai.count_dies == 0);
    }
    { Fixture fixture; fixture.Die(11); assert(!fixture.ai.count_dies && fixture.ai.wave == 1); fixture.BeginWaves(); fixture.Die(10); assert(fixture.ai.count_dies == 2 && fixture.ai.wave == 2); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(2); assert(fixture.ai.count_dies == 2 && fixture.ai.wave == 2); fixture.Die(0); fixture.Die(1); assert(fixture.ai.wave == 3 && fixture.ai.count_dies == 2 && !fixture.player.credits); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(0); fixture.ai.Reset(); fixture.Die(1); assert(!fixture.ai.count_dies && fixture.ai.activeWave.empty() && !fixture.ai.releasingWave && !fixture.player.credits); }
    {
        Fixture fixture; fixture.map.actors.erase(fixture.summons[0]->guid); fixture.map.actors.erase(fixture.summons[1]->guid);
        fixture.BeginWaves(); fixture.Die(2); fixture.Die(3); assert(fixture.ai.wave == 3);
    }
    {
        Fixture fixture;
        fixture.player.onCredit = [&]() { assert(fixture.actor.health == 80 && fixture.summons[10]->casts == 1 && fixture.summons[11]->casts == 1 && !fixture.ai.releasingWave); };
        for (auto& summon : fixture.summons) summon->onActivation = [&](Creature* actor) {
            if (actor->entry == 94813 || !actor->alive) return;
            uint32 pendingWave = fixture.ai.wave;
            assert(fixture.ai.releasingWave); actor->alive = false; fixture.guardian.NotifyDeath(actor);
            assert(fixture.ai.releasingWave && fixture.ai.wave == pendingWave && fixture.ai.first_phase);
        };
        fixture.ai.events.events.clear(); fixture.ai.events.RescheduleEvent(EVENT_4, 0); fixture.ai.UpdateAI(0);
        assert(fixture.ai.wave == 7 && !fixture.ai.first_phase && !fixture.ai.stop_combat && fixture.player.credits == 1);
        assert(fixture.ai.activeWave.empty() && !fixture.ai.releasingWave && !fixture.ai.count_dies && fixture.actor.health == 80);
    }
    {
        Fixture fixture; for (uint32 index = 1; index < 12; ++index) fixture.map.actors.erase(fixture.summons[index]->guid);
        fixture.BeginWaves(); fixture.Die(0); assert(fixture.ai.count_dies == 1 && fixture.ai.wave == 2 && !fixture.player.credits);
    }
    { Fixture fixture; fixture.ai.SummonedCreatureDies(nullptr, &fixture.player); assert(!fixture.ai.count_dies); }
    {
        Fixture fixture; fixture.BeginWaves(); TempSummon unrelated; unrelated.guid = {987}; unrelated.entry = 97123; unrelated.alive = false;
        fixture.guardian.NotifyDeath(&unrelated); unrelated.summoner = &fixture.actor; fixture.guardian.NotifyDeath(&unrelated);
        fixture.actor.IsAIEnabled = false; fixture.Die(0); fixture.actor.IsAIEnabled = true;
        assert(fixture.ai.count_dies == 2 && fixture.ai.wave == 2 && !fixture.player.credits);
        fixture.Die(0); assert(fixture.ai.count_dies == 1);
    }
    { Fixture fixture; fixture.BeginWaves(); fixture.ai.count_dies = 0; fixture.Die(0); assert(!fixture.ai.count_dies && fixture.ai.wave == 2 && !fixture.player.credits); }
    {
        Fixture fixture; fixture.ai.UpdateAI(33000);
        for (uint32 tick = 0; tick < 3; ++tick) fixture.ai.UpdateAI(0);
        assert(fixture.ai.wave == 2 && fixture.ai.stop_combat && fixture.ai.events.events.at(EVENT_5) == 34000);
        fixture.actor.victim = false; fixture.ai.UpdateAI(30000);
        assert(!fixture.actor.conversations && fixture.ai.events.clock == 33000);
        for (uint32 index = 0; index < 10; ++index) fixture.Die(index);
        assert(!fixture.ai.stop_combat && !fixture.ai.first_phase && fixture.player.credits == 1 && !fixture.actor.conversations);
        fixture.ai.UpdateAI(1000); assert(!fixture.actor.conversations && fixture.ai.events.clock == 33000);
        fixture.actor.victim = true; fixture.actor.unitState = UNIT_STATE_CASTING; fixture.ai.UpdateAI(1000);
        assert(!fixture.actor.conversations && fixture.ai.events.events.count(EVENT_5));
        fixture.actor.unitState = 0; fixture.ai.UpdateAI(0);
        assert(fixture.actor.conversations == 1 && !fixture.ai.events.events.count(EVENT_5));
        fixture.ai.UpdateAI(15000); fixture.ai.UpdateAI(0); fixture.ai.UpdateAI(0);
        assert(fixture.actor.conversations == 1);
    }
    { Fixture fixture; fixture.BeginWaves(); fixture.ai.Reset(); fixture.ai.UpdateAI(60000); assert(!fixture.actor.conversations && !fixture.ai.events.events.count(EVENT_5)); }
    std::cout << "PASS: extracted native wave/start/update/death methods retain2/2/1/2/3 counted opponents and two auxiliary windshapers, player/pet/guardian notifications, once-only tracked deaths and original final Windwall/80pct/46427 requests. Live/duplicate/unselected/null/stale-reset callbacks cannot advance; activation-time deaths defer until stable snapshot release. Combat-gated498 resumes once after waves/victim/casting gates and Reset clears its pending event. Missing actors do not grant completion; replacement/retry/full Reset and client combat/VFX remain open.\n";
#endif
}
