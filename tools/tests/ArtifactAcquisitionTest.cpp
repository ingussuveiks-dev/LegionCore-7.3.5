#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
#define TC_LOG_INFO(...) ((void)0)
enum {CLASS_HUNTER=3,CLASS_PRIEST=5,CLASS_DEATH_KNIGHT=6,CLASS_MAGE=8,CLASS_WARLOCK=9,CLASS_DRUID=11,
 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,GAMEOBJECT_FIELD_FLAGS=0,GO_FLAG_IN_USE=1,GO_FLAG_NOT_SELECTABLE=2,
 GO_JUST_DEACTIVATED=3,SMART_EVENT_GO_STATE_CHANGED=70,CRITERIA_TYPE_OWN_ITEM=36,GAMEOBJECT_TYPE_FISHINGNODE=17};
struct Player;
using Unit=Player;using LootState=uint32;
struct Script {uint32 calls=0,event=0,state=0;Unit* invoker=nullptr;
 void ProcessEventsFor(uint32 e,Unit* u,uint32 s){++calls;event=e;state=s;invoker=u;}
};
struct SmartGameObjectAI {Script script;Script* GetScript(){return &script;}void OnStateChanged(uint32,Unit*);};
#include "OnStateChanged.inc"
struct GameObject {
 uint32 entry=0,flags=0,m_lootState=0;bool spawned=true;int map=1;SmartGameObjectAI ai;
 uint32 GetEntry(){return entry;}uint32 GetGoType(){return 3;}bool isSpawned(){return spawned;}bool HasFlag(uint32,uint32 mask){return (flags&mask)!=0;}
 int GetMap(){return map;}uint32 getLootState(){return m_lootState;}SmartGameObjectAI* AI(){return &ai;}
 void SetLootState(LootState,Unit*);
};
struct ScriptManager {void OnGameObjectLootStateChanged(GameObject*,LootState,Unit*){}} scriptManager;
auto sScriptMgr=&scriptManager;
#include "SetLootState.inc"
struct Player {
 uint32 cls=9,quest=40623,status=3,ownItemEvents=0,lastItem=0;bool rewarded=false,alive=true,teleport=false,owned=false,bank=false;
 bool phase=true,visible=true,los=true;float distance=2;int map=1;
 uint32 GetGUIDLow(){return 1;}bool GetQuestRewardStatus(uint32 q){return q==quest&&rewarded;}
 uint32 GetQuestStatus(uint32 q){return q==quest?status:0;}uint32 getClass(){return cls;}
 bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}int GetMap(){return map;}
 bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}
 bool IsWithinLOSInMap(GameObject*){return los;}float GetExactDist(GameObject*){return distance;}
 bool HasItemCount(uint32,uint32 count,bool includeBank){assert(count==1&&includeBank);return owned||bank;}
 void UpdateAchievementCriteria(uint32 type,uint32 item,uint32 amount){assert(type==36&&amount==1);++ownItemEvents;lastItem=item;}
};
#include "Acquisition.inc"
struct Loot{bool empty=false;bool isLooted(){return empty;}};
bool NativeCompletedLoot(Loot* loot,GameObject* go){
#include "CompletedLootPredicate.inc"
}
int main()
{
 for(auto row:ArtifactAcquisition::Pickups){
  Player p;p.cls=row.playerClass;p.quest=row.quest;GameObject go;go.entry=row.object;
  assert(!ArtifactAcquisition::Use(&p,&go)&&!go.ai.script.calls); // real native loot required
  // Full bags leave real loot present: native LootHandler only deactivates a
  // fully looted chest. The extracted native event bridge receives no call.
  assert(go.getLootState()!=GO_JUST_DEACTIVATED);
  Loot pending;assert(!NativeCompletedLoot(&pending,&go));
  pending.empty=true;assert(NativeCompletedLoot(&pending,&go));
  p.owned=true;assert(ArtifactAcquisition::Use(&p,&go));
  assert(go.ai.script.calls==1&&go.ai.script.event==70&&go.ai.script.state==3&&go.ai.script.invoker==&p);
  assert(p.ownItemEvents==1&&p.lastItem==row.item);
  assert(ArtifactAcquisition::Use(&p,&go)&&go.ai.script.calls==1&&p.ownItemEvents==1);
  go=GameObject{};go.entry=row.object;p.owned=false;p.bank=true;assert(ArtifactAcquisition::Use(&p,&go)&&go.ai.script.calls==1);
  if(row.alternate){go=GameObject{};go.entry=row.object;p.quest=row.alternate;assert(ArtifactAcquisition::Use(&p,&go)&&go.ai.script.calls==1);}
  for(int failure=0;failure<13;++failure){
   p=Player{};p.cls=row.playerClass;p.quest=row.quest;p.owned=true;go=GameObject{};go.entry=row.object;
   if(failure==0)p.cls=1;if(failure==1)p.quest=999;if(failure==2)p.rewarded=true;if(failure==3)p.status=0;
   if(failure==4)p.alive=false;if(failure==5)p.teleport=true;if(failure==6)p.phase=false;if(failure==7)p.visible=false;
   if(failure==8)p.los=false;if(failure==9)p.distance=6;if(failure==10)p.map=2;if(failure==11)go.flags=GO_FLAG_NOT_SELECTABLE;if(failure==12)go.spawned=false;
   assert(ArtifactAcquisition::Use(&p,&go)&&!go.ai.script.calls);
  }
 }
 Player p;GameObject go;go.entry=252054;assert(!ArtifactAcquisition::Use(&p,&go)); // existing Scepter handler unchanged
 std::cout<<"PASS: eight production artifact pickups; ordinary loot path, owned/bank copy and real SmartAI loot-state callback, once-only and class/quest/visibility/location gates\n";
}
