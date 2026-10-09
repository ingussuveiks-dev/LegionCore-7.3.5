// World boundary for compiling the production quest event handlers.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <memory>
using uint32 = uint32_t;
using uint64 = uint64_t;
using int32 = int32_t;
using ObjectGuid = uint64;
constexpr int QUEST_STATUS_INCOMPLETE = 1, QUEST_STATUS_COMPLETE = 2;
constexpr int UNIT_FIELD_FLAGS = 1, UNIT_FIELD_NPC_FLAGS = 2, UNIT_NPC_FLAG_QUESTGIVER = 2;
constexpr int UNIT_FLAG_IMMUNE_TO_PC = 1, UNIT_FLAG_NON_ATTACKABLE = 2, UNIT_FLAG_NOT_SELECTABLE = 4;
constexpr int UNIT_STAND_STATE_KNEEL = 1, UNIT_STAND_STATE_STAND = 0, REACT_AGGRESSIVE = 1, REACT_PASSIVE = 0,
              TEMPSUMMON_TIMED_DESPAWN = 1;
constexpr int GOSSIP_SENDER_MAIN = 1, DEFAULT_GOSSIP_MESSAGE = 1;
enum class GossipOptionNpc
{
    None
};
using DamageEffectType = int;
using SpellEffIndex = int;
using SpellCastResult = int;
constexpr int SPELL_CAST_OK = 0, SPELL_FAILED_BAD_TARGETS = 1;
constexpr float INVALID_HEIGHT = -100000.0f;
struct Player;
struct Creature;
struct TempSummon;
struct CreatureAI;
std::vector<Creature*> creatures;
std::map<ObjectGuid, Player*> players;
struct WorldObject
{
    virtual ~WorldObject() = default;
};
struct Unit : WorldObject
{
    Player* ownerPlayer = nullptr;
    bool alive = true;
    virtual Player* ToPlayer()
    {
        return nullptr;
    }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself()
    {
        return ToPlayer() ? ToPlayer() : ownerPlayer;
    }
    bool IsAlive()
    {
        return alive;
    }
};
struct Map
{
    float ground = 140;
    float GetHeight(float, float, float)
    {
        return ground;
    }
};
struct Menus
{
    void ClearMenus()
    {
    }
};
struct Player : Unit
{
    ObjectGuid guid = 1;
    uint32 map = 1116;
    float x = 8195, y = -589, z = 140;
    bool combat = false, inWorld = true, teleportOK = true, los = true, item = true;
    std::map<uint32, int> quests;
    std::map<std::pair<uint32, uint32>, uint32> objectives;
    std::set<uint32> rewarded, auras;
    std::vector<uint32> credit, spells;
    Map world;
    Menus menus;
    Menus* PlayerTalkClass = &menus;
    unsigned teleports = 0;
    Player* ToPlayer() override
    {
        return this;
    }
    ObjectGuid GetGUID()
    {
        return guid;
    }
    bool IsInWorld()
    {
        return inWorld;
    }
    bool isInCombat()
    {
        return combat;
    }
    uint32 GetMapId()
    {
        return map;
    }
    int GetQuestStatus(uint32 q)
    {
        return quests[q];
    }
    bool GetQuestRewardStatus(uint32 q)
    {
        return rewarded.count(q) != 0;
    }
    uint32 GetQuestObjectiveData(uint32 q, uint32 o)
    {
        return objectives[{q, o}];
    }
    float GetDistance2d(float a, float b)
    {
        return std::hypot(x - a, y - b);
    }
    float GetDistance(Creature*);
    bool IsWithinLOSInMap(Creature*)
    {
        return los;
    }
    Map* GetMap()
    {
        return &world;
    }
    bool TeleportTo(uint32, float a, float b, float c, float)
    {
        if (!teleportOK)
            return false;
        ++teleports;
        x = a;
        y = b;
        z = c;
        return true;
    }
    void KilledMonsterCredit(uint32 entry)
    {
        credit.push_back(entry);
    }
    bool HasAura(uint32 s)
    {
        return auras.count(s) != 0;
    }
    bool HasItemCount(uint32, uint32)
    {
        return item;
    }
    void CastSpell(Unit*, uint32 s, bool)
    {
        spells.push_back(s);
    }
    void GetCreatureListWithEntryInGridAppend(std::list<Creature*>&, uint32, float);
    void PrepareQuestMenu(ObjectGuid)
    {
    }
    void ADD_GOSSIP_ITEM(GossipOptionNpc, const char*, int, int)
    {
    }
    void SEND_GOSSIP_MENU(int, ObjectGuid)
    {
    }
    void CLOSE_GOSSIP_MENU()
    {
    }
};
struct Creature : Unit
{
    ObjectGuid guid = 100, owner = 1;
    uint32 entry = 0, flags = 0, faction = 35, timer = 90000;
    float x = 0, y = 0, z = 0, multiplier = 1;
    uint64 health = 100, maximum = 100;
    bool IsAIEnabled = true, removed = false;
    int stand = 0;
    CreatureAI* ai = nullptr;
    std::vector<uint32> spells;
    unsigned spawnLimit = 3, spawnAttempts = 0;
    virtual TempSummon* ToTempSummon()
    {
        return nullptr;
    }
    ObjectGuid GetGUID()
    {
        return guid;
    }
    uint32 GetEntry()
    {
        return entry;
    }
    CreatureAI* AI()
    {
        return ai;
    }
    void setFaction(uint32 f)
    {
        faction = f;
    }
    void SetReactState(int)
    {
    }
    void RemoveFlag(int, uint32 f)
    {
        flags &= ~f;
    }
    void SetFlag(int, uint32 f)
    {
        flags |= f;
    }
    void SetStandState(int s)
    {
        stand = s;
    }
    void SetFullHealth()
    {
        health = maximum;
    }
    void CombatStop(bool)
    {
    }
    void DeleteThreatList()
    {
    }
    float GetHealthMultiplierForTarget(Unit*)
    {
        return multiplier;
    }
    uint64 GetHealth(Unit*)
    {
        return health;
    }
    uint64 GetMaxHealth(Unit*)
    {
        return maximum;
    }
    float GetPositionX()
    {
        return x;
    }
    float GetPositionY()
    {
        return y;
    }
    float GetPositionZ()
    {
        return z;
    }
    void DespawnOrUnsummon(uint32 = 0)
    {
        removed = true;
    }
    void CastSpell(Unit*, uint32 s, bool)
    {
        spells.push_back(s);
    }
    Creature* SummonCreature(uint32, float, float, float, float, int, uint32, ObjectGuid);
    void GetCreatureListWithEntryInGrid(std::list<Creature*>& out, uint32 e, float range)
    {
        for (auto c : creatures)
            if (!c->removed && c->entry == e && std::hypot(x - c->x, y - c->y) <= range)
                out.push_back(c);
    }
};
struct TempSummon : Creature
{
    TempSummon* ToTempSummon() override
    {
        return this;
    }
    ObjectGuid GetSummonerGUID()
    {
        return owner;
    }
};
struct CreatureAI
{
    virtual ~CreatureAI() = default;
    virtual uint32 GetData(uint32) const
    {
        return 0;
    }
    virtual void SetData(uint32, uint32)
    {
    }
    virtual void Reset()
    {
    }
    virtual void DoAction(int32)
    {
    }
    virtual void DamageTaken(Unit*, uint32&, DamageEffectType)
    {
    }
    virtual void UpdateAI(uint32)
    {
    }
    virtual void OnSpellClick(Unit*)
    {
    }
    virtual void SummonedCreatureDies(Creature*, Unit*)
    {
    }
    virtual void SummonedCreatureDespawn(Creature*)
    {
    }
    virtual void AttackStart(Unit*)
    {
    }
};
struct ScriptedAI : CreatureAI
{
    Creature* me;
    ScriptedAI(Creature* c) : me(c)
    {
        c->ai = this;
    }
    bool UpdateVictim()
    {
        return true;
    }
    void DoMeleeAttackIfReady()
    {
    }
    void EnterEvadeMode()
    {
        Reset();
    }
};
std::vector<std::unique_ptr<TempSummon>> spawned;
Creature* Creature::SummonCreature(uint32 e, float a, float b, float c, float, int, uint32, ObjectGuid o)
{
    if (spawnAttempts++ % 3 >= spawnLimit)
        return nullptr;
    auto s = std::make_unique<TempSummon>();
    s->entry = e;
    s->x = a;
    s->y = b;
    s->z = c;
    s->owner = o;
    s->guid = 1000 + spawned.size();
    s->IsAIEnabled = false;
    Creature* result = s.get();
    creatures.push_back(result);
    spawned.push_back(std::move(s));
    return result;
}
float Player::GetDistance(Creature* c)
{
    return std::hypot(x - c->x, y - c->y);
}
void Player::GetCreatureListWithEntryInGridAppend(std::list<Creature*>& out, uint32 e, float range)
{
    for (auto c : creatures)
        if (!c->removed && c->entry == e && GetDistance(c) <= range)
            out.push_back(c);
}
struct SummonList
{
    std::vector<Creature*> summons;
    SummonList(Creature*)
    {
    }
    void Summon(Creature* c)
    {
        summons.push_back(c);
    }
    void DespawnAll()
    {
        for (auto c : summons)
            c->DespawnOrUnsummon();
        summons.clear();
    }
};
struct ObjectAccessor
{
    static Player* GetPlayer(Creature&, ObjectGuid id)
    {
        return players.count(id) ? players[id] : nullptr;
    }
};
struct GameObject
{
    uint32 entry = 0;
    ObjectGuid owner = 1;
    ObjectGuid GetOwnerGUID()
    {
        return owner;
    }
    uint32 GetEntry()
    {
        return entry;
    }
};
struct GameObjectScript
{
    GameObjectScript(const char*)
    {
    }
    virtual ~GameObjectScript() = default;
    virtual bool OnGossipHello(Player*, GameObject*)
    {
        return false;
    }
};
struct CreatureScript
{
    CreatureScript(const char*)
    {
    }
    virtual ~CreatureScript() = default;
    virtual CreatureAI* GetAI(Creature*) const
    {
        return nullptr;
    }
    virtual bool OnGossipHello(Player*, Creature*)
    {
        return false;
    }
    virtual bool OnGossipSelect(Player*, Creature*, uint32, uint32)
    {
        return false;
    }
};
struct Callback
{
    void operator+=(int)
    {
    }
};
struct SpellScript
{
    Unit* caster = nullptr;
    Creature* hit = nullptr;
    Unit* GetCaster()
    {
        return caster;
    }
    Creature* GetHitCreature()
    {
        return hit;
    }
    virtual void Register() {};
    Callback OnCheckCast, OnObjectTargetSelect, OnEffectHitTarget;
};
struct SpellScriptLoader
{
    SpellScriptLoader(const char*)
    {
    }
    virtual SpellScript* GetSpellScript() const
    {
        return nullptr;
    }
};
#define PrepareSpellScript(T) public:
#define SpellCheckCastFn(...) 0
#define SpellObjectTargetSelectFn(...) 0
#define SpellEffectFn(...) 0
namespace CampaignRecovery
{
bool Active(Player* p, uint32 q)
{
    return p->GetQuestStatus(q) == QUEST_STATUS_INCOMPLETE || p->GetQuestStatus(q) == QUEST_STATUS_COMPLETE;
}
bool Owned(Creature* c, Player* p)
{
    return c->ToTempSummon() && c->ToTempSummon()->GetSummonerGUID() == p->GetGUID();
}
} // namespace CampaignRecovery
#include "campaign-handlers.inc"
int main()
{
    Player p;
    players[p.guid] = &p;
    // Gate: actual owner interaction, Grubnor first, no duplicate gate credit.
    go_bloodmaul_shadow_gate gate;
    GameObject go;
    go.entry = 229026;
    p.quests[34381] = QUEST_STATUS_INCOMPLETE;
    assert(gate.OnGossipHello(&p, &go));
    p.objectives[{34381, 272612}] = 1;
    assert(!gate.OnGossipHello(&p, &go));
    assert(p.spells.back() == 158396);
    go.owner = 2;
    assert(gate.OnGossipHello(&p, &go));
    go.owner = 1;
    p.objectives[{34381, 272613}] = 1;
    assert(gate.OnGossipHello(&p, &go));
    // Each distinct ritual totem contributes once, only in native objective order.
    TempSummon first, second;
    first.entry = 78386;
    second.entry = 78393;
    npc_bloodmaul_ritual_totem::AI a(&first), b(&second);
    p.quests[34319] = QUEST_STATUS_INCOMPLETE;
    p.credit.clear();
    b.OnSpellClick(&p);
    assert(p.credit.empty());
    a.OnSpellClick(&p);
    a.OnSpellClick(&p);
    assert(p.credit.size() == 1);
    p.objectives[{34319, 272761}] = 1;
    b.OnSpellClick(&p);
    assert(p.credit.size() == 2);
    // Purification rejects live, foreign-owned, used, out-of-LOS or missing-item targets.
    TempSummon soul;
    soul.entry = 77958;
    soul.x = p.x;
    soul.y = p.y;
    creatures = {&soul};
    npc_bloodmaul_campaign_enemy::AI soulAI(&soul);
    spell_bloodmaul_purify_soul::Script purify;
    purify.caster = &p;
    purify.hit = &soul;
    p.quests[34469] = QUEST_STATUS_INCOMPLETE;
    assert(purify.Check() == SPELL_FAILED_BAD_TARGETS);
    soul.alive = false;
    soul.owner = 2;
    assert(purify.Check() == SPELL_FAILED_BAD_TARGETS);
    soul.owner = 1;
    p.item = false;
    assert(purify.Check() == SPELL_FAILED_BAD_TARGETS);
    p.item = true;
    p.los = false;
    assert(purify.Check() == SPELL_FAILED_BAD_TARGETS);
    p.los = true;
    assert(purify.Check() == SPELL_CAST_OK);
    purify.Hit(0);
    assert(soul.spells == std::vector<uint32>{158282});
    purify.Hit(0);
    assert(soul.spells.size() == 1);
    // Two waves: spawn/duplicate/unrelated deaths and despawns never count.
    creatures.clear();
    TempSummon tracker;
    tracker.x = p.x;
    tracker.y = p.y;
    npc_seismic_tremor_tracker::AI waves(&tracker);
    p.quests[34027] = QUEST_STATUS_INCOMPLETE;
    p.credit.clear();
    waves.Start(&p);
    assert(waves.living.size() == 3 && p.credit.empty());
    Creature stranger;
    stranger.guid = 999;
    waves.SummonedCreatureDies(&stranger, &p);
    assert(waves.living.size() == 3);
    std::vector<Creature*> current = creatures;
    for (auto c : current)
        waves.SummonedCreatureDies(c, &p);
    assert(waves.wave == 1 && waves.delay == 2000 && p.credit.empty());
    waves.SummonedCreatureDies(current[0], &p);
    waves.UpdateAI(1999);
    assert(waves.wave == 1);
    waves.UpdateAI(1);
    assert(waves.wave == 2 && waves.living.size() == 3);
    current = creatures;
    for (auto c : current)
        waves.SummonedCreatureDies(c, &p);
    assert(p.credit == std::vector<uint32>{77225} && !waves.wave);
    waves.Start(&p);
    waves.SummonedCreatureDespawn(creatures.back());
    assert(!waves.wave && p.credit.size() == 1);
    tracker.spawnLimit = 2;
    waves.Start(&p);
    assert(!waves.wave && p.credit.size() == 1);
    tracker.spawnLimit = 3;
    waves.Start(&p);
    p.alive = false;
    waves.UpdateAI(1);
    assert(!waves.wave);
    p.alive = true;
    waves.Start(&p);
    p.quests[34027] = 0;
    waves.UpdateAI(1);
    assert(!waves.wave);
    // Scene arrival requires correct origin/quest and successful teleport.
    p.quests[34029] = QUEST_STATUS_INCOMPLETE;
    p.credit.clear();
    p.teleportOK = false;
    SeismicCampaign::FinishRide(&p, false);
    assert(p.credit.empty());
    p.teleportOK = true;
    p.map = 1;
    SeismicCampaign::FinishRide(&p, false);
    assert(p.credit.empty());
    p.map = 1116;
    SeismicCampaign::FinishRide(&p, false);
    assert(p.credit == std::vector<uint32>{77311} && p.teleports == 1);
    SeismicCampaign::FinishRide(&p, false);
    assert(p.teleports == 1);
    p.quests[34033] = QUEST_STATUS_INCOMPLETE;
    SeismicCampaign::FinishRide(&p, true);
    assert(p.teleports == 1);
    p.objectives[{34033, 272438}] = 1;
    p.world.ground = INVALID_HEIGHT;
    SeismicCampaign::FinishRide(&p, true);
    assert(p.teleports == 1);
    p.world.ground = 140;
    SeismicCampaign::FinishRide(&p, true);
    assert(p.teleports == 2);
    // Council surrender uses target-relative health and scaled lethal damage;
    // one or two opponents and third-party attacks cannot finish the quest.
    TempSummon akama, maladaar, naielle;
    akama.entry = 84973;
    maladaar.entry = 84974;
    naielle.entry = 84975;
    creatures = {&akama, &maladaar, &naielle};
    p.x = p.y = 0;
    p.quests[36169] = QUEST_STATUS_INCOMPLETE;
    p.credit.clear();
    npc_exarch_council_trial::AI aa(&akama), ma(&maladaar), na(&naielle);
    aa.DoAction(1);
    ma.DoAction(1);
    na.DoAction(1);
    Player other;
    other.guid = 2;
    uint32 damage = 1000;
    aa.DamageTaken(&other, damage, 0);
    assert(!damage && !aa.defeated);
    damage = 50;
    aa.DamageTaken(&p, damage, 0);
    assert(damage == 50 && !aa.defeated);
    akama.multiplier = 0.01f;
    damage = 2;
    aa.DamageTaken(&p, damage, 0);
    assert(!damage && aa.defeated && p.credit.empty());
    Unit pet;
    pet.ownerPlayer = &p;
    damage = 10000;
    ma.DamageTaken(&pet, damage, 0);
    assert(!damage && p.credit.empty());
    damage = 10000;
    na.DamageTaken(&p, damage, 0);
    assert(!damage && p.credit == std::vector<uint32>{84974});
    damage = 10000;
    na.DamageTaken(&p, damage, 0);
    assert(p.credit.size() == 1);
    akama.health = 3;
    aa.DoAction(1);
    assert(akama.health == 100 && !aa.defeated && akama.stand == UNIT_STAND_STATE_STAND);
    p.alive = false;
    aa.UpdateAI(1);
    assert(!aa.fighting && akama.faction == 35);
    std::cout << "PASS: production gate, ritual, purification, six-kill waves, travel and scaled council surrender "
                 "handlers\n";
}
