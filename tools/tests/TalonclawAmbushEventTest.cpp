#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using uint32=uint32_t;
constexpr double M_PI=3.14159265358979323846;
struct ObjectGuid {uint32 value=0;bool operator==(ObjectGuid const& other)const{return value==other.value;}bool operator!=(ObjectGuid const& other)const{return !(*this==other);}};
namespace std {template<>struct hash<ObjectGuid>{size_t operator()(ObjectGuid const& value)const{return value.value;}};}
using GuidSet=std::unordered_set<ObjectGuid>;
enum {CLASS_HUNTER=3,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SCENE_LAUNCH=1,
    SMART_SCRIPT_TYPE_EVENTOBJECT=13,SMART_EVENT_EVENTOBJECT_ONTRIGGER=89,SMART_EVENT_EVENTOBJECT_OFFTRIGGER=91,
    TYPEID_EVENTOBJECT=11,GRID_MAP_TYPE_MASK_EVENTOBJECT=128,PLAYER_FIELD_PLAYER_FLAGS=1,PLAYER_FLAGS_GHOST=1,EO_FLAG_ONE_ACTION=1};
uint32 errors=0;
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
struct Map {uint32 identity=1511;template<class Visitor>void VisitAll(float,float,float,Visitor& visitor){visitor.Visit();}};
struct WorldObject {
    Map* map=nullptr;float position=0;uint32 phase=1,type=0;
    Map* GetMap()const{return map;}uint32 GetMapId(){return map->identity;}uint32 GetTypeId(){return type;}
    bool IsInWorld()const{return true;}float GetPositionX()const{return position;}float GetPositionY()const{return 0;}uint32 GetPhaseMask()const{return phase;}
    bool InSamePhase(WorldObject* other){return phase==other->phase;}
    bool InSamePhase(uint32 mask){return phase==mask;}
    bool IsWithinDistInMap(WorldObject* other,float radius){return map==other->map&&std::fabs(position-other->position)<=radius;}
};
struct Player:WorldObject {
    ObjectGuid guid{1};uint32 instance=42,klass=3,quest=QUEST_STATUS_INCOMPLETE,attemptMist=0,attemptScene=0,mistCasts=0,sceneCasts=0;
    bool alive=true,world=true,contact=true,teleport=false,rewarded=false,failMist=false,failScene=false,partialScene=false,launch=false;
    std::map<uint32,ObjectGuid> auras;
    ObjectGuid GetGUID(){return guid;}uint32 GetGUIDLow(){return guid.value;}uint32 GetInstanceId(){return instance;}
    bool IsInWorld(){return world;}bool CanContact(){return contact;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}
    uint32 getClass(){return klass;}bool GetQuestRewardStatus(uint32){return rewarded;}uint32 GetQuestStatus(uint32){return quest;}
    bool HasAura(uint32 spell,ObjectGuid caster={}){auto found=auras.find(spell);return found!=auras.end()&&(!caster.value||found->second==caster);}
    void RemoveAurasDueToSpell(uint32 spell,ObjectGuid caster={}){if(HasAura(spell,caster))auras.erase(spell);}
    bool HasSceneStatus(uint32 scene,uint32 status){assert(scene==1145&&status==SCENE_LAUNCH);return launch;}
    float GetDistance(WorldObject* other){return std::fabs(position-other->position);}bool HasFlag(uint32,uint32){return false;}
    void TeleportTo(uint32,float,float,float,float){assert(false);}
    void CastSpell(Player*,uint32,bool){assert(false);}
    void Cast(uint32 spell){
        if(spell==198883){++attemptMist;if(!failMist){auras[spell]=guid;++mistCasts;}}
        else{assert(spell==197615);++attemptScene;if(!failScene){auras[spell]=guid;if(!partialScene){launch=true;++sceneCasts;}}}
    }
};
struct EventObjectTemplate {float radius=24;uint32 WorldSafeLocID=0,SpellID=0,Flags=0;};
struct EventObject:WorldObject {
    EventObjectTemplate definition;EventObjectTemplate* eventTemplate=&definition;
    GuidSet affectedPlayers,affectedPermamentPlayers;bool m_isUpdate=false;uint32 entry=108;
    EventObject(){type=TYPEID_EVENTOBJECT;}uint32 GetEntry(){return entry;}
    bool ActivatedForPlayer(Player* player)const;
    void Update(uint32);
};
struct Scenario {uint32 identity=973,step=9;uint32 GetScenarioId(){return identity;}uint32 GetCurrentStep(){return step;}};
struct ScenarioMgr {bool present=true;uint32 instance=42;Scenario scenario;Scenario* GetScenario(uint32 identity){return present&&identity==instance?&scenario:nullptr;}} scenarios;
auto sScenarioMgr=&scenarios;
struct ConditionMgr {
    bool available=true,allow=true;
    std::vector<int> GetConditionsForSmartEvent(uint32 entry,uint32 root,uint32 source){assert(entry==108&&(root==0||root==2)&&source==13);return available?std::vector<int>{1}:std::vector<int>{};}
    bool IsObjectMeetingSmartEventConditions(uint32,uint32 root,uint32,Player*,EventObject*){return allow&&scenarios.scenario.identity==973&&scenarios.scenario.step==(root==0?10u:11u);}
} conditions;
auto sConditionMgr=&conditions;
std::vector<Player*> players;
std::vector<WorldObject*> objects;
using EventObjectMapType=std::vector<EventObject*>;
namespace ObjectAccessor {Player* GetPlayer(WorldObject&,ObjectGuid guid){for(auto player:players)if(player->guid==guid)return player;return nullptr;}}
namespace Trinity {
struct AnyPlayerInObjectRangeCheck {WorldObject* actor;float radius;bool alive;AnyPlayerInObjectRangeCheck(WorldObject* actor,float radius,bool alive):actor(actor),radius(radius),alive(alive){}};
template<class Check>struct PlayerListSearcher {
    WorldObject* actor;std::list<Player*>& result;Check& check;
    PlayerListSearcher(WorldObject* actor,std::list<Player*>& result,Check& check):actor(actor),result(result),check(check){}
    void Visit(){for(auto player:players)if(player->IsWithinDistInMap(actor,check.radius)&&(!check.alive||player->alive))result.push_back(player);}
};
template<class Do>struct WorldObjectWorker {
    uint32 i_mapTypeMask,i_phaseMask;Do const& i_do;
    WorldObjectWorker(WorldObject* player,Do const& callback,uint32 mask):i_mapTypeMask(mask),i_phaseMask(player->GetPhaseMask()),i_do(callback){assert(mask==GRID_MAP_TYPE_MASK_EVENTOBJECT);}
    void Visit(){EventObjectMapType entries;for(auto object:objects)if(object->GetTypeId()==TYPEID_EVENTOBJECT)entries.push_back(static_cast<EventObject*>(object));Visit(entries);}
#include "Worker.inc"
};
template<class Visitor>void VisitNearbyWorldObject(WorldObject*,float,Visitor& visitor){visitor.Visit();}
#include "VisitAll.inc"
}
struct WorldSafeLocsEntry {uint32 MapID;struct {float X,Y,Z,O;}Loc;};
struct Locations {WorldSafeLocsEntry* LookupEntry(uint32){return nullptr;}} sWorldSafeLocsStore;
struct SmartScript {
    EventObject* actor=nullptr;static inline uint32 calls=0,exits=0;
    void OnInitialize(EventObject* event,void*){actor=event;}
    void ProcessEventsFor(uint32 type,Player* player,uint32 entry){
        assert(actor&&entry==actor->entry);++calls;
        if(type==91){++exits;return;}
        assert(type==89);if(entry!=108)return;
        if(sConditionMgr->IsObjectMeetingSmartEventConditions(108,0,13,player,actor))player->Cast(198883);
        if(sConditionMgr->IsObjectMeetingSmartEventConditions(108,2,13,player,actor))player->Cast(197615);
    }
};
struct EventObjectScript {explicit EventObjectScript(char const*){}virtual bool IsTriggerMeets(Player*,EventObject*){return true;}virtual bool OnTrigger(Player*,EventObject*,bool){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogout(Player*){}};
#include "Lifecycle.inc"
#include "Legacy.inc"
struct ScriptManager {
    EventObjectScript* active=nullptr;
    bool IsEOMeetConditions(Player* player,EventObject* event){return active->IsTriggerMeets(player,event);}
    bool OnEventObject(Player* player,EventObject* event,bool enter){return active->OnTrigger(player,event,enter);}
} scripts;
auto sScriptMgr=&scripts;
#include "Activated.inc"
#include "EventUpdate.inc"
struct Fixture {
    Map map;Player player,party;EventObject event;eventobject_talonclaw_ambush scoped;SmartEventObject legacy;player_talonclaw_ambush_event recovery;
    Fixture(){scenarios={};conditions={};errors=0;player.map=party.map=event.map=&map;party.guid={2};party.position=100;players={&player,&party};objects={&event};
#ifdef BASELINE
        scripts.active=&legacy;
#else
        scripts.active=&scoped;
#endif
    }
    ~Fixture(){recovery.OnLogout(&player);recovery.OnLogout(&party);}
    void Tick(uint32 diff=500){
#ifndef BASELINE
        for(auto member:players)recovery.OnUpdate(member,diff);
#endif
        event.Update(diff);
    }
};
int main(){
    _set_abort_behavior(0,_CALL_REPORTFAULT);
    {
        Fixture fixture;fixture.Tick();scenarios.scenario.step=10;fixture.Tick();
        assert(fixture.player.mistCasts==1);
        for(uint32 repeat=0;repeat<10;++repeat)fixture.Tick(16);assert(fixture.player.mistCasts==1);
        scenarios.scenario.step=11;fixture.Tick(500);assert(fixture.player.sceneCasts==1&&!fixture.player.HasAura(198883));
        for(uint32 repeat=0;repeat<10;++repeat)fixture.Tick();assert(fixture.player.sceneCasts==1);
        fixture.player.RemoveAurasDueToSpell(197615,fixture.player.guid);fixture.player.launch=false;
        fixture.Tick();assert(fixture.player.sceneCasts==1);
        fixture.player.position=30;fixture.Tick();fixture.player.position=0;fixture.Tick();assert(fixture.player.sceneCasts==1);
        fixture.party.position=0;fixture.Tick();assert(fixture.party.sceneCasts==1&&fixture.player.sceneCasts==1);
    }
    for(bool scene:{false,true}){
        Fixture fixture;scenarios.scenario.step=scene?11:10;fixture.player.failMist=!scene;fixture.player.failScene=scene;
        fixture.Tick();assert(!fixture.player.mistCasts&&!fixture.player.sceneCasts);
        fixture.player.failMist=fixture.player.failScene=false;fixture.Tick();
        assert((scene?fixture.player.sceneCasts:fixture.player.mistCasts)==1);
    }
    {
        Fixture fixture;scenarios.scenario.step=11;fixture.player.failScene=true;fixture.Tick();fixture.player.failScene=false;
        fixture.Tick(16);assert(fixture.player.attemptScene==1);fixture.Tick(484);assert(fixture.player.sceneCasts==1);
    }
    {
        Fixture fixture;scenarios.scenario.step=11;fixture.event.affectedPlayers.insert(fixture.player.guid);objects.clear();fixture.Tick();assert(!fixture.player.sceneCasts);
        objects.push_back(&fixture.event);fixture.Tick();assert(fixture.player.sceneCasts==1);
    }
    {
        Fixture fixture;scenarios.scenario.step=11;fixture.player.partialScene=true;fixture.Tick();
        assert(errors==1&&!fixture.player.HasAura(197615)&&!fixture.player.sceneCasts);
        fixture.player.partialScene=false;fixture.Tick();assert(fixture.player.sceneCasts==1);
    }
    {
        Fixture fixture;scenarios.scenario.step=10;fixture.Tick();fixture.player.quest=QUEST_STATUS_NONE;fixture.Tick();assert(!fixture.player.HasAura(198883));
        fixture.player.quest=QUEST_STATUS_INCOMPLETE;fixture.Tick();assert(fixture.player.mistCasts==2);
        fixture.player.instance=scenarios.instance=43;fixture.Tick();assert(fixture.player.mistCasts==3);
        scenarios.scenario.step=11;fixture.Tick();assert(fixture.player.sceneCasts==1);
        fixture.recovery.OnLogout(&fixture.player);assert(!fixture.player.HasAura(197615));fixture.Tick();assert(fixture.player.sceneCasts==2);
    }
    for(uint32 failure=0;failure<12;++failure){
        Fixture fixture;scenarios.scenario.step=11;
        switch(failure){case 0:fixture.player.alive=false;break;case 1:fixture.player.quest=0;break;case 2:fixture.player.rewarded=true;break;
            case 3:fixture.player.klass=1;break;case 4:fixture.player.teleport=true;break;case 5:fixture.player.contact=false;break;
            case 6:fixture.player.phase=2;break;case 7:fixture.player.position=25;break;case 8:scenarios.present=false;break;
            case 9:scenarios.scenario.identity=1099;break;case 10:conditions.available=false;break;case 11:conditions.allow=false;break;}
        fixture.Tick();assert(!fixture.player.sceneCasts&&!fixture.event.affectedPlayers.count(fixture.player.guid));
    }
    {
        Fixture fixture;scenarios.scenario.step=11;fixture.player.auras[197615]={99};fixture.Tick();assert(!fixture.player.sceneCasts);
        fixture.recovery.OnLogout(&fixture.player);assert(fixture.player.HasAura(197615,{99}));
    }
    {
        Fixture fixture;scenarios.scenario.step=11;fixture.player.quest=QUEST_STATUS_COMPLETE;fixture.Tick();assert(fixture.player.sceneCasts==1);
        fixture.map.identity=1220;fixture.Tick();assert(!fixture.player.HasAura(197615));
    }
    {
        Fixture fixture;scenarios.scenario.step=10;fixture.player.auras[198883]={99};fixture.Tick();fixture.recovery.OnLogout(&fixture.player);
        assert(!fixture.player.mistCasts&&fixture.player.HasAura(198883,{99}));
    }
    {
        Fixture fixture;fixture.event.entry=109;fixture.player.klass=1;uint32 before=SmartScript::calls;fixture.Tick();assert(SmartScript::calls==before+1);
    }
    std::cout<<"PASS: extracted native EventObject Update/cache and scoped controller recover early entry, in-place10->11, failed/partial casts, independent party/re-entry, abandon/logout/new-instance cleanup and admission failures without touching foreign auras. Spatial/cast/scenario services controlled; native action/scene/client rendering verified separately.\n";
}
