#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>

using uint32 = uint32_t;
using uint8 = uint8_t;
using ObjectGuid = unsigned;
enum QuestStatus { QUEST_STATUS_NONE, QUEST_STATUS_INCOMPLETE, QUEST_STATUS_COMPLETE, QUEST_STATUS_FAILED };
enum { UNIT_FIELD_CREATED_BY_SPELL, REACT_PASSIVE, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN };
struct Creature;
struct Player;

struct Unit
{
    virtual bool IsPlayer() const { return false; }
    virtual Player* ToPlayer() { return nullptr; }
};

struct Player : Unit
{
    bool IsPlayer() const override { return true; }
    Player* ToPlayer() override { return this; }
    bool IsAlive() const { return alive; }
    bool GetQuestRewardStatus(uint32) const { return rewarded; }
    bool HasItemCount(uint32 item) const { return item == 22693 && hasItem; }
    int GetQuestObjectiveData(uint32 quest, uint32 entry) const {
        assert(quest == 8490 && entry == 16364); return credits;
    }
    auto GetSummonList(uint32 entry) { assert(entry == 16364); return &summons; }
    QuestStatus GetQuestStatus(uint32 quest) const { assert(quest == 8490); return status; }
    ObjectGuid GetGUID() const { return guid; }
    void KilledMonsterCredit(uint32 entry) { assert(entry == 16364); ++credits; }

    ObjectGuid guid = 1;
    QuestStatus status = QUEST_STATUS_INCOMPLETE;
    std::list<ObjectGuid> summons;
    bool alive = true, rewarded = false, hasItem = true, nearby = true;
    int credits = 0;
};

struct QuestObjectScope { ObjectGuid Owner = 1; bool Cancelled = false; };
struct TempSummon { Unit* owner = nullptr; Unit* GetSummoner() const { return owner; } };
struct CreatureAI
{
    virtual void IsSummonedBy(Unit*) {}
    virtual void JustSummoned(Creature*) {}
    virtual void SummonedCreatureDespawn(Creature*) {}
    virtual void JustDied(Unit*) {}
    virtual void UpdateAI(uint32) {}
    void AttackStart(Creature*) { ++attacks; }
    unsigned attacks = 0;
};

struct Creature
{
    TempSummon* ToTempSummon() { return &summon; }
    bool IsAlive() const { return alive; }
    bool HasQuestObjectScope() const { return bool(scope); }
    bool IsQuestObjectExpired() const { return !scope || scope->Cancelled; }
    std::weak_ptr<QuestObjectScope> GetQuestObjectScope() const { return scope; }
    uint32 GetUInt32Value(unsigned) const { return spell; }
    uint32 GetGUIDLow() const { return id; }
    uint32 GetEntry() const { return 16364; }
    void SetReactState(unsigned) { passive = true; }
    bool IsWithinDistInMap(Player* p, float range) { assert(range == 35.0f); return p->nearby; }
    CreatureAI* AI() { return ai; }
    void DespawnOrUnsummon() { despawned = true; }
    Creature* SummonCreature(uint32 entry, float, float, float, float, unsigned, uint32 duration) {
        assert(entry == 17086 && duration == 60000);
        if (++summonCalls == failSummon)
            return nullptr;
        ai->JustSummoned(wraith);
        return wraith;
    }

    CreatureAI* ai = nullptr;
    Creature* wraith = nullptr;
    TempSummon summon;
    std::shared_ptr<QuestObjectScope> scope = std::make_shared<QuestObjectScope>();
    uint32 id = 1234, spell = 28247;
    bool alive = true, passive = false, despawned = false;
    unsigned summonCalls = 0, failSummon = 0, cleanupCalls = 0;
};

struct Scripted_NoMovementAI : CreatureAI
{
    explicit Scripted_NoMovementAI(Creature* creature) : me(creature) {}
    void Talk(unsigned text, ObjectGuid target) { assert(text == 0 && target == 1); ++talks; }
    Creature* me;
    unsigned talks = 0;
};
struct CreatureScript { explicit CreatureScript(char const*) {} };
struct SummonList
{
    explicit SummonList(Creature* creature) : owner(creature) {}
    void Summon(Creature*) { ++count; }
    void Despawn(Creature*) { --count; }
    void DespawnAll() { owner->cleanupCalls += count; count = 0; }
    Creature* owner;
    unsigned count = 0;
};
uint32 urand(uint32 first, uint32) { return first; }
unsigned summonWarnings = 0;
#define TC_LOG_WARN(...) (++summonWarnings)
#include "CrystalAI.inc"

