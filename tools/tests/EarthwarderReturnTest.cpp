#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
using uint16=std::uint16_t;
using uint32 = uint32_t;
using uint8 = uint8_t;
using LootState = uint32;
#include "CriteriaTypes.inc"
enum { CLASS_WARRIOR = 1, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1, TYPEID_PLAYER = 4,
    GO_ACTIVATED = 2, GO_JUST_DEACTIVATED = 3, GAMEOBJECT_FIELD_FLAGS = 17, GO_FLAG_NOT_SELECTABLE = 16,
    NULL_BAG = 255, NULL_SLOT = 255, GAMEOBJECT_TYPE_CHEST = 3, GAMEOBJECT_TYPE_FISHINGNODE = 17 };
struct ItemPosCountVec:std::vector<uint32>{void emplace_back(std::uint16_t,uint32){assert(false);}};
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_INVENTORY_FULL };
constexpr uint32 INVENTORY_SLOT_BAG_0=255,EQUIPMENT_SLOT_MAINHAND=15;
constexpr InventoryResult EQUIP_ERR_INV_FULL=EQUIP_ERR_INVENTORY_FULL;

struct Item { };
struct LootItem
{
    struct
    {
        uint32 ItemID = 128289, RandomPropertiesID = 0;
        struct { std::vector<uint32> BonusListIDs; uint32 Context = 0; } ItemBonus;
    } item;
    uint32 count = 1;
    bool freeforall = false, is_looted = false;
    std::set<uint32> GetAllowedLooters() const { return {}; }
};
struct Loot
{uint32 objGuid=0;
    LootItem item;
    uint32 gold = 0, unlootedCount = 1;
#include "NativeIsLooted.inc"
};
struct Map { } liveMap, otherMap;
struct ScenarioStep { bool IsBonusObjective() const { return false; } } nativeSteps[6];
struct Scenario
{
    uint32 identity = 909, currentStep = 6;
    std::vector<ScenarioStep*> steps;
    Scenario() { for (auto& step : nativeSteps) steps.push_back(&step); }
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return currentStep; }
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
struct GameObject;
struct GameObjectAI;
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
struct GameObject;struct Unit;
struct ScriptMgr
{ template<class... Args> bool CanAutoEquipLoot(Args&&...){return false;}
    void OnGameObjectLootStateChanged(GameObject* object, uint32 state, Unit* player);
} scriptManager;
auto* sScriptMgr = &scriptManager;
struct Player : Unit
{
    bool world = true, contact = true, alive = true, teleporting = false, rewarded = false, ownsWeapon = false, storeWorks = true;
    bool phase = true, visible = true, los = true;
    float distance = 1;
    uint32 klass = CLASS_WARRIOR, status = QUEST_STATUS_INCOMPLETE, mapId = 1495, instanceId = 42;
    uint32 pickupCredits = 0, arrivalCredits = 0, scriptEvents = 0, scenarioEvents = 0, storeRequests = 0;
    uint32 inventoryErrors = 0;
    bool room = true;
    Item storedItem;
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
    bool InSamePhase(GameObject*) const { return phase; }
    bool canSeeOrDetect(GameObject*) const { return visible; }
    bool IsWithinLOSInMap(GameObject*) const { return los; }
    bool IsWithinDistInMap(GameObject*, float radius) const { return distance <= radius; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest == 39191); return status; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest == 39191); return rewarded; }
    bool HasItemCount(uint32 identity, uint32 count, bool = false) const { assert(identity == 128289 && count == 1); return ownsWeapon; }
    void AddItem(uint32 identity, uint32 count) { assert(identity == 128289 && count == 1); ++storeRequests; ownsWeapon = room && storeWorks; }
    InventoryResult CanStoreNewItem(uint32, uint32, ItemPosCountVec& destination, uint32 identity, uint32 count, void*, bool quest)
    {
        assert(identity == 128289 && count == 1 && !quest);
        if (!room) return EQUIP_ERR_INVENTORY_FULL;
        destination.push_back(1);
        return EQUIP_ERR_OK;
    }
    Item* StoreNewItem(ItemPosCountVec const&, uint32 identity, bool, uint32, std::set<uint32> const&, std::vector<uint32> const&, uint32,bool refund=false,bool autoEquip=false) {assert(!refund&&!autoEquip);
        assert(identity == 128289);
        if (!storeWorks) return nullptr;
        ownsWeapon = true;
        return &storedItem;
    }
    void SendEquipError(InventoryResult result, void*, void*, uint32 identity) { assert(result == EQUIP_ERR_INVENTORY_FULL && identity == 128289); ++inventoryErrors; }
    void StoreLootItem(Loot* loot)
    {
        LootItem* item = &loot->item;
        void* qitem = nullptr;
#include "NativeStoreInventory.inc"
#include "NativeConsumeLoot.inc"
    }
    void UpdateAchievementCriteria(uint32 criterion, uint32 asset, uint32 amount = 0)
    {
        if (criterion == CRITERIA_TYPE_SCRIPT_EVENT_2) { assert(asset == 45785 && !amount); ++scriptEvents; manager.scenario.currentStep = 6; }
        else { assert(criterion == CRITERIA_TYPE_COMPLETE_SCENARIO && asset == 909 && amount == 1); ++scenarioEvents; }
    }
    void KilledMonsterCredit(uint32 identity) { if (identity == 94851) ++pickupCredits; else { assert(identity == 103739); ++arrivalCredits; } }
};
struct Creature { uint32 despawns = 0; void DespawnOrUnsummon(uint32 delay) { assert(delay == 1000); ++despawns; } } pickupTrigger;
struct GameObject
{
    bool actor = true, world = true, spawned = true, notSelectable = false;
    uint32 entry = 248831, mapId = 1495, instanceId = 42, m_lootState = GO_ACTIVATED;
    Map* map = &liveMap;
    bool IsInWorld() const { return world; }
    bool isSpawned() const { return spawned; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
    Map* GetMap() const { return map; }
    float GetInteractionDistance() const { return 5; }
    uint32 getLootState() const { return m_lootState; }
    void SetLootState(uint32 state, Unit*);
    GameObjectAI* AI() const;
    uint32 GetGoType() const { return GAMEOBJECT_TYPE_CHEST; }
    bool HasFlag(uint32 field, uint32 flag) const { assert(field == GAMEOBJECT_FIELD_FLAGS && flag == GO_FLAG_NOT_SELECTABLE); return notSelectable; }
    Creature* FindNearestCreature(uint32 identity, float radius) { assert(identity == 94854 && radius == 10); return actor ? &pickupTrigger : nullptr; }
};
struct GameObjectAI
{
    explicit GameObjectAI(GameObject* object) : go(object) { }
    virtual ~GameObjectAI() = default;
    virtual bool GossipHello(Player*, bool) { return false; }
    virtual void OnStateChanged(uint32, Unit*) { }
protected:
    GameObject* go;
};
struct GameObjectScript;
GameObjectScript* currentScript = nullptr;
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { currentScript = this; }
    virtual ~GameObjectScript() = default;
    virtual bool OnGossipHello(Player*, GameObject*) { return false; }
    virtual GameObjectAI* GetAI(GameObject* object) const { return new GameObjectAI(object); }
    virtual void OnLootStateChanged(GameObject*, uint32, Unit*) { }
};
#include "Policy.inc"
#include "PickupPolicy.inc"
#include "Callback.inc"
void ScriptMgr::OnGameObjectLootStateChanged(GameObject* object, uint32 state, Unit* player){ assert(currentScript); currentScript->OnLootStateChanged(object, state, player); }
GameObjectAI* GameObject::AI() const { static GameObjectAI defaultAI(nullptr); return &defaultAI; }
#include "NativeSetLootState.inc"
bool CompletedLoot(Loot* loot, GameObject* go)
{
#include "NativeCompleteLoot.inc"
}
void Begin(Player& player, bool missingActor = false)
{
    GameObject weapon; weapon.actor = !missingActor; go_art_earthwarder script;
    manager.scenario.currentStep = 5;
#ifdef BASELINE
    script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
#else
    player.ownsWeapon = true;
    weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
#endif
    assert(player.pending.size() == 1 && player.scenes == 1 && player.pickupCredits == 1 && player.scriptEvents == 1 && player.scenarioEvents == 1);
}
#if !defined(PICKUP_ADMISSION) && !defined(REAL_LOOT)
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
            case 11: player.ownsWeapon = false; break;
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
        manager = {}; Player player; player.status = status; player.castWorks = accepted; player.ownsWeapon = owned;
        Begin(player, missingActor); player.Flush(); player.Flush();
        assert(player.jumps == 1);
