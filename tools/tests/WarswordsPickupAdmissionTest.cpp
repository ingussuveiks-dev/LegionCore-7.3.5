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
    void CastSpell(Unit*, uint32 identity, bool = false) { assert(identity == 205443 || identity == 192085); ++casts; }
    void CreateConversation(uint32 identity) { assert(identity == 849); ++conversations; }
    void AddDelayedEvent(uint32 delay, std::function<void()>) { assert(delay == 12000); ++delays; }
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
    bool alive = true, contact = true, phase = true, rewarded = false;
    Map* map = &instanceMap;
    float distance = 1.f;
    bool room = true, storeWorks = true;
    uint32 inventoryErrors = 0;
    Item storedItem;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() override { return TYPEID_PLAYER; }
    bool IsAlive() const { return alive; }
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
    uint32 identity = 944, step = 6;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    bool available = true;
    Scenario scenario;
    Scenario* GetScenario(uint32 identity) { return available && identity == 42 ? &scenario : nullptr; }
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
        if (invalid == 14) scenarioManager.scenario.step = 0;
        if (invalid == 15) scenarioManager.scenario.step = 5;
        if (invalid == 16) scenarioManager.scenario.step = 7;
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
                scenarioManager.scenario.step = 7;
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
    std::cout << "PASS: actual opening rejects invalid/stale contexts; activation no longer grants items/progression, and completed/owned loot requires possession. Eight claim paths retain presentation; inventory/world services are controlled, equip/return remain separate.\n";
#endif
}
