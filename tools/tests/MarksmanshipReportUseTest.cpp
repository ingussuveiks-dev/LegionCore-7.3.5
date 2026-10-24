#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
using uint32 = uint32_t;
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_DEBUG(...) ((void)0)
enum { CLASS_WARRIOR=1,CLASS_HUNTER=3,CLASS_PRIEST=5,CLASS_DEATH_KNIGHT=6,CLASS_MAGE=8,CLASS_WARLOCK=9,CLASS_DRUID=11,
    QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=2,
    GO_JUST_DEACTIVATED=3,SMART_EVENT_GOSSIP_HELLO=64,SMART_EVENT_RESPAWN=11,SMART_EVENT_GO_STATE_CHANGED=70,
    CRITERIA_TYPE_OWN_ITEM=36,CRITERIA_TYPE_USE_GAMEOBJECT=68,GAMEOBJECT_TYPE_CHEST=3,GAMEOBJECT_TYPE_GARRISON_SHIPMENT=38,
    SPELL_AURA_MOD_STEALTH=16,SPELL_AURA_MOD_INVISIBILITY=18 };
struct Player;
using Unit = Player;
struct GameObject;
struct Map { GameObject* object=nullptr; GameObject* GetGameObject(uint32); } worldMap, otherMap;
struct Scenario { uint32 id=972,step=7; uint32 GetScenarioId() const{return id;} uint32 GetCurrentStep() const{return step;} };
struct ScenarioManager { Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;} } scenarioManager;
auto* sScenarioMgr=&scenarioManager;
struct GameObjectAI {
    explicit GameObjectAI(GameObject*){}
    virtual ~GameObjectAI()=default;
    virtual bool GossipHello(Player*,bool){return false;}
    virtual bool GossipUse(Player*){return false;}
    virtual void InitializeAI(){}
    virtual void Reset(){}
    virtual void UpdateAI(uint32){}
    virtual void OnStateChanged(uint32,Unit*){}
};
struct SmartScript {
    uint32 initialized=0,resets=0,elapsed=0,respawns=0,gossip=0,state=0;
    Unit* invoker=nullptr;GameObject* base=nullptr;
    void OnInitialize(GameObject* object){++initialized;base=object;}
    void OnReset(){++resets;}
    void OnUpdate(uint32 diff){elapsed+=diff;}
    void ProcessEventsFor(uint32 event,Unit* player=nullptr,uint32=0,uint32=0,bool=false,void const* = nullptr,GameObject* = nullptr){
        invoker=player;
        if(event==SMART_EVENT_GOSSIP_HELLO)++gossip;
        else if(event==SMART_EVENT_RESPAWN)++respawns;
        else {assert(event==SMART_EVENT_GO_STATE_CHANGED);++state;}
    }
};
struct SmartGameObjectAI : GameObjectAI {
    GameObject* const go;SmartScript mScript;
    explicit SmartGameObjectAI(GameObject*);
    ~SmartGameObjectAI() override;
    SmartScript* GetScript();
    void InitializeAI() override;void Reset() override;void UpdateAI(uint32) override;
    bool GossipHello(Player*,bool) override;bool GossipUse(Player*) override;
    void OnStateChanged(uint32,Unit*) override;
};
struct GameObject {
    uint32 entry=248419,flags=0,lootState=0,mapId=1489,scriptId=1;
    bool spawned=true,world=true;Map* map=&worldMap;GameObjectAI* ai=nullptr;
    bool IsInWorld() const{return world;}
    uint32 GetEntry() const{return entry;}uint32 GetMapId() const{return mapId;}
    uint32 GetScriptId() const{return scriptId;}uint32 GetGUIDLow() const{return 10;}
    Map* GetMap() const{return map;}bool isSpawned() const{return spawned;}
    bool HasFlag(uint32,uint32 mask) const{return (flags&mask)!=0;}
    uint32 getLootState() const{return lootState;}
    void SetLootState(uint32,Player*){assert(false);}
    GameObjectAI* AI() const{return ai;}
    std::string GetAIName() const{return "SmartGameObjectAI";}
    bool IsWithinDistInMap(Player*,float) const;
    float GetInteractionDistance() const{return 10;}
    struct Info {uint32 type=GAMEOBJECT_TYPE_CHEST;bool IsUsableMounted()const{return false;}} info;
    Info const* GetGOInfo() const{return &info;}
    uint32 GetGoType() const{return info.type;}
    void Use(Player*){assert(false);}
};
GameObject* Map::GetGameObject(uint32 guid){return guid==10?object:nullptr;}
struct Player {
    uint32 cls=CLASS_HUNTER,quest=40419,status=QUEST_STATUS_INCOMPLETE,instance=42,uses=0,owns=0,auraRemovals=0;
    bool alive=true,teleport=false,rewarded=false,owned=true,bank=false,phase=true,visible=true,los=true;
    bool remote=false,mounted=false,vehicle=false;float distance=2;Map* map=&worldMap;
    bool world=true,contact=true;
    bool IsInWorld() const{return world;}bool CanContact() const{return contact;}
    bool isGameMaster() const{return false;}Player* ToPlayer(){return this;}
    bool IsAlive() const{return alive;}bool IsBeingTeleported() const{return teleport;}
    uint32 getClass() const{return cls;}uint32 GetInstanceId() const{return instance;}uint32 GetGUIDLow() const{return 7;}
    uint32 GetQuestStatus(uint32 id) const{return id==quest?status:0;}
    bool GetQuestRewardStatus(uint32 id) const{return id==quest&&rewarded;}
    Map* GetMap() const{return map;}
    bool InSamePhase(GameObject*) const{return phase;}bool canSeeOrDetect(GameObject*) const{return visible;}
    bool IsWithinLOSInMap(GameObject*) const{return los;}float GetExactDist(GameObject*) const{return distance;}
    bool HasItemCount(uint32,uint32,bool includeBank) const{return owned||(includeBank&&bank);}
    Player* GetUnitBeingMoved(){return remote?nullptr:this;}
    bool IsOnVehicle(Player*) const{return vehicle;}bool IsMounted() const{return mounted;}bool NeedDismount() const{return false;}
    void CastSpell(GameObject*,uint32,bool){assert(false);}
    void UpdateAchievementCriteria(uint32 type,uint32,uint32=0){if(type==CRITERIA_TYPE_USE_GAMEOBJECT)++uses;else{assert(type==CRITERIA_TYPE_OWN_ITEM);++owns;}}
    void RemoveAurasByType(uint32 type){assert(type==16||type==18);++auraRemovals;}
    struct Garrison {void CompleteShipments(GameObject*){assert(false);}};
    Garrison* GetGarrison(){return nullptr;}
};
bool GameObject::IsWithinDistInMap(Player* player,float range)const{return player->map==map&&player->distance<=range;}
#include "SmartLifecycle.inc"
#include "StateChanged.inc"
namespace ArtifactAcquisition {
#include "Acquisition.inc"
}
namespace TalonclawWorldPickup {void Retire(GameObject*,uint32,Unit*){}}
struct GameObjectScript {
    explicit GameObjectScript(char const*){}
    virtual ~GameObjectScript()=default;
    virtual bool OnGossipHello(Player*,GameObject*){return false;}
    virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}
    virtual void OnLootStateChanged(GameObject*,uint32,Unit*){}
};
#include "Binding.inc"
#include "BindingChoice.inc"
PickupBinding binding;
struct ScriptManager {
    GameObjectAI* GetGameObjectAI(GameObject* object){return object->scriptId?binding.GetAI(object):nullptr;}
    bool OnGossipHello(Player* player,GameObject* object){return binding.OnGossipHello(player,object);}
} scriptManager;
auto* sScriptMgr=&scriptManager;
struct Factory {
    std::string key()const{return "SmartGameObjectAI";}
    GameObjectAI* Create(GameObject* object)const{return new SmartGameObjectAI(object);}
} factory;
struct Registry {Factory* GetRegistryItem(std::string const&){return &factory;}} registry;
auto* sGameObjectAIRegistry=&registry;
struct NullGameObjectAI : GameObjectAI {explicit NullGameObjectAI(GameObject* object):GameObjectAI(object){}};
namespace FactorySelector {
#include "SelectAI.inc"
}
namespace WorldPackets {namespace GameObject {struct GameObjReportUse {uint32 Guid=10;};}}
struct WorldSession {
    Player* _player=nullptr;Player* GetPlayer(){return _player;}
    void HandleGameobjectReportUse(WorldPackets::GameObject::GameObjReportUse&);
};
#include "ReportUse.inc"
struct Fixture {
    GameObject object;Player player;std::unique_ptr<GameObjectAI> ai;WorldSession session;
    Fixture(){scenarioManager={};worldMap.object=&object;otherMap.object=nullptr;ai.reset(FactorySelector::SelectGameObjectAI(&object));object.ai=ai.get();session._player=&player;}
    SmartScript& Script(){auto* smart=dynamic_cast<SmartGameObjectAI*>(ai.get());assert(smart);return *smart->GetScript();}
    void Report(uint32 guid=10){WorldPackets::GameObject::GameObjReportUse packet{guid};session.HandleGameobjectReportUse(packet);}
};
int main(){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    {
        Fixture fixture;fixture.player.phase=false;fixture.Report();
        assert(fixture.Script().gossip==0);
        assert(fixture.player.uses==1&&fixture.player.auraRemovals==2);
    }
    for(uint32 invalid=0;invalid<20;++invalid){
        Fixture fixture;
        switch(invalid){
            case 0:fixture.player.alive=false;break;
            case 1:fixture.player.teleport=true;break;
            case 2:fixture.player.cls=CLASS_PRIEST;break;
            case 3:fixture.player.quest=41631;break;
            case 4:fixture.player.status=0;break;
            case 5:fixture.player.status=5;break;
            case 6:fixture.player.rewarded=true;break;
            case 7:fixture.player.visible=false;break;
            case 8:fixture.player.los=false;break;
            case 9:fixture.player.distance=6;break;
            case 10:fixture.object.flags=GO_FLAG_NOT_SELECTABLE;break;
            case 11:fixture.object.flags=GO_FLAG_IN_USE;break;
            case 12:fixture.object.spawned=false;break;
            case 13:fixture.object.lootState=GO_JUST_DEACTIVATED;break;
            case 14:scenarioManager.present=false;break;
            case 15:scenarioManager.scenario.id=1136;break;
            case 16:scenarioManager.scenario.step=6;break;
            case 17:scenarioManager.scenario.step=9;break;
            case 18:fixture.player.instance=43;break;
            case 19:fixture.object.mapId=1;break;
        }
        fixture.Report();assert(fixture.Script().gossip==0&&fixture.Script().state==0&&fixture.player.owns==0);
        assert(fixture.ai->GossipHello(&fixture.player,true));
        fixture.ai->GossipUse(&fixture.player);assert(fixture.Script().gossip==0);
    }
    for(uint32 stage=0;stage<=14;++stage)for(uint32 status:{0u,1u,3u,5u,6u})for(uint32 ownership=0;ownership<3;++ownership){
        Fixture fixture;scenarioManager.scenario.step=stage;fixture.player.status=status;
        fixture.player.owned=ownership==1;fixture.player.bank=ownership==2;fixture.Report();
        bool allowed=(stage==7||stage==8)&&(status==1||status==3);
        assert(fixture.Script().gossip==uint32(allowed)&&fixture.Script().state==0&&fixture.player.owns==0);
        assert(fixture.object.lootState==0);
    }
    {
        Fixture fixture;fixture.ai->InitializeAI();fixture.ai->Reset();fixture.ai->UpdateAI(123);
        fixture.ai->OnStateChanged(3,&fixture.player);fixture.Report();fixture.Report();
        auto& script=fixture.Script();
        assert(script.initialized==1&&script.base==&fixture.object&&script.respawns==1&&script.resets==1&&script.elapsed==123);
        assert(script.state==1&&script.gossip==2&&script.invoker==&fixture.player);
        assert(fixture.ai->GossipHello(nullptr,false)&&script.gossip==2);
        fixture.player.phase=false;fixture.Report();assert(script.gossip==2);
        fixture.player.phase=true;fixture.Report();assert(script.gossip==3);
    }
    {
        Fixture fixture;fixture.Report(99);assert(!fixture.Script().gossip&&!fixture.player.uses);
        fixture.player.remote=true;fixture.Report();assert(!fixture.Script().gossip&&!fixture.player.uses);
        fixture.player.remote=false;fixture.player.distance=11;fixture.Report();assert(!fixture.Script().gossip&&!fixture.player.uses);
    }
    {
        Fixture fixture;fixture.object.entry=248834;fixture.player.phase=false;fixture.Report();
        assert(fixture.Script().gossip==1);
        fixture.object.scriptId=0;std::unique_ptr<GameObjectAI> native(FactorySelector::SelectGameObjectAI(&fixture.object));
        assert(dynamic_cast<SmartGameObjectAI*>(native.get()));
    }
    std::cout<<"PASS: actual report-use packet handler, AI selection and inherited SmartAI lifecycle preserve native dispatch while scoped Thasdorah gossip rejects invisible/dead/early/invalid interactions, without deactivation or extra credit\n";
}