#ifdef BASELINE
        assert(player.storeRequests == uint32(!owned));
        assert(player.arrivalCredits == 1);
#else
        assert(!player.storeRequests);
        assert(!player.arrivalCredits);
#endif
    }
    {
        manager = {}; Player player; player.storeWorks = false;
#ifdef BASELINE
        Begin(player); assert(!player.ownsWeapon); player.Flush();
        assert(player.jumps == 1 && player.arrivalCredits == 1);
        assert(player.pickupCredits == 1);
#else
        GameObject weapon; go_art_earthwarder script; manager.scenario.currentStep = 5;
        weapon.SetLootState(GO_ACTIVATED, &player); weapon.SetLootState(GO_JUST_DEACTIVATED, &player); player.Flush();
        assert(!player.jumps && !player.arrivalCredits);
        assert(!player.pickupCredits && !player.storeRequests);
#endif
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
    std::cout << "PASS: actual full Earthwarder callback/native Scenario completion reject18 stale returns and unowned completed loot;16 live active-status/cast-result/owned/missing-actor paths retain one native jump request and no103739. No GO lifetime capture or script grant; client/equip/arrival remain unverified.\n";
#endif
}
#elif defined(PICKUP_ADMISSION)
int main()
{
    for (uint32 invalid = 0; invalid < 28; ++invalid)
    {
        manager = {}; manager.scenario.currentStep = 5; Player player; GameObject weapon; go_art_earthwarder script;
        switch (invalid)
        {
            case 0: player.world = false; break;
            case 1: player.contact = false; break;
            case 2: player.alive = false; break;
            case 3: player.teleporting = true; break;
            case 4: player.klass = 3; break;
            case 5: weapon.world = false; break;
            case 6: weapon.spawned = false; break;
            case 7: weapon.entry = 248832; break;
            case 8: weapon.mapId = 1511; break;
            case 9: player.map = &otherMap; break;
            case 10: player.instanceId = 43; break;
            case 11: player.phase = false; break;
            case 12: player.visible = false; break;
            case 13: player.los = false; break;
            case 14: player.distance = 5.01f; break;
            case 15: weapon.notSelectable = true; break;
            case 16: weapon.m_lootState = GO_JUST_DEACTIVATED; break;
            case 17: player.rewarded = true; break;
            case 18: player.status = 0; break;
            case 19: player.status = 5; break;
            case 20: manager.available = false; break;
            case 21: manager.scenario.identity = 1068; break;
            case 22: manager.scenario.currentStep = 0; break;
            case 23: manager.scenario.currentStep = 4; break;
            case 24: manager.scenario.currentStep = 6; break;
            case 25: manager.scenario.currentStep = 7; break;
            case 26: weapon.m_lootState = 1; break;
            case 27: player.map = nullptr; weapon.map = nullptr; break;
        }
        bool rejected = script.OnGossipHello(&player, &weapon);
        auto* ai = script.GetAI(&weapon);
        bool report = ai->GossipHello(&player, false), opening = ai->GossipHello(&player, true);
        delete ai;
#ifdef BASELINE
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
        assert(!rejected && !report && !opening && player.storeRequests == 1 && player.pickupCredits == 1 && player.pending.size() == 1);
#else
        player.ownsWeapon = invalid != 16;
        weapon.m_lootState = invalid == 26 ? 1 : GO_JUST_DEACTIVATED;
        script.OnLootStateChanged(&weapon, GO_JUST_DEACTIVATED, &player);
        assert(rejected == (invalid != 26) && report == rejected && opening == rejected);
        assert(!player.storeRequests && !player.pickupCredits && !player.scriptEvents && !player.scenes && player.pending.empty());
#endif
    }
    for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)})
        for (bool owned : {false, true}) for (bool missingActor : {false, true})
    {
        manager = {}; manager.scenario.currentStep = 5; Player player; player.status = status; player.ownsWeapon = owned; player.distance = 5;
        GameObject weapon; weapon.actor = !missingActor; go_art_earthwarder script;
#ifdef BASELINE
        assert(!script.OnGossipHello(&player, &weapon)); auto* ai = script.GetAI(&weapon);
        assert(!ai->GossipHello(&player, false) && !ai->GossipHello(&player, true)); delete ai;
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
        assert(player.pickupCredits == 1 && player.scenes == 1 && player.scriptEvents == 1 && player.storeRequests == uint32(!owned));
#else
        assert(script.OnGossipHello(&player, &weapon) == owned);
        if (!owned) { player.ownsWeapon = true; weapon.SetLootState(GO_JUST_DEACTIVATED, &player); }
        assert(player.pickupCredits == 1 && player.scenes == 1 && player.scriptEvents == 1 && !player.storeRequests);
#endif
    }
    {
        manager = {}; manager.scenario.currentStep = 5; Player player; GameObject weapon; go_art_earthwarder script; Unit pet;
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, nullptr); script.OnLootStateChanged(&weapon, GO_ACTIVATED, &pet); script.OnLootStateChanged(&weapon, 3, &player);
        assert(!player.pickupCredits && pet.pending.empty());
