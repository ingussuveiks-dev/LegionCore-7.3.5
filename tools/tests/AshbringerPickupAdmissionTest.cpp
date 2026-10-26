#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
using uint8=uint8_t;
using uint32=uint32_t;
using ItemPosCountVec=std::vector<uint32>;
#include "Constants.inc"
enum {GAMEOBJECT_FIELD_FLAGS=17,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
#define TC_LOG_WARN(...) ((void)0)
struct GameObject;
struct Player;
struct Unit {virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct InstanceScript
{
    uint32 step=3,credits=0;
    uint32 getScenarionStep(){return step;}
    void DoUpdateAchievementCriteria(uint32 type,uint32 value){assert(type==92&&value==46130);++credits;}
};
struct Map {};
struct Player:Unit
{
    bool live=true,contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,range=true,rewarded=false,owned=false;
    uint32 klass=CLASS_PALADIN,instanceId=7,casts=0;
    QuestStatus status=QUEST_STATUS_INCOMPLETE;
    Map* map=nullptr;
    InstanceScript* instance=nullptr;
    Player* ToPlayer()override{return this;}
    bool IsInWorld(){return live;}
    bool CanContact(){return contact;}
    bool IsAlive(){return alive;}
    bool IsBeingTeleported(){return teleport;}
    uint32 getClass(){return klass;}
    Map* GetMap(){return map;}
    uint32 GetInstanceId(){return instanceId;}
    bool InSamePhase(GameObject*){return phase;}
    bool canSeeOrDetect(GameObject*){return visible;}
    bool IsWithinLOSInMap(GameObject*){return los;}
    bool IsWithinDistInMap(GameObject*,float distance){assert(distance==5.0f);return range;}
    bool GetQuestRewardStatus(uint32 quest){assert(quest==38376);return rewarded;}
    QuestStatus GetQuestStatus(uint32 quest){assert(quest==38376);return status;}
    InstanceScript* GetInstanceScript(){return instance;}
    bool HasItemCount(uint32 item){assert(item==120978);return owned;}
    InventoryResult CanStoreItem(uint8,uint8,ItemPosCountVec&,uint32,uint32,void*,bool,uint32*,bool)const{return EQUIP_ERR_OK;}
#include "CapacityBridge.inc"
    void SendEquipError(InventoryResult,void*,void*,uint32){assert(false);}
    SpellCastResult CastSpell(Player* target,uint32 spell,bool triggered){assert(target==this&&spell==180850&&triggered);++casts;return SPELL_CAST_OK;}
    uint32 GetGUIDLow(){return 1;}
};
struct GameObjectAI
{
    GameObject* go;
    explicit GameObjectAI(GameObject* object):go(object){}
    virtual ~GameObjectAI()=default;
    virtual bool GossipHello(Player*,bool){return false;}
    void OnStateChanged(LootState,Unit*){}
};
struct Creature {uint32 despawns=0;void DespawnOrUnsummon(){++despawns;}};
struct GameObject
{
    bool live=true,spawned=true,blocked=false;
    uint32 entry=247318,mapId=1500,instanceId=7,deletes=0;
    Map* map=nullptr;
    Creature* trigger=nullptr;
    LootState m_lootState=GO_READY;
    bool IsInWorld(){return live;}
    bool isSpawned(){return spawned;}
    uint32 GetEntry(){return entry;}
    uint32 GetMapId(){return mapId;}
    uint32 GetInstanceId(){return instanceId;}
    Map* GetMap(){return map;}
    float GetInteractionDistance(){return 5.0f;}
    bool HasFlag(uint32 field,uint32 flag){assert(field==GAMEOBJECT_FIELD_FLAGS&&flag==GO_FLAG_NOT_SELECTABLE);return blocked;}
    GameObjectAI* AI(){static GameObjectAI passive(nullptr);return &passive;}
    void SetLootState(LootState,Unit* =nullptr);
    Creature* FindNearestCreature(uint32 entryId,float radius){assert(entryId==103454&&radius==10.f);return trigger;}
    void Delete(){++deletes;}
};
struct Scenario
{
    uint32 id=775,step=3;
    uint32 GetScenarioId(){return id;}
    uint32 GetCurrentStep(){return step;}
};
struct ScenarioManager
{
    Scenario* scenario=nullptr;
    Scenario* GetScenario(uint32 instance){assert(instance==7);return scenario;}
} scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct GameObjectScript
{
    explicit GameObjectScript(char const*){}
    virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*){return false;}
    virtual GameObjectAI* GetAI(GameObject* object)const{return new GameObjectAI(object);}
    virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}
};
#include "Pickup.inc"
go_bs_the_ashbringer* activePickup=nullptr;
struct ScriptManager {void OnGameObjectLootStateChanged(GameObject* object,LootState state,Unit* target){activePickup->OnLootStateChanged(object,state,target);}} scriptManager;
auto* sScriptMgr=&scriptManager;
#include "SetLootState.inc"
int main()
{
    Map map,otherMap;InstanceScript instance;Scenario scenario;Player player;GameObject object;Creature trigger;
    player.map=&map;player.instance=&instance;object.map=&map;object.trigger=&trigger;scenarioManager.scenario=&scenario;
    go_bs_the_ashbringer script;activePickup=&script;std::unique_ptr<GameObjectAI> ai(script.GetAI(&object));
#ifdef BASELINE
    scenario.id=933;player.klass=CLASS_WARRIOR;player.status=QUEST_STATUS_NONE;player.alive=false;player.phase=false;player.range=false;
    assert(!script.OnGossipHello(&player,&object)&&!ai->GossipHello(&player,true));object.SetLootState(GO_JUST_DEACTIVATED,&player);
    assert(player.casts==1&&instance.credits==1&&trigger.despawns==1);
    std::cout<<"PASS pinned30be60aa: unrelated933/non-Paladin/inactive/dead/phase-hidden/distant context starts retrieval and direct46130/cleanup with legacy step3; no initial gate\n";
#else
    std::vector<std::function<void()>> deny={
        [&]{player.live=false;},[&]{player.contact=false;},[&]{player.alive=false;},[&]{player.teleport=true;},[&]{player.klass=CLASS_WARRIOR;},
        [&]{object.live=false;},[&]{object.spawned=false;},[&]{object.entry=249420;},[&]{object.mapId=1495;},
        [&]{player.map=nullptr;},[&]{player.map=&otherMap;},[&]{player.instanceId=8;},[&]{player.phase=false;},[&]{player.visible=false;},
        [&]{player.los=false;},[&]{player.range=false;},[&]{object.blocked=true;},[&]{player.rewarded=true;},
        [&]{player.status=QUEST_STATUS_NONE;},[&]{player.status=QUEST_STATUS_FAILED;},[&]{player.status=QUEST_STATUS_REWARDED;},
        [&]{scenarioManager.scenario=nullptr;},[&]{scenario.id=933;},[&]{scenario.id=961;},[&]{scenario.step=2;},[&]{scenario.step=4;},
        [&]{player.instance=nullptr;},[&]{instance.step=2;},[&]{instance.step=4;}
    };
    for(auto const& invalidate:deny)
    {
        Player original=player;GameObject originalObject=object;InstanceScript originalInstance=instance;Scenario originalScenario=scenario;
        invalidate();assert(!AshbringerAcquisition::CanRetrieve(&player,&object));
        assert(script.OnGossipHello(&player,&object)&&ai->GossipHello(&player,true)&&ai->GossipHello(&player,false));
        for(bool owned:{false,true}){player.owned=owned;object.SetLootState(GO_JUST_DEACTIVATED,&player);script.OnLootStateChanged(&object,GO_ACTIVATED,&player);}
        assert(!player.casts&&!instance.credits&&!trigger.despawns&&!object.deletes);
        player=original;object=originalObject;instance=originalInstance;scenario=originalScenario;scenarioManager.scenario=&scenario;
    }
    assert(!AshbringerAcquisition::CanRetrieve(nullptr,&object)&&!AshbringerAcquisition::CanRetrieve(&player,nullptr));
    assert(script.OnGossipHello(nullptr,&object)&&ai->GossipHello(nullptr,true));Unit pet;script.OnLootStateChanged(&object,3,&pet);script.OnLootStateChanged(nullptr,3,&player);
    assert(!player.casts&&!instance.credits);
    for(QuestStatus status:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(bool owned:{false,true})for(bool present:{false,true})
    {
        player.status=status;player.owned=owned;object.trigger=present?&trigger:nullptr;player.casts=0;instance.credits=0;trigger.despawns=0;
        assert(AshbringerAcquisition::CanRetrieve(&player,&object));assert(!script.OnGossipHello(&player,&object)&&!ai->GossipHello(&player,true));
        object.SetLootState(GO_JUST_DEACTIVATED,&player);assert(player.casts==1&&instance.credits==1&&trigger.despawns==uint32(present));
    }
    std::cout<<"PASS29 invalid contexts: script/AI return denial and repeated owned/unowned callbacks stop before retrieval/credit/cleanup; null/nonplayer fail closed. Report-use ignores the return, retaining native generic credit/stealth removal. Eight valid active/complete x carried/empty x missing/present actor contexts preserve native retrieval, no direct grant. Services controlled; delayed context, party recipient eligibility, persisted retry/equip and client/VFX remain open\n";
#endif
}
