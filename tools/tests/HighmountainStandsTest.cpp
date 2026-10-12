#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
enum QuestStatus {QUEST_STATUS_NONE,QUEST_STATUS_INCOMPLETE,QUEST_STATUS_COMPLETE,QUEST_STATUS_REWARDED};
enum {SPELL_AURA_ACTIVATE_SCENE=430};
#include "QuestStatusData.inc"
struct ObjectGuid {unsigned value;bool operator<(ObjectGuid const& g)const{return value<g.value;}};
struct Position {float x=0,y=0,z=0,o=0;};
struct Player:Position {
 ObjectGuid guid;QuestStatusData quest;unsigned progress=0,credits=0,sceneCasts=0,teleports=0,repairs=0;
 bool alive=true,online=true,vehicle=false,transport=false,flight=false,flying=false,pending=false,phase=true,aura=false,otherScene=false,failScene=false,failTeleport=false,tracking=false;
 unsigned map=1220;
 explicit Player(unsigned id):guid{id}{quest.Status=QUEST_STATUS_INCOMPLETE;}
 ObjectGuid GetGUID(){return guid;}bool IsInWorld(){return online;}bool IsAlive(){return alive;}unsigned GetMapId(){return map;}
 bool GetVehicle(){return vehicle;}bool GetTransport(){return transport;}bool isInFlight(){return flight;}bool IsFlying(){return flying;}bool IsBeingTeleported(){return pending;}
 float GetExactDist(Position const* p){return std::sqrt((x-p->x)*(x-p->x)+(y-p->y)*(y-p->y)+(z-p->z)*(z-p->z));}
 int GetQuestStatus(unsigned id){assert(id==42512);return quest.Status;}
 unsigned GetQuestObjectiveData(unsigned id,unsigned entry){assert(id==42512&&entry==107704);return progress;}
 bool HasPhaseId(unsigned id){assert(id==9180);return phase;}
 bool HasAuraType(unsigned id){assert(id==430);return aura||otherScene;}
 bool HasAura(unsigned id){assert(id==213725);return aura;}
 void RemoveAurasDueToSpell(unsigned id){assert(id==213725);aura=false;}
 void CastSpell(Player* p,unsigned id,bool triggered){assert(p==this&&triggered);if(id==213725){++sceneCasts;if(!failScene){aura=true;tracking=true;}}else{assert(id==197461);++teleports;if(!failTeleport)pending=true;}}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void AddDelayedEvent(uint64,std::function<void()> callback){callback();}
 QuestStatusData* getQuestStatus(unsigned){return &quest;}
 void KilledMonsterCredit(unsigned entry){assert(entry==107704&&!pending&&!progress);++progress;++credits;quest.Status=QUEST_STATUS_COMPLETE;}
 void IncompleteQuest(unsigned id){assert(id==42512);quest.Status=QUEST_STATUS_INCOMPLETE;++repairs;}
 void Reaccept(){quest=QuestStatusData{};quest.Status=QUEST_STATUS_INCOMPLETE;progress=credits=0;}
};
#include "QuestDelayedEvent.inc"
struct SpellScene {unsigned MiscValue;};
struct SceneTriggerScript {explicit SceneTriggerScript(char const*){}virtual bool OnTrigger(Player*,SpellScene const*,std::string){return false;}};
struct PlayerScript {explicit PlayerScript(char const*){}virtual void OnUpdate(Player*,uint32){}virtual void OnLogin(Player*){}virtual void OnLogout(Player*){}virtual void OnMapChanged(Player*){}};
#include "HighmountainStands.inc"
void Move(Player& p,Position const& pos){static_cast<Position&>(p)=pos;}
int main()
{
 using namespace HighmountainStands;
 player_highmountain_stands script;scene_highmountain_stands scene;SpellScene native{1287},wrong{1};
 Player p(1),q(2);Move(p,Confront);Move(q,Confront);
 scene.OnTrigger(&p,&native,"complete");script.OnUpdate(&p,0);assert(p.sceneCasts==1&&!p.teleports&&!p.credits&&p.tracking);
 assert(!scene.OnTrigger(&p,&wrong,"complete"));
 script.OnUpdate(&q,0);assert(q.sceneCasts==1&&runs.size()==2);
 scene.OnTrigger(&p,&native,"IMPACT");scene.OnTrigger(&p,&native,"EXPLOSION");scene.OnTrigger(&p,&native,"SLEEP");scene.OnTrigger(&p,&native,"TELEPORT05");
 script.OnUpdate(&p,4999);assert(!p.teleports&&!p.credits);
 script.OnUpdate(&p,2001);scene.OnTrigger(&p,&native,"TELEPORT05");script.OnUpdate(&p,0);assert(p.teleports==1&&p.pending&&!p.credits&&!q.teleports);
 scene.OnTrigger(&p,&native,"TELEPORT05");script.OnUpdate(&p,500);assert(p.teleports==1&&!p.credits);
 Move(p,Arrival);script.OnUpdate(&p,500);assert(!p.credits); // Server position alone is not an acknowledgement.
 p.pending=false;script.OnUpdate(&p,0);assert(p.credits==1&&p.quest.Status==QUEST_STATUS_COMPLETE&&p.aura);
 scene.OnTrigger(&p,&native,"complete");p.aura=false;script.OnUpdate(&p,0);assert(!runs.count(p.guid)&&p.credits==1);
 scene.OnTrigger(&p,&native,"complete");script.OnUpdate(&p,30000);assert(p.credits==1&&p.teleports==1);
 // Missing all client callbacks still recovers only after a valid server-started scene.
 script.OnUpdate(&q,29999);assert(!q.teleports);script.OnUpdate(&q,1);assert(q.teleports==1&&!q.credits);
 q.pending=false;Move(q,Arrival);script.OnUpdate(&q,0);assert(q.credits==1&&!q.aura&&!runs.count(q.guid));
 // Explicit early skip follows the same acknowledged transfer before credit.
 p.Reaccept();Move(p,Confront);script.OnUpdate(&p,0);scene.OnTrigger(&p,&native,"complete");p.aura=false;
 script.OnUpdate(&p,0);assert(p.pending&&!p.credits);Move(p,Arrival);p.pending=false;script.OnUpdate(&p,0);assert(p.credits==1&&!runs.count(p.guid));
 // No initial scene for malformed context, other cinematics, or acceptance away from Dargrul.
 for(unsigned cause=0;cause<11;++cause){
  p.Reaccept();Move(p,Confront);unsigned casts=p.sceneCasts;
  if(cause==0)p.quest.Status=QUEST_STATUS_NONE;if(cause==1)p.alive=false;if(cause==2)p.map=1;
  if(cause==3)p.phase=false;if(cause==4)p.x+=16;if(cause==5)p.z+=16;if(cause==6)p.vehicle=true;
  if(cause==7)p.transport=true;if(cause==8)p.flight=true;if(cause==9)p.otherScene=true;if(cause==10)p.pending=true;
  script.OnUpdate(&p,30000);scene.OnTrigger(&p,&native,"complete");assert(p.sceneCasts==casts&&!runs.count(p.guid)&&!p.credits);
  p.alive=p.phase=true;p.map=1220;p.vehicle=p.transport=p.flight=p.otherScene=p.pending=false;
 }
 p.Reaccept();Move(p,Confront);p.failScene=true;script.OnUpdate(&p,0);assert(!runs.count(p.guid));p.failScene=false;
 script.OnUpdate(&p,0);p.failTeleport=true;script.OnUpdate(&p,30000);script.OnUpdate(&p,10000);assert(!p.credits&&!runs.count(p.guid)&&!p.aura);p.failTeleport=false;
 for(unsigned cause=0;cause<7;++cause){
  p.Reaccept();Move(p,Confront);script.OnUpdate(&p,0);assert(runs.count(p.guid));
  if(cause==0)p.Reaccept();if(cause==1)p.alive=false;if(cause==2)p.online=false;if(cause==3)p.map=1;
  if(cause==4)p.x+=200;if(cause==5)p.quest.Status=QUEST_STATUS_REWARDED;if(cause==6)p.pending=true;
  scene.OnTrigger(&p,&native,"complete");script.OnUpdate(&p,1);assert(!runs.count(p.guid)&&!p.aura&&!p.credits);
  p.alive=p.online=true;p.map=1220;p.pending=false;
 }
 p.Reaccept();Move(p,Confront);script.OnUpdate(&p,0);script.OnLogout(&p);scene.OnTrigger(&p,&native,"complete");assert(!runs.count(p.guid)&&!p.aura&&!p.credits);
 p.quest.Status=QUEST_STATUS_COMPLETE;script.OnLogin(&p);assert(p.repairs==1&&p.quest.Status==QUEST_STATUS_INCOMPLETE);
 p.quest.Status=QUEST_STATUS_COMPLETE;p.progress=1;script.OnLogin(&p);assert(p.repairs==1&&p.quest.Status==QUEST_STATUS_COMPLETE);
 p.quest.Status=QUEST_STATUS_REWARDED;p.progress=0;script.OnLogin(&p);assert(p.repairs==1&&p.quest.Status==QUEST_STATUS_REWARDED);
 std::cout<<"PASS: production scene proximity gate, native callback sequencing, duplicate/stale events, two players, acknowledged landing before credit, skip/fallback, failed launch/teleport, quest lifetime, death/logout/map/range cancellation and legacy login repair.\n";
}