#ifndef BASELINE
        script.OnLootStateChanged(nullptr, GO_ACTIVATED, &player); assert(script.OnGossipHello(nullptr, &weapon) && script.OnGossipHello(&player, nullptr));
        assert(!script.OnGossipHello(&player, &weapon)); player.status = 0; player.ownsWeapon = true;
        weapon.SetLootState(GO_JUST_DEACTIVATED, &player); assert(!player.pickupCredits);
        player.status = QUEST_STATUS_INCOMPLETE; weapon.SetLootState(GO_JUST_DEACTIVATED, &player); assert(player.pickupCredits == 1);
        weapon.SetLootState(GO_JUST_DEACTIVATED, &player); assert(player.pickupCredits == 1);
#endif
    }
#ifdef BASELINE
    std::cout << "BASELINE: pinned3a38903e admits27 invalid opening/report contexts and28 invalid direct activations, granting legacy pickup/progression. Not live loot.\n";
#else
    std::cout << "PASS: actual Earthwarder script/factory gates27 invalid openings and28 invalid/unowned completed-loot callbacks; eight active/owned/missing-trigger paths and native distance boundary retained. Revoked context, retry, null/pet and modeled completed-stage duplicate checked; no script grant. Phase rendering, actual scene/equip and client traversal remain unverified.\n";
