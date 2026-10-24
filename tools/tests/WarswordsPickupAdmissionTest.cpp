#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
using uint32 = uint32_t;
using LootState = uint32;
using ItemPosCountVec = std::vector<uint32>;
#include "CriteriaTypes.inc"
enum { CLASS_WARRIOR = 1, TYPEID_PLAYER = 4, GO_ACTIVATED = 2, GO_JUST_DEACTIVATED = 3, QUEST_STATUS_INCOMPLETE = 3, QUEST_STATUS_COMPLETE = 1,
    NULL_BAG = 255, NULL_SLOT = 255, GAMEOBJECT_TYPE_CHEST = 3, GAMEOBJECT_TYPE_FISHINGNODE = 17 };
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_INVENTORY_FULL };
struct Item { };
struct LootItem
{
    struct
    {
        uint32 ItemID = 128908, RandomPropertiesID = 0;
        struct { std::vector<uint32> BonusListIDs; uint32 Context = 0; } ItemBonus;
    } item;
    uint32 count = 1;
    bool freeforall = false, is_looted = false;
    std::set<uint32> GetAllowedLooters() const { return {}; }
};
struct Loot
{
    LootItem item;
    uint32 gold = 0, unlootedCount = 1;
#include "NativeIsLooted.inc"
};
struct Player;
struct Unit
{
    virtual ~Unit() = default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual uint32 GetTypeId() { return 3; }
    uint32 casts = 0, conversations = 0, delays = 0;
    bool jumpWorks = true;
    uint32 jumpRequests = 0, acceptedJumps = 0;
    std::vector<std::function<void()>> pendingEvents;
    void CastSpell(Unit*, uint32 identity, bool = false) { assert(identity == 205443 || identity == 192085); ++casts; if (identity == 192085) { ++jumpRequests; if (jumpWorks) ++acceptedJumps; } }
    void CreateConversation(uint32 identity) { assert(identity == 849); ++conversations; }
    void AddDelayedEvent(uint32 delay, std::function<void()> callback) { assert(delay == 12000); ++delays; pendingEvents.push_back(std::move(callback)); }
    void FlushDelayedEvents() { auto callbacks = std::move(pendingEvents); pendingEvents.clear(); for (auto& callback : callbacks) callback(); }
};
struct Map {} instanceMap, otherMap;
struct Creature { uint32 despawns = 0; void DespawnOrUnsummon(uint32 delay) { assert(delay == 1000); ++despawns; } } soul;
struct GameObjectAI { void OnStateChanged(uint32, Unit*) { } } objectAI;
struct GameObject
{
    uint32 entry = 248832, mapId = 1511, instance = 42;
    Map* map = &instanceMap;
    float interactionRange = 5.f;
    bool soulPresent = true;
    uint32 m_lootState = GO_ACTIVATED;
    uint32 getLootState() const { return m_lootState; }
    uint32 GetGoType() const { return GAMEOBJECT_TYPE_CHEST; }
    GameObjectAI* AI() const { return &objectAI; }
    void SetLootState(uint32 state, Unit*);
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return map; }
    float GetInteractionDistance() const { return interactionRange; }
    Creature* FindNearestCreature(uint32 identity, float range) { assert(identity == 98678 && range == 10.f); return soulPresent ? &soul : nullptr; }
};
struct Player : Unit
{
    uint32 status = QUEST_STATUS_INCOMPLETE, playerClass = CLASS_WARRIOR, credits = 0, achievements = 0, items = 0;
    bool alive = true, contact = true, phase = true, rewarded = false, world = true, teleporting = false;
    uint32 mapId = 1511, instance = 42;
    Map* map = &instanceMap;
    float distance = 1.f;
    bool room = true, storeWorks = true;
    uint32 inventoryErrors = 0;
    Item storedItem;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() override { return TYPEID_PLAYER; }
    bool IsAlive() const { return alive; }
    bool IsInWorld() const { return world; }
    bool IsBeingTeleported() const { return teleporting; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    bool CanContact() const { return contact; }
    uint32 getClass() const { return playerClass; }
    Map* GetMap() const { return map; }
    bool InSamePhase(GameObject*) const { return phase; }
    bool IsWithinDistInMap(GameObject* weapon, float range) const { assert(range == weapon->GetInteractionDistance()); return distance <= range; }
    bool GetQuestRewardStatus(uint32 identity) const { assert(identity == 40043); return rewarded; }
    uint32 GetQuestStatus(uint32 identity) const { assert(identity == 40043); return status; }
    bool HasItemCount(uint32 identity, uint32 count, bool = false) { assert(identity == 128908 && count == 1); return items != 0; }
    bool AddItem(uint32 identity, uint32 count) { assert(identity == 128908 && count == 1); if (!room) return false; ++items; return true; }
    InventoryResult CanStoreNewItem(uint32, uint32, ItemPosCountVec& destination, uint32 identity, uint32 count, void*, bool quest)
    {
        assert(identity == 128908 && count == 1 && !quest);
        if (!room)
            return EQUIP_ERR_INVENTORY_FULL;
        destination.push_back(1);
        return EQUIP_ERR_OK;
    }
    Item* StoreNewItem(ItemPosCountVec const&, uint32 identity, bool, uint32, std::set<uint32> const&, std::vector<uint32> const&, uint32)
    {
        assert(identity == 128908);
        if (!storeWorks)
            return nullptr;
        ++items;
        return &storedItem;
    }
    void SendEquipError(InventoryResult result, void*, void*, uint32 identity) { assert(result == EQUIP_ERR_INVENTORY_FULL && identity == 128908); ++inventoryErrors; }
    void StoreLootItem(Loot* loot)
    {
        LootItem* item = &loot->item;
        void* qitem = nullptr;
#include "NativeStoreInventory.inc"
#include "NativeConsumeLoot.inc"
    }
    void UpdateAchievementCriteria(uint32 type, uint32 identity, uint32 = 0)
    {
        assert((type == 92 && identity == 47017) || (type == 152 && identity == 944));
        ++achievements;
    }
    void KilledMonsterCredit(uint32 identity) { assert(identity == 98033 || identity == 103739); ++credits; }
};
struct Scenario
{
    uint32 identity = 944, currentStep = 6;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return currentStep; }
    uint32 GetStepCount(bool bonus) const { assert(!bonus); return 7; }
#include "ScenarioCompleted.inc"
};
struct ScenarioMgr
{
    bool available = true, acceptOtherInstance = false;
    Scenario scenario;
    Scenario* GetScenario(uint32 identity) { return available && (identity == 42 || (identity == 43 && acceptOtherInstance)) ? &scenario : nullptr; }
} scenarioManager;
auto* sScenarioMgr = &scenarioManager;
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { }
    virtual ~GameObjectScript() = default;
    virtual bool OnGossipHello(Player*, GameObject*) { return false; }
    virtual void OnLootStateChanged(GameObject*, uint32, Unit*) { }
};
#include "PickupAdmission.inc"
#include "PickupObject.inc"
GameObjectScript* currentScript = nullptr;
struct ScriptMgr
{
    void OnGameObjectLootStateChanged(GameObject* object, uint32 state, Unit* player) { assert(currentScript); currentScript->OnLootStateChanged(object, state, player); }
} scriptManager;
auto* sScriptMgr = &scriptManager;
#include "NativeSetLootState.inc"
bool CompletedLoot(Loot* loot, GameObject* go) {
#include "NativeCompleteLoot.inc"
}
int main()
{
    go_art_warswords script;
    currentScript = &script;
    uint32 baselineBypasses = 0;
    for (uint32 invalid = 0; invalid < 18; ++invalid)
    {
        scenarioManager = {};
        soul = {};
        Player player;
        GameObject weapon;
        Unit pet;
        if (invalid == 0) player.alive = false;
        if (invalid == 1) player.contact = false;
        if (invalid == 2) player.playerClass = 3;
        if (invalid == 3) player.map = &otherMap;
        if (invalid == 4) player.phase = false;
        if (invalid == 5) player.distance = 5.01f;
        if (invalid == 6) player.rewarded = true;
        if (invalid == 7) player.status = 0;
        if (invalid == 8) player.status = 6;
        if (invalid == 9) weapon.entry = 248419;
        if (invalid == 10) weapon.mapId = 1220;
        if (invalid == 11) weapon.instance = 43;
        if (invalid == 12) scenarioManager.available = false;
        if (invalid == 13) scenarioManager.scenario.identity = 909;
        if (invalid == 14) scenarioManager.scenario.currentStep = 0;
        if (invalid == 15) scenarioManager.scenario.currentStep = 5;
        if (invalid == 16) scenarioManager.scenario.currentStep = 7;
        if (invalid == 17) player.status = 5;
#ifdef BASELINE
#ifdef LOOT_BASELINE
        assert(script.OnGossipHello(&player, &weapon));
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
        assert(!player.achievements && !player.items && !player.credits);
#else
        assert(!script.OnGossipHello(&player, &weapon));
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
        assert(player.achievements == 2 && player.items == 1 && player.credits == 1);
        ++baselineBypasses;
#endif
#else
        assert(script.OnGossipHello(&player, &weapon));
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
        assert(!player.achievements && !player.items && !player.credits && !player.casts && !player.delays && !soul.despawns);
        script.OnLootStateChanged(&weapon, GO_ACTIVATED, &pet);
        assert(!pet.casts && !pet.delays);
#endif
    }
#ifdef BASELINE
#ifndef LOOT_BASELINE
    assert(baselineBypasses == 18);
#endif
    scenarioManager = {};
    Player full;
    full.room = false;
    GameObject unopened;
    script.OnLootStateChanged(&unopened, GO_ACTIVATED, &full);
    assert(full.items == 0 && full.achievements == 2 && full.credits == 1 && full.delays == 1);
#ifdef LOOT_BASELINE
    std::cout << "BASELINE: pinned c3728f9e retains admission but grants progression/scene/return with zero items after full-bag AddItem failure. Controlled storage result, not live character traversal.\n";
#else
    std::cout << "BASELINE:18 invalid/stale direct interactions bypass admission; full-bag AddItem failure also grants progression with zero items. Controlled services, not live packet exploit.\n";
#endif
#else
    for (uint32 failure : {0u, 1u})
    {
        scenarioManager = {};
        Player player;
        GameObject weapon;
        Loot loot;
        player.room = failure != 0;
        player.storeWorks = failure != 1;
        assert(!script.OnGossipHello(&player, &weapon));
        weapon.SetLootState(GO_ACTIVATED, &player);
        player.StoreLootItem(&loot);
        assert(!loot.isLooted() && !loot.item.is_looted && loot.unlootedCount == 1 && player.items == 0);
        assert(player.inventoryErrors == uint32(failure == 0));
        assert(!CompletedLoot(&loot, &weapon));
        weapon.SetLootState(GO_ACTIVATED, &player);
        assert(!player.achievements && !player.credits && !player.casts && !player.delays);
        player.room = player.storeWorks = true;
        player.StoreLootItem(&loot);
        assert(loot.isLooted() && loot.item.is_looted && loot.unlootedCount == 0 && player.items == 1);
        assert(CompletedLoot(&loot, &weapon));
        weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
        assert(player.items == 1 && player.achievements == 2 && player.credits == 1 && player.casts == 1 && player.delays == 1);
    }
    for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)})
        for (bool soulPresent : {false, true})
            for (uint32 held : {0u, 1u})
            {
                scenarioManager = {};
                Player player;
                player.status = status;
                player.items = held;
                GameObject weapon;
                weapon.soulPresent = soulPresent;
                assert(script.OnGossipHello(&player, &weapon) == bool(held));
                if (!held)
                {
                    script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
                    weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
                    assert(!player.achievements && !player.items);
                    player.items = 1;
                    weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
                }
                assert(player.achievements == 2 && player.items == 1 && player.credits == 1 && player.conversations == 1 && player.delays == 1);
                assert(player.casts == 1);
                scenarioManager.scenario.currentStep = 7;
                assert(script.OnGossipHello(&player, &weapon));
                script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
                assert(player.achievements == 2 && player.delays == 1);
            }
    scenarioManager = {};
    Player player;
    GameObject weapon;
    assert(script.OnGossipHello(nullptr, &weapon) && script.OnGossipHello(&player, nullptr));
    script.OnLootStateChanged(nullptr, GO_ACTIVATED, &player);
    script.OnLootStateChanged(&weapon, GO_ACTIVATED, nullptr);
    assert(!player.achievements && !player.items);
    assert(!script.OnGossipHello(&player, &weapon));
    player.status = 0;
    script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
    assert(!player.achievements && !player.items);
    player.status = QUEST_STATUS_INCOMPLETE;
    player.distance = weapon.interactionRange;
    assert(!script.OnGossipHello(&player, &weapon));
    script.OnLootStateChanged(&weapon, GO_ACTIVATED, &player);
    assert(!player.achievements && !player.items);
    player.items = 1;
    weapon.SetLootState(GO_JUST_DEACTIVATED, &player);
    assert(player.achievements == 2 && player.items == 1);
    for (bool jumpWorks : {false, true})
    {
        scenarioManager = {};
        Player traveler;
        traveler.items = 1;
        traveler.jumpWorks = jumpWorks;
        GameObject pickup;
        assert(script.OnGossipHello(&traveler, &pickup));
        assert(traveler.credits == 1 && traveler.casts == 1 && traveler.jumpRequests == 0);
        scenarioManager.scenario.currentStep = 7;
        traveler.FlushDelayedEvents();
        assert(traveler.jumpRequests == 1 && traveler.acceptedJumps == uint32(jumpWorks));
#ifdef RETURN_BASELINE
        assert(traveler.credits == 2);
#else
        assert(traveler.credits == 1);
#endif
        traveler.FlushDelayedEvents();
        assert(traveler.jumpRequests == 1);
    }
