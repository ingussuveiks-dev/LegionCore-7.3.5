#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
using uint32 = uint32_t;
using uint8 = uint8_t;
using ObjectGuid = uint32;
enum { UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC = 1, UNIT_FLAG_IMMUNE_TO_NPC = 2, UNIT_FLAG_NOT_ATTACKABLE_1 = 4,
    REACT_PASSIVE = 0, REACT_AGGRESSIVE = 2, EVENT_1 = 1, EVENT_2 = 2, EVENT_3 = 3, EVENT_4 = 4,
    UNIT_STATE_CASTING = 1, UNIT_STATE_STUNNED = 2, CRITERIA_TYPE_SCRIPT_EVENT_2 = 92 };
struct Creature;
struct TempSummon;
struct ScriptedAI;
struct Map { } instanceMap, otherMap;
struct Unit
{
    virtual ~Unit() = default;
    virtual Creature* ToCreature() { return nullptr; }
    virtual bool IsCreature() const { return false; }
    bool IsAIEnabled = true;
    virtual ScriptedAI* AI() { return nullptr; }
    Unit* GetCharmerOrOwnerPlayerOrPlayerItself() { return nullptr; }
    void UpdateAchievementCriteria(uint32, uint32) { assert(false); }
    void CreateConversation(uint32) { assert(false); }
    void NotifyDeath(Creature* creature);
};
using Player = Unit;
struct Creature : Unit
{
    ObjectGuid guid = 1;
    uint32 entry = 98602, flags = 0, react = REACT_AGGRESSIVE;
    uint32 casts = 0, moves = 0, auraRemovals = 0, attempts = 0;
    bool alive = true, world = true, predelete = false, aura = false, castWorks = true;
    bool victim = true;
    Map* map = &instanceMap;
    ScriptedAI* brain = nullptr;
    std::vector<int> results;
    std::vector<std::unique_ptr<TempSummon>> children;
    std::function<void(Creature*)> onVisual;
    std::function<void()> onCast;
    Creature* ToCreature() override { return this; }
    bool IsCreature() const override { return true; }
    ScriptedAI* AI() override { return brain; }
    uint32 GetEntry() const { return entry; }
    ObjectGuid GetGUID() const { return guid; }
    bool IsAlive() const { return alive; }
    bool HasAura(uint32 identity) const { assert(identity == 195996); return aura; }
    bool HasUnitState(uint32) const { return false; }
    bool IsInWorld() const { return world; }
    bool IsPreDelete() const { return predelete; }
    Map* GetMap() const { return map; }
    virtual TempSummon* ToTempSummon() { return nullptr; }
    void StopAttack() { }
    void SetFlag(uint32, uint32 mask) { flags |= mask; }
    void RemoveFlag(uint32, uint32 mask) { flags &= ~mask; }
    void SetReactState(uint32 state) { react = state; }
    void RemoveAura(uint32 spell) { assert(spell == 195996); aura = false; ++auraRemovals; }
    float GetPositionX() const { return 3515; }
    float GetPositionY() const { return 1800; }
    float GetPositionZ() const { return 0.6f; }
    Creature* GetMotionMaster() { return this; }
    void MoveRandom(float radius) { assert(radius == 10); ++moves; }
    void CastSpell(Creature* target, uint32 spell) { assert(target == this && spell == 197852); ++casts; if (onVisual) onVisual(this); }
    Creature* SummonCreature(uint32, float, float, float, float);
    void DespawnOrUnsummon();
};
struct TempSummon : Creature
{
    Unit* summoner = nullptr;
    Unit* GetSummoner() { return summoner; }
    TempSummon* ToTempSummon() override { return this; }
    void InitSummon();
};
struct ObjectAccessor
{
    static std::unordered_map<ObjectGuid, Creature*> creatures;
    template<class Target> static Target* GetObjectInWorld(ObjectGuid guid, Target*) { auto found = creatures.find(guid); return found == creatures.end() ? nullptr : found->second; }
#include "ObjectInMap.inc"
    static Creature* GetCreature(Creature const&, ObjectGuid);
};
std::unordered_map<ObjectGuid, Creature*> ObjectAccessor::creatures;
using WorldObject = Creature;
#include "GetCreature.inc"
struct EventMap
{
    uint32 scheduled = 0, elapsed = 0;
    void Reset() { scheduled = 0; elapsed = 0; }
    void Update(uint32 diff) { elapsed += diff; }
    uint32 ExecuteEvent() const { return 0; }
    void RescheduleEvent(uint32 identity, uint32 delay) { assert(identity == EVENT_3 && delay == 51000); ++scheduled; }
};
struct SummonList
{
    std::vector<Creature*> tracked;
    explicit SummonList(Creature*) { }
    void Summon(Creature* summon) { tracked.push_back(summon); }
    void Despawn(Creature* summon) { for (auto& trackedSummon : tracked) if (trackedSummon == summon) trackedSummon = nullptr; }
    void DespawnAll() { auto pending = std::move(tracked); tracked.clear(); for (auto summon : pending) if (summon) summon->DespawnOrUnsummon(); }
};
struct ScriptedAI
{
    Creature* me;
    explicit ScriptedAI(Creature* actor) : me(actor) { actor->brain = this; }
    virtual ~ScriptedAI() = default;
    virtual void Reset() { }
    virtual void JustSummoned(Creature*) { }
    virtual void SummonedCreatureDies(Creature*, Unit*) { }
    virtual void SummonedCreatureDespawn(Creature*) { }
    virtual void JustDied(Unit*) { }
    virtual void UpdateAI(uint32) { }
    virtual void IsSummonedBy(Unit*) { }
    void Talk(uint32 identity) { assert(identity == 2); }
    bool UpdateVictim() const { return me->victim; }
    void DoMeleeAttackIfReady() { ++me->moves; }
    void DoCast(uint32 spell) { assert(spell == 195996); ++me->casts; me->aura = me->castWorks; if (me->onCast) me->onCast(); }
};
#include "InitSummon.inc"
void Unit::NotifyDeath(Creature* creature)
{
#include "DeathDispatch.inc"
}
int irand(int minimum, int maximum) { assert(minimum == -5 && maximum == 5); return 0; }
struct FinalBoss : ScriptedAI
{
    explicit FinalBoss(Creature* actor) : ScriptedAI(actor), summons(me) { introDone = false; }
    void UpdateIntro(uint32) { }
#include "Fields.inc"
#include "Reset.inc"
#include "JustSummoned.inc"
#include "SummonedCreatureDies.inc"
#include "SummonedCreatureDespawn.inc"
#include "UpdateSpectreRush.inc"
#include "BeginSpectreRush.inc"
#include "JustDied.inc"
#include "UpdateAI.inc"
};
Creature* Creature::SummonCreature(uint32 identity, float positionX, float positionY, float positionZ, float orientation)
{
    assert(identity == 99609 && positionX == GetPositionX() && positionY == GetPositionY() && positionZ == GetPositionZ() && orientation == 0);
    ++attempts;
    int result = results.empty() ? 1 : results.front();
    if (!results.empty()) results.erase(results.begin());
    if (!result) return nullptr;
    auto summon = std::make_unique<TempSummon>();
    summon->entry = identity;
    summon->guid = 10 + uint32(children.size());
    summon->summoner = this;
    summon->IsAIEnabled = false;
    summon->world = result != 2;
    summon->map = map;
    summon->onVisual = onVisual;
    auto pointer = summon.get();
    children.push_back(std::move(summon));
    if (pointer->world) ObjectAccessor::creatures[pointer->guid] = pointer;
    pointer->InitSummon();
    return pointer;
}
void Creature::DespawnOrUnsummon()
{
    world = false;
    ObjectAccessor::creatures.erase(guid);
    if (auto summon = ToTempSummon())
        if (summon->summoner && summon->summoner->IsAIEnabled) summon->summoner->AI()->SummonedCreatureDespawn(this);
}
struct Fixture
{
    Creature actor;
    FinalBoss ai{&actor};
    Unit player, pet, guardian, npc;
    Fixture() { ObjectAccessor::creatures.clear(); ai.Reset(); }
    ~Fixture() { ObjectAccessor::creatures.clear(); }
    void Kill(Creature* summon, Unit* killer) { summon->alive = false; killer->NotifyDeath(summon); }
    Creature* Child(uint32 index) { return actor.children.at(index).get(); }
    void Unlocked() { assert(actor.flags == 0 && actor.react == REACT_AGGRESSIVE && !actor.aura); }
};
int main()
{
#ifdef BASELINE
    {
        Fixture fixture; fixture.actor.results = {1, 0}; fixture.ai.BeginSpectreRush();
        fixture.Kill(fixture.Child(0), &fixture.pet); fixture.ai.UpdateSpectreRush(60000);
        assert(fixture.actor.attempts == 2 && fixture.actor.flags == 7 && fixture.actor.aura);
    }
    {
        Fixture fixture; fixture.ai.BeginSpectreRush(); fixture.Kill(fixture.Child(0), &fixture.guardian);
        fixture.guardian.NotifyDeath(fixture.Child(0)); fixture.Unlocked(); assert(fixture.Child(1)->alive);
    }
    {
        Fixture fixture; fixture.ai.BeginSpectreRush(); fixture.ai.Reset(); assert(fixture.actor.aura);
    }
    std::cout << "BASELINE: one missing summon locks immunity; duplicate same-GUID death releases with a living opponent; reset retains Spectre Rush aura\n";
#else
    uint32 cases = 0;
    for (int first : {0, 1, 2}) for (int second : {0, 1, 2})
        for (bool castWorks : {false, true}) for (uint32 killerType = 0; killerType < 4; ++killerType)
    {
        Fixture fixture; fixture.actor.castWorks = castWorks; fixture.actor.results = {first, second}; fixture.ai.BeginSpectreRush();
        assert(fixture.ai.spectreActive && fixture.actor.flags == 7 && fixture.actor.react == REACT_PASSIVE && fixture.actor.attempts == 2);
        fixture.ai.BeginSpectreRush(); assert(fixture.actor.attempts == 2 && fixture.actor.casts == 1);
        fixture.ai.UpdateSpectreRush(999); assert(fixture.actor.attempts == 2);
        fixture.ai.UpdateSpectreRush(1); assert(fixture.actor.attempts == 2u + uint32(first != 1) + uint32(second != 1));
        Unit* killer = std::vector<Unit*>{&fixture.player, &fixture.pet, &fixture.guardian, &fixture.npc}[killerType];
        auto firstSummon = ObjectAccessor::GetCreature(fixture.actor, fixture.ai.spectres[0]);
        auto secondSummon = ObjectAccessor::GetCreature(fixture.actor, fixture.ai.spectres[1]);
        assert(firstSummon && secondSummon && firstSummon != secondSummon);
        fixture.Kill(firstSummon, killer); killer->NotifyDeath(firstSummon);
        assert(fixture.ai.count_adds == 1 && fixture.ai.spectreActive);
        firstSummon->DespawnOrUnsummon(); fixture.ai.UpdateSpectreRush(1000);
        assert(fixture.ai.count_adds == 1 && fixture.actor.attempts == 2u + uint32(first != 1) + uint32(second != 1));
        fixture.Kill(secondSummon, killer); fixture.Unlocked(); assert(!fixture.ai.spectreActive);
        auto attempts = fixture.actor.attempts; fixture.ai.UpdateSpectreRush(60000); assert(fixture.actor.attempts == attempts);
        ++cases;
    }
    for (uint32 disappearance = 0; disappearance < 5; ++disappearance)
    {
        Fixture fixture; fixture.ai.BeginSpectreRush(); auto stale = fixture.Child(0);
        if (disappearance == 0) stale->DespawnOrUnsummon();
        if (disappearance == 1) ObjectAccessor::creatures.erase(stale->guid);
        if (disappearance == 2) stale->predelete = true;
        if (disappearance == 3) stale->map = &otherMap;
        if (disappearance == 4) stale->alive = false;
        fixture.ai.UpdateSpectreRush(1000); assert(fixture.actor.attempts == 3 && fixture.ai.count_adds == 0);
        fixture.Kill(stale, &fixture.pet); assert(fixture.ai.count_adds == 0);
        fixture.Kill(fixture.Child(2), &fixture.guardian); fixture.Kill(fixture.Child(1), &fixture.player); fixture.Unlocked();
    }
    {
        Fixture fixture; fixture.ai.BeginSpectreRush(); Creature foreign; foreign.entry = 99609; foreign.guid = 99; foreign.alive = false;
        fixture.ai.SummonedCreatureDies(&foreign, &fixture.npc); fixture.ai.SummonedCreatureDies(nullptr, nullptr);
        fixture.ai.SummonedCreatureDies(fixture.Child(0), &fixture.player);
        foreign.entry = 1; fixture.ai.SummonedCreatureDies(&foreign, &fixture.player); assert(fixture.ai.count_adds == 0);
    }
    for (bool death : {false, true})
    {
        Fixture fixture; fixture.ai.BeginSpectreRush(); auto stale = fixture.Child(0);
        if (death) { fixture.actor.alive = false; fixture.ai.JustDied(nullptr); } else fixture.ai.Reset();
        assert(!fixture.ai.spectreActive && !fixture.Child(0)->world && !fixture.Child(1)->world);
        fixture.ai.UpdateSpectreRush(60000); assert(fixture.actor.attempts == 2);
        fixture.actor.alive = true; fixture.ai.Reset(); fixture.Unlocked(); fixture.ai.BeginSpectreRush();
        fixture.Kill(stale, &fixture.pet); assert(fixture.ai.count_adds == 0 && fixture.ai.spectreActive);
        fixture.Kill(fixture.Child(2), &fixture.pet); fixture.Kill(fixture.Child(3), &fixture.guardian); fixture.Unlocked();
    }
    for (bool resetDuringCast : {false, true})
    {
        Fixture fixture;
        if (resetDuringCast) fixture.actor.onCast = [&] { fixture.ai.Reset(); };
        else fixture.actor.onVisual = [&](Creature*) { fixture.ai.Reset(); };
        fixture.ai.BeginSpectreRush(); assert(!fixture.ai.spectreActive && !fixture.ai.events.scheduled);
        assert(fixture.actor.attempts == (resetDuringCast ? 0u : 1u)); fixture.Unlocked();
    }
    {
        Fixture fixture;
        fixture.actor.onVisual = [&](Creature* summon) { fixture.Kill(summon, &fixture.pet); };
        fixture.ai.BeginSpectreRush(); fixture.Unlocked(); assert(fixture.actor.attempts == 2 && fixture.ai.count_adds == 2);
    }
    {
        Fixture fixture; fixture.actor.results = {0, 0, 0, 0, 0, 0}; fixture.ai.BeginSpectreRush();
        fixture.ai.UpdateSpectreRush(1000); fixture.ai.UpdateSpectreRush(1000); assert(fixture.actor.attempts == 6 && fixture.ai.count_adds == 0);
        fixture.ai.UpdateSpectreRush(1000); assert(fixture.actor.attempts == 8);
        fixture.Kill(fixture.Child(0), &fixture.pet); fixture.Kill(fixture.Child(1), &fixture.guardian); fixture.Unlocked();
    }
    for (bool victim : {false, true}) for (bool castWorks : {false, true})
    {
        Fixture fixture; fixture.actor.victim = victim; fixture.actor.castWorks = castWorks;
        fixture.actor.results = {0, 1}; fixture.ai.BeginSpectreRush(); fixture.ai.UpdateAI(1000);
        assert(fixture.actor.attempts == 3 && fixture.ai.events.elapsed == 0 && fixture.actor.moves == 0);
        fixture.Kill(fixture.Child(0), &fixture.pet); fixture.Kill(fixture.Child(1), &fixture.guardian); fixture.Unlocked();
        fixture.ai.UpdateAI(1000); assert(fixture.ai.events.elapsed == (victim ? 1000u : 0u));
    }
    {
        Fixture fixture; fixture.ai.BeginSpectreRush();
        fixture.Kill(fixture.Child(0), &fixture.pet); fixture.Kill(fixture.Child(1), &fixture.guardian); fixture.Unlocked();
        fixture.ai.BeginSpectreRush(); assert(fixture.actor.attempts == 4 && fixture.ai.count_adds == 0);
        fixture.pet.NotifyDeath(fixture.Child(0)); assert(fixture.ai.count_adds == 0);
        fixture.Kill(fixture.Child(2), &fixture.guardian); fixture.Kill(fixture.Child(3), &fixture.pet); fixture.Unlocked();
    }
    std::cout << "PASS: " << cases << " summon-result/cast/killer paths; actual native InitSummon/death owner dispatch and map resolver, bounded missing/uninserted/despawn/predelete/map/dead retries, GUID-only exactly-once deaths, synchronous deaths, reset/death/reentrant epoch cleanup. Geometry, spell visuals, real combat/group and client traversal are not executed.\n";
#endif
}
