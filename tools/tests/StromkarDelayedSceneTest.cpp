#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using uint32=uint32_t;using QuestStatus=uint32;
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(x) assert(x)
enum {CONDITION_ITEM=2,CONDITION_CLASS=15,CONDITION_QUESTREWARDED=8,CONDITION_QUESTTAKEN=9,CONDITION_QUEST_COMPLETE=28,CONDITION_MAPID=22,CONDITION_ALIVE=36,CONDITION_SCENARION_STEP=44,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_REWARDED=6};
struct InstanceMap {uint32 id=42;uint32 GetInstanceId(){return id;}};
struct Map {bool present=true;InstanceMap instance;InstanceMap*ToInstanceMap(){return present?&instance:nullptr;}}world;
struct Scenario {uint32 id=1037,step=9;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenario;
struct Manager {bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct Player {
 bool alive=true,item=true,rewarded=false;uint32 klass=1,map=1539,status=3;
 Player*ToPlayer(){return this;}Player*ToUnit(){return this;}bool IsCreature(){return false;}bool isPet(){return false;}Player*GetOwner(){return nullptr;}
 struct Threat{Player*getHostilTarget(){return nullptr;}}threat;Threat&getThreatManager(){return threat;}
 bool IsAlive(){return alive;}uint32 getClassMask(){return 1u<<(klass-1);}uint32 GetMapId(){return map;}Map*GetMap(){return &world;}bool InInstance(){return world.present;}
 bool HasItemCount(uint32 id,uint32 n,bool bank){assert(id==128910&&n==1&&!bank);return item;}
 uint32 GetQuestStatus(uint32 id){assert(id==41105);return status;}bool GetQuestRewardStatus(uint32 id){assert(id==41105);return rewarded;}
};
using Unit=Player;
struct ConditionSourceInfo {Player*mConditionTargets[2];};
struct Condition {
 uint32 ElseGroup=0,ConditionType=0,ConditionTarget=0,ConditionValue1=0,ConditionValue2=0,ConditionValue3=0,ReferenceId=0;bool NegativeCondition=false;
 bool isLoaded(){return true;}bool Meets(ConditionSourceInfo&s){Player*object=s.mConditionTargets[ConditionTarget];if(!object)return false;bool condMeets=false;
 switch(ConditionType){
#include "SceneConditions.inc"
 default:assert(false);}
 return NegativeCondition?!condMeets:condMeets;}
};
using ConditionList=std::vector<Condition*>;using ConditionReferenceContainer=std::map<uint32,ConditionList>;
struct ConditionMgr {ConditionReferenceContainer ConditionReferenceStore;bool IsObjectMeetToConditionList(ConditionSourceInfo&,ConditionList const&)const;};
#include "SceneElseGroups.inc"
std::vector<Condition>Read(char const*path){std::ifstream in(path);assert(in);std::vector<Condition>result;Condition c;while(in>>c.ElseGroup>>c.ConditionType>>c.ConditionTarget>>c.ConditionValue1>>c.ConditionValue2>>c.ConditionValue3>>c.NegativeCondition)result.push_back(c);return result;}
bool Admit(std::vector<Condition>&rows,Player&p){ConditionList list;for(auto&r:rows)list.push_back(&r);ConditionSourceInfo info{{&p,nullptr}};return ConditionMgr{}.IsObjectMeetToConditionList(info,list);}
int main(int argc,char**argv){
 assert(argc==3);auto before=Read(argv[1]),after=Read(argv[2]);assert(before.size()==14&&after.size()==28);
 uint32 cases=0;
 for(uint32 step=0;step<=11;++step)for(uint32 status:{0u,1u,3u,6u})for(uint32 klass=1;klass<=12;++klass)for(bool item:{false,true})for(bool alive:{false,true}){
  Player p;p.status=status;p.klass=klass;p.item=item;p.alive=alive;scenario.step=step;
  bool base=(status==1||status==3)&&klass==1&&item&&alive;
  assert(Admit(before,p)==(base&&step==9));assert(Admit(after,p)==(base&&(step==9||step==10)));++cases;
 }
 Player p;scenario.step=10;assert(!Admit(before,p)&&Admit(after,p)); // 49109 already set, actual GO sends49110 before delayed cast.
 for(uint32 status:{1u,6u}){p.status=status;p.rewarded=true;assert(!Admit(after,p));}p.status=3;p.rewarded=false;
 p.map=0;assert(!Admit(after,p));p.map=1539;scenario.id=991;assert(!Admit(after,p));scenario.id=1037;
 world.present=false;assert(!Admit(after,p));world.present=true;manager.present=false;assert(!Admit(after,p));manager.present=true;
 std::cout<<"PASS "<<cases<<" native condition/ElseGroup combinations plus completed-stage delayed scene, reward/map/scenario/instance failures. Scenario event advancement, spell/scene/inventory/client services remain separate endpoints.\n";
}
