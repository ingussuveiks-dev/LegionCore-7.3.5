#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
using uint32 = uint32_t;
using LootState = uint32;
using SpellCastResult = uint32;
using ItemPosCountVec = std::vector<uint32>;
#include "CriteriaTypes.inc"
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, QUEST_STATUS_COMPLETE=1, TYPEID_PLAYER=4,
    GO_READY=1, GO_ACTIVATED=2, GO_JUST_DEACTIVATED=3, GAMEOBJECT_FIELD_FLAGS=17, GO_FLAG_NOT_SELECTABLE=16,
    INVENTORY_SLOT_BAG_0=255, EQUIPMENT_SLOT_MAINHAND=15, NULL_BAG=255, NULL_SLOT=255,
    SPELL_CAST_OK=0, SCENE_LAUNCH=1, SCENE_COMPLETE=3, GAMEOBJECT_TYPE_FISHINGNODE=17 };
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
enum InventoryResult { EQUIP_ERR_OK, EQUIP_ERR_INVENTORY_FULL };
struct Map { } liveMap, otherMap;
struct Item { };
struct LootItem
{
    struct
    {
        uint32 ItemID=128823,RandomPropertiesID=0;
        struct {std::vector<uint32> BonusListIDs;uint32 Context=0;} ItemBonus;
    } item;
    uint32 count=1;
    bool freeforall=false,is_looted=false;
    std::set<uint32> GetAllowedLooters() const {return {};}
};
struct Loot
{
    LootItem item;
    uint32 gold=0,unlootedCount=1;
#ifdef CURRENT_LOOT
#include "IsLooted.inc"
#endif
};
struct Scenario
{
    uint32 id=1092,step=10;
    uint32 GetScenarioId() const { return id; }
    uint32 GetCurrentStep() const { return step; }
    bool IsCompleted(bool) const {return step==11;}
};
struct ScenarioManager
{
    Scenario scenario;
    bool present=true;
    Scenario* GetScenario(uint32 instance) { return present && instance==42 ? &scenario : nullptr; }
} manager;
auto* sScenarioMgr=&manager;
struct Player;
struct GameObject;
struct Unit
{
    virtual ~Unit()=default;
    virtual Player* ToPlayer() { return nullptr; }
    virtual uint32 GetTypeId() const { return 3; }
    uint32 scenes=0,returns=0,castResult=0;
    bool launched=false,sceneComplete=false,sceneAura=false;
    std::vector<std::function<void()>> pending;
    uint32 CastSpell(Unit* target,uint32 spell,bool triggered=false)
    {
        assert(target==this);
        if(spell==211838){assert(triggered);++scenes;if(!castResult){launched=true;sceneComplete=false;sceneAura=true;}}
        else{assert(spell==211862 && !triggered);++returns;}
        return castResult;
    }
    void AddDelayedEvent(uint32 delay,std::function<void()> callback) { assert(delay==12000);pending.push_back(std::move(callback)); }
};
struct Player : Unit
{
    bool world=true,contact=true,alive=true,teleport=false,rewarded=false,owned=false,bankOnly=false,room=true,storeWorks=true,equipped=false;
    bool phase=true,visible=true,los=true;
    uint32 klass=2,status=3,instance=42,mapId=1539,events=0,completed=0,pickup=0,arrival=0,grants=0,inventoryErrors=0;
    float distance=1;
    Map* map=&liveMap;
    Item artifact;
    Player* ToPlayer() override { return this; }
    uint32 GetTypeId() const override { return TYPEID_PLAYER; }
    bool IsInWorld() const { return world; }
    bool CanContact() const { return contact; }
    bool IsAlive() const { return alive; }
    bool IsBeingTeleported() const { return teleport; }
    uint32 getClass() const { return klass; }
    Map* GetMap() const { return map; }
    uint32 GetInstanceId() const { return instance; }
    uint32 GetMapId() const {return mapId;}
    uint32 GetGUIDLow() const {return 1;}
    bool HasSceneStatus(uint32 scene,uint32 state) const {assert(scene==1267);return state==SCENE_COMPLETE?sceneComplete:launched;}
    bool HasAura(uint32 spell) const {assert(spell==211838);return sceneAura;}
    bool InSamePhase(GameObject*) const { return phase; }
    bool canSeeOrDetect(GameObject*) const { return visible; }
    bool IsWithinLOSInMap(GameObject*) const { return los; }
    bool IsWithinDistInMap(GameObject*,float radius) const { return distance<=radius; }
    bool GetQuestRewardStatus(uint32 quest) const { assert(quest==42120);return rewarded; }
    uint32 GetQuestStatus(uint32 quest) const { assert(quest==42120);return status; }
    bool HasItemCount(uint32 item,uint32 count) const { assert(item==128823 && count==1);return owned; }
    Item* GetItemByEntry(uint32 item) { assert(item==128823);return owned && !bankOnly?&artifact:nullptr; }
    Item* GetItemByPos(uint32 bag,uint32 slot) {assert(bag==255 && slot==15);return owned && equipped?&artifact:nullptr;}
    void AddItem(uint32 item,uint32 count) { assert(item==128823 && count==1);++grants;owned=room && storeWorks; }
#ifdef CURRENT_LOOT
    InventoryResult CanStoreNewItem(uint32,uint32,ItemPosCountVec& destination,uint32 item,uint32 count,void*,bool quest)
    {
        assert(item==128823 && count==1 && !quest);
        if(!room)return EQUIP_ERR_INVENTORY_FULL;
        destination.push_back(1);return EQUIP_ERR_OK;
    }
    Item* StoreNewItem(ItemPosCountVec const&,uint32 item,bool,uint32,std::set<uint32> const&,std::vector<uint32> const&,uint32)
    {
        assert(item==128823);if(!storeWorks)return nullptr;owned=true;return &artifact;
    }
    void SendEquipError(InventoryResult result,void*,void*,uint32 item){assert(result==EQUIP_ERR_INVENTORY_FULL && item==128823);++inventoryErrors;}
    void StoreLootItem(Loot* loot)
    {
        LootItem* item=&loot->item;
        void* qitem=nullptr;
#include "StoreInventory.inc"
#include "ConsumeLoot.inc"
    }
#endif
    void UpdateAchievementCriteria(uint32 type,uint32 asset,uint32 amount=0)
    {
        if(type==CRITERIA_TYPE_SCRIPT_EVENT_2){assert(asset==50966 && !amount);++events;manager.scenario.step=11;}
        else{assert(type==CRITERIA_TYPE_COMPLETE_SCENARIO && asset==1092 && amount==1);++completed;}
    }
    void KilledMonsterCredit(uint32 entry) { if(entry==106357)++pickup;else{assert(entry==105892);++arrival;} }
};
struct Creature { uint32 despawns=0;void DespawnOrUnsummon(uint32 delay){assert(delay==1000);++despawns;} } pickupTrigger;
struct GameObjectAI;
struct GameObject
{
    bool world=true,spawned=true,notSelectable=false,actor=true;
    uint32 entry=249824,mapId=1539,instance=42,m_lootState=GO_ACTIVATED;
    Map* map=&liveMap;
    bool IsInWorld() const { return world; }
    bool isSpawned() const { return spawned; }
    uint32 GetEntry() const { return entry; }
    uint32 GetMapId() const { return mapId; }
    uint32 GetInstanceId() const { return instance; }
    Map* GetMap() const { return map; }
    float GetInteractionDistance() const { return 5; }
    bool HasFlag(uint32 field,uint32 flag) const { assert(field==17 && flag==16);return notSelectable; }
    uint32 getLootState() const { return m_lootState; }
    Creature* FindNearestCreature(uint32 entryId,float radius) { assert(entryId==106781 && radius==10);return actor?&pickupTrigger:nullptr; }
    void SetLootState(LootState,Unit* = nullptr);
    uint32 GetGoType() const {return 3;}
    GameObjectAI* AI() const;
};
struct GameObjectAI
{
    explicit GameObjectAI(GameObject* object):go(object) { }
    virtual ~GameObjectAI()=default;
    virtual bool GossipHello(Player*,bool) { return false; }
    virtual void OnStateChanged(uint32,Unit*) { }
protected:
    GameObject* go;
};
struct GameObjectScript;
GameObjectScript* currentScript=nullptr;
struct GameObjectScript
{
    explicit GameObjectScript(char const*) { currentScript=this; }
    virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*) { return false; }
    virtual GameObjectAI* GetAI(GameObject* object) const { return new GameObjectAI(object); }
    virtual void OnLootStateChanged(GameObject*,uint32,Unit*) { }
};
#include "Admission.inc"
#include "Callback.inc"
struct ScriptManager
{
    void OnGameObjectLootStateChanged(GameObject* object,uint32 state,Unit* target) { assert(currentScript);currentScript->OnLootStateChanged(object,state,target); }
} scriptManager;
auto* sScriptMgr=&scriptManager;
GameObjectAI* GameObject::AI() const { static GameObjectAI passive(nullptr);return &passive; }
#include "SetLootState.inc"
#ifdef CURRENT_LOOT
bool CompletedLoot(Loot* loot,GameObject* go)
{
#include "CompleteLoot.inc"
}
#endif
int main()
{
#ifdef LOOT_BASELINE
    for(uint32 failure:{0u,1u})
    {
        manager={};Player player;player.room=failure!=0;player.storeWorks=failure!=1;
        GameObject weapon;go_art_silverhand script;weapon.SetLootState(GO_ACTIVATED,&player);
        assert(!player.owned && player.grants==1 && player.events==1 && player.pickup==1 && player.arrival==1 && player.pending.size()==1);
    }
    std::cout<<"BASELINE7af29f67: eligible activation invokes grant/early credits/return even when controlled storage fails; no native loot consumed.\n";
#else
    for(uint32 invalid=0;invalid<32;++invalid)
    {
        manager={};Player player;GameObject weapon;go_art_silverhand script;
        switch(invalid)
        {
            case 0:player.world=false;break;
            case 1:player.contact=false;break;
            case 2:player.alive=false;break;
            case 3:player.teleport=true;break;
            case 4:player.klass=5;break;
            case 5:weapon.world=false;break;
            case 6:weapon.spawned=false;break;
            case 7:weapon.entry=249825;break;
            case 8:weapon.mapId=1220;break;
            case 9:player.map=&otherMap;break;
            case 10:player.instance=43;break;
            case 11:player.phase=false;break;
            case 12:player.visible=false;break;
            case 13:player.los=false;break;
            case 14:player.distance=5.01f;break;
            case 15:weapon.notSelectable=true;break;
            case 16:player.rewarded=true;break;
            case 17:player.status=0;break;
            case 18:player.status=5;break;
            case 19:manager.present=false;break;
            case 20:manager.scenario.id=991;break;
            case 21:manager.scenario.id=1037;break;
            case 22:manager.scenario.step=0;break;
            case 23:manager.scenario.step=9;break;
            case 24:manager.scenario.step=11;break;
            case 25:manager.scenario.step=12;break;
            case 26:player.map=nullptr;weapon.map=nullptr;break;
            case 27:weapon.instance=43;break;
            case 28:weapon.m_lootState=GO_JUST_DEACTIVATED;break;
            case 29:weapon.m_lootState=1;break;
            case 30:manager.scenario.step=11;player.owned=true;player.bankOnly=true;break;
            case 31:manager.scenario.step=12;player.owned=true;break;
        }
#ifdef BASELINE
        bool ownedBefore=player.owned;
#endif
        auto* ai=script.GetAI(&weapon);
        bool denied=script.OnGossipHello(&player,&weapon),reported=ai->GossipHello(&player,false),opened=ai->GossipHello(&player,true);
        delete ai;
#ifdef BASELINE
        script.OnLootStateChanged(&weapon,GO_ACTIVATED,&player);
        assert(!denied && !reported && !opened && player.grants==uint32(!ownedBefore) && player.pickup==1 && player.arrival==1);
#else
        assert(denied==(invalid!=29) && reported==denied && opened==denied);
        if(invalid!=24 && invalid!=28 && invalid!=30)player.owned=true;
        if(invalid!=29)weapon.m_lootState=GO_JUST_DEACTIVATED;
        script.OnLootStateChanged(&weapon,GO_JUST_DEACTIVATED,&player);
        assert(!player.grants && !player.events && !player.pickup && !player.arrival && !player.scenes && player.pending.empty());
#endif
    }
    for(uint32 status:{1u,3u})for(bool owned:{false,true})for(bool actor:{false,true})
    {
        manager={};Player player;player.status=status;player.owned=owned;player.distance=5;
        GameObject weapon;weapon.actor=actor;go_art_silverhand script;
#ifdef BASELINE
        assert(!script.OnGossipHello(&player,&weapon));
        auto* ai=script.GetAI(&weapon);assert(!ai->GossipHello(&player,false) && !ai->GossipHello(&player,true));delete ai;
        weapon.SetLootState(GO_ACTIVATED,&player);
        assert(player.events==1 && player.completed==1 && player.pickup==1 && player.arrival==1 && player.scenes==1 && player.grants==uint32(!owned));
#else
        auto* ai=script.GetAI(&weapon);assert(!ai->GossipHello(&player,false) && !ai->GossipHello(&player,true));delete ai;
        assert(script.OnGossipHello(&player,&weapon)==owned);
        if(!owned)
        {
            weapon.SetLootState(GO_ACTIVATED,&player);assert(!player.scenes && !player.grants);
            Loot loot;player.StoreLootItem(&loot);assert(CompletedLoot(&loot,&weapon));
            weapon.SetLootState(GO_JUST_DEACTIVATED,&player);
        }
        assert(player.scenes==1 && !player.events && !player.completed && !player.pickup && !player.arrival && !player.grants && player.pending.size()==1);
        weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.scenes==1);
#endif
    }
    {
        manager={};Player player;GameObject weapon;go_art_silverhand script;Unit pet,guardian;
        script.OnLootStateChanged(&weapon,GO_ACTIVATED,nullptr);script.OnLootStateChanged(&weapon,GO_ACTIVATED,&pet);script.OnLootStateChanged(&weapon,GO_ACTIVATED,&guardian);
        assert(!player.events && !pet.scenes && !guardian.scenes);
#ifndef BASELINE
        assert(script.OnGossipHello(nullptr,&weapon) && script.OnGossipHello(&player,nullptr));
        script.OnLootStateChanged(nullptr,GO_ACTIVATED,&player);
        player.status=0;player.owned=true;weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(!player.scenes);
        player.status=3;weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.scenes==1);
        manager={};manager.scenario.step=11;player={};player.owned=true;
        weapon.m_lootState=GO_READY;assert(script.OnGossipHello(&player,&weapon));
        assert(!player.grants && player.scenes==1 && !player.pickup);
