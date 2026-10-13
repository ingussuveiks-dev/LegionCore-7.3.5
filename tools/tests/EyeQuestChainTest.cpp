#include <memory>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <iostream>
using uint64=uint64_t;using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
using EncounterState=int;
enum {QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=1,DONE=3,IN_PROGRESS=1,ACTION_1=1};
struct ObjectGuid {unsigned id=0;bool operator==(ObjectGuid b)const{return id==b.id;}static const ObjectGuid Empty;};const ObjectGuid ObjectGuid::Empty{};
struct Position{float x=0,y=0,z=0,o=0;};
struct Creature;struct Player;struct GameObject;
struct Group {ObjectGuid GetLeaderGUID(){return {};}};
struct QuestStatusData {int Status=1;std::shared_ptr<uint8> ScriptLifetime;};
struct Player {
 bool alive=true,teleport=false,vehicle=false,flight=false,world=true,transport=false,flying=false,phase=true,visible=true,los=true;
 std::map<uint32,QuestStatusData> statuses;QuestStatusData* getQuestStatus(uint32 q){statuses[q].Status=quests[q];return &statuses[q];}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 bool IsInWorld(){return world;}bool GetTransport(){return transport;}bool IsFlying(){return flying;}bool InSamePhase(GameObject*){return phase;}bool canSeeOrDetect(GameObject*){return visible;}bool IsWithinLOSInMap(GameObject*){return los;}unsigned map=1220;Position pos;std::map<unsigned,int> quests;std::map<unsigned,unsigned> credits;std::vector<unsigned> spells;std::vector<std::function<void()>> events;
 bool IsAlive(){return alive;}unsigned GetMapId(){return map;}int GetQuestStatus(unsigned q){return quests[q];}bool IsBeingTeleported(){return teleport;}
 float reach=1.5f;
 float GetExactDist(Position const* p){return std::sqrt((pos.x-p->x)*(pos.x-p->x)+(pos.y-p->y)*(pos.y-p->y)+(pos.z-p->z)*(pos.z-p->z));}float GetExactDist(GameObject*);
 float GetDistance(Position p){return std::max(0.0f,GetExactDist(&p)-reach);}float GetDistance(GameObject*);
 unsigned GetQuestObjectiveData(unsigned,unsigned c){return credits[c];}void KilledMonsterCredit(unsigned c){++credits[c];}void AddDelayedEvent(unsigned,std::function<void()> f){events.push_back(f);}
 void* GetVehicle(){return vehicle?this:nullptr;}bool isInFlight(){return flight;}void CastSpell(Player*,unsigned s,bool){spells.push_back(s);teleport=true;}
 Group* GetGroup(){return nullptr;}Creature* FindNearestCreature(unsigned,float,bool){return nullptr;}
};
#include "QuestDelayedEvent.inc"
struct CreatureAI{unsigned talks=0,actions=0;void ZoneTalk(unsigned){++talks;}void Talk(unsigned){}void DoAction(bool){++actions;}};
struct Creature {ObjectGuid guid;unsigned entry;bool visible=true,dead=false;CreatureAI ai;unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}void SetVisible(bool v){visible=v;}bool isDead(){return dead;}CreatureAI* AI(){return &ai;}};
struct GameObject {ObjectGuid guid;unsigned entry=244534,map=1220,zone=7502;Position pos;bool deleted=false;std::list<Player*> players;
 unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}void Delete(){deleted=true;}
 float GetExactDist(Position const* p){Player t;t.pos=pos;return t.GetExactDist(p);}
 float GetDistance(Position p){return std::max(0.0f,GetExactDist(&p)-0.306f);}void GetPlayerListInGrid(std::list<Player*>& p,float){p=players;}
};
float Player::GetExactDist(GameObject* g){return GetExactDist(&g->pos);}
// Match WorldObject::GetDistance: edge distance includes BOTH objects' sizes.
float Player::GetDistance(GameObject* g){return std::max(0.0f,GetExactDist(g)-reach-0.306f);}
struct ObjectAccessor{static Player* GetPlayer(Player&,ObjectGuid){return nullptr;}};
struct InstanceMap{std::map<unsigned,Creature*> creatures;std::map<unsigned,GameObject*> objects;Creature* GetCreature(ObjectGuid id){auto it=creatures.find(id.id);return it==creatures.end()?nullptr:it->second;}GameObject* GetGameObject(ObjectGuid id){auto it=objects.find(id.id);return it==objects.end()?nullptr:it->second;}};
struct InstanceScript {
 InstanceMap* instance;int states[5]={};InstanceScript(InstanceMap* m):instance(m){}virtual ~InstanceScript()=default;
 virtual void Initialize(){}virtual void OnCreatureCreate(Creature*){}virtual void OnGameObjectCreate(GameObject*){}virtual bool SetBossState(uint32 i,EncounterState s){if(states[i]==s)return false;states[i]=s;return true;}int GetBossState(unsigned i){return states[i];}
 virtual void SetData(uint32,uint32){}virtual ObjectGuid GetGuidData(uint32)const{return {};}virtual uint32 GetData(uint32)const{return 0;}virtual void OnPlayerEnter(Player*){}
 void LoadDoorData(int){}void AddDoor(GameObject*,bool){}void DoCastSpellOnPlayers(unsigned){}void DoRemoveAurasDueToSpellOnPlayers(unsigned){}
};
const int doorData=0;const int IN_MILLISECONDS=1000;
#include "../../src/server/scripts/Legion/EyeofAzshara/eye_of_azshara.h"
#include "EyeInstanceMethods.inc"
struct GameObjectAI {GameObject* go;GameObjectAI(GameObject* g):go(g){}virtual ~GameObjectAI()=default;virtual void UpdateAI(unsigned){}};
struct GameObjectScript{GameObjectScript(char const*){}virtual ~GameObjectScript()=default;virtual GameObjectAI* GetAI(GameObject*)const{return nullptr;}};
#include "../../src/server/scripts/Legion/EyeofAzshara/eye_of_azshara_quests.cpp"
int main(){
 InstanceMap map;EyeInstance inst(&map);Player p;p.quests[38286]=1;inst.OnPlayerEnter(&p);inst.OnPlayerEnter(&p);assert(p.credits[106847]==1 && p.credits[91784]==0 && p.credits[96028]==0);
 Creature stone{{10},106780};map.creatures[10]=&stone;inst.OnCreatureCreate(&stone);assert(!stone.visible);
 for(unsigned i=0;i<4;++i)inst.SetBossState(i,DONE); // bosses cleared before the final grid loads
 Creature boss{{11},96028};map.creatures[11]=&boss;inst.CheckBossTimer=0;inst.OnCreatureCreate(&boss);assert(inst.CheckBossTimer==1);
 Creature a{{12},98173},b{{13},100248},c{{14},100249},d{{15},100250};
 for(auto n:{&a,&b,&c,&d}){map.creatures[n->guid.id]=n;inst.OnCreatureCreate(n);}inst.DoEventCreatures();assert(boss.visible && a.ai.actions==1 && boss.ai.talks==1);
 for(unsigned i=0;i<100;++i)inst.OnCreatureCreate(&a); // duplicate/grid reload cannot overflow four slots
 assert(inst.NagasContainerGUID[0]==a.guid && inst.NagasContainerGUID[3]==d.guid);
 inst.DoEventCreatures();assert(boss.ai.talks==1);
 GameObject bubble;bubble.guid={16};bubble.entry=240788;map.objects[16]=&bubble;inst.OnGameObjectCreate(&bubble);inst.DoEventCreatures();assert(bubble.deleted);
 inst.SetBossState(DATA_WRATH_OF_AZSHARA,DONE);assert(stone.visible);
 Creature lateStone{{17},106780};map.creatures[17]=&lateStone;inst.OnCreatureCreate(&lateStone);assert(lateStone.visible);
 InstanceMap fresh;EyeInstance locked(&fresh);Creature lockedBoss{{18},96028};lockedBoss.visible=false;fresh.creatures[18]=&lockedBoss;locked.OnCreatureCreate(&lockedBoss);locked.DoEventCreatures();assert(!lockedBoss.visible);
 GameObject upper;upper.pos=EyeQuests::UpperPad;p.pos=upper.pos;p.quests[42213]=1;upper.players={&p};go_eye_portrait_teleporter::AI portal(&upper);
 portal.UpdateAI(500);assert(p.spells.back()==192293 && p.credits[106815]==0);
 portal.UpdateAI(500);assert(p.spells.size()==1); // pending teleport cannot be dispatched twice
 auto callback=p.events.front();p.events.clear();callback();assert(p.credits[106815]==0 && p.events.size()==1); // no acknowledgement
 p.teleport=false;callback();assert(p.credits[106815]==0); // failed teleport remains upstairs
 p.pos=EyeQuests::Arrival;callback();assert(p.credits[106815]==1);callback();assert(p.credits[106815]==1);
 p.credits[106815]=0;p.alive=false;callback();assert(p.credits[106815]==0);p.alive=true;p.map=1;callback();assert(p.credits[106815]==0);p.map=1220;
 p.quests[42213]=0;callback();assert(p.credits[106815]==0);p.quests[42213]=1;
 // The shared portrait-room pad must confirm Tears of Elune independently.
 p.quests[42213]=0;p.quests[40890]=1;p.credits[109750]=0;p.pos=upper.pos;p.teleport=false;
 portal.UpdateAI(500);auto tearCallback=p.events.back();p.events.clear();
 assert(p.credits[109750]==0);tearCallback();assert(p.credits[109750]==0);
 p.teleport=false;tearCallback();assert(p.credits[109750]==0);
 p.pos=EyeQuests::Arrival;tearCallback();tearCallback();assert(p.credits[109750]==1 && p.credits[106815]==0);
 p.quests[40890]=0;p.quests[42213]=1;p.spells.resize(1);p.events.clear();
 p.pos=upper.pos;p.pos.z+=5;portal.UpdateAI(500);assert(p.spells.size()==1);p.pos=upper.pos;p.vehicle=true;portal.UpdateAI(500);assert(p.spells.size()==1);p.vehicle=false;
 GameObject lower;lower.entry=244560;lower.pos=EyeQuests::LowerPad;lower.players={&p};go_eye_portrait_teleporter::AI back(&lower);p.pos=lower.pos;back.UpdateAI(500);assert(p.spells.back()==192295 && p.credits[106815]==0);
 p.teleport=false;p.pos={-846.86f,4469.6f,736.04f,0};
 assert(p.GetDistance(&upper)<2.0f && p.GetExactDist(&upper)>2.0f); // old handler reactivated on arrival
 portal.UpdateAI(500);assert(p.spells.size()==2);
 for(float reach:{1.5f,5.0f,10.0f}) {
  p.reach=reach;
  for(int i=0;i<20;++i){p.pos=EyeQuests::Arrival;back.UpdateAI(500);p.pos={-846.86f,4469.6f,736.04f,0};portal.UpdateAI(500);}
  assert(p.spells.size()==2); // both arrival points remain safe, including enlarged characters
 }
 p.reach=1.5f;p.pos=upper.pos;p.pos.x+=2.01f;portal.UpdateAI(500);assert(p.spells.size()==2);
 p.pos=upper.pos;p.pos.x+=1.99f;portal.UpdateAI(500);assert(p.spells.size()==3&&p.spells.back()==192293);
 p.teleport=false;p.pos=lower.pos;back.UpdateAI(500);assert(p.spells.size()==4&&p.spells.back()==192295); // walking back onto a pad still works

 p={};p.quests[42454]=1;p.pos=upper.pos;upper.players={&p};portal.UpdateAI(500);assert(p.spells.back()==192293&&!p.credits[107587]);
 auto hammerCallback=p.events.back();p.events.clear();hammerCallback();assert(!p.credits[107587]&&p.events.size()==1);
 p.teleport=false;hammerCallback();assert(!p.credits[107587]); // Failed cast remains upstairs.
 p.pos=EyeQuests::Arrival;hammerCallback();hammerCallback();assert(p.credits[107587]==1);
 for(int reason=0;reason<10;++reason){Player q;q.quests[42454]=1;q.pos=EyeQuests::Arrival;
  if(reason==0)q.alive=false;if(reason==1)q.world=false;if(reason==2)q.map=1;if(reason==3)q.vehicle=true;if(reason==4)q.transport=true;if(reason==5)q.flight=true;if(reason==6)q.flying=true;if(reason==7)q.quests[42454]=0;if(reason==8)q.quests[42454]=2;if(reason==9)q.pos.z+=10;
  EyeQuests::ConfirmHammerTeleport(&q,0);assert(!q.credits[107587]);
 }
 p.credits[107587]=0;p.statuses[42454].ScriptLifetime.reset();hammerCallback();assert(!p.credits[107587]); // Abandon/reaccept invalidates the old callback.
 p.teleport=true;p.events.clear();EyeQuests::ConfirmHammerTeleport(&p,0);assert(p.events.empty());
 {Player q;q.quests[42454]=1;q.pos=EyeQuests::Arrival;q.pos.x+=4;EyeQuests::ConfirmHammerTeleport(&q,0);assert(q.credits[107587]==1);} // Running between acknowledgement and the next poll.
 for(int reason=0;reason<5;++reason){p={};p.quests[42454]=1;p.pos=upper.pos;
  if(reason==0)p.transport=true;if(reason==1)p.flying=true;if(reason==2)p.phase=false;if(reason==3)p.visible=false;if(reason==4)p.los=false;
  portal.UpdateAI(500);assert(p.spells.empty()&&p.events.empty());
 }
 p={};p.quests[42454]=1;p.pos=lower.pos;back.UpdateAI(500);assert(p.events.empty()&&!p.credits[107587]);
 std::cout<<"PASS: Hammer credit only after acknowledged upper-pad arrival; duplicate, stale quest lifetime, timeout, invalid state and lower-pad exclusions.\n";
 std::cout<<"PASS: actual instance methods and portal handlers; entry credit, no kill substitution, late grids, duplicate naga slots, locked/final Tidestone, teleport acknowledgement/failure, wrong map/floor/death/vehicle/quest, return and no bounce.\n";
}
