#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>
using uint32 = uint32_t;
using uint8 = uint8_t;
#include "CriteriaTypes.inc"
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, TYPEID_PLAYER = 4,
    GO_ACTIVATED = 2 };
struct Map { } liveMap;
struct ScenarioStep { bool IsBonusObjective() const { return false; } } nativeSteps[6];
struct Scenario
{
    uint32 identity = 909, currentStep = 6;
    std::vector<ScenarioStep*> steps;
    Scenario() { for (auto& step : nativeSteps) steps.push_back(&step); }
    uint32 GetScenarioId() const { return identity; }
    bool IsCompleted(bool) const;
    uint8 GetStepCount(bool) const;
};
#include "Completed.inc"
#include "Count.inc"
struct ScenarioManager
{
    Scenario scenario;
    bool available = true;
    Scenario* GetScenario(uint32 instanceId) { return available && (instanceId == 42 || instanceId == 43) ? &scenario : nullptr; }
} manager;
auto* sScenarioMgr = &manager;
struct Player;
struct Unit
{
    virtual ~Unit() = default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual uint32 GetTypeId() const { return 3; }
    std::vector<std::function<void()>> pending;
    uint32 scenes = 0, jumps = 0;
    bool castWorks = true;
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) { assert(delay == 12000); pending.push_back(std::move(callback)); }
    uint32 CastSpell(Unit* target, uint32 spell, bool triggered = false)
    {
        assert(target == this);
        if (spell == 205340) { assert(triggered); ++scenes; }
        else { assert(spell == 192085 && !triggered); ++jumps; }
        return castWorks ? 0 : 1;
    }
    void Flush() { auto events = std::move(pending); pending.clear(); for (auto& event : events) event(); }
};
struct Player : Unit
{
    bool world = true, contact = true, alive = true, teleporting = false, rewarded = false, item = false, storeWorks = true;
    uint32 klass = CLASS_WARRIOR, status = QUEST_STATUS_INCOMPLETE, mapId = 1495, instanceId = 42;
    uint32 pickupCredits = 0, arrivalCredits = 0, scriptEvents = 0, scenarioEvents = 0, storeRequests = 0;
    Map* map = &liveMap;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() const override { return TYPEID_PLAYER; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 getClass() const { return klass; }
    Map* GetMap() const { return map; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest == 39191); return status; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest == 39191); return rewarded; }
    bool HasItemCount(uint32 identity, uint32 count, bool = false) const { assert(identity == 128289 && count == 1); return item; }
    void AddItem(uint32 identity, uint32 count) { assert(identity == 128289 && count == 1); ++storeRequests; item = storeWorks; }
    void UpdateAchievementCriteria(uint32 criterion, uint32 asset, uint32 amount = 0)
    {
        if (criterion == CRITERIA_TYPE_SCRIPT_EVENT_2) { assert(asset == 45785 && !amount); ++scriptEvents; }
        else { assert(criterion == CRITERIA_TYPE_COMPLETE_SCENARIO && asset == 909 && amount == 1); ++scenarioEvents; }
    }
    void KilledMonsterCredit(uint32 identity) { if (identity == 94851) ++pickupCredits; else { assert(identity == 103739); ++arrivalCredits; } }
};
struct Creature { uint32 despawns = 0; void DespawnOrUnsummon(uint32 delay) { assert(delay == 1000); ++despawns; } } pickupTrigger;
struct GameObject
{
    bool actor = true;
    uint32 GetInstanceId() const { return 42; }
    Creature* FindNearestCreature(uint32 identity, float radius) { assert(identity == 94854 && radius == 10); return actor ? &pickupTrigger : nullptr; }
};
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { }
    virtual ~GameObjectScript() = default;
    virtual void OnLootStateChanged(GameObject*, uint32, Unit*) { }
};
#include "Policy.inc"
#include "Callback.inc"
void Begin(Player& player, bool missingActor = false)
{
    GameObject weapon; weapon.actor = !missingActor; go_art_earthwarder script;
    script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
    assert(player.pending.size() == 1 && player.scenes == 1 && player.pickupCredits == 1 && player.scriptEvents == 1 && player.scenarioEvents == 1);
}
int main()
{
    for (uint32 invalid = 0; invalid < 18; ++invalid)
    {
        manager = {}; Player player; Begin(player);
        switch (invalid)
        {
            case 0: player.world = false; break;
            case 1: player.contact = false; break;
            case 2: player.alive = false; break;
            case 3: player.teleporting = true; break;
            case 4: player.klass = 3; break;
            case 5: player.map = nullptr; break;
            case 6: player.mapId = 1220; break;
            case 7: player.instanceId = 43; break;
            case 8: player.rewarded = true; break;
            case 9: player.status = 0; break;
            case 10: player.status = 5; break;
            case 11: player.item = false; break;
            case 12: manager.available = false; break;
            case 13: manager.scenario.identity = 944; break;
            case 14: manager.scenario.currentStep = 5; break;
            case 15: manager.scenario.currentStep = 7; break;
            case 16: manager.scenario.steps.clear(); break;
            case 17: player.mapId = 1479; break;
        }
        player.Flush();
#ifdef BASELINE
        assert(player.jumps == 1 && player.arrivalCredits == 1);
#else
        assert(!player.jumps && !player.arrivalCredits);
#endif
    }
    for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)})
        for (bool accepted : {false, true}) for (bool owned : {false, true}) for (bool missingActor : {false, true})
    {
        manager = {}; Player player; player.status = status; player.castWorks = accepted; player.item = owned;
        Begin(player, missingActor); player.Flush(); player.Flush();
        assert(player.jumps == 1 && player.storeRequests == uint32(!owned));
#ifdef BASELINE
        assert(player.arrivalCredits == 1);
#else
        assert(!player.arrivalCredits);
#endif
    }
    {
        manager = {}; Player player; player.storeWorks = false; Begin(player); assert(!player.item); player.Flush();
#ifdef BASELINE
        assert(player.jumps == 1 && player.arrivalCredits == 1);
#else
        assert(!player.jumps && !player.arrivalCredits);
#endif
        assert(player.pickupCredits == 1);
    }
    {
        GameObject weapon; go_art_earthwarder script; Player player; Unit pet;
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, nullptr);
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &pet);
        script.OnLootStateChanged(&weapon, 3, &player);
        assert(player.pending.empty() && pet.pending.empty() && !player.pickupCredits);
    }
#ifdef BASELINE
    std::cout << "BASELINE: pinnedb5c32cab attempts jump and grants103739 for18 stale contexts, failed casts and failed storage; not a real arrival.\n";
#else
    std::cout << "PASS: actual full Earthwarder callback/native Scenario completion reject18 stale returns and failed storage;16 live active-status/cast-result/owned/missing-actor paths retain only one native jump request and no103739. No GO lifetime capture. Existing activation/direct AddItem/early94851 remain reproduced and UNFIXED; no actual client/loot/equip/arrival proof.\n";
#endif
}