#if !defined(RETURN_BASELINE)
    for (uint32 invalid = 0; invalid < 18; ++invalid)
    {
        scenarioManager = {}; Player traveler; traveler.items = 1;
        {
            GameObject pickup;
            assert(script.OnGossipHello(&traveler, &pickup));
        }
        scenarioManager.scenario.currentStep = 7;
        if (invalid == 0) traveler.world = false;
        if (invalid == 1) traveler.contact = false;
        if (invalid == 2) traveler.alive = false;
        if (invalid == 3) traveler.teleporting = true;
        if (invalid == 4) traveler.playerClass = 8;
        if (invalid == 5) traveler.map = nullptr;
        if (invalid == 6) traveler.mapId = 1220;
        if (invalid == 7) { traveler.instance = 43; scenarioManager.acceptOtherInstance = true; }
        if (invalid == 8) traveler.rewarded = true;
        if (invalid == 9) traveler.status = 0;
        if (invalid == 10) traveler.status = 5;
        if (invalid == 11) traveler.items = 0;
        if (invalid == 12) scenarioManager.available = false;
        if (invalid == 13) scenarioManager.scenario.identity = 973;
        if (invalid == 14) scenarioManager.scenario.currentStep = 6;
        if (invalid == 15) scenarioManager.scenario.currentStep = 8;
        if (invalid == 16) scenarioManager.scenario.currentStep = 0;
        if (invalid == 17) traveler.mapId = 1479;
        traveler.FlushDelayedEvents();
#ifdef DEPARTURE_BASELINE
        assert(traveler.jumpRequests == 1);
#else
        assert(!traveler.jumpRequests);
#endif
        assert(traveler.credits == 1 && traveler.achievements == 2);
    }
