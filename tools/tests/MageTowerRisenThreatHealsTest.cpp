#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <vector>

using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int32 = std::int32_t;
using SpellEffIndex = unsigned;
using DamageEffectType = uint32;

struct Creature;
struct Player;
struct InstanceMap;
struct InstanceScript;
struct SmartAI;

struct ObjectGuid
{
    uint64 value = 0;
    bool IsEmpty() const { return value == 0; }
    void Clear() { value = 0; }
    bool operator==(ObjectGuid const& other) const { return value == other.value; }
    bool operator<(ObjectGuid const& other) const { return value < other.value; }
};

struct SpellInfo { uint32 Id; };
constexpr int SMART_EVENT_UPDATE = 60;

struct Unit
{
    uint32 entry = 0;
    int map = 1710;
    int instance = 4;
    int phase = 1;
    bool inWorld = true;
    bool deleted = false;
    bool alive = true;
    bool inRange = true;
    ObjectGuid guid;
    Creature* creature = nullptr;
    Player* owner = nullptr;

    uint32 GetEntry() const { return entry; }
    int GetMapId() const { return map; }
    int GetInstanceId() const { return instance; }
    bool IsInWorld() const { return inWorld; }
    bool IsPreDelete() const { return deleted; }
    bool IsAlive() const { return alive; }
    bool IsInMap(Unit const* unit) const { return map == unit->map && instance == unit->instance; }
    bool InSamePhase(Unit const* unit) const { return phase == unit->phase; }
    bool IsWithinDistInMap(Unit const* unit, float) const { return IsInMap(unit) && unit->inRange; }
    Creature* ToCreature() const { return creature; }
    Player* GetCharmerOrOwnerPlayerOrPlayerItself() const { return owner; }
    Player* ToPlayer() const { return player; }
    ObjectGuid GetGUID() const { return guid; }
    Player* player = nullptr;
};

using WorldObject = Unit;
enum AuraEffectHandleModes { AURA_EFFECT_HANDLE_REAL };
enum RemoveMode { AURA_REMOVE_BY_EXPIRE, AURA_REMOVE_BY_DEATH, AURA_REMOVE_BY_CANCEL, AURA_REMOVE_BY_ENEMY_SPELL };
enum { EFFECT_0, SPELL_AURA_DUMMY, TARGET_UNIT_SRC_AREA_ALLY, SPELLVALUE_BASE_POINT0 };
struct AuraEffect {};

struct Aura
{
    Unit* caster;
    Unit* GetCaster() const { return caster; }
};

struct Creature : Unit
{
    int spawn = 0;
    unsigned health = 1;
    Aura* draw = nullptr;
    bool corrupted = false;
    InstanceMap* world = nullptr;
    InstanceScript* arena = nullptr;
    SmartAI* ai = nullptr;
    int32 damageSnapshot = 0;
    unsigned explosions = 0;

    Creature() { creature = this; }
    bool IsFullHealth() const { return health == 100; }
    int GetDBTableGUIDLow() const { return spawn; }
    Aura* GetAura(uint32 aura) const { return aura == 237846 ? draw : nullptr; }
    bool HasAura(uint32 aura, ObjectGuid caster) const { return corrupted && aura == 237847 && caster == guid; }
    InstanceMap* GetMap() const { return world; }
    InstanceScript* GetInstanceScript() const { return arena; }
    SmartAI* AI() const { return ai; }
    void CastCustomSpell(uint32 id, int slot, int32 damage, Unit* target, bool triggered, void*, AuraEffect const* effect)
    {
        assert(id == 237189 && slot == SPELLVALUE_BASE_POINT0 && target->ToPlayer() && triggered && effect);
        damageSnapshot = damage;
        ++explosions;
    }
};

struct Group
{
    int guid = 9;
    bool isLFGGroup() const { return true; }
    int GetMembersCount() const { return 1; }
    int GetGUID() const { return guid; }
};

struct Player : Unit
{
    Group group;
    bool pending = false;
    uint64 health = 1;
    Player() { player = this; owner = this; }
    Group const* GetGroup() const { return &group; }
    bool IsBeingTeleportedFar() const { return pending; }
    uint64 GetHealth() const { return health; }
};