#endif
}
#else
int main()
{
    for (uint32 failure : {0u, 1u})
    {
        manager = {}; manager.scenario.currentStep = 5; Player player; GameObject weapon; go_art_earthwarder script; Loot loot;
        player.room = failure != 0; player.storeWorks = failure != 1;
        assert(!script.OnGossipHello(&player, &weapon));
        weapon.SetLootState(GO_ACTIVATED, &player);
#ifdef BASELINE
        assert(!player.ownsWeapon && player.storeRequests == 1);
        assert(player.pickupCredits == 1 && player.scriptEvents == 1 && player.scenes == 1 && player.pending.size() == 1);
#else
        player.StoreLootItem(&loot);
        assert(!loot.isLooted() && !loot.item.is_looted && loot.unlootedCount == 1 && !player.ownsWeapon);
        assert(!CompletedLoot(&loot, &weapon));
        assert(player.inventoryErrors == uint32(failure == 0));
        weapon.SetLootState(GO_ACTIVATED, &player);
        assert(!player.pickupCredits && !player.scriptEvents && !player.scenes && player.pending.empty() && !player.storeRequests);
        player.room = player.storeWorks = true;
        player.StoreLootItem(&loot);
        assert(loot.isLooted() && loot.item.is_looted && !loot.unlootedCount && player.ownsWeapon && CompletedLoot(&loot, &weapon));
        weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
        assert(player.pickupCredits == 1 && player.scriptEvents == 1 && player.scenes == 1 && player.pending.size() == 1 && !player.storeRequests);
        weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
        assert(player.pickupCredits == 1);
#endif
    }
#ifdef BASELINE
    std::cout << "BASELINE: pinned61117f3c bypasses real loot and advances on activation; failed AddItem still advances without weapon. Controlled storage failure, not live traversal.\n";
#else
    std::cout << "PASS: extracted native inventory-space/store/consume/isLooted/release predicate and SetLootState dispatch keep both failure paths unconsumed and uncredited; freeing space/retrying stores128289 before one claim. Activation has no grant/progress; actual network, equip and rendered scene remain unverified.\n";
#endif
}
#endif