struct SummonPropertiesEntry { unsigned ID = 64; };
struct Map
{
    unsigned GetId() const { return id; }
    Creature* GetCreature(ObjectGuid guid) { return creatures[guid]; }
    Creature* Admit(uint32 entry, uint32 spellId, Unit* summoner, SummonPropertiesEntry const* properties)
    {
#include "CrystalAdmission.inc"
        return &admitted;
    }
    unsigned id = 530;
    std::map<ObjectGuid, Creature*> creatures;
    Creature admitted;
};

struct Defense
{
    Defense() : ai(&crystal)
    {
        crystal.ai = &ai;
        crystal.wraith = &wraith;
        crystal.summon.owner = &owner;
        wraith.ai = &wraithAI;
    }
    void Begin() { ai.IsSummonedBy(&owner); }
    Player owner;
    Creature crystal, wraith;
    CreatureAI wraithAI;
    npc_infused_crystal::npc_infused_crystalAI ai;
};

int main()
{
    {
        Defense d;
        Player unrelated;
        unrelated.guid = 2;
        d.Begin();
        d.ai.UpdateAI(999);
        assert(d.crystal.summonCalls == 0 && d.crystal.passive);
        d.ai.UpdateAI(1);
        d.ai.UpdateAI(29999);
        assert(d.crystal.summonCalls == 3 && d.owner.credits == 0);
        d.ai.UpdateAI(1);
        d.ai.UpdateAI(28999);
        assert(d.crystal.summonCalls == 6 && d.owner.credits == 0);
        d.ai.UpdateAI(1);
        d.ai.UpdateAI(60000);
        assert(d.owner.credits == 1 && unrelated.credits == 0 && d.ai.talks == 1);
        assert(d.crystal.despawned && d.crystal.cleanupCalls == 6 && d.wraithAI.attacks == 6);
    }
    {
        Defense d;
        d.Begin();
        d.ai.UpdateAI(60000);
        assert(d.crystal.summonCalls == 3 && d.owner.credits == 0);
        d.ai.UpdateAI(60000);
        assert(d.crystal.summonCalls == 6 && d.owner.credits == 0);
        d.ai.UpdateAI(29000);
        assert(d.owner.credits == 1);
    }
    for (unsigned event = 0; event < 6; ++event)
    {
        Defense d;
        d.Begin();
        d.ai.UpdateAI(1000);
        switch (event)
        {
            case 0: d.ai.JustDied(nullptr); break;
            case 1: d.owner.status = QUEST_STATUS_NONE; break;
            case 2: d.crystal.scope->Cancelled = true; break;
            case 3: d.owner.nearby = false; break;
            case 4: d.owner.alive = false; break;
            case 5: d.crystal.summon.owner = nullptr; break;
        }
        d.ai.UpdateAI(60000);
        assert(d.owner.credits == 0 && d.crystal.cleanupCalls == 3 && d.owner.hasItem);
        assert(d.owner.status != QUEST_STATUS_FAILED);
    }
    {
        Defense d;
        d.crystal.failSummon = 2;
        d.Begin();
        d.ai.UpdateAI(1000);
        d.ai.UpdateAI(60000);
        assert(d.crystal.despawned && d.crystal.cleanupCalls == 1 && d.owner.credits == 0 && summonWarnings == 1);
    }
    {
        Defense d;
        Map map;
        SummonPropertiesEntry properties;
        Unit nonPlayer;
        assert(map.Admit(16364, 28247, &d.owner, &properties));
        assert(!map.Admit(16364, 28247, &nonPlayer, &properties));
        assert(!map.Admit(16364, 28247, &d.owner, nullptr));
        map.creatures[1] = &d.crystal;
        d.owner.summons.push_back(1);
        assert(!map.Admit(16364, 28247, &d.owner, &properties));
        d.crystal.scope->Cancelled = true;
        assert(map.Admit(16364, 28247, &d.owner, &properties));
        d.owner.status = QUEST_STATUS_COMPLETE;
        assert(map.Admit(16364, 28247, &d.owner, &properties));
        d.owner.credits = 1;
        assert(!map.Admit(16364, 28247, &d.owner, &properties));
        d.owner.credits = 0;
        d.owner.hasItem = false;
        assert(!map.Admit(16364, 28247, &d.owner, &properties));
        assert(map.Admit(16364, 0, &nonPlayer, nullptr));
    }
    std::cout << "PASS exact crystal admission and AI timer, two waves, owner credit, cancellation, combat loss and retry.\n";
}
