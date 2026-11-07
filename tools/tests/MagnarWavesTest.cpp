#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
using uint8 = uint8_t;
using uint32 = uint32_t;
using int32 = int32_t;
#include "UnitTypeMask.inc"
enum { REACT_PASSIVE, REACT_DEFENSIVE, UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE = 0x2000000,
    UNIT_FLAG_IMMUNE_TO_PC = 0x100, UNIT_FLAG_IMMUNE_TO_NPC = 0x200, UNIT_FLAG_NOT_ATTACKABLE_1 = 0x80,
    UNIT_STATE_CASTING = 1, UNIT_STATE_STUNNED = 2, CRITERIA_TYPE_SCRIPT_EVENT_2 = 92,
    CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1,
    EVENT_1 = 1, EVENT_2, EVENT_3, EVENT_4, EVENT_5 };
struct Unit;
struct Player;
struct ObjectGuid
{
    uint32 identity = 0;
    Unit* unit = nullptr;
    uint32 high = 0;
    bool IsPlayer() const;
    void Clear() { identity = 0; unit = nullptr; high = 0; }
    bool operator==(ObjectGuid const& other) const { return identity == other.identity && high == other.high; }
    bool operator<(ObjectGuid const& other) const { return high < other.high || (high == other.high && identity < other.identity); }
};
struct Position { float positionX, positionY, positionZ, orientation; };
#include "Positions.inc"
struct Creature;
#include "StartHeader.inc"
#include "CombatHeader.inc"
bool duelRebindAdmitted=false,duelResumeReady=false,duelResetAdmitted=false;
bool MagnarCombat::NeedsReset(Creature*,CreatureAI const*) { return duelResumeReady; }
bool MagnarCombat::Rebind(Creature*,CreatureAI const*,std::uint64_t& health) { if(!duelRebindAdmitted)return false;duelRebindAdmitted=false;duelResumeReady=true;health=80;return true; }
void MagnarCombat::Remember(Creature*, CreatureAI const*, std::array<ObjectGuid,12> const&, std::array<bool,12> const&) { }
bool MagnarCombat::Restore(Creature*, CreatureAI const*, SummonList&, std::array<ObjectGuid,12>&, std::array<bool,12>&) { return duelResumeReady; }
#include "../../src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/hruthnir_pillik.h"
// Ordinary wave fixtures have no shared instance. Only the focused Reset
// notification cases opt into observing the bridge; real consumers are native.
bool observeResetNotifications=false;
std::vector<std::pair<Creature*,Creature*>> resetNotifications;
std::function<void()> onResetNotification;
void MagnarStart::ResetOffer(Creature* companion, Creature* boss) { assert(observeResetNotifications&&companion&&boss);resetNotifications.emplace_back(companion,boss);if(onResetNotification)onResetNotification(); }
struct InstanceScript { ObjectGuid companion;ObjectGuid GetGuidData(uint32 type) { assert(type==PillikHandoff::Companion);return companion; } };
struct TempSummon;
struct Unit;
struct CreatureAI
{
    uint32 attacks = 0;
    std::function<void()> onAttack;
    virtual ~CreatureAI() = default;
    virtual void Reset() { }
    virtual void EnterCombat(Unit*) { }
    virtual void JustSummoned(Creature*) { }
    virtual void SummonedCreatureDies(Creature*, Unit*) { }
    virtual void DoAction(int32) { }
    virtual void UpdateAI(uint32) { }
    virtual void AttackStart(Unit*) { ++attacks; if (onAttack) onAttack(); }
};
struct Unit
{
    virtual ~Unit() = default;
    Unit* owner = nullptr;
    Unit* charmer = nullptr;
    bool available = true;
    virtual Player* ToPlayer() { return nullptr; }
    virtual Player const* ToPlayer() const { return nullptr; }
    virtual Creature* ToCreature() { return nullptr; }
    ObjectGuid GetCharmerOrOwnerGUID() const { return {0, charmer ? charmer : owner}; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const;
    static Creature* GetCreature(Unit const& source, ObjectGuid guid);
    bool IsAIEnabled = true;
    void NotifyDeath(Creature* creature);
};
struct Scenario
{
    uint32 identity = 909, step = 3;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 identity) { return available && identity == 42 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct Map;
struct Player : Unit
{
    uint32 credits = 0, status = QUEST_STATUS_INCOMPLETE, playerClass = CLASS_WARRIOR, instance = 42;
    bool world = true, contact = true, phase = true, rewarded = false, alliance = false, alive = true;
    float distance = 25;
    Map* map = nullptr;
    Player* ToPlayer() override { return this; }
    Player const* ToPlayer() const override { return this; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    uint32 getClass() const { return playerClass; }
    Map* GetMap() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    bool InSamePhase(Creature*) const { return phase; }
    bool IsWithinDistInMap(Creature*, float range) const { assert(range == 100.f); return distance <= range; }
    bool GetQuestRewardStatus(uint32 identity) const { assert(identity == 39191); return rewarded; }
    uint32 GetQuestStatus(uint32 identity) const { assert(identity == 39191); return status; }
    std::function<void()> onCredit;
    void UpdateAchievementCriteria(uint32 type, uint32 event) { assert(type == 92 && (event == 46427 || event == 45784)); ++credits; if (onCredit) onCredit(); scenarioManager.scenario.step = event == 46427 ? 4 : 5; }
};
bool ObjectGuid::IsPlayer() const { return unit && unit->ToPlayer(); }
struct Conversation { void Remove() { assert(false); } };
struct ObjectAccessor
{
    static Conversation* GetConversation(Unit const&, ObjectGuid) { return nullptr; }
    static Player* GetPlayer(Unit const&, ObjectGuid guid) { return guid.unit && guid.unit->available ? guid.unit->ToPlayer() : nullptr; }
};
#include "OwnerPlayer.inc"
struct MotionMaster
{
    uint32 jumps = 0;
    void MoveJump(float, float, float, float, float, float) { ++jumps; }
};
struct Map
{
    struct Reference { Player* player; Player* getSource() const { return player; } };
    std::vector<Reference> players;
    auto const& GetPlayers() const { return players; }
    std::map<ObjectGuid, Creature*> actors;
    Creature* GetCreature(ObjectGuid guid) { auto found = actors.find(guid); return found == actors.end() ? nullptr : found->second; }
};
struct Creature : Unit
{
    ObjectGuid guid{0};
    uint32 entry = 0, mapId = 1495, instance = 42, flags = 0, faction = 14, health = 100, conversations = 0, casts = 0, removed = 0, unitState = 0;
    bool alive = true, victim = true, world = true, despawning = false, accessory = false;
    uint32 despawns = 0;
    CreatureAI* ai = nullptr;
    Creature* ally = nullptr;
    Creature* marker = nullptr;
    Player* player = nullptr;
    Map* map = nullptr;
    MotionMaster motion;
    std::function<void(Creature*)> onActivation;
    std::function<void(Creature*)> onDespawn;
    std::function<Creature*(uint32, Position const&)> spawn;
    Creature* ToCreature() override { return this; }
    virtual TempSummon* ToTempSummon() { return nullptr; }
    CreatureAI* AI() const { return ai; }
    bool isInCombat() const { assert(duelResetAdmitted); return false; } Unit* getVictim() const { assert(false); return nullptr; }
    CreatureAI* GetDisabledAI() const { return nullptr; }
    InstanceScript* instanceScript = nullptr;
    InstanceScript* GetInstanceScript() const { return instanceScript; }
    ObjectGuid GetGUID() const { return guid; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return world; }
    bool IsDespawn() const { return despawning; }
    bool HasUnitTypeMask(uint32 mask) const { assert(mask == UNIT_MASK_ACCESSORY); return accessory; }
    void DespawnOrUnsummon() { assert(!despawning); ++despawns; despawning = true; world = false; map->actors.erase(guid); if (onDespawn) onDespawn(this); }
    Map* GetMap() const { return map; }
    float GetDistance(Player* target) const { return target->distance; }
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
    std::function<void()> onSetHealth;
    void SetHealth(std::uint64_t value) { assert(value<=std::numeric_limits<uint32>::max());health=static_cast<uint32>(value);if(onSetHealth){auto callback=onSetHealth;callback();} }
    Creature* SummonCreature(uint32 identity, Position const& pose) { return spawn(identity, pose); }
};
Creature* Unit::GetCreature(Unit const& source, ObjectGuid guid)
{
    Creature* owner = const_cast<Unit&>(source).ToCreature();
    return owner && owner->GetMap() ? owner->GetMap()->GetCreature(guid) : nullptr;
}
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
    explicit SummonList(Creature* owner = nullptr) : me(owner) { }
    Creature* me;
    std::recursive_mutex m_lock;
    std::set<ObjectGuid> actors;
    bool empty() const { return actors.empty(); }
    auto begin() { return actors.begin(); }
    auto erase(std::set<ObjectGuid>::iterator position) { return actors.erase(position); }
    void Summon(Creature* actor) { actors.insert(actor->GetGUID()); }
    void Despawn(Creature* actor) { actors.erase(actor->GetGUID()); }
#include "DespawnAll.inc"
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
    explicit Magnar(Creature* actor) : me(actor), summons(actor) { actor->ai = this; wave = 1; count_dies = 0; first_phase = true; Reset(); }
    bool UpdateVictim() const { return me->victim; }
    void DoCast(uint32) { }
    void DoMeleeAttackIfReady() { }
#include "Fields.inc"
#include "Handoff.inc"
#include "Reset.inc"
#include "EnterCombat.inc"
#include "JustSummoned.inc"
#include "Death.inc"
#include "Advance.inc"
#include "Eligible.inc"
#include "Readiness.inc"
#include "Start.inc"
#include "Update.inc"
#include "Launch.inc"
#include "Summon.inc"
#include "Reserve.inc"
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
    uint32 failSpawns = 0;
    explicit Fixture(bool start = true)
    {
        scenarioManager = {}; player.map = &map; pet.owner = guardian.owner = &player;
        map.players = {{nullptr}, {&player}};
        actor.entry = 96034; actor.map = &map; actor.ai = &ai; actor.player = &player; actor.ally = &ally;
        ally.ai = &allyAI; ally.map = &map; ally.entry = 96468;
        actor.spawn = [&](uint32 identity, Position const&) -> Creature* {
            if (failSpawns) { --failSpawns; return nullptr; }
            auto summon = std::make_unique<TempSummon>(); summon->entry = identity;
            summon->guid = {uint32(summons.size() + 1)}; summon->summoner = &actor; summon->map = &map;
            summon->ai = &allyAI;
            summon->ally = &ally; summon->marker = &marker;
            TempSummon* result = summon.get(); map.actors[result->guid] = result; summons.push_back(std::move(summon));
            ai.JustSummoned(result); return result;
        };
        if (start)
        {
            ai.DoAction(1);
            assert(summons.size() == 12 && ai.trashList.size() == 12);
        }
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
    void FinishWaves(Unit* killer = nullptr)
    {
        BeginWaves();
        for (uint32 index = 0; index < 10; ++index) Die(index, killer);
    }
};
static void TestDuelHealthCallbacks()
{
    for(unsigned mode=0;mode<7;++mode){duelResumeReady=duelResetAdmitted=false;Fixture fixture;auto live=std::make_unique<Magnar>(&fixture.actor);CreatureAI replacement;auto generation=live->reserveGeneration;unsigned calls=0;fixture.actor.onSetHealth=[&]{++calls;if(mode==0){fixture.actor.ai=nullptr;live.reset();}if(mode==1)fixture.actor.ai=&replacement;if(mode==2)fixture.actor.IsAIEnabled=false;if(mode==3)fixture.actor.world=false;if(mode==4)fixture.actor.alive=false;if(mode==5)fixture.actor.despawning=true;if(mode==6)live->UpdateAI(0);};duelRebindAdmitted=duelResetAdmitted=true;live->UpdateAI(0);assert(calls==1&&!duelRebindAdmitted&&fixture.actor.health==80);if(mode==0)assert(!live);else{if(mode<6)assert(live->reserveGeneration==generation&&live->first_phase);fixture.actor.onSetHealth={};fixture.actor.ai=live.get();fixture.actor.IsAIEnabled=fixture.actor.world=fixture.actor.alive=true;fixture.actor.despawning=false;live->UpdateAI(0);assert(!live->first_phase&&live->wave==7&&live->reserveGeneration==generation+1&&calls==1);live->UpdateAI(0);assert(live->reserveGeneration==generation+1&&calls==1);}fixture.actor.onSetHealth={};duelResumeReady=duelResetAdmitted=false;}
    std::cout<<"PASS:7 actual UpdateAI health-callback cases stop after AI deletion/replacement/disable/world/death/despawn;readmission resumes once without HP replay,and reentrant same-AI Reset is not repeated. Receipt/ready admission controlled,actual typed bridge separately tested.\n";
}
int main()
{
#ifdef BASELINE
    { Fixture fixture; fixture.Die(11); assert(fixture.ai.count_dies == 255 && fixture.ai.wave == 1); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(0); fixture.Die(0); assert(fixture.ai.wave == 3 && fixture.summons[1]->alive); }
    { Fixture fixture; fixture.BeginWaves(); fixture.Die(10); fixture.Die(11); assert(fixture.ai.wave == 3 && fixture.summons[0]->alive && fixture.summons[1]->alive); }
    std::cout << "BASELINE: pinned6e6d1715 underflows before waves and advances from a duplicate or two unselected deaths while both intended opponents can remain alive. Native death notification body, controlled actors/clock.\n";
#elif defined(CREDIT_BASELINE)
    {
        Fixture fixture; Player bystander; bystander.playerClass = 2; bystander.status = 0; bystander.map = &fixture.map;
        fixture.actor.player = &bystander; fixture.FinishWaves(&fixture.pet);
        assert(bystander.credits == 1 && !fixture.player.credits && !fixture.ai.first_phase && fixture.ai.wave == 7);
    }
    {
        Fixture fixture; fixture.actor.player = nullptr; fixture.map.players.clear(); fixture.FinishWaves(&fixture.ally);
        assert(!fixture.player.credits && !fixture.ai.first_phase && !fixture.ai.stop_combat && fixture.ai.wave == 7);
        fixture.actor.player = &fixture.player; fixture.map.players = {{&fixture.player}}; fixture.ai.UpdateAI(1000);
        assert(!fixture.player.credits && fixture.ai.wave == 7);
    }
    std::cout << "BASELINE: pinnedc0f8efce sends46427 to an unrelated nearest non-Warrior despite a pet owner, and permanently opens the final duel without credit when no recipient is present. Controlled scenario/map services.\n";
#elif defined(START_BASELINE)
    {
        Fixture fixture; fixture.ai.UpdateAI(1000); fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 24 && fixture.ai.trashList.size() == 12 && fixture.ai.summons.actors.size() == 24);
        assert(fixture.ai.events.events.at(EVENT_4) == 34000);
    }
    {
        Fixture fixture(false); fixture.actor.ally = nullptr; fixture.ai.DoAction(99);
        assert(fixture.summons.size() == 12 && fixture.actor.motion.jumps == 1 && fixture.ai.events.events.empty());
        fixture.actor.ally = &fixture.ally; fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 24 && fixture.ai.events.events.at(EVENT_4) == 33000);
    }
    {
        Fixture fixture(false); scenarioManager.scenario.step = 2; fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 12 && fixture.ai.events.events.at(EVENT_4) == 33000);
    }
    std::cout << "BASELINE: pinned5d5a1f24 duplicates12 existing summons and postpones wave timer on a repeated start; wrong action/missing companion still mutates the encounter, retry duplicates actors, and a premature scenario step can start combat. Controlled services, not client traversal.\n";
#elif defined(RESET_BASELINE)
    {
        Fixture fixture; fixture.BeginWaves(); fixture.Die(0); fixture.ai.Reset();
        assert(fixture.ai.wave == 2 && fixture.ai.trashList.size() == 11 && fixture.ai.summons.actors.size() == 11);
        fixture.ai.DoAction(1); assert(fixture.summons.size() == 24 && fixture.ai.summons.actors.size() == 23 && fixture.summons[1]->world);
        fixture.ai.events.events.clear(); fixture.ai.events.RescheduleEvent(EVENT_4, 0); fixture.ai.UpdateAI(0);
        assert(fixture.ai.wave == 3 && fixture.ai.count_dies == 2);
    }
    std::cout << "BASELINE: pinned6eb208cf leaves wave2 and11 old owned/reserve actors on first-phase Reset, restart adds another12 and starts at wave2 instead of wave1. Native summon cleanup is available but never called. Controlled actor/map services.\n";
#elif defined(RETRY_BASELINE)
    Fixture fixture(false);
    fixture.ai.first_phase = false; fixture.ai.wave = 7; scenarioManager.scenario.step = 4;
    fixture.ai.EnterCombat(&fixture.player);
    for (uint32 frame = 0; frame < 6; ++frame) fixture.ai.UpdateAI(frame ? 1 : 33000);
    assert(fixture.ai.wave == 8 && fixture.ai.stop_combat && fixture.actor.motion.jumps == 1 && fixture.ai.count_dies == 0);
    std::cout << "BASELINE: pinnedf91fbfc7 final-duel re-entry re-arms first-phase33000ms transition, jumps/protects the boss and advances nonexistent wave7 to8 with no tracked opponent able to release it. Controlled combat/event services, not a live client replay.\n";
#elif defined(RESERVE_BASELINE)
    Fixture fixture(false); fixture.failSpawns = 11; fixture.ai.DoAction(1);
    assert(fixture.summons.size() == 1); fixture.BeginWaves(); fixture.Die(0);
    fixture.failSpawns = 0; fixture.ai.UpdateAI(60000);
    assert(fixture.summons.size() == 1 && fixture.ai.count_dies == 1 && fixture.ai.wave == 2 && !fixture.player.credits);
    std::cout << "COUNTERFACTUAL pinned a2073e254: one successful reserve spawn and eleven failures permanently leaves wave1 waiting for an opponent that does not exist; repairing spawn availability cannot retry the original roster. Not a passing ordinary encounter.\n";
#elif defined(READINESS_BASELINE)
    for (bool companion : {false, true}) {
        Fixture fixture;
        if(companion)fixture.ally.IsAIEnabled=false;else fixture.summons[0]->IsAIEnabled=false;
        fixture.BeginWaves();
        assert(fixture.ai.activeWave.size()==2 && fixture.allyAI.attacks==4);
    }
    {
        Fixture fixture;bool removed=false;
        fixture.summons[0]->onActivation=[&](Creature*){if(!removed){removed=true;for(uint32 i=1;i<12;++i)fixture.map.actors.erase(fixture.summons[i]->guid);}};
        fixture.BeginWaves();fixture.Die(0);fixture.ai.UpdateAI(1000);
        assert(fixture.ai.count_dies==1 && fixture.ai.activeWave.empty() && fixture.summons.size()==23 && !fixture.player.credits);
    }
    std::cout<<"COUNTERFACTUAL pinned fb01c9a6a dispatches AttackStart through disabled selected/companion AI; a selected actor disappearing during another activation is replaced as unselected, leaving count1 with no tracked opponent. Not an ordinary successful encounter. Null AI risk is traced statically, not crashed deliberately.\n";
#else
    TestDuelHealthCallbacks();
    uint32 readinessCases=0;
    for(bool alliance:{false,true})for(uint32 status:{1u,3u})for(uint32 killer=0;killer<3;++killer)for(uint32 mode=0;mode<4;++mode){
        Fixture fixture;fixture.player.alliance=alliance;fixture.player.status=status;
        if(mode==0)fixture.summons[0]->IsAIEnabled=false;
        if(mode==1)fixture.summons[0]->ai=nullptr;
        if(mode==2)fixture.ally.IsAIEnabled=false;
        if(mode==3)fixture.ally.ai=nullptr;
        fixture.BeginWaves();
        assert(fixture.ai.pendingWave==1 && fixture.ai.activeWave.empty() && fixture.allyAI.attacks==0 && fixture.summons.size()==12);
        fixture.summons[0]->IsAIEnabled=true;fixture.summons[0]->ai=&fixture.allyAI;
        fixture.ally.IsAIEnabled=true;fixture.ally.ai=&fixture.allyAI;
        fixture.ai.UpdateAI(999);assert(fixture.ai.pendingWave==1);
        fixture.ai.UpdateAI(1);assert(!fixture.ai.pendingWave && fixture.ai.activeWave.size()==2 && fixture.ai.pendingActivation.empty() && fixture.allyAI.attacks==4);
        fixture.ai.UpdateAI(1000);assert(fixture.allyAI.attacks==4);
        Unit* owner=killer==0?static_cast<Unit*>(&fixture.player):killer==1?&fixture.pet:&fixture.guardian;
        for(uint32 i=0;i<10;++i)fixture.Die(i,owner);
        assert(fixture.player.credits==1 && fixture.ai.wave==7 && fixture.ai.pendingActivation.empty());++readinessCases;
    }
    for(uint32 mode=0;mode<9;++mode){
        Fixture fixture;Map other;
        switch(mode){case 0:fixture.actor.ally=nullptr;break;case 1:fixture.ally.world=false;break;case 2:fixture.ally.alive=false;break;case 3:fixture.ally.despawning=true;break;case 4:fixture.ally.map=&other;break;case 5:fixture.ally.instance=43;break;case 6:fixture.summons[0]->despawning=true;break;case 7:fixture.summons[0]->map=&other;break;case 8:fixture.summons[0]->instance=43;break;}
        fixture.BeginWaves();assert(fixture.ai.pendingWave==1 && fixture.ai.activeWave.empty() && !fixture.player.credits);
        fixture.actor.ally=&fixture.ally;fixture.ally.world=fixture.ally.alive=true;fixture.ally.despawning=false;fixture.ally.map=&fixture.map;fixture.ally.instance=42;
        fixture.summons[0]->despawning=false;fixture.summons[0]->map=&fixture.map;fixture.summons[0]->instance=42;
        fixture.ai.UpdateAI(1000);assert(!fixture.ai.pendingWave && fixture.ai.activeWave.size()==2);
        for(uint32 i=0;i<10;++i)fixture.Die(i);assert(fixture.player.credits==1);++readinessCases;
    }
    for(uint32 mode=0;mode<3;++mode){
        Fixture fixture;bool changed=false;auto old=fixture.summons[1]->guid;
        fixture.summons[0]->onActivation=[&](Creature*){if(changed)return;changed=true;if(mode==0)fixture.summons[1]->IsAIEnabled=false;else if(mode==1)fixture.summons[1]->ai=nullptr;else fixture.map.actors.erase(old);};
        fixture.BeginWaves();assert(fixture.ai.count_dies==2 && fixture.ai.activeWave.size()==2 && fixture.ai.pendingActivation.count(old));
        assert(fixture.allyAI.attacks==2);
        fixture.summons[1]->IsAIEnabled=true;fixture.summons[1]->ai=&fixture.allyAI;fixture.ai.UpdateAI(1000);
        assert(fixture.ai.pendingActivation.empty() && fixture.ai.count_dies==2 && fixture.allyAI.attacks==4);
        fixture.Die(0);if(mode==2){fixture.Die(1);assert(fixture.ai.count_dies==1);fixture.Die(12);}else fixture.Die(1);
        for(uint32 i=2;i<10;++i)fixture.Die(i);assert(fixture.player.credits==1);++readinessCases;
    }
    {
        Fixture fixture;bool removed=false;fixture.summons[0]->onActivation=[&](Creature*){if(removed)return;removed=true;for(uint32 i=1;i<12;++i)fixture.map.actors.erase(fixture.summons[i]->guid);};
        fixture.BeginWaves();assert(fixture.ai.activeWave.size()==2 && fixture.ai.pendingActivation.count(fixture.summons[1]->guid));
        fixture.Die(0);fixture.ai.UpdateAI(1000);assert(fixture.summons.size()==23 && fixture.ai.count_dies==1 && fixture.ai.activeWave.count(fixture.summons[12]->guid));
        fixture.Die(1);assert(fixture.ai.count_dies==1);for(uint32 i=12;i<21;++i)fixture.Die(i,&fixture.guardian);assert(fixture.player.credits==1);++readinessCases;
    }
    for(bool restart:{false,true}){
        Fixture fixture;bool once=false;
        fixture.summons[0]->onActivation=[&](Creature*){if(once)return;once=true;fixture.ai.Reset();if(restart)fixture.ai.DoAction(1);};
        fixture.ai.events.events.clear();fixture.ai.events.RescheduleEvent(EVENT_4,0);fixture.ai.UpdateAI(0);
        assert(fixture.ai.wave==1 && !fixture.ai.count_dies && fixture.ai.activeWave.empty() && fixture.ai.pendingActivation.empty() && !fixture.ai.releasingWave && !fixture.ai.restoringReserve && !fixture.player.credits);
        assert(!fixture.ai.events.events.count(EVENT_5));
        if(restart){assert(fixture.summons.size()==24 && fixture.ai.events.events.at(EVENT_4)==33000);fixture.BeginWaves();for(uint32 i=12;i<22;++i)fixture.Die(i);assert(fixture.player.credits==1);}++readinessCases;
    }
    for(bool restart:{false,true}){
        Fixture fixture(false);bool once=false;auto spawn=fixture.actor.spawn;
        fixture.actor.spawn=[&](uint32 id,Position const&p){auto actor=spawn(id,p);if(!once){once=true;fixture.ai.Reset();if(restart)fixture.ai.DoAction(1);}return actor;};
        fixture.ai.DoAction(1);assert(fixture.ai.wave==1 && !fixture.ai.restoringReserve && fixture.ai.pendingActivation.empty());
        assert(fixture.actor.motion.jumps==(restart?1u:0u));
        if(restart){assert(fixture.summons.size()==13 && fixture.ai.trashList.size()==12);fixture.BeginWaves();for(uint32 i=1;i<11;++i)fixture.Die(i);assert(fixture.player.credits==1);}else assert(fixture.ai.trashList.empty() && fixture.ai.events.events.empty());++readinessCases;
    }
    {
        Fixture fixture;fixture.BeginWaves();TempSummon other;other.guid=fixture.summons[0]->guid;other.guid.high=1;other.alive=false;
        fixture.ai.SummonedCreatureDies(&other,&fixture.player);assert(fixture.ai.count_dies==2 && fixture.ai.activeWave.size()==2);++readinessCases;
    }
    {
        Fixture fixture;bool removed=false;auto old=fixture.summons[0]->guid;
        fixture.summons[0]->onActivation=[&](Creature*){if(!removed){removed=true;fixture.map.actors.erase(old);}};
        fixture.BeginWaves();assert(fixture.ai.pendingActivation.count(old) && fixture.ai.activeWave.count(old) && fixture.ai.count_dies==2);
        fixture.ai.UpdateAI(1000);assert(!fixture.ai.pendingActivation.count(old) && fixture.ai.pendingActivation.empty() && fixture.ai.activeWave.count(fixture.summons[12]->guid));
        fixture.Die(12);fixture.Die(1);for(uint32 i=2;i<10;++i)fixture.Die(i);assert(fixture.player.credits==1);++readinessCases;
    }
    {
        Fixture fixture;bool once=false;
        fixture.allyAI.onAttack=[&]{if(!once){once=true;fixture.ally.IsAIEnabled=false;}};
        fixture.BeginWaves();assert(fixture.ai.pendingActivation.size()==2 && fixture.allyAI.attacks==1 && fixture.ai.count_dies==2);
        fixture.ally.IsAIEnabled=true;fixture.ai.UpdateAI(1000);
        assert(fixture.ai.pendingActivation.empty() && fixture.allyAI.attacks==5);
        for(uint32 i=0;i<10;++i)fixture.Die(i);assert(fixture.player.credits==1);++readinessCases;
    }
    {
        Fixture fixture;bool once=false;
        fixture.summons[10]->onActivation=[&](Creature*){if(once)return;once=true;fixture.ai.Reset();};
        fixture.BeginWaves();for(uint32 i=0;i<7;++i)fixture.Die(i);
        assert(once && fixture.ai.wave==1 && fixture.ai.pendingActivation.empty() && !fixture.ai.restoringReserve && !fixture.ai.releasingWave && !fixture.player.credits && fixture.actor.health==100);
        assert(!fixture.summons[10]->casts && !fixture.summons[11]->casts);++readinessCases;
    }
    std::cout<<"PASS "<<readinessCases<<" readiness/activation cases: disabled/null selected or companion AI retains original batch; same-actor1s retry,48 faction/status/player-pet-guardian full waves, scope refusals, mid-release lost/disabled actor, full GUID, Reset and new-attempt callback cancellation; no fabricated death/progress. Controlled engine endpoints.\n";
    {
        Fixture fixture(false); fixture.failSpawns = 11; fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 1); fixture.failSpawns = 100; fixture.BeginWaves();
        assert(fixture.ai.pendingWave == 1 && fixture.ai.activeWave.empty() && fixture.ai.count_dies == 2);
        fixture.failSpawns = 100; fixture.ai.UpdateAI(0);
        assert(fixture.summons.size() == 1 && fixture.ai.pendingWave == 1 && !fixture.player.credits);
        fixture.failSpawns = 0; fixture.ai.UpdateAI(999); assert(fixture.summons.size() == 1);
        fixture.ai.UpdateAI(1); assert(fixture.summons.size() == 12 && !fixture.ai.pendingWave && fixture.ai.activeWave.size() == 2);
        for (uint32 index = 0; index < 10; ++index) fixture.Die(index, &fixture.guardian);
        assert(fixture.player.credits == 1 && fixture.ai.wave == 7 && scenarioManager.scenario.step == 4);
    }
    {
        Fixture fixture; fixture.BeginWaves(); auto old = fixture.summons[0]->guid;
        fixture.map.actors.erase(old); fixture.ai.UpdateAI(1000);
        assert(fixture.summons.size() == 13 && fixture.ai.activeWave.count(fixture.summons[12]->guid) && !fixture.ai.activeWave.count(old));
        assert(fixture.ai.count_dies == 2 && !fixture.player.credits && fixture.summons[12]->flags == 0);
        fixture.Die(0); assert(fixture.ai.count_dies == 2);
        fixture.Die(12, &fixture.pet); fixture.Die(1, &fixture.guardian);
        for (uint32 index = 2; index < 10; ++index) fixture.Die(index);
        assert(fixture.player.credits == 1 && fixture.ai.wave == 7);
    }
    {
        Fixture fixture; fixture.BeginWaves(); fixture.actor.IsAIEnabled = false;
        fixture.Die(0, &fixture.pet); assert(fixture.ai.count_dies == 2);
        fixture.actor.IsAIEnabled = true; fixture.ai.UpdateAI(1000);
        assert(fixture.ai.count_dies == 1 && fixture.ai.reserveDeaths[0] && fixture.summons.size() == 12);
        for (uint32 index = 1; index < 10; ++index) fixture.Die(index);
        assert(fixture.player.credits == 1 && fixture.ai.wave == 7);
    }
    {
        Fixture fixture; fixture.map.actors.erase(fixture.summons[0]->guid);
        fixture.player.contact = false; fixture.ai.UpdateAI(1000); assert(fixture.summons.size() == 12);
        fixture.player.contact = true; fixture.ai.UpdateAI(0); assert(fixture.summons.size() == 13 && !fixture.player.credits);
        fixture.ai.Reset(); fixture.ai.UpdateAI(60000); assert(fixture.summons.size() == 13 && !fixture.ai.pendingWave);
    }
    {
        Fixture fixture; fixture.BeginWaves(); for (uint32 i=0;i<7;++i) fixture.Die(i);
        assert(fixture.ai.wave==6 && fixture.ai.count_dies==3);
        fixture.map.actors.erase(fixture.summons[9]->guid); fixture.map.actors.erase(fixture.summons[10]->guid);
        fixture.ai.UpdateAI(1000); assert(fixture.summons.size()==13 && fixture.summons[12]->casts==1);
        assert(fixture.ai.activeWave.count(fixture.summons[12]->guid) && fixture.ai.count_dies==3);
        fixture.Die(7); fixture.Die(8); fixture.Die(12); assert(fixture.player.credits==1);
        fixture.ai.UpdateAI(60000); assert(fixture.summons.size()==13);
    }
    std::cout << "PASS: actual reserve retry retains original12 entry/pose slots, delays incomplete batches, replaces only missing active GUID without death credit, rejects stale old death, recovers exact corpse after disabled-parent callback, defers absent participant and cancels on Reset. Partial failures followed by retry finish native2/2/1/2/3 and exactly one46427. Controlled native map/spawn/criteria services.\n";
    {
        Fixture fixture(false);
        fixture.ai.first_phase = false; fixture.ai.wave = 7;
        fixture.ai.finaleStarted = fixture.ai.finalePending = fixture.ai.stop_combat = true;
        fixture.ai.finaleTimer = 34064; fixture.actor.faction = 35;
        fixture.actor.flags = UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1;
        fixture.ai.Reset();
        assert(!fixture.ai.finalePending && !fixture.ai.finaleTimer && fixture.actor.flags == UNIT_FLAG_IMMUNE_TO_NPC && fixture.actor.faction == 14);
        fixture.ai.EnterCombat(&fixture.player);
        for (uint32 frame = 0; frame < 6; ++frame) fixture.ai.UpdateAI(frame ? 1 : 33000);
        assert(fixture.ai.wave == 7 && !fixture.ai.stop_combat && !fixture.actor.motion.jumps && !fixture.actor.conversations);
        std::cout << "PASS: final-duel re-entry retains combat spells without re-arming the first-phase33000ms jump/protection/nonexistent-wave softlock.\n";
    }
    for (bool victim : {false, true}) for (uint32 state : {0u, 1u, 2u, 3u})
    {
        Fixture fixture(false);
        fixture.ai.first_phase = false; fixture.ai.wave = 7;
        fixture.ai.finaleStarted = fixture.ai.finalePending = fixture.ai.stop_combat = true;
        fixture.ai.finaleInstanceId = 42; fixture.ai.finalePlayer = {0, &fixture.player}; fixture.ai.finaleTimer = 34064;
        scenarioManager.scenario.step = 4;
        fixture.actor.victim = victim; fixture.actor.unitState = state;
        fixture.ai.UpdateAI(34063); assert(!fixture.player.credits && !fixture.actor.despawns);
        fixture.ai.UpdateAI(1); assert(fixture.player.credits == 1 && !fixture.actor.despawns);
        fixture.ai.UpdateAI(5999); assert(!fixture.actor.despawns);
        fixture.ai.UpdateAI(1); assert(fixture.actor.despawns == 1 && !fixture.actor.conversations);
    }
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
    for (bool alliance : {false, true}) for (uint32 status : {1u, 3u}) for (uint32 source = 0; source < 5; ++source)
    {
        Fixture fixture; Player peer, bystander; Unit charmed;
        fixture.player.alliance = alliance; fixture.player.status = status; fixture.player.alive = false;
        peer.map = bystander.map = &fixture.map; peer.distance = 5; bystander.distance = 1; bystander.playerClass = 2;
        fixture.map.players = {{&bystander}, {nullptr}, {&peer}, {&fixture.player}}; fixture.actor.player = &bystander;
        charmed.charmer = &fixture.player;
        Unit* killer = source == 0 ? static_cast<Unit*>(&fixture.player) : source == 1 ? &fixture.pet : source == 2 ? &fixture.guardian : source == 3 ? &charmed : &fixture.ally;
        fixture.FinishWaves(killer);
        assert(!bystander.credits && fixture.player.credits == uint32(source != 4) && peer.credits == uint32(source == 4));
        assert(scenarioManager.scenario.step == 4 && fixture.ai.wave == 7 && !fixture.ai.first_phase && !fixture.ai.stop_combat);
        for (uint32 retry = 0; retry < 3; ++retry) { fixture.ai.AdvanceWaveIfReady(killer); fixture.ai.UpdateAI(0); fixture.Die(9, killer); }
        assert(fixture.player.credits + peer.credits == 1);
    }
    for (uint32 invalid = 0; invalid < 19; ++invalid)
    {
        Fixture fixture; Map unrelated;
        fixture.BeginWaves(); for(uint32 i=0;i<9;++i)fixture.Die(i,&fixture.pet);
        if (invalid == 0) fixture.player.world = false;
        if (invalid == 1) fixture.player.contact = false;
        if (invalid == 2) fixture.player.playerClass = 2;
        if (invalid == 3) fixture.player.map = &unrelated;
        if (invalid == 4) fixture.player.instance = 43;
        if (invalid == 5) fixture.player.phase = false;
        if (invalid == 6) fixture.player.distance = 101;
        if (invalid == 7) fixture.player.rewarded = true;
        if (invalid == 8) fixture.player.status = 0;
        if (invalid == 9) fixture.player.status = 5;
        if (invalid == 10) fixture.player.status = 6;
        if (invalid == 11) scenarioManager.available = false;
        if (invalid == 12) scenarioManager.scenario.identity = 944;
        if (invalid == 13) scenarioManager.scenario.step = 2;
        if (invalid == 14) scenarioManager.scenario.step = 4;
        if (invalid == 15) scenarioManager.scenario.step = 5;
        if (invalid == 16) fixture.actor.entry = 96035;
        if (invalid == 17) fixture.actor.mapId = 1220;
        if (invalid == 18) fixture.actor.instance = 43;
        fixture.Die(9,&fixture.pet);
        assert(!fixture.player.credits && fixture.ai.first_phase && fixture.ai.stop_combat && fixture.ai.wave == 6 && !fixture.ai.count_dies);
        assert((fixture.actor.flags & UNIT_FLAG_IMMUNE_TO_PC) && (fixture.actor.flags & UNIT_FLAG_NOT_SELECTABLE));
        fixture.actor.victim = false; fixture.actor.unitState = UNIT_STATE_CASTING | UNIT_STATE_STUNNED;
        fixture.ai.UpdateAI(30000); assert(!fixture.player.credits && fixture.ai.first_phase && fixture.ai.wave == 6);
        fixture.player.world = fixture.player.contact = fixture.player.phase = true;
        fixture.player.playerClass = CLASS_WARRIOR; fixture.player.map = &fixture.map; fixture.player.instance = 42;
        fixture.player.distance = 100; fixture.player.rewarded = false; fixture.player.status = QUEST_STATUS_INCOMPLETE;
        fixture.actor.entry = 96034; fixture.actor.mapId = 1495; fixture.actor.instance = 42; scenarioManager = {};
        fixture.ai.UpdateAI(0);
        assert(fixture.player.credits == 1 && !fixture.ai.first_phase && !fixture.ai.stop_combat && fixture.ai.wave == 7);
        assert((fixture.actor.flags & UNIT_FLAG_IMMUNE_TO_NPC) && !(fixture.actor.flags & (UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1)));
        fixture.ai.UpdateAI(30000); assert(fixture.player.credits == 1);
    }
    {
        Fixture fixture; fixture.map.players.clear(); fixture.player.available = false; fixture.actor.player = nullptr;
        fixture.FinishWaves(&fixture.guardian); fixture.actor.victim = false;
        assert(fixture.ai.wave == 6 && !fixture.player.credits);
        fixture.ai.Reset(); fixture.map.players = {{&fixture.player}}; fixture.ai.UpdateAI(0);
        assert(fixture.ai.wave == 1 && !fixture.player.credits && !fixture.ai.stop_combat);
    }
    {
        Fixture fixture; Player peer; peer.map = &fixture.map; fixture.map.players = {{&peer}};
        fixture.player.available = false; fixture.FinishWaves(&fixture.guardian);
        assert(!fixture.player.credits && peer.credits == 1);
    }
    {
        Fixture fixture;
        fixture.player.onCredit = [&]() {
            assert(fixture.ai.wave == 7 && !fixture.ai.first_phase && !fixture.ai.stop_combat);
            fixture.ai.AdvanceWaveIfReady(&fixture.pet); fixture.ai.UpdateAI(0); fixture.Die(9, &fixture.guardian);
        };
        fixture.actor.onActivation = [&](Creature*) { fixture.ai.AdvanceWaveIfReady(&fixture.pet); };
        fixture.FinishWaves(); assert(fixture.player.credits == 1);
    }
    {
        Fixture fixture; fixture.map.players.clear(); fixture.FinishWaves(&fixture.ally); fixture.actor.victim = false;
        fixture.map.players = {{&fixture.player}}; fixture.ai.UpdateAI(0); assert(fixture.player.credits == 1);
    }
    for (uint32 invalid = 0; invalid < 28; ++invalid)
    {
        Fixture fixture(false); Map unrelated; int32 action = 1;
        fixture.actor.flags = UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1;
        uint32 flags = fixture.actor.flags;
        if (invalid == 0) action = 0;
        if (invalid == 1) action = 2;
        if (invalid == 2) action = -1;
        if (invalid == 3) fixture.actor.world = false;
        if (invalid == 4) fixture.actor.alive = false;
        if (invalid == 5) fixture.actor.despawning = true;
        if (invalid == 6) fixture.actor.entry = 96035;
        if (invalid == 7) fixture.actor.mapId = 1511;
        if (invalid == 8) fixture.player.status = 0;
        if (invalid == 9) fixture.player.rewarded = true;
        if (invalid == 10) fixture.player.playerClass = 2;
        if (invalid == 11) fixture.player.phase = false;
        if (invalid == 12) fixture.player.distance = 101;
        if (invalid == 13) fixture.player.contact = false;
        if (invalid == 14) fixture.player.instance = 43;
        if (invalid == 15) fixture.player.map = &unrelated;
        if (invalid == 16) scenarioManager.available = false;
        if (invalid == 17) scenarioManager.scenario.identity = 944;
        if (invalid == 18) scenarioManager.scenario.step = 2;
        if (invalid == 19) scenarioManager.scenario.step = 4;
        if (invalid == 20) scenarioManager.scenario.step = 5;
        if (invalid == 21) fixture.actor.ally = nullptr;
        if (invalid == 22) fixture.ally.alive = false;
        if (invalid == 23) fixture.ally.IsAIEnabled = false;
        if (invalid == 24) fixture.ally.ai = nullptr;
        if (invalid == 25) fixture.map.players.clear();
        if (invalid == 26) fixture.actor.map = nullptr;
        if (invalid == 27) fixture.player.world = false;
        fixture.ai.DoAction(action);
        assert(!fixture.ai.encounterStarted && fixture.summons.empty() && fixture.ai.trashList.empty() && fixture.ai.events.events.empty());
        assert(!fixture.actor.motion.jumps && fixture.actor.flags == flags);
        fixture.actor.world = fixture.actor.alive = fixture.player.world = fixture.player.contact = fixture.player.phase = true;
        fixture.actor.despawning = false; fixture.actor.entry = 96034; fixture.actor.mapId = 1495; fixture.actor.map = &fixture.map;
        fixture.player.status = QUEST_STATUS_INCOMPLETE; fixture.player.rewarded = false; fixture.player.playerClass = CLASS_WARRIOR;
        fixture.player.distance = 100; fixture.player.instance = 42; fixture.player.map = &fixture.map; scenarioManager = {};
        fixture.actor.ally = &fixture.ally; fixture.ally.alive = fixture.ally.IsAIEnabled = true; fixture.ally.ai = &fixture.allyAI;
        fixture.map.players = {{&fixture.player}}; fixture.ai.DoAction(1);
        assert(fixture.ai.encounterStarted && fixture.summons.size() == 12 && fixture.ai.trashList.size() == 12 && fixture.actor.motion.jumps == 1);
        assert(fixture.ai.events.events.at(EVENT_4) == 33000 && fixture.actor.flags == 0);
    }
    for (bool alliance : {false, true}) for (uint32 status : {1u, 3u})
    {
        Fixture fixture(false); fixture.player.alliance = alliance; fixture.player.status = status;
        auto spawn = fixture.actor.spawn;
        fixture.actor.spawn = [&](uint32 identity, Position const& pose) { fixture.ai.DoAction(1); return spawn(identity, pose); };
        fixture.actor.onActivation = [&](Creature*) { fixture.ai.DoAction(1); };
        fixture.ai.DoAction(1); fixture.ai.UpdateAI(1000);
        auto reserve = fixture.ai.trashList; auto events = fixture.ai.events.events;
        fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 12 && fixture.ai.summons.actors.size() == 12 && fixture.ai.trashList == reserve && fixture.ai.events.events == events);
        assert(fixture.actor.motion.jumps == 1 && fixture.ai.events.events.at(EVENT_4) == 33000);
        fixture.BeginWaves(); auto active = fixture.ai.activeWave; events = fixture.ai.events.events;
        fixture.ai.DoAction(1);
        assert(fixture.summons.size() == 12 && fixture.ai.activeWave == active && fixture.ai.count_dies == 2 && fixture.ai.wave == 2 && fixture.ai.events.events == events);
        for (uint32 index = 0; index < 10; ++index) fixture.Die(index, &fixture.guardian);
        fixture.ai.DoAction(1); assert(fixture.summons.size() == 12 && fixture.player.credits == 1 && fixture.ai.wave == 7);
        fixture.ai.Reset(); assert(!fixture.ai.encounterStarted);
    }
    for (bool alliance : {false, true}) for (uint32 status : {1u, 3u}) for (uint32 deaths = 0; deaths < 10; ++deaths)
    {
        Fixture fixture; fixture.player.alliance = alliance; fixture.player.status = status;
        if (deaths) fixture.BeginWaves();
        for (uint32 index = 0; index < deaths; ++index) fixture.Die(index, &fixture.pet);
        for (uint32 index = deaths; index < 12; ++index)
            fixture.summons[index]->onDespawn = [&](Creature* removed) {
                assert(fixture.ai.encounterStarted && fixture.ai.releasingWave && fixture.ai.activeWave.empty() && !fixture.ai.count_dies && fixture.ai.wave == 1);
                fixture.ai.DoAction(1); assert(fixture.summons.size() == 12);
                removed->alive = false; fixture.guardian.NotifyDeath(removed);
                assert(fixture.ai.encounterStarted && fixture.ai.releasingWave && fixture.ai.wave == 1 && !fixture.player.credits);
            };
        fixture.ai.Reset();
        assert(fixture.ai.wave == 1 && fixture.ai.first_phase && !fixture.ai.stop_combat && !fixture.ai.encounterStarted && !fixture.ai.releasingWave);
        assert(fixture.ai.trashList.empty() && fixture.ai.activeWave.empty() && fixture.ai.summons.actors.empty() && fixture.ai.events.events.empty() && !fixture.ai.count_dies);
        for (uint32 index = deaths; index < 12; ++index) assert(fixture.summons[index]->despawns == 1 && !fixture.summons[index]->world);
        fixture.ai.DoAction(1); assert(fixture.summons.size() == 24 && fixture.ai.summons.actors.size() == 12 && fixture.ai.trashList.size() == 12);
        assert(fixture.ai.events.events.at(EVENT_4) == 33000); fixture.BeginWaves();
        auto active = fixture.ai.activeWave;
        for (uint32 index = 0; index < 12; ++index) fixture.ai.SummonedCreatureDies(fixture.summons[index].get(), &fixture.guardian);
        assert(fixture.ai.activeWave == active && fixture.ai.wave == 2 && fixture.ai.count_dies == 2);
        for (uint32 index = 12; index < 22; ++index) fixture.Die(index, &fixture.guardian);
        assert(fixture.player.credits == 1 && fixture.ai.wave == 7 && !fixture.ai.first_phase && scenarioManager.scenario.step == 4);
    }
    {
        Fixture fixture; Creature unrelated; unrelated.guid = {999}; unrelated.map = &fixture.map; fixture.map.actors[unrelated.guid] = &unrelated;
        fixture.map.actors.erase(fixture.summons[0]->guid); fixture.summons[1]->accessory = true;
        fixture.ai.Reset(); fixture.ai.Reset();
        assert(fixture.ai.wave == 1 && fixture.ai.summons.actors.empty() && fixture.ai.trashList.empty());
        assert(!fixture.summons[0]->despawns && !fixture.summons[1]->despawns && fixture.summons[1]->world && !unrelated.despawns && unrelated.world);
        for (uint32 index = 2; index < 12; ++index) assert(fixture.summons[index]->despawns == 1);
    }
    {
        Fixture fixture; fixture.FinishWaves(); fixture.ai.Reset();
        assert(!fixture.ai.first_phase && fixture.ai.wave == 7 && !fixture.ai.encounterStarted);
        assert(fixture.summons[10]->world && fixture.summons[11]->world && !fixture.summons[10]->despawns && !fixture.summons[11]->despawns);
    }
    {
        Fixture fixture; fixture.BeginWaves(); fixture.actor.IsAIEnabled = false; fixture.Die(0); fixture.actor.IsAIEnabled = true;
        assert(fixture.ai.count_dies == 2 && fixture.ai.summons.actors.size() == 12);
        fixture.ai.Reset(); assert(fixture.ai.wave == 1 && fixture.ai.summons.actors.empty());
        for (auto const& summon : fixture.summons) assert(summon->despawns == 1);
        fixture.ai.DoAction(1); assert(fixture.summons.size() == 24 && fixture.ai.summons.actors.size() == 12);
    }
    uint32 resetCases=0;
    auto attach=[](Fixture&f,InstanceScript&instance){f.ally.guid={90001,&f.ally,1};instance.companion=f.ally.guid;f.map.actors[f.ally.guid]=&f.ally;f.actor.instanceScript=&instance;observeResetNotifications=true;resetNotifications.clear();onResetNotification={};};
    for(bool first:{false,true})for(bool accepted:{false,true})for(bool releasing:{false,true}){
        Fixture f(false);InstanceScript instance;attach(f,instance);f.ai.first_phase=first;f.ai.encounterStarted=accepted;f.ai.releasingWave=releasing;f.ai.Reset();assert(resetNotifications.size()==uint32(first&&!releasing));if(!resetNotifications.empty())assert(resetNotifications[0]==std::make_pair(&f.ally,&f.actor));++resetCases;
    }
    for(uint32 fault=0;fault<10;++fault){Fixture f(false);InstanceScript instance;attach(f,instance);CreatureAI other;switch(fault){case 0:f.actor.IsAIEnabled=false;break;case 1:f.actor.ai=&other;break;case 2:f.actor.world=false;break;case 3:f.actor.alive=false;break;case 4:f.actor.despawning=true;break;case 5:f.actor.entry=1;break;case 6:f.actor.mapId=1;break;case 7:f.actor.instanceScript=nullptr;break;case 8:instance.companion={99999,nullptr,1};break;case 9:f.map.actors.erase(f.ally.guid);break;}f.ai.Reset();assert(resetNotifications.empty());++resetCases;}
    {Fixture f;InstanceScript instance;attach(f,instance);f.summons[0]->onDespawn=[&](Creature*){++f.ai.reserveGeneration;};f.ai.Reset();assert(resetNotifications.empty());++resetCases;}
    {Fixture f(false);auto live=std::make_unique<Magnar>(&f.actor);InstanceScript instance;attach(f,instance);onResetNotification=[&]{f.actor.ai=nullptr;live.reset();};live->Reset();assert(!live&&resetNotifications.size()==1);onResetNotification={};f.actor.ai=&f.ai;++resetCases;}
    observeResetNotifications=false;
    std::cout<<"PASS reset notification actual-method cases="<<resetCases<<": fresh and accepted first-phase Reset use the same companion bridge; second-phase/releasing/actor/instance/GUID refusals and lost cleanup epoch retained; actual AI deletion during notification is last access. Consumer acknowledgement and actual native replacement independently tested.\n";
    std::cout << "PASS: extracted native wave/start/update/death methods retain2/2/1/2/3 counted opponents and two auxiliary windshapers, player/pet/guardian notifications, once-only tracked deaths and original final Windwall/80pct/46427 requests. Live/duplicate/unselected/null/stale-reset callbacks cannot advance; activation-time deaths defer until stable snapshot release. Combat-gated498 resumes once after waves/victim/casting gates and Reset clears its pending event. Missing actors do not grant completion; replacement/retry/full Reset and client combat/VFX remain open.\n";
    std::cout << "PASS:40 faction/status/source wave-credit combinations prefer actual player/pet/guardian/charmer owner over eligible peers; dead owner remains eligible.19 invalid contexts retain the protected completed-wave state and retry before victim/cast/stun gates only after readmission. Missing/null/unavailable owner uses eligible native-map fallback, Reset cancels pending credit, and reentrant flag/credit/death/tick calls cannot duplicate46427. Shared scenario services controlled, not actual multiplayer/client traversal.\n";
    std::cout << "PASS:28 invalid start requests do not spawn, move, change flags or consume a start; explicit readmitted retry succeeds. Four faction/status starts claim before reentrant summon/flag callbacks; duplicate requests cannot add12 actors, discard reserve/active GUIDs or postpone timers before/during/after waves. Full encounter rebuild, missing-spawn recovery and client traversal remain open.\n";
    std::cout << "PASS:40 first-phase reset/restart runs at0..9 counted deaths clean only the tracked remainder through actual native SummonList::DespawnAll, restore wave1 and protect cleanup against reentrant death/start. A fresh12-GUID roster completes all10 deaths; stale old GUIDs cannot consume it. Missing/accessory/unrelated actors and repeated Reset preserve native cleanup policy; final-phase Reset leaves its existing wave7/auxiliary behavior unchanged. Actual gossip/pose/health/final-despawn/re-entry recovery and client traversal remain separate.\n";
#endif
}