#endif
    }
#ifdef BASELINE
    std::cout<<"BASELINE b7b06b71:32 invalid direct activations still invoke legacy effects; no script/open/AI admission. Controlled inventory/credit sinks, not native quest persistence or live client.\n";
#else
    for(uint32 failure:{0u,1u})
    {
        manager={};Player player;player.room=failure!=0;player.storeWorks=failure!=1;
        GameObject weapon;go_art_silverhand script;Loot loot;
        assert(!script.OnGossipHello(&player,&weapon));weapon.SetLootState(GO_ACTIVATED,&player);
        player.StoreLootItem(&loot);assert(!loot.isLooted() && !CompletedLoot(&loot,&weapon) && !player.owned && !player.scenes && !player.events);
        assert(player.inventoryErrors==uint32(failure==0));
        player.room=player.storeWorks=true;player.StoreLootItem(&loot);
        assert(loot.isLooted() && loot.item.is_looted && !loot.unlootedCount && player.owned);
        weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.scenes==1 && !player.grants && !player.pickup && !player.arrival);
    }
    {
        manager={};Player player;player.owned=player.launched=true;GameObject weapon;weapon.m_lootState=GO_READY;go_art_silverhand script;
        assert(script.OnGossipHello(&player,&weapon));assert(player.scenes==1 && player.sceneAura && player.pending.size()==1);
        weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.scenes==1);
        player.sceneAura=false;weapon.m_lootState=GO_READY;
        assert(script.OnGossipHello(&player,&weapon));assert(player.scenes==2 && !player.grants);
    }
    {
        manager={};Player player;player.owned=true;player.castResult=1;GameObject weapon;go_art_silverhand script;warnings=0;
        assert(script.OnGossipHello(&player,&weapon));assert(weapon.m_lootState==GO_READY && player.scenes==1 && player.pending.empty() && warnings==1);
        player.castResult=0;assert(script.OnGossipHello(&player,&weapon));assert(player.scenes==2 && player.pending.size()==1 && !player.grants);
        player.sceneComplete=true;player.equipped=true;manager.scenario.step=11;
        weapon.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.scenes==2);
        player.pending[0]();assert(player.returns==1 && !player.arrival);
        player.castResult=1;uint32 before=warnings;player.pending[0]();assert(warnings==before+1 && !player.arrival);
    }
    for(uint32 invalid=0;invalid<17;++invalid)
    {
        manager={};manager.scenario.step=11;Player player;player.owned=player.equipped=player.sceneComplete=true;
        switch(invalid)
        {
            case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.alive=false;break;
            case 3:player.teleport=true;break;case 4:player.klass=1;break;case 5:player.mapId=1220;break;
            case 6:player.instance=43;break;case 7:player.rewarded=true;break;case 8:player.status=0;break;
            case 9:player.sceneComplete=false;break;case 10:player.owned=false;break;case 11:player.bankOnly=true;break;
            case 12:player.equipped=false;break;case 13:manager.present=false;break;case 14:manager.scenario.id=1037;break;
            case 15:manager.scenario.step=10;break;
            case 16:player.map=nullptr;break;
        }
        assert(!SilverHandAcquisition::CanReturn(&player,42));
    }
    std::cout<<"PASS: native storage/consume/release and real completed-state dispatch; full-bag/store failure retains unlooted item, retry stores before scene, activation grants/credits nothing. Scoped opening/claim, existing-copy recovery, active-aura versus stale-launch replay, failed scene reopening and17 stale returns checked. Spell/scene/quest sinks and world services controlled; native211842 and client/late-scene return remain separate.\n";
#endif
#endif
}
