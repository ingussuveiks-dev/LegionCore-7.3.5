#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <iostream>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
using EncounterState=int;
enum {QUEST_STATUS_INCOMPLETE=1,DONE=3,IN_PROGRESS=1,ACTION_1=1};
struct ObjectGuid {unsigned id=0;bool operator==(ObjectGuid b)const{return id==b.id;}static const ObjectGuid Empty;};const ObjectGuid ObjectGuid::Empty{};
struct Position{float x=0,y=0,z=0,o=0;};
struct Creature;struct Player;struct GameObject;
struct Group {ObjectGuid GetLeaderGUID(){return {};}};
struct Player {
 bool alive=true,teleport=false,vehicle=false,flight=false;unsigned map=1220;Position pos;std::map<unsigned,int> quests;std::map<unsigned,unsigned> credits;std::vector<unsigned> spells;std::vector<std::function<void()>> events;
 bool IsAlive(){return alive;}unsigned GetMapId(){return map;}int GetQuestStatus(unsigned q){return quests[q];}bool IsBeingTeleported(){return teleport;}
 float GetDistance(Position p){return std::sqrt((pos.x-p.x)*(pos.x-p.x)+(pos.y-p.y)*(pos.y-p.y)+(pos.z-p.z)*(pos.z-p.z));}float GetDistance(GameObject*);
 unsigned GetQuestObjectiveData(unsigned,unsigned c){return credits[c];}void KilledMonsterCredit(unsigned c){++credits[c];}void AddDelayedEvent(unsigned,std::function<void()> f){events.push_back(f);}
 void* GetVehicle(){return vehicle?this:nullptr;}bool isInFlight(){return flight;}void CastSpell(Player*,unsigned s,bool){spells.push_back(s);teleport=true;}
 Group* GetGroup(){return nullptr;}Creature* FindNearestCreature(unsigned,float,bool){return nullptr;}
};
struct CreatureAI{unsigned talks=0,actions=0;void ZoneTalk(unsigned){++talks;}void Talk(unsigned){}void DoAction(bool){++actions;}};
struct Creature {ObjectGuid guid;unsigned entry;bool visible=true,dead=false;CreatureAI ai;unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}void SetVisible(bool v){visible=v;}bool isDead(){return dead;}CreatureAI* AI(){return &ai;}};
struct GameObject {ObjectGuid guid;unsigned entry=244534,map=1220,zone=7502;Position pos;bool deleted=false;std::list<Player*> players;
 unsigned GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}unsigned GetMapId(){return map;}unsigned GetZoneId(){return zone;}void Delete(){deleted=true;}
 float GetDistance(Position p){Player t;t.pos=pos;return t.GetDistance(p);}void GetPlayerListInGrid(std::list<Player*>& p,float){p=players;}
};
float Player::GetDistance(GameObject* g){return GetDistance(g->pos);}
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
 p.teleport=false;p.pos={-846.86f,4469.6f,736.04f,0};portal.UpdateAI(500);assert(p.spells.size()==2); // no arrival bounce
 std::cout<<"PASS: actual instance methods and portal handlers; entry credit, no kill substitution, late grids, duplicate naga slots, locked/final Tidestone, teleport acknowledgement/failure, wrong map/floor/death/vehicle/quest, return and no bounce.\n";
}
