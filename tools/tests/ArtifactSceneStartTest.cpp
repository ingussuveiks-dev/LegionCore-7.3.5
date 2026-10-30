#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
using uint32=uint32_t;using int64=int64_t;using QuestStatus=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(value) assert(value)
enum {CONDITION_ITEM=2,CONDITION_CLASS=15,CONDITION_MAPID=22,CONDITION_SCENARION_STEP=44,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28,CONDITION_QUESTREWARDED=8,CONDITION_ALIVE=36,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_REWARDED=6};
enum {SPELL_CAST_OK,SPELL_FAILED_DONT_REPORT,SPELL_FAILED_CASTER_AURASTATE,SPELL_FAILED_BAD_TARGETS,CONDITION_SOURCE_TYPE_SPELL=17,SMART_ACTION_INVOKER_CAST=85,SMART_TARGET_ACTION_INVOKER_PLAYER=130};
#include "CastFlags.inc"
uint32 expectedQuest,expectedItem,expectedClass,expectedMap,expectedScenario,firstStage;
struct Player;struct Unit;
struct ObjectGuid {Unit* unit=nullptr;};
struct InstanceMap {uint32 id=42;uint32 GetInstanceId()const{return id;}} instanceMap;
struct Map {bool instanced=true;InstanceMap* ToInstanceMap(){return instanced?&instanceMap:nullptr;}} worldMap;
struct Unit {
 virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}
 uint32 mapId=expectedMap,classMask=expectedClass;bool alive=true,available=true,world=true;
 unsigned attempts=0,accepted=0;bool nativeCastWorks=true;std::vector<std::function<void()>> queue;
 uint32 getClassMask()const{return classMask;}uint32 GetMapId()const{return mapId;}bool IsAlive()const{return alive;}
 bool IsCreature()const{return false;}bool isPet()const{return false;}Unit* GetOwner()const{return nullptr;}
 struct Threat{Unit* getHostilTarget()const{return nullptr;}} threat;Threat& getThreatManager(){return threat;}
 bool InInstance()const{return worldMap.instanced;}Map* GetMap(){return &worldMap;}ObjectGuid GetGUID(){return {this};}
 bool IsInWorld(){return world;}bool InSamePhase(Unit*){return true;}bool IsWithinDistInMap(Unit*,float){return true;}
 bool HasQuestObjectScope(){return false;}bool CanTargetQuestObject(Unit*){return true;}bool HasAura(uint32){return false;}
 void InterruptNonMeleeSpells(bool){}template<class F>void AddDelayedEvent(uint32 delay,F&& f){assert(delay==200);queue.emplace_back(f);}
 void Flush(){auto pending=std::move(queue);queue.clear();for(auto& f:pending)f();}
 void CastSpell(Unit*,uint32,bool);
};
using WorldObject=Unit;
struct Session {void SendNotification(uint32){assert(false);}};
struct Player:Unit {
 uint32 quest=expectedQuest,status=3;bool rewarded=false,owned=true,bank=false;
 Player* ToPlayer()override{return this;}uint32 GetGUID()const{return 1;}
 QuestStatus GetQuestStatus(uint32 value)const{return value==quest?status:0;}
 bool GetQuestRewardStatus(uint32 value)const{return value==quest&&rewarded;}
 bool HasItemCount(uint32 item,uint32 count,bool includeBank)const{assert(item==expectedItem&&count==1);return owned||(bank&&includeBank);}
 Session* GetSession(){return nullptr;}
};
struct Scenario {uint32 id=expectedScenario,step=firstStage;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}};
struct ScenarioManager {Scenario scenario;bool present=true;Scenario* GetScenario(uint32 instance){return present&&instance==42?&scenario:nullptr;}} scenarioManager;
auto sScenarioMgr=&scenarioManager;
struct Condition;
struct ConditionSourceInfo {Unit* mConditionTargets[2]{};Condition* mLastFailedCondition=nullptr;explicit ConditionSourceInfo(Unit* unit){mConditionTargets[0]=unit;}};
struct Condition {
 uint32 ElseGroup=0,ConditionType=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0,ConditionTarget=0,ErrorTextId=0;bool NegativeCondition=false;
 bool isLoaded()const{return true;}
 bool Meets(ConditionSourceInfo& sourceInfo){Unit* object=sourceInfo.mConditionTargets[0];if(!object)return false;bool condMeets=false;
 switch(ConditionType){
#include "Conditions.inc"
 default:assert(false);
 }bool result=NegativeCondition?!condMeets:condMeets;if(!result)sourceInfo.mLastFailedCondition=this;return result;}
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {
 ConditionReferenceContainer ConditionReferenceStore;std::map<uint32,ConditionList> spells;bool baseline=false;
 ConditionList GetConditionsForNotGroupedEntry(uint32 source,uint32 spell){assert(source==17);return baseline?ConditionList{}:spells[spell];}
 bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;
 bool IsObjectMeetToConditions(ConditionSourceInfo& info,ConditionList const& rows){return rows.empty()||IsObjectMeetToConditionList(info,rows);}
} conditionManager;
auto sConditionMgr=&conditionManager;
#include "ElseGroups.inc"
struct SpellInfo {uint32 Id=0;};struct SpellMgr {SpellInfo info;SpellInfo const* GetSpellInfo(uint32 id){info.Id=id;return &info;}} spellManager;
auto sSpellMgr=&spellManager;
struct Spell {
 Unit* m_caster;SpellInfo const* m_spellInfo;struct Targets{Unit* target;Unit* GetObjectTarget(){return target;}}m_targets;
 uint32 CheckSource(){
#include "SpellConditionBridge.inc"
 return SPELL_CAST_OK;
 }
};
void Unit::CastSpell(Unit* target,uint32 id,bool triggered){assert(target==this&&!triggered);++attempts;SpellInfo info{id};Spell spell{this,&info,{target}};if(spell.CheckSource()==SPELL_CAST_OK&&nativeCastWorks)++accepted;}
struct ObjectAccessor {
 static Unit* GetUnit(Unit const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit:nullptr;}
 static Unit* GetWorldObject(Unit const&,ObjectGuid guid){return guid.unit&&guid.unit->available?guid.unit:nullptr;}
};
// Owner resolution is a controlled boundary here; Test-ScriptPetCredit executes
// the real helpers separately. Every altered admission case uses the real Player.
Player* GetScriptCreditPlayer(Unit* unit){return unit?unit->ToPlayer():nullptr;}
Unit* GetScriptCreditCaster(Unit* unit,SpellInfo const*){return unit;}
using ObjectList=std::list<Unit*>;
struct Event {
 struct{struct{uint32 spell=0,flags=0;}cast;}action;
 struct{struct{uint32 dist=0;}playerDistance;}target;
 uint32 GetTargetType()const{return 7;}
};
bool IsUnit(Unit* unit){return unit!=nullptr;}
struct Script {
 Unit* last;Unit* GetLastInvoker(){return last;}Unit* GetBaseObject(){return last;}
 ObjectList* GetTargets(Event const&,Unit*){return new ObjectList{last};}
 void Start(uint32 spell){Event e;e.action.cast.spell=spell;Unit* unit=last;switch(SMART_ACTION_INVOKER_CAST){
#include "InvokerCast.inc"
 }}
};
int main(int argc,char** argv){
 _set_abort_behavior(0,_CALL_REPORTFAULT);assert(argc==3);auto id=uint32(std::stoul(argv[2]));
 switch(id){case 190435:expectedQuest=38990;expectedItem=128292;expectedClass=32;expectedMap=1480;expectedScenario=901;firstStage=6;break;
 case 196023:expectedQuest=11997;expectedItem=128820;expectedClass=128;expectedMap=1480;expectedScenario=957;firstStage=4;break;
 case 200152:expectedQuest=40740;expectedItem=128402;expectedClass=32;expectedMap=1489;expectedScenario=940;firstStage=9;break;
 case 199502:expectedQuest=40570;expectedItem=128940;expectedClass=512;expectedMap=1528;expectedScenario=983;firstStage=6;break;
 case 217814:expectedQuest=42765;expectedItem=128938;expectedClass=512;expectedMap=1625;expectedScenario=1137;firstStage=7;break;default:assert(false);}
 std::ifstream input(argv[1]);assert(input);std::vector<Condition> rows(28);for(auto& c:rows){uint32 target;assert(input>>c.ElseGroup>>c.ConditionType>>target>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition);assert(!target);conditionManager.spells[id].push_back(&c);}std::string tail;assert(!(input>>tail));
 auto reset=[](){worldMap.instanced=true;instanceMap.id=42;scenarioManager={};};reset();
 {conditionManager.baseline=true;Player player;Script script{&player};script.Start(id);assert(player.queue.size()==1&&!player.attempts);player.status=0;player.mapId=1;player.Flush();assert(player.accepted==1);conditionManager.baseline=false;}
 for(uint32 step=0;step<=14;++step)for(uint32 status:{0u,1u,3u,5u,6u}){reset();Player player;Script script{&player};script.Start(id);assert(player.queue.size()==1&&!player.attempts);scenarioManager.scenario.step=step;player.status=status;player.Flush();assert(player.attempts==1);assert(player.accepted==unsigned((status==1||status==3)&&(step==firstStage||step==firstStage+1)));}
 for(unsigned invalid=0;invalid<13;++invalid){reset();Player player;Script script{&player};script.Start(id);assert(player.queue.size()==1);switch(invalid){case 0:player.alive=false;break;case 1:player.owned=false;break;case 2:player.owned=false;player.bank=true;break;case 3:player.mapId=1;break;case 4:player.classMask=1;break;case 5:player.quest=1;break;case 6:player.status=QUEST_STATUS_REWARDED;player.rewarded=true;break;case 7:scenarioManager.present=false;break;case 8:scenarioManager.scenario.id=1;break;case 9:instanceMap.id=43;break;case 10:worldMap.instanced=false;break;case 11:player.available=false;break;case 12:player.nativeCastWorks=false;break;}player.Flush();assert(!player.accepted);assert(player.attempts==unsigned(invalid!=11));}
 reset();{Player player;Script script{&player};script.Start(id);player.owned=false;player.Flush();assert(!player.accepted);player.owned=true;script.Start(id);player.Flush();assert(player.accepted==1&&player.attempts==2);player.mapId=1;script.Start(999);player.Flush();assert(player.accepted==2);} // unrelated source17 remains native
 reset();{Unit pet;Script script{&pet};script.Start(id);pet.Flush();assert(pet.attempts==1&&!pet.accepted);}
 std::cout<<"PASS actual native action85 delayed200ms -> ordinary CastSpell -> CheckCast source17 bridge + native conditions/ElseGroups, spell"<<id<<": missing-rule stale baseline,75 late stage/status cases,13 changed contexts, bank/item loss, native failure/new request and unrelated spell; time/world/effect/owner endpoints controlled.\n";
}
