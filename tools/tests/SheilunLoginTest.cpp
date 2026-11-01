#include "ScenarioResume.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
#include <limits>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int8=int8_t;using int32=int32_t;
constexpr double M_PI=3.14159265358979323846;
constexpr int CLASS_MONK=10,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
constexpr uint8 INVENTORY_SLOT_BAG_0=255;
#include "Slots.inc"
unsigned errors=0,logs=0,cases=0;
#define TC_LOG_INFO(...) (++logs)
#define TC_LOG_ERROR(...) (++errors)
struct ObjectGuid {uint64 low=1,high=2;bool operator==(ObjectGuid const&o)const{return low==o.low&&high==o.high;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}};
struct Item {uint32 entry=128937,count=1;ObjectGuid owner;uint32 GetEntry()const{return entry;}uint32 GetCount()const{return count;}ObjectGuid GetOwnerGUID()const{return owner;}};
struct Bag {std::vector<Item*>items=std::vector<Item*>(4);uint32 GetBagSize(){return uint32(items.size());}Item*GetItemByPos(uint32 n){return items.at(n);}};
struct Player;
struct Map {bool added=true,garrison=false;unsigned additions=0;bool AddPlayerToMap(Player*){++additions;return added;}bool IsGarrison(){return garrison;}};
struct Scenario {Map*map=nullptr;uint32 id=1007;Map*GetMap(){return map;}uint32 GetScenarioId(){return id;}};
struct ScenarioManager {Scenario*value=nullptr;Scenario*GetScenario(uint32){return value;}}scenarios;auto sScenarioMgr=&scenarios;
struct WorldSafeLocsEntry {uint16 MapID=1514;struct {float X=937.987854f,Y=3716.6814f,Z=189.062347f,O=85.2750015f;}Loc;};
struct SafeStore {WorldSafeLocsEntry value;bool missing=false;WorldSafeLocsEntry const*LookupEntry(uint32 id){assert(id==5710);return missing?nullptr:&value;}}sWorldSafeLocsStore;
struct MapManager {static bool IsValidMapCoord(uint32,float x,float y,float z,float o){return std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&std::isfinite(o)&&std::abs(x)<17000&&std::abs(y)<17000&&std::abs(z)<17000;}};
struct AreaTriggerStruct{uint32 target_mapId=1;float target_X=2,target_Y=3,target_Z=4;};
struct AreaStore {bool present=false;AreaTriggerStruct value;AreaTriggerStruct const*GetGoBackTrigger(uint32){return present?&value:nullptr;}}areas;auto sAreaTriggerDataStore=&areas;
struct QuestObjective{int32 ObjectID;int8 StorageIndex;uint32 ID=0;int32 Amount=1;};
struct Quest{std::vector<QuestObjective>objectives{{101880,0,281594},{102479,1,281598},{101880,2,281599}};auto const&GetObjectives()const{return objectives;}};
struct QuestStore{Quest q;bool missing=false;Quest const*GetQuestTemplate(uint32 id){assert(id==41003);return missing?nullptr:&q;}}quests;auto sQuestDataStore=&quests;
struct Player {
 uint32 mapId=1541,cls=10,m_scenarioLoginId=0,m_scenarioLoginInstance=0,selector=0;ScenarioResume m_scenarioResume;
 int status=1,claim=1,returnCredit=0;bool rewarded=false,access=false,teleportSuccess=true,gm=false;ObjectGuid guid;
 uint32 m_homebindMapId=1;float m_homebindX=2,m_homebindY=3,m_homebindZ=4;
 uint32 teleports=0,destination=9999;float x=0,y=0,z=0,o=0;Map map;
 std::map<uint8,Item*>slots;std::map<uint8,Bag*>bags;uint8 inventoryEnd=43;
 Item*GetItemByPos(uint8 bag,uint8 slot)const{assert(bag==255);auto i=slots.find(slot);return i==slots.end()?nullptr:i->second;}
 Bag*GetBagByPos(uint8 slot)const{auto i=bags.find(slot);return i==bags.end()?nullptr:i->second;}uint8 GetInventoryEndSlot()const{return inventoryEnd;}
 Item*GetItemByEntry(uint32,bool=false)const;
 uint32 GetMapId()const{return mapId;}int getClass()const{return cls;}bool IsQuestRewarded(uint32 id)const{assert(id==41003);return rewarded;}bool isGameMaster()const{return gm;}
 int GetQuestStatus(uint32 id)const{assert(id==41003);return status;}uint32 GetInstanceId(){return 42;}
 int32 GetQuestObjectiveData(uint32,uint32)const;int32 GetQuestObjectiveData(Quest const*,int8 index)const{return index==1?claim:returnCredit;}
 bool TeleportTo(uint32 target,float a,float b,float c,float d){++teleports;destination=target;x=a;y=b;z=c;o=d;return teleportSuccess;}
 void SetScenarioId(uint32 id){selector=id;}bool ExitCompletedSheilunLogin();bool HasPendingSheilunReturn()const;
 bool ExitCompletedBrokenShoreLogin(){return false;}bool ExitCompletedDarkRidersLogin(){return false;}
 Map*GetMap(){return &map;}bool CheckInstanceLoginValid(){return access;}float GetOrientation(){return 2;}uint32 GetGUIDLow(){return 1;}ObjectGuid GetGUID()const{return guid;}
};
#include "Inventory.inc"
#include "Objective.inc"
#include "Recovery.inc"
#include "PendingReturn.inc"
void Login(Player*player){
#include "Admission.inc"
}
struct Run {
 Player p;Item staff;Bag bag;Scenario progress;
 Run(){sWorldSafeLocsStore=SafeStore{};areas=AreaStore{};quests=QuestStore{};scenarios.value=nullptr;errors=logs=0;p.slots[23]=&staff;progress.map=&p.map;}
 void Checkpoint(){p.m_scenarioLoginId=p.selector=1007;p.m_scenarioLoginInstance=42;assert(p.m_scenarioResume.Read("1 1541 1007 42 1000 -1073 -3143 27 0"));}
 void Accepted(){Checkpoint();auto before=p.m_scenarioResume.Write();Login(&p);assert(p.map.additions==1&&p.teleports==1);
#ifdef BASELINE
  assert(p.destination==1&&errors==1&&p.m_scenarioResume.Write()==before&&p.selector==1007);
#else
  assert(p.destination==1514&&!errors&&logs==1&&std::abs(p.x-sWorldSafeLocsStore.value.Loc.X)<0.001f&&std::abs(p.y-sWorldSafeLocsStore.value.Loc.Y)<0.001f&&std::abs(p.z-sWorldSafeLocsStore.value.Loc.Z)<0.001f);
  assert(std::abs(p.o-1.4883295f)<0.0001f&&!p.selector&&!p.m_scenarioLoginId&&!p.m_scenarioLoginInstance&&p.m_scenarioResume.Write().empty());
#endif
  assert(p.claim==1&&p.returnCredit==0&&!p.rewarded&&staff.count==1&&staff.entry==128937&&staff.owner==p.guid);++cases;
 }
};
int main(){
 for(int state:{QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE})for(bool added:{false,true})for(int location:{0,1,2,3,4}){
  Run r;r.p.status=state;r.p.map.added=added;r.p.slots.clear();
  if(location==0)r.p.slots[23]=&r.staff;
  if(location==1){r.bag.items[2]=&r.staff;r.p.bags[19]=&r.bag;}
  if(location==2)r.p.slots[16]=&r.staff;
  if(location==3)r.p.slots[192]=&r.staff;
  if(location==4){r.p.slots[46]=&r.staff;r.p.inventoryEnd=47;}
  r.Accepted();
 }
 for(bool added:{false,true}){Run r;r.p.map.added=added;Login(&r.p);
#ifdef BASELINE
  assert(r.p.destination==1&&errors==1);
#else
  assert(r.p.destination==1514&&!errors);
#endif
  ++cases;
 }
#ifndef BASELINE
 for(int mode=0;mode<19;++mode){Run r;
  switch(mode){case 0:break;case 1:r.p.status=QUEST_STATUS_INCOMPLETE;break;
   case 2:r.p.mapId=1514;break;case 3:r.p.cls=9;break;case 4:r.p.gm=true;break;case 5:r.p.rewarded=true;break;
   case 6:r.p.status=0;break;case 7:r.p.claim=0;break;case 8:r.p.slots.clear();break;case 9:r.staff.count=0;break;
   case 10:r.staff.owner.high=3;break;case 11:r.p.returnCredit=1;break;case 12:quests.missing=true;break;
   case 13:quests.q.objectives[2].ID=281598;break;case 14:quests.q.objectives[2].StorageIndex=1;break;
   case 15:quests.q.objectives[2].ObjectID=102479;break;case 16:quests.q.objectives[2].Amount=2;break;
   case 17:r.p.slots.clear();r.p.slots[47]=&r.staff;break;case 18:r.staff.entry=128938;break;}
  auto old=r.p.m_scenarioResume.Write();assert(r.p.HasPendingSheilunReturn()==(mode<2));
  assert(!r.p.teleports&&r.p.m_scenarioResume.Write()==old&&r.staff.count==(mode==9?0u:1u));++cases;
 }
 for(int mode=0;mode<22;++mode){Run r;r.Checkpoint();Map foreign;
  switch(mode){case 0:r.p.mapId=1514;break;case 1:r.p.cls=9;break;case 2:r.p.m_scenarioLoginId=1002;break;
   case 3:r.p.m_scenarioResume.scenario=1002;break;case 4:r.p.rewarded=true;break;case 5:r.p.status=0;break;case 6:r.p.status=5;break;
   case 7:r.p.claim=0;break;case 8:r.p.slots.clear();break;case 9:r.staff.owner.high=3;break;case 10:r.staff.count=0;break;
   case 11:r.staff.entry=128938;break;case 12:r.p.slots.clear();r.p.slots[47]=&r.staff;break;
   case 13:r.p.slots.clear();r.bag.items[0]=&r.staff;r.p.bags[75]=&r.bag;break;
   case 14:r.p.slots.clear();r.p.slots[94]=&r.staff;break;case 15:r.p.slots.clear();r.p.slots[43]=&r.staff;break;
   case 16:sWorldSafeLocsStore.missing=true;break;case 17:sWorldSafeLocsStore.value.MapID=1515;break;
   case 18:sWorldSafeLocsStore.value.Loc.X=std::numeric_limits<float>::infinity();break;
   case 19:sWorldSafeLocsStore.value.Loc.O=std::numeric_limits<float>::quiet_NaN();break;
   case 20:scenarios.value=&r.progress;r.progress.id=1002;break;case 21:scenarios.value=&r.progress;r.progress.map=&foreign;break;}
  auto checkpoint=r.p.m_scenarioResume.Write();assert(!r.p.ExitCompletedSheilunLogin()&&!r.p.teleports&&r.p.m_scenarioResume.Write()==checkpoint&&r.p.selector==1007);++cases;
 }
 for(bool present:{false,true}){Run r;if(present)scenarios.value=&r.progress;r.Accepted();}
 {Run r;r.Checkpoint();r.p.teleportSuccess=false;auto old=r.p.m_scenarioResume.Write();assert(!r.p.ExitCompletedSheilunLogin()&&r.p.teleports==1&&r.p.m_scenarioResume.Write()==old&&r.p.selector==1007);++cases;}
 {Run r;r.p.teleportSuccess=false;Login(&r.p);assert(r.p.teleports==2&&r.p.destination==1&&errors==1);++cases;}
 for(bool goBack:{false,true}){Run r;r.p.claim=0;r.p.slots.clear();areas.present=goBack;areas.value.target_mapId=1220;Login(&r.p);assert(r.p.teleports==1&&r.p.destination==(goBack?1220u:1u)&&errors==1&&!logs);++cases;}
 {Run r;r.p.access=true;Login(&r.p);assert(!r.p.teleports&&!errors&&!logs);++cases;}
 {Run r;r.p.map.garrison=true;Login(&r.p);assert(!r.p.teleports&&!errors&&!logs);++cases;}
#endif
#ifdef BASELINE
 std::cout<<"EXPECTED_BROKEN "<<cases<<" pinned native post-AddPlayer login caller cases: claimed/inventory Sheilun owners fall back home after completed checkpoint clearing. Extracted current recovery is not called by this old caller. No modeled native crash or successful ordinary-run claim.\n";
#else
 std::cout<<"PASS "<<cases<<" actual Sheilun recovery, native post-AddPlayer admission, objective lookup and inventory/bag scan under ASAN: owned/equipped/loading-bag/child/expanded slots, bank-only/prefill/foreign-owner denial, statuses/map/scenario/checkpoint/data guards, native5710 degree conversion, accepted live/garrison paths and failure fallback. Teleport/world/session endpoints controlled; native landing data/collision and client separate.\n";
#endif
}
