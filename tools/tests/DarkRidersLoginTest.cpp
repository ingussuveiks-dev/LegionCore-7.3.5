#include "ScenarioResume.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using int8=int8_t;
constexpr int CLASS_WARLOCK=9,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
int errors=0;
struct CreatureData{uint32 id=100812,mapid=0;float posX=-11024.6f,posY=-2006.07f,posZ=25.1777f,orientation=5.46723f;std::set<uint32>PhaseID;};
struct ObjectManager{CreatureData revil;bool missing=false;CreatureData const* GetCreatureData(uint32 id){assert(id==373228);return missing?nullptr:&revil;}}objectManager;auto sObjectMgr=&objectManager;
struct MapManager{static bool IsValidMapCoord(uint32,float x,float y,float z,float o){return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o);}};
struct QuestObjective{uint32 ObjectID;int8 StorageIndex;};
struct Quest{std::vector<QuestObjective> objectives{{103473,0},{100813,3}};auto const& GetObjectives()const{return objectives;}};
struct QuestStore{Quest q;Quest const* GetQuestTemplate(uint32 id){return id==40623?&q:nullptr;}}questStore;auto sQuestDataStore=&questStore;
struct Player;
struct Map{bool added=true,garrison=false;int additions=0;bool AddPlayerToMap(Player*){++additions;return added;}bool IsGarrison(){return garrison;}};
struct AreaTriggerStruct{uint32 target_mapId;float target_X,target_Y,target_Z;};
struct AreaData{AreaTriggerStruct const*GetGoBackTrigger(uint32){return nullptr;}}areaData;auto sAreaTriggerDataStore=&areaData;
struct Player{
 uint32 mapId=1533,cls=9,m_scenarioLoginId=0,m_scenarioLoginInstance=0,selector=0;
 ScenarioResume m_scenarioResume;
 int status=1,claim=1,complete=1;bool rewarded=false,access=false,teleportSuccess=true;
 uint32 m_homebindMapId=1;float m_homebindX=1352.49f,m_homebindY=-4396.55f,m_homebindZ=29.129f;
 uint32 teleports=0,destination=9999;float x=0,y=0,z=0,o=0;Map map;
 uint32 GetMapId(){return mapId;}int getClass(){return cls;}bool IsQuestRewarded(uint32 id){assert(id==40623);return rewarded;}
 int GetQuestStatus(uint32 id){assert(id==40623);return status;}uint32 GetInstanceId(){return 1;}
 int32 GetQuestObjectiveData(uint32,uint32)const;int32 GetQuestObjectiveData(Quest const*,int8 index)const{return index==0?claim:complete;}
 bool TeleportTo(uint32 target,float a,float b,float c,float d){++teleports;destination=target;x=a;y=b;z=c;o=d;return teleportSuccess;}
 void SetScenarioId(uint32 id){selector=id;}bool ExitCompletedDarkRidersLogin();
 bool ExitCompletedBrokenShoreLogin(){return false;} // This fixture is scoped to map1533.
 Map*GetMap(){return &map;}bool CheckInstanceLoginValid(){return access;}float GetOrientation(){return 2;}uint32 GetGUIDLow(){return 1;}
};
#include "ObjectiveLookup.inc"
#include "CompletedExit.inc"
void login(Player* player){
#include "PostAddAdmission.inc"
}
void oldLogin(Player*player){
#include "OldPostAddAdmission.inc"
}
int main(){
 Player old;oldLogin(&old);assert(old.map.additions==1&&old.teleports==1&&old.destination==1&&errors==1);
 for(int state:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE}){
  Player p;p.status=state;p.complete=state==QUEST_STATUS_COMPLETE?1:0;p.m_scenarioLoginId=988;p.m_scenarioLoginInstance=42;p.selector=988;
  p.m_scenarioResume.Read("1 1533 988 42 1000 -11017 -1996 24 1");
  errors=0;login(&p);assert(p.map.additions==1&&p.destination==0&&p.teleports==1&&!errors);
  assert(p.x==objectManager.revil.posX&&p.y==objectManager.revil.posY&&p.z==objectManager.revil.posZ);
  assert(!p.selector&&!p.m_scenarioLoginId&&!p.m_scenarioLoginInstance&&p.m_scenarioResume.Write().empty());
  assert(p.status==state&&p.claim==1&&p.complete==(state==1?1:0)); // routing does not award or reset quests
 }
 // Both zero checkpoint and a consumed temporary selector are supported.
 Player fresh;login(&fresh);assert(fresh.destination==0);
 Player valid;valid.access=true;login(&valid);assert(!valid.teleports); // live accepted run retained
 for(int mode=0;mode<10;++mode){Player p;
  switch(mode){case 0:p.mapId=1522;break;case 1:p.cls=6;break;case 2:p.m_scenarioLoginId=1014;break;
   case 3:p.rewarded=true;break;case 4:p.status=0;break;case 5:p.claim=0;break;
   case 6:objectManager.missing=true;break;case 7:objectManager.revil.id=999;break;
   case 8:objectManager.revil.mapid=1;break;case 9:objectManager.revil.PhaseID={5714};break;}
  assert(!p.ExitCompletedDarkRidersLogin()&&!p.teleports);objectManager=ObjectManager{};
 }
 Player failure;failure.teleportSuccess=false;assert(!failure.ExitCompletedDarkRidersLogin()&&failure.teleports==1);
 Player failedExit;failedExit.teleportSuccess=false;errors=0;login(&failedExit);assert(failedExit.teleports==2&&failedExit.destination==1&&errors==1);
 std::cout<<"PASS: actual post-AddPlayer admission reproduces completed-run home fallback and routes native claimed/complete Dark Riders to outdoor Revil; real objective lookup, consumed/zero selectors, accepted live run and quest/class/map/record/data/failure gates preserved\n";
}
