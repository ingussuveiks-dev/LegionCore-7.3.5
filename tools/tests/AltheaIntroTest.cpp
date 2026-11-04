#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;
enum{CLASS_ROGUE=4,QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
struct Player;struct Creature;struct Unit;struct Conversation;
struct Progress{uint64 Counter=0;};
struct Scenario{uint32 id=1123,stage=4;std::map<uint32,Progress> progress;auto& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap()const{return &progress;}uint32 GetScenarioId()const{return id;}bool CanUpdateCriteria(uint32 leaf)const{return stage==4?leaf==48577:stage==5?(leaf==48585||leaf==48597):false;}};
struct ScenarioMgr{Scenario* value;Scenario* GetScenario(uint32){return value;}} manager;auto sScenarioMgr=&manager;
struct Map{};
struct Unit{virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct World:Unit{Map* map=nullptr;bool world=true,preDelete=false,alive=true,expired=false,scope=true;uint32 entry=108218,mapId=1620;bool IsInWorld()const{return world;}bool IsPreDelete()const{return preDelete;}bool IsAlive()const{return alive;}bool IsQuestObjectExpired()const{return expired;}bool CanTargetQuestObject(World const*)const{return scope;}Map* GetMap()const{return map;}uint32 GetMapId()const{return mapId;}uint32 GetEntry()const{return entry;}uint32 GetInstanceId()const{return 1;}};
struct Player:World{
 bool contact=true,gm=false,teleport=false,phase=true,reward=false,failCredit=false;uint32 klass=4,quest=42504,status=3,events=0;float distance=30;std::function<void()> onCredit;
 Player* ToPlayer()override{return this;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}uint32 getClass()const{return klass;}bool HasPendingTeleport()const{return teleport;}bool InSamePhase(World*)const{return phase;}
 bool GetQuestRewardStatus(uint32 q)const{return q==quest&&reward;}uint32 GetQuestStatus(uint32 q)const{return q==quest?status:0;}
 void UpdateAchievementCriteria(uint32 kind,uint32 event){assert(kind==92&&event==51549);++events;if(!failCredit&&manager.value->CanUpdateCriteria(48577)){++manager.value->progress[48577].Counter;manager.value->stage=5;}if(onCredit)onCredit();}
};
struct Motion{uint32 jumps=0;void MoveJump(float x,float y,float z,float,float speed,float height){assert(x==-8614.58f&&y==403.17f&&z==102.92f&&speed==5&&height==5);++jumps;}};
struct AIBase{virtual ~AIBase()=default;uint32 attacks=0;Unit* target=nullptr;std::function<void()> onAttack;virtual void AttackStart(Unit* who){++attacks;target=who;if(onAttack)onAttack();}};
struct Creature:World{
 AIBase* ai=nullptr;Creature* garona=nullptr;std::vector<Creature*> guards;std::list<Player*> players;Motion motion;uint32 removals=0;
 AIBase* AI()const{return ai;}float GetExactDist2d(Player* p)const{return p->distance;}float GetOrientation()const{return 0;}void GetPlayerListInGrid(std::list<Player*>& out,float r){assert(r==50);out=players;}
 Creature* FindNearestCreature(uint32 id,float range,bool living){assert(living);if(id==108214){assert(range==60);return garona&&garona->world&&garona->alive?garona:nullptr;}assert(range==40&&(id==108216||id==108217));for(auto g:guards)if(g->entry==id&&g->world&&g->alive)return g;return nullptr;}
 Motion* GetMotionMaster(){return &motion;}void RemoveAura(uint32 id){assert(id==214873);++removals;}
};
static void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* source,uint32 entry,float range){assert(range==30);for(auto g:source->guards)if(g->world&&g->entry==entry)out.push_back(g);}
struct ScriptedAI:AIBase{Creature* me;explicit ScriptedAI(Creature* c):me(c){}virtual void MoveInLineOfSight(Unit*){}};
struct EventMap{};
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}};struct ObjectMgr{Generator generator;template<HighGuid>Generator* GetGenerator(){return &generator;}} objects;auto sObjectMgr=&objects;
struct Services{std::map<uint32,uint32> attempts,accepted;std::map<uint32,bool> fail;std::vector<std::unique_ptr<Conversation>> conversations;std::function<void(uint32)> onCreate;uint32 warnings=0;} service;
#define TC_LOG_WARN(...) (++service.warnings)
struct Conversation{bool CreateConversation(uint32,uint32 id,Player*,void*,Player&){++service.attempts[id];if(service.onCreate)service.onCreate(id);if(service.fail[id])return false;++service.accepted[id];service.conversations.emplace_back(this);return true;}};
struct Intro:ScriptedAI{
 bool introDone=false,introDone2=false,checkdies=false;uint32 timerforcheck=0;EventMap events;
 using ScriptedAI::ScriptedAI;
#include "IntroHelpers.inc"
#include "MoveInLineOfSight.inc"
};
struct Run{
 Map map;Scenario scenario;Creature source,garona,a,b,c;Player player,gm;AIBase garonaAI,aAI,bAI,cAI;Intro ai{&source};
 Run(uint32 quest=42504){service=Services{};manager.value=&scenario;source.ai=&ai;source.map=&map;source.garona=&garona;source.guards={&a,&b,&c};source.players={&gm,&player};player.map=gm.map=garona.map=a.map=b.map=c.map=&map;player.quest=gm.quest=quest;gm.gm=true;gm.distance=10;garona.entry=108214;garona.ai=&garonaAI;a.entry=b.entry=108216;c.entry=108217;a.ai=&aAI;b.ai=&bAI;c.ai=&cAI;}
 ~Run(){service.conversations.clear();}
 void Start(){ai.MoveInLineOfSight(&player);}void Dead(){a.alive=b.alive=c.alive=false;}
};
int main(){uint32 cases=0;
 for(uint32 quest:{42504u,42627u}){
  {Run r(quest);r.player.distance=50;r.ai.MoveInLineOfSight(&r.gm);Unit pet;r.ai.MoveInLineOfSight(&pet);r.ai.MoveInLineOfSight(nullptr);assert(!r.ai.introDone&&!r.ai.introDone2);r.Start();r.Start();assert(r.ai.introDone&&!r.ai.introDone2&&r.player.events==1&&service.accepted[2422]==1);++cases;}
  {Run r(quest);r.Start();r.Start();assert(r.ai.introDone&&r.ai.introDone2&&r.ai.checkdies&&r.aAI.attacks==1&&r.bAI.attacks==1&&r.cAI.attacks==1&&r.aAI.target==&r.garona);r.ai.UpdateIntro(1000);assert(!r.source.motion.jumps);r.Dead();r.ai.UpdateIntro(999);assert(!r.source.motion.jumps);r.ai.UpdateIntro(1);r.ai.UpdateIntro(1000);assert(r.ai.introReleased&&!r.ai.checkdies&&r.source.motion.jumps==1&&r.source.removals==1&&r.ai.target==&r.player&&service.accepted[2425]==1);++cases;}
  std::vector<std::function<void(Run&)>> refusals={
   [](Run&r){r.source.world=false;},[](Run&r){r.source.preDelete=true;},[](Run&r){r.source.alive=false;},[](Run&r){r.source.entry=108219;},[](Run&r){r.source.mapId=1;},[](Run&r){r.source.expired=true;},
   [](Run&r){r.player.world=false;},[](Run&r){r.player.preDelete=true;},[](Run&r){r.player.alive=false;},[](Run&r){r.player.contact=false;},[](Run&r){r.player.gm=true;},[](Run&r){r.player.klass=1;},[](Run&r){r.player.teleport=true;},[](Run&r){r.player.map=nullptr;},[](Run&r){r.player.phase=false;},[](Run&r){r.player.distance=55.01f;},[](Run&r){r.player.expired=true;},[](Run&r){r.player.scope=false;},[](Run&r){r.source.scope=false;},[](Run&r){r.player.status=0;},[](Run&r){r.player.reward=true;},[](Run&r){r.scenario.id=1;},[](Run&r){r.scenario.stage=3;},[](Run&r){r.scenario.stage=6;},[](Run&r){r.scenario.progress[48585].Counter=1;}
  };
  for(auto const& refuse:refusals){Run r(quest);refuse(r);r.Start();assert(!r.ai.introDone&&!r.ai.introDone2&&!r.player.events&&service.attempts.empty()&&!r.aAI.attacks);++cases;}
  {Run r(quest);r.player.failCredit=true;r.Start();r.Start();assert(!r.ai.introDone&&!r.ai.introDone2&&r.player.events==1&&service.warnings==1&&service.attempts.empty());r.player.failCredit=false;r.ai.UpdateIntro(1000);r.Start();assert(r.ai.introDone&&r.ai.introDone2&&r.player.events==2);++cases;}
  {Run r(quest);r.player.onCredit=[&]{r.player.status=0;r.Start();};r.Start();assert(!r.ai.introDone&&!r.ai.introDone2&&r.player.events==1);r.player.onCredit={};r.player.status=3;r.Start();assert(r.ai.introDone&&r.ai.introDone2&&r.player.events==1);++cases;}
  {Run r(quest);r.player.onCredit=[&]{r.Start();};service.onCreate=[&](uint32){r.Start();};r.aAI.onAttack=[&]{r.Start();r.ai.UpdateIntro(1000);};r.Start();r.Start();assert(r.player.events==1&&service.accepted[2422]==1&&service.accepted[2424]==1&&r.aAI.attacks==1&&r.ai.introRetries[0]==1000&&r.ai.introRetries[1]==1000);++cases;}
  for(uint32 id:{2422u,2424u,2425u}){Run r(quest);service.fail[id]=true;r.Start();if(id==2425){r.Dead();r.ai.UpdateIntro(1000);}assert(r.ai.introDone&&r.ai.introDone2&&service.warnings==1);auto attempts=service.attempts[id];r.Start();assert(service.attempts[id]==attempts);service.fail[id]=false;r.ai.UpdateIntro(1000);r.Start();assert(service.accepted[id]==1&&r.player.events==1);if(id==2425)assert(r.source.motion.jumps==1);++cases;}
  for(uint32 mode=0;mode<5;++mode){Run r(quest);if(mode==0)r.source.garona=nullptr;if(mode==1)r.garona.ai=nullptr;if(mode==2)r.garona.scope=false;if(mode==3)r.a.ai=nullptr;if(mode==4)r.a.scope=false;r.Start();assert(r.ai.introDone&&!r.ai.introDone2&&!r.aAI.attacks);r.source.garona=&r.garona;r.garona.ai=&r.garonaAI;r.garona.scope=true;r.a.ai=&r.aAI;r.a.scope=true;r.Start();assert(r.ai.introDone2&&r.aAI.attacks==1);++cases;}
  {Run r(quest);r.Start();r.Dead();r.player.status=0;r.ai.UpdateIntro(1000);assert(r.ai.checkdies&&!r.source.motion.jumps&&!r.source.removals);r.player.status=3;r.ai.UpdateIntro(1000);assert(!r.ai.checkdies&&r.source.motion.jumps==1&&r.ai.target==&r.player);++cases;}
  {Run r(quest);r.scenario.stage=5;r.scenario.progress[48577].Counter=1;r.Start();assert(r.ai.introDone&&r.ai.introDone2&&!r.player.events);++cases;}
  {Run r(quest);r.scenario.stage=5;r.Start();assert(!r.ai.introDone&&!r.ai.introDone2&&!r.player.events);++cases;}
  {Run r(quest);r.player.status=QUEST_STATUS_COMPLETE;r.Start();assert(r.ai.introDone&&r.ai.introDone2);++cases;}
 }
 std::cout<<"PASS "<<cases<<" ASAN complete actual Althea intro helper/LOS contexts: native receipt boundary, both quests, GM/pet/null/source/player/stage/scope gates, independent failed voice retries, mandatory companion/AI retry, reentrancy, guard timer and qualified release. Engine factories/grid/criteria boundaries controlled; native/client checks remain separate\n";
}
