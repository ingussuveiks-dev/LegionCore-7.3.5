#include <cassert>
#include <cstdint>
#include <iostream>
#include <functional>
#include <set>
#include <vector>
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8 = uint8_t;
using LootState = uint32;
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, GO_READY=1, GO_ACTIVATED=2,
    GO_JUST_DEACTIVATED=3, GAMEOBJECT_FIELD_FLAGS=17, GO_FLAG_NOT_SELECTABLE=16, DATA_STAGE_FINAL=5,
    CRITERIA_TYPE_SCRIPT_EVENT_2=92, DATA_YRGRIM_EVENT=6, SPELL_TRUTHGUARD_CREDIT=210133,
    SPELL_TRUTHGUARD_SCENE=210132, NPC_TRUTHGUARD_OATHSEEKER=106190,
    NULL_BAG=255, NULL_SLOT=255, GAMEOBJECT_TYPE_CHEST=3, GAMEOBJECT_TYPE_FISHINGNODE=17,
    INVENTORY_SLOT_BAG_0=255, EQUIPMENT_SLOT_MAINHAND=15, EQUIPMENT_SLOT_OFFHAND=16,
    ITEM_FIELD_CREATOR=1, ITEM_FIELD_DYNAMIC_FLAGS=2, ITEM_FLAG_CHILD=4 };