struct Scenario
{
    int step = 4;
    int GetScenarioId() const { return 1281; }
    bool IsCompleted(bool) const { return step > 6; }
    int GetCurrentStep() const { return step; }
};

struct ScenarioManager
{
    Scenario scenario;
    Scenario* GetScenario(int instance) { return instance == 4 ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;

struct LfgManager
{
    int GetDungeon(int group) const { return group == 9 ? 1471 : 0; }
} lfgManager;
auto* sLFGMgr = &lfgManager;

struct SmartAI
{
    struct Script
    {
        bool announced = false;
        bool HasRunEvent(unsigned id, int event) const { return id == 31 && event == SMART_EVENT_UPDATE && announced; }
    } script;

    explicit SmartAI(Creature* creature) : me(creature) {}
    virtual ~SmartAI() = default;
    virtual void Reset() {}
    virtual void OnHealApplied(Unit*, uint64, uint64) {}
    virtual void DamageTaken(Unit*, uint32&, DamageEffectType) {}
    virtual void SpellHit(Unit*, SpellInfo const*) {}
    Script* GetScript() { return &script; }
    Creature* me;
};

struct PlayerRef
{
    Player* player;
    Player* getSource() const { return player; }
};

struct PlayerList : std::vector<PlayerRef>
{
    bool isEmpty() const { return empty(); }
};

struct InstanceMap
{
    std::map<ObjectGuid, Creature*> creatures;
    PlayerList players;
    uint32 GetInstanceId() const { return 4; }
    PlayerList const& GetPlayers() const { return players; }
    Creature* GetCreature(ObjectGuid guid) const
    {
        auto found = creatures.find(guid);
        return found == creatures.end() ? nullptr : found->second;
    }
};

struct InstanceScript
{
    explicit InstanceScript(InstanceMap* map) : instance(map) {}
    virtual ~InstanceScript() = default;
    virtual void OnCreatureCreate(Creature*) {}
    virtual void OnCreatureRemove(Creature*) {}
    virtual void OnUnitDeath(Unit*) {}
    virtual void Update(uint32) {}
    virtual uint32 GetData(uint32) const { return 0; }
    InstanceMap* instance;
};

struct InstanceMapScript
{
    InstanceMapScript(char const*, uint32) {}
    virtual ~InstanceMapScript() = default;
    virtual InstanceScript* GetInstanceScript(InstanceMap*) const = 0;
};

unsigned warnings = 0;
void TC_LOG_WARN(char const*, char const*, uint32) { ++warnings; }

struct Hook
{
    void operator+=(int) {}
};

struct AuraScript
{
    struct Application
    {
        RemoveMode mode = AURA_REMOVE_BY_EXPIRE;
        RemoveMode GetRemoveMode() const { return mode; }
    } application;
    Unit* auraCaster = nullptr;
    Unit* target = nullptr;
    Hook AfterEffectRemove;
    virtual ~AuraScript() = default;
    virtual bool Validate(SpellInfo const*) = 0;
    virtual void Register() = 0;
    bool ValidateSpellInfo(std::initializer_list<uint32> spells) { return spells.size() == 1 && *spells.begin() == 237189; }
    Application const* GetTargetApplication() const { return &application; }
    Unit* GetCaster() const { return auraCaster; }
    Unit* GetTarget() const { return target; }
};

struct SpellScript
{
    struct SpellValue { float EffectBasePoints[1]; } values{};
    Unit* caster = nullptr;
    Unit* hit = nullptr;
    int32 hitDamage = 0;
    Hook OnObjectAreaTargetSelect;
    Hook OnEffectHitTarget;
    Hook OnHit;
    virtual ~SpellScript() = default;
    virtual void Register() = 0;
    Unit* GetCaster() const { return caster; }
    Unit* GetHitUnit() const { return hit; }
    SpellValue const* GetSpellValue() const { return &values; }
    int32 GetHitDamage() const { return hitDamage; }
    void SetHitDamage(int32 damage) { hitDamage = damage; }
};

#define PrepareAuraScript(name) public:
#define PrepareSpellScript(name) public:
#define AuraEffectRemoveFn(...) 0
#define SpellObjectAreaTargetSelectFn(...) 0
#define SpellEffectFn(...) 0
#define SpellHitFn(...) 0

#include "RisenThreatHeals.inc"

int main()
{
    Creature erdris;
    erdris.entry = 118488;
    erdris.spawn = 14534095;
    Aura attraction{ &erdris };
    Creature soul;
    soul.entry = 119476;
    soul.draw = &attraction;
    Player healer;
    Unit healingTotem;
    healingTotem.owner = &healer;
    RisenThreatSoulAI ai(&soul);

    for (uint32 entry = 119476; entry <= 119480; ++entry)
    {
        soul.entry = entry;
        ai.Reset();
        soul.health = 99;
        ai.OnHealApplied(&healer, 80, 80);
        assert(!ai.WasFullyHealed());
        soul.health = 100;
        ai.OnHealApplied(&healer, 80, 0);
        assert(!ai.WasFullyHealed());
        ai.OnHealApplied(&erdris, 1, 1);
        assert(!ai.WasFullyHealed());

        healer.phase = 2;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(!ai.WasFullyHealed());
        healer.phase = 1;
        healer.pending = true;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(!ai.WasFullyHealed());
        healer.pending = false;
        ai.OnHealApplied(&healingTotem, 8, 1);
        assert(ai.WasFullyHealed());

        erdris.spawn = 6;
        assert(!ai.WasFullyHealed());
        erdris.spawn = 14534095;
        soul.draw = nullptr;
        assert(!ai.WasFullyHealed());
        soul.draw = &attraction;

        uint32 damage = 1;
        ai.DamageTaken(&erdris, damage, 0);
        assert(!ai.WasFullyHealed());
    }

    soul.entry = 119470;
    soul.draw = nullptr;
    ai.OnHealApplied(&healer, 10, 10);
    assert(!ai.WasFullyHealed());
    scenarioManager.scenario.step = 2;
    ai.OnHealApplied(&healer, 10, 10);
    assert(ai.WasFullyHealed());
    scenarioManager.scenario.step = 3;
    assert(!ai.WasFullyHealed());
    std::cout << "PASS actual full heals, controlled healers, all five wave actors, staircase rescue and stale state guards\n";

    InstanceMap map;
    map.players.push_back({ &healer });
    instance_mage_tower_risen_threat::RisenThreatInstance arena(&map);
    Creature jarod;
    Creature granny;
    Creature callie;
    jarod.entry = 118447;
    jarod.spawn = 410012;
    jarod.guid = { 1 };
    granny.entry = 118448;
    granny.spawn = 410010;
    granny.guid = { 2 };
    callie.entry = 118451;
    callie.spawn = 410013;
    callie.guid = { 3 };
    erdris.guid = { 4 };
    erdris.world = &map;
    erdris.arena = &arena;
    SmartAI bossAI(&erdris);
    erdris.ai = &bossAI;
    for (Creature* actor : { &jarod, &granny, &callie, &erdris })
    {
        map.creatures.emplace(actor->guid, actor);
        arena.OnCreatureCreate(actor);
    }

    scenarioManager.scenario.step = 5;
    assert(arena.GetData(1) == 0);
    scenarioManager.scenario.step = 4;
    soul.entry = 119476;
    soul.arena = &arena;
    soul.world = &map;
    soul.draw = &attraction;
    ai.OnHealApplied(&healer, 10, 10);
    assert(ai.WasFullyHealed());

    Creature transformed;
    transformed.entry = 119476;
    transformed.guid = { 5 };
    transformed.world = &map;
    transformed.arena = &arena;
    transformed.corrupted = true;
    RisenThreatSoulAI transformedAI(&transformed);
    SpellInfo corruption{ 237847 };
    transformedAI.SpellHit(&erdris, &corruption);

    scenarioManager.scenario.step = 5;
    assert(arena.GetData(1) == 0);
    bossAI.script.announced = true;
    assert(arena.GetData(1) == 1);
    transformedAI.SpellHit(&transformed, &corruption);
    transformedAI.SpellHit(&transformed, &corruption);
    assert(arena.GetData(1) == 0);
    Creature second;
    second.entry = 119477;
    second.guid = { 6 };
    second.world = &map;
    second.arena = &arena;
    second.corrupted = true;
    RisenThreatSoulAI secondAI(&second);
    secondAI.SpellHit(&second, &corruption);
    arena.OnCreatureRemove(&transformed);
    assert(warnings == 1 && arena.GetData(1) == 0);
    arena.OnUnitDeath(&transformed);
    assert(arena.GetData(1) == 0);
    arena.OnUnitDeath(&second);
    healer.inRange = false;
    assert(arena.GetData(1) == 0);
    healer.inRange = true;
    assert(arena.GetData(1) == 1);

    scenarioManager.scenario.step = 6;
    assert(arena.GetData(2) == 1);

    spell_mage_tower_erdris_ignite_soul ignite;
    AuraEffect effect;
    ignite.auraCaster = &erdris;
    ignite.target = &healer;
    healer.health = 1234567;
    for (RemoveMode mode : { AURA_REMOVE_BY_CANCEL, AURA_REMOVE_BY_DEATH, AURA_REMOVE_BY_ENEMY_SPELL })
    {
        ignite.application.mode = mode;
        ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    }
    assert(!erdris.explosions);
    ignite.application.mode = AURA_REMOVE_BY_EXPIRE;
    ignite.auraCaster = &jarod;
    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    ignite.auraCaster = &erdris;
    ignite.target = &healingTotem;
    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    ignite.target = &healer;
    healer.pending = true;
    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    healer.pending = false;
    erdris.alive = false;
    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    erdris.alive = true;
    assert(!erdris.explosions);

    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    assert(erdris.explosions == 1 && erdris.damageSnapshot == 1234567);
    healer.health = 765432;
    spell_mage_tower_erdris_ignite_soul_damage explosion;
    explosion.caster = &erdris;
    explosion.values.EffectBasePoints[0] = float(erdris.damageSnapshot);
    std::list<WorldObject*> targets{ &transformed, &healer, &healingTotem };
    explosion.SelectAllies(targets);
    assert((targets == std::list<WorldObject*>{ &jarod, &granny, &callie }));
    explosion.hit = &jarod;
    explosion.ApplyDamage();
    assert(explosion.hitDamage == 0);
    explosion.ConfirmHit(EFFECT_0);
    explosion.ApplyDamage();
    assert(explosion.hitDamage == 0);
    explosion.ConfirmHit(EFFECT_0);
    explosion.hitDamage = 100;
    explosion.ApplyDamage();
    assert(explosion.hitDamage == 1234567);
    explosion.hit = &granny;
    explosion.hitDamage = 100;
    explosion.ApplyDamage();
    assert(explosion.hitDamage == 100);
    explosion.hit = &erdris;
    explosion.ConfirmHit(EFFECT_0);
    explosion.ApplyDamage();
    assert(explosion.hitDamage == 100);
    callie.phase = 2;
    explosion.SelectAllies(targets);
    assert((targets == std::list<WorldObject*>{ &jarod, &granny }));
    callie.phase = 1;
    assert(GetIgniteSoulDamage(0) == 0);
    assert(GetIgniteSoulDamage(1e12) == std::numeric_limits<int32>::max());

    arena.OnUnitDeath(&callie);
    assert(arena.GetData(2) == 0);
    ignite.Explode(&effect, AURA_EFFECT_HANDLE_REAL);
    assert(erdris.explosions == 1);
    scenarioManager.scenario.step = 0;
    arena.Update(1);
    scenarioManager.scenario.step = 5;
    assert(arena.GetData(1) == 0);
    std::cout << "PASS real corruption roster, death-only completion, ally loss, range, restart and final boss gate\n";
    std::cout << "PASS Ignite Soul expiry, exact challenger health snapshot, same-phase trio and stale aura cleanup\n";
}