#ifdef DEPARTURE_BASELINE
    std::cout << "BASELINE: pinned d7c3ac3b requests192085 in18 stale departure contexts including abandon, death, map change and a different live944 instance. Controlled callback, not a live client teleport.\n";
#else
    assert(!WarswordsAcquisition::CanReturn(nullptr, 42));
    for (uint32 status : {uint32(QUEST_STATUS_INCOMPLETE), uint32(QUEST_STATUS_COMPLETE)})
    {
        scenarioManager = {}; Player traveler; traveler.items = 1; traveler.status = status;
        GameObject pickup; assert(script.OnGossipHello(&traveler, &pickup));
        scenarioManager.scenario.currentStep = 7; traveler.FlushDelayedEvents();
        assert(traveler.jumpRequests == 1 && traveler.credits == 1);
    }
    std::cout << "PASS: actual native completion predicate and installed callback reject18 stale contexts; original instance, live active quest and real possession required, no GO lifetime capture or departure credit. Missing button/retry and real teleport/client remain separate.\n";
#endif
#endif
#ifdef RETURN_BASELINE
    std::cout << "BASELINE: pinned6c6a6fc7 awards103739 when the timer requests192085, including a failed jump; no native arrival occurred. Controlled cast/timer services.\n";
#else
    std::cout << "PASS: completed pickup and failed/accepted legacy jump requests do not grant103739 before arrival; existing Skyhold actor credit is checked separately.\n";
#endif
    std::cout << "PASS: actual opening rejects invalid/stale contexts; activation no longer grants items/progression, and completed/owned loot requires possession. Eight claim paths retain presentation; inventory/world services are controlled, equip/return remain separate.\n";
#endif
}