using ItemPosCountVec = std::vector<uint32>;
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_INVENTORY_FULL, EQUIP_ERR_CANT_EQUIP };
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct Item
{
    uint32 entry=128866,guid=1,creator=0,child=2;
    uint16 position=0xFF20;
    bool childFlag=false;
    uint32 GetEntry() const { return entry; }
    uint32 GetGUID() const { return guid; }
    uint32 GetChildItem() const { return child; }
    uint32 GetGuidValue(uint32 field) const { assert(field==ITEM_FIELD_CREATOR); return creator; }
    bool HasFlag(uint32 field,uint32 flag) const { assert(field==ITEM_FIELD_DYNAMIC_FLAGS && flag==ITEM_FLAG_CHILD); return childFlag; }
    bool IsEquipped() const { return position>=0xFF00 && position<0xFF13; }
    uint16 GetPos() const { return position; }
    uint8 GetBagSlot() const { return uint8(position>>8); }
    uint8 GetSlot() const { return uint8(position&255); }
};
struct LootItem
{
    struct
    {
        uint32 ItemID=128866, RandomPropertiesID=0;
        struct { std::vector<uint32> BonusListIDs; uint32 Context=0; } ItemBonus;
    } item;
    uint32 count=1;
    bool freeforall=false, is_looted=false;
    std::set<uint32> GetAllowedLooters() const { return {}; }
};
struct Loot
{
    LootItem item;
    uint32 gold=0, unlootedCount=1;
#include "NativeIsLooted.inc"
};
struct Map { } liveMap, otherMap;
struct Scenario
{
    uint32 identity=1082, step=5;
    uint32 GetScenarioId() const { return identity; }
    uint32 GetCurrentStep() const { return step; }
};
struct ScenarioMgr
{
    Scenario scenario;
    bool available=true;
    Scenario* GetScenario(uint32 testInstance) { assert(testInstance==42 || testInstance==43); return available?&scenario:nullptr; }
} manager;
auto* sScenarioMgr=&manager;
struct InstanceScript
{
    uint32 events=0, data=6;
    uint32 getScenarionStep() const { return manager.scenario.step; }
    uint32 GetData(uint32 identity) const { assert(identity==DATA_YRGRIM_EVENT); return data; }
    void SetData(uint32 identity,uint32 value) { assert(identity==DATA_YRGRIM_EVENT && value==7); data=value; }
    void DoUpdateAchievementCriteria(uint32 kind,uint32 asset)
    {
        assert(kind==92 && asset==50545); ++events; manager.scenario.step=6;
    }
} testInstance;
struct Player;
struct GameObject;
struct GameObjectAI;
struct Unit { virtual ~Unit()=default; virtual Player* ToPlayer() { return nullptr; } };
struct Player : Unit
{
    bool world=true,contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    bool carried=false,bank=false,instancePresent=true;
    bool room=true,storeWorks=true;
    uint32 inventoryErrors=0;
    Item storedItem;
    Item childItem{128867,2,1,0,0xFFCB,true};
    bool childPresent=true,swapWorks=true,childSwapWorks=true;
    InventoryResult equipResult=EQUIP_ERR_OK,childResult=EQUIP_ERR_OK;
    uint32 swaps=0,childSwaps=0;
    uint32 klass=2,status=3,instanceId=42,casts=0,team=0;
    float distance=1;
    Map* map=&liveMap;
    Player* ToPlayer() override { return this; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return klass; }
    Map* GetMap() const { return map; }
    uint32 GetInstanceId() const { return instanceId; }
    bool InSamePhase(GameObject*) const { return phase; }
    bool canSeeOrDetect(GameObject*) const { return visible; }
    bool IsWithinLOSInMap(GameObject*) const { return los; }
    bool IsWithinDistInMap(GameObject*,float radius) const { return distance<=radius; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest==42017); return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest==42017); return status; }
    bool HasItemCount(uint32 item,uint32 count=1,bool includeBank=false) const
    {
        assert(item==128866 && count==1); return carried || (includeBank && bank);
    }
    InstanceScript* GetInstanceScript() { return instancePresent?&testInstance:nullptr; }
    Item* GetItemByEntry(uint32 identity) { assert(identity==128866); return carried?&storedItem:nullptr; }
    Item* GetChildItemByGuid(uint32 identity) { return childPresent && identity==childItem.guid?&childItem:nullptr; }
    InventoryResult CanEquipItem(uint32 slot,uint16& destination,Item* item,bool swap)
    {
        assert(slot==NULL_SLOT && item==&storedItem && swap); destination=0xFF10; return equipResult;
    }
    InventoryResult CanEquipChildItem(Item* item) { assert(item==&storedItem); return childResult; }
    void SendEquipError(InventoryResult result,Item* item)
    {
        assert(result!=EQUIP_ERR_OK && item==&storedItem); ++inventoryErrors;
    }
    void SwapItem(uint16 source,uint16 destination)
    {
        assert(source==storedItem.position && destination==0xFF10); ++swaps;
        if(swapWorks) { storedItem.position=destination; if(childSwapWorks) childItem.position=0xFF0F; }
    }
    void EquipChildItem(uint8 bag,uint8 slot,Item* parent)
    {
        assert(bag==255 && slot==16 && parent==&storedItem); ++childSwaps;
        if(childSwapWorks) childItem.position=0xFF0F;
    }
    Item* GetItemByPos(uint8 bag,uint8 slot)
    {
        assert(bag==255); uint16 position=uint16((bag<<8)|slot);
        if(carried && storedItem.position==position) return &storedItem;
        return childPresent && childItem.position==position?&childItem:nullptr;
    }
    InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& destination,uint32 identity,uint32 count,void*,bool quest)
    {
        assert(identity==128866 && count==1 && !quest);
        if(!room) return EQUIP_ERR_INVENTORY_FULL;
        destination.push_back(1); return EQUIP_ERR_OK;
    }
    Item* StoreNewItem(ItemPosCountVec const&,uint32 identity,bool,uint32,std::set<uint32> const&,std::vector<uint32> const&,uint32)
    {
        assert(identity==128866);
        if(!storeWorks) return nullptr;
        carried=true; return &storedItem;
    }
    void SendEquipError(InventoryResult result,void*,void*,uint32 identity)
    {
        assert(result==EQUIP_ERR_INVENTORY_FULL && identity==128866); ++inventoryErrors;
    }
    void StoreLootItem(Loot* loot)
    {
        LootItem* item=&loot->item;
        void* qitem=nullptr;
#include "NativeStoreInventory.inc"
#include "NativeConsumeLoot.inc"
    }
    void CastSpell(Player* target,uint32 spell,bool triggered)
    {
        assert(target==this && triggered && (spell==210132 || spell==210133)); ++casts;
    }
};
struct Creature { uint32 despawns=0; void DespawnOrUnsummon() { ++despawns; } } testTrigger;
struct GameObject
{
    uint32 entry=249420,mapId=1495,instanceId=42,m_lootState=2,deletes=0;
    bool world=true,spawned=true,notSelectable=false,actor=true;
    Map* map=&liveMap;
    std::vector<std::function<void()>> delayed;
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) { assert(delay==1); delayed.push_back(callback); }
    void Drain() { auto pending=std::move(delayed); delayed.clear(); for(auto& callback:pending) callback(); }
    bool IsInWorld() const { return world; }
    bool isSpawned() const { return spawned; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instanceId; }
    uint32 GetGoType() const { return GAMEOBJECT_TYPE_CHEST; }
    Map* GetMap() const { return map; }
    float GetInteractionDistance() const { return 5; }
    bool HasFlag(uint32 field,uint32 flag) const { assert(field==17 && flag==16); return notSelectable; }
    uint32 getLootState() const { return m_lootState; }
    Creature* FindNearestCreature(uint32 identity,float radius,bool alive)
    {
        assert(identity==106190 && radius==20 && alive); return actor?&testTrigger:nullptr;
    }
    void Delete() { ++deletes; }
    GameObjectAI* AI() const;
    void SetLootState(uint32 state,Unit* player=nullptr);
};
struct GameObjectAI
{
    explicit GameObjectAI(GameObject* object):go(object) { }
    virtual ~GameObjectAI()=default;
    virtual bool GossipHello(Player*,bool) { return false; }
    virtual void OnStateChanged(uint32,Unit*) { }
    GameObject* go;
};
struct GameObjectScript;
GameObjectScript* currentScript=nullptr;
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { currentScript=this; }
    virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*) { return false; }
    virtual void OnLootStateChanged(GameObject*,uint32,Unit*) { }
    virtual GameObjectAI* GetAI(GameObject* object) const { return new GameObjectAI(object); }
};
#include "Policy.inc"
#include "Callback.inc"
struct ScriptMgr
{
    void OnGameObjectLootStateChanged(GameObject* object,uint32 state,Unit* player)
    {
        currentScript->OnLootStateChanged(object,state,player);
    }
} scriptManager;
auto* sScriptMgr=&scriptManager;
GameObjectAI* GameObject::AI() const { static GameObjectAI defaultAI(nullptr); return &defaultAI; }
#include "NativeSetLootState.inc"
bool CompletedLoot(Loot* loot,GameObject* go)
{
#include "NativeCompleteLoot.inc"
}
void Reset() { manager={}; testInstance={}; testTrigger={}; warnings=0; }
int main()
{
#ifdef BASELINE
    for(bool owned : {false,true})
    {
        Reset(); Player player; player.carried=owned; GameObject weapon; go_sr_truthguard script;
        script.OnLootStateChanged(&weapon,owned?GO_ACTIVATED:GO_JUST_DEACTIVATED,&player);
        assert(testInstance.events==1 && player.casts==2 && weapon.deletes==uint32(owned));
    }
    Reset(); Player player; player.klass=3; player.status=0; player.phase=false; player.carried=true;
    manager.scenario.identity=1068; GameObject weapon; weapon.mapId=1220; go_sr_truthguard script;
    assert(!script.OnGossipHello(&player,&weapon));
    script.OnLootStateChanged(&weapon,GO_ACTIVATED,&player);
    assert(testInstance.events==1 && player.casts==2);
    std::cout << "PASS pinnedaa988ac6 baseline: reported completion without128866 and mere activation with an owned item broadcast50545/cast both spells; wrong class/quest/scenario/map/phase admits the same callback. Controlled dispatch, not native off-route progression or client.\n";
#elif defined(EQUIP_BASELINE)
    for(uint32 failure=0;failure<3;++failure)
    {
        Reset(); Player player; player.carried=true; GameObject chest; go_sr_truthguard pickup;
        if(failure==0) player.childPresent=false;
        if(failure==1) player.equipResult=EQUIP_ERR_CANT_EQUIP;
        if(failure==2) player.swapWorks=false;
        assert(pickup.OnGossipHello(&player,&chest));
        assert(testInstance.events==1 && testInstance.data==7 && player.casts==2 && testTrigger.despawns==1 && !player.swaps);
        assert(!player.GetItemByPos(255,16) && !player.GetItemByPos(255,15));
    }
    std::cout << "PASS pinned667fbc74: carried parent alone advances/despawns/casts with missing child or failed equip/swap services; no equipped-pair acknowledgment. Cast service controlled, not a real spell/inventory/client replay.\n";
#else
    for(uint32 failure=0;failure<10;++failure)
    {
        Reset(); Player player; player.carried=true; GameObject chest; go_sr_truthguard pickup;
        switch(failure)
        {
            case 0: player.childPresent=false; break;
            case 1: player.storedItem.child=0; break;
            case 2: player.childItem.entry=128288; break;
            case 3: player.childItem.creator=7; break;
            case 4: player.childItem.childFlag=false; break;
            case 5: player.equipResult=EQUIP_ERR_CANT_EQUIP; break;
            case 6: player.childResult=EQUIP_ERR_INVENTORY_FULL; break;
            case 7: player.swapWorks=false; break;
            case 8: player.childSwapWorks=false; break;
            case 9: player.storedItem.position=0xFF0F; player.childItem.position=0xFF10; break;
        }
        assert(pickup.OnGossipHello(&player,&chest));
        assert(!testInstance.events && testInstance.data==6 && !player.casts && !testTrigger.despawns && chest.delayed.size()==1);
        assert(warnings==uint32(failure<5 || failure>=7) && player.inventoryErrors==uint32(failure==5 || failure==6));
        player.status=0; chest.Drain(); assert(chest.m_lootState==GO_READY);
        assert(pickup.OnGossipHello(&player,&chest) && !testInstance.events && !player.casts);
        player.status=3; player.childPresent=true; player.storedItem.child=2;
        player.storedItem.position=0xFF20; player.childItem={128867,2,1,0,0xFFCB,true};
        player.equipResult=player.childResult=EQUIP_ERR_OK; player.swapWorks=player.childSwapWorks=true;
        assert(pickup.OnGossipHello(&player,&chest));
        assert(testInstance.events==1 && player.casts==2 && player.GetItemByPos(255,16)==&player.storedItem && player.GetItemByPos(255,15)==&player.childItem);
        chest.Drain(); assert(chest.m_lootState==GO_JUST_DEACTIVATED);
        chest.SetLootState(3,&player); assert(testInstance.events==1 && player.casts==2);
    }
    for(bool childEquipped : {false,true})
    {
        Reset(); Player player; player.carried=true; player.storedItem.position=0xFF10;
        if(childEquipped) player.childItem.position=0xFF0F;
        assert(TruthguardAcquisition::EquipPair(&player));
        assert(!player.swaps && player.childSwaps==uint32(!childEquipped));
    }
    for(bool rejected : {false,true})
    {
        Reset(); Player player; player.carried=true; player.storedItem.position=0xFF10;
        player.childResult=rejected?EQUIP_ERR_INVENTORY_FULL:EQUIP_ERR_OK;
        player.childSwapWorks=false; GameObject chest; go_sr_truthguard pickup;
        assert(pickup.OnGossipHello(&player,&chest) && !testInstance.events && !player.casts && !player.swaps);
        assert(player.childSwaps==uint32(!rejected) && player.inventoryErrors==uint32(rejected) && warnings==uint32(!rejected));
        chest.Drain(); player.childResult=EQUIP_ERR_OK; player.childSwapWorks=true;
        assert(pickup.OnGossipHello(&player,&chest) && testInstance.events==1 && player.casts==2);
    }
    Reset(); Player missingParent; assert(!TruthguardAcquisition::EquipPair(&missingParent) && warnings==1);
    Reset(); Player deferred; deferred.carried=true; deferred.childPresent=false; GameObject changed; go_sr_truthguard deferredPickup;
    assert(deferredPickup.OnGossipHello(&deferred,&changed) && changed.delayed.size()==1);
    changed.SetLootState(GO_ACTIVATED,&deferred); changed.Drain();
    assert(changed.getLootState()==GO_ACTIVATED && !testInstance.events && !deferred.casts);
    std::cout << "PASS: ten missing/corrupt child/equip/swap failures produce no progression/cast/despawn; deferred scoped ready reset, revoked-quest rejection and repaired-existing-pair retry; already equipped and hidden-child recovery. Native inventory services controlled; no item creation.\n";
    for(uint32 invalid=0;invalid<27;++invalid)
    {
        Reset(); Player player; GameObject weapon; go_sr_truthguard script;
        switch(invalid)
        {
            case 0: player.world=false; break; case 1: player.contact=false; break;
            case 2: player.alive=false; break; case 3: player.teleport=true; break;
            case 4: player.klass=3; break; case 5: weapon.world=false; break;
            case 6: weapon.spawned=false; break; case 7: weapon.entry=248831; break;
            case 8: weapon.mapId=1220; break; case 9: player.map=&otherMap; break;
            case 10: player.instanceId=43; break; case 11: player.phase=false; break;
            case 12: player.visible=false; break; case 13: player.los=false; break;
            case 14: player.distance=5.01f; break; case 15: weapon.notSelectable=true; break;
            case 16: player.rewarded=true; break; case 17: player.status=0; break;
            case 18: player.status=5; break; case 19: manager.available=false; break;
            case 20: manager.scenario.identity=909; break; case 21: manager.scenario.identity=1068; break;
            case 22: manager.scenario.step=0; break; case 23: manager.scenario.step=4; break;
            case 24: manager.scenario.step=6; break; case 25: manager.scenario.step=7; break;
            case 26: player.map=nullptr; weapon.map=nullptr; break;
        }
        assert(script.OnGossipHello(&player,&weapon));
        auto* ai=script.GetAI(&weapon); assert(ai->GossipHello(&player,false) && ai->GossipHello(&player,true)); delete ai;
        player.carried=true; weapon.SetLootState(3,&player);
        assert(!testInstance.events && !player.casts && !testTrigger.despawns && !weapon.deletes);
    }
    for(uint32 status : {1u,3u}) for(uint32 team : {0u,1u}) for(bool bank : {false,true}) for(bool missing : {false,true})
    {
        Reset(); Player player; player.status=status; player.team=team;
        player.bank=bank; player.carried=!bank; player.distance=5;
        GameObject weapon; weapon.actor=!missing; go_sr_truthguard script;
        assert(script.OnGossipHello(&player,&weapon));
        if(bank)
        {
            assert(!testInstance.events && !player.casts && weapon.m_lootState==2);
            auto* ai=script.GetAI(&weapon); assert(ai->GossipHello(&player,false) && ai->GossipHello(&player,true)); delete ai;
            player.carried=true; player.bank=false;
            assert(script.OnGossipHello(&player,&weapon));
        }
        assert(testInstance.events==1 && testInstance.data==7 && player.casts==2 && testTrigger.despawns==uint32(!missing) && !weapon.deletes);
        weapon.SetLootState(3,&player); assert(testInstance.events==1 && player.casts==2);
    }
    for(uint32 reported : {1u,2u,3u}) for(uint32 stored : {1u,2u,3u}) for(bool owned : {false,true})
    {
        Reset(); Player player; player.carried=owned; GameObject weapon; weapon.m_lootState=stored; go_sr_truthguard script;
        script.OnLootStateChanged(&weapon,reported,&player);
        assert(testInstance.events==uint32(reported==3 && stored==3 && owned));
    }
    Reset(); Player player; GameObject weapon; go_sr_truthguard script; Unit pet;
    assert(script.OnGossipHello(nullptr,&weapon) && script.OnGossipHello(&player,nullptr));
    assert(!script.OnGossipHello(&player,&weapon));
    auto* ai=script.GetAI(&weapon); assert(!ai->GossipHello(&player,false) && !ai->GossipHello(&player,true)); delete ai;
    script.OnLootStateChanged(nullptr,3,&player); script.OnLootStateChanged(&weapon,3,nullptr); weapon.SetLootState(3,&pet);
    assert(!testInstance.events && !player.casts);
    weapon.SetLootState(3,&player); assert(!testInstance.events);
    player.carried=true; player.status=0; weapon.SetLootState(3,&player); assert(!testInstance.events);
    player.status=3; weapon.SetLootState(3,&player); assert(testInstance.events==1 && player.casts==2);
    Reset(); Player stopped; stopped.carried=true; GameObject deactivated; deactivated.m_lootState=3;
    assert(script.OnGossipHello(&stopped,&deactivated));
    ai=script.GetAI(&deactivated); assert(ai->GossipHello(&stopped,false) && ai->GossipHello(&stopped,true)); delete ai;
    assert(!testInstance.events && !stopped.casts);
    stopped.instancePresent=false; script.OnLootStateChanged(&deactivated,3,&stopped);
    assert(!testInstance.events && !stopped.casts);
    Reset(); Player bankOnly; bankOnly.bank=true; script.OnLootStateChanged(&deactivated,3,&bankOnly);
    assert(!testInstance.events && !bankOnly.casts);
    for(uint32 failure : {0u,1u})
    {
        Reset(); Player trainee; GameObject chest; go_sr_truthguard pickup; Loot loot;
        trainee.room=failure!=0; trainee.storeWorks=failure!=1;
        assert(!pickup.OnGossipHello(&trainee,&chest));
        chest.SetLootState(2,&trainee); trainee.StoreLootItem(&loot);
        assert(!trainee.carried && !loot.isLooted() && !loot.item.is_looted && loot.unlootedCount==1);
        assert(!CompletedLoot(&loot,&chest) && !testInstance.events && !trainee.casts);
        assert(trainee.inventoryErrors==uint32(failure==0));
        trainee.room=trainee.storeWorks=true; trainee.StoreLootItem(&loot);
        assert(trainee.carried && loot.isLooted() && loot.item.is_looted && !loot.unlootedCount && CompletedLoot(&loot,&chest));
        chest.SetLootState(3,&trainee); chest.SetLootState(3,&trainee);
        assert(testInstance.events==1 && trainee.casts==2);
    }
    std::cout << "PASS: extracted native inventory-space/store/consume/isLooted/release predicate leaves full-bag/null-store failures unconsumed/uncredited; retry stores128866 before one completed claim. Storage services controlled; paired-item/native equip/network/client remain separate.\n";
    std::cout << "PASS:27 invalid use/open/report contexts denied by extracted script/AI;18 reported/stored/carried combinations;16 active-status/team-label/bank/missing-actor replays. Bank-only opening/report/callback cannot claim; projected withdrawal retries without grant. Native SetLootState dispatch, null/nonplayer and revoked-context retry checked; duplicate claim exits after modeled stage advance. Storage/equip/failed casts/group/client not certified.\n";
#endif
}
