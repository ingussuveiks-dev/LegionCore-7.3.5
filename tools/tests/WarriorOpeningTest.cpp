#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum{TYPEID_UNIT=0,TYPEID_PLAYER=1,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CLASS_WARRIOR=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_STATE_CASTING=1,UNIT_STATE_SIGHTLESS=2,REACT_AGGRESSIVE=1,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=1,UNIT_FLAG_IMMUNE_TO_NPC=2,UNIT_FLAG_NOT_ATTACKABLE_1=4,EVENT_1=1,EVENT_2,EVENT_3,EVENT_4};
static uint32 warnings=0,errors=0;
#define TC_LOG_WARN(...) (++warnings)
#define TC_LOG_ERROR(...) (++errors)
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(value,...) assert(value)
struct ObjectGuid
{
 uint64 low=0,high=0;
 bool operator==(ObjectGuid g)const{return low==g.low&&high==g.high;}
 bool operator!=(ObjectGuid g)const{return !(*this==g);}
 bool operator<(ObjectGuid g)const{return high==g.high?low<g.low:high<g.high;}
 uint64 GetLowPart()const{return low;}uint64 GetHighPart()const{return high;}
 std::string ToString()const{return std::to_string(low)+":"+std::to_string(high);}
};
#include "Functions.inc"
#include "DeathState.inc"
struct Map;struct Player;struct Creature;struct Conversation;struct CreatureAI;struct InstanceScript;
struct Unit
{
 FunctionProcessor m_Functions;Player*owner=nullptr;
 virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 virtual uint32 GetTypeId(){return TYPEID_UNIT;}virtual ObjectGuid GetGUID()const{return {};}
 virtual Map*GetMap()const{return nullptr;}virtual bool IsAlive()const{return true;}virtual bool isInFlight()const{return false;}
 Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner;}
 #include "UnitTimer.inc"
};
struct Ref{Player*player;Player*getSource()const{return player;}};
struct Map
{
 uint32 id=1500,instance=42;bool unload=false;InstanceScript*script=nullptr;
 std::vector<Ref>players;std::map<ObjectGuid,Creature*>actors;std::map<ObjectGuid,Conversation*>conversations;
 uint32 GetId(){return id;}uint32 GetInstanceId(){return instance;}bool IsMapUnload(){return unload;}auto const&GetPlayers(){return players;}
 Creature*GetCreature(ObjectGuid key){auto i=actors.find(key);return i==actors.end()?nullptr:i->second;}
}world;
struct Criterion{uint64 Counter=0;};struct Achievement{std::map<uint32,Criterion>data;auto const*GetCriteriaProgressMap(){return &data;}};
struct Scenario{uint32 id=933,step=0;Achievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Achievement&GetAchievementMgr(){return achievement;}}scenarioFixture;
struct Scenarios{bool missing=false;Scenario*GetScenario(uint32 key){return missing||key!=world.instance?nullptr:&scenarioFixture;}}scenarioManager;auto*sScenarioMgr=&scenarioManager;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8>ScriptLifetime;};
static std::map<ObjectGuid,Player*>players;
struct Player:Unit
{
 ObjectGuid guid{1,0xAABBCCDDAABBCCDD};Map*map=&world;uint32 cls=1,activeQuest=38904,questState=3,marker=0,team=67;
 bool alive=true,flight=false,contact=true,worlded=true,gm=false,pending=false,phase=true,rewarded=false,allyVisible=true;float distance=10;
 QuestStatusData quest;std::vector<uint32>events;std::map<uint32,bool>refused;std::function<void(uint32)>onEvent;
 Player*ToPlayer()override{return this;}uint32 GetTypeId()override{return TYPEID_PLAYER;}ObjectGuid GetGUID()const override{return guid;}Map*GetMap()const override{return map;}
 uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return map->instance;}uint32 getClass(){return cls;}bool IsAlive()const override{return alive;}bool isInFlight()const override{return flight;}
 bool IsInWorld(){return worlded;}bool CanContact(){return contact;}bool isGameMaster(){return gm;}bool HasPendingTeleport(){return pending;}bool InSamePhase(Creature*){return phase;}
 bool canSeeOrDetect(Unit const*,bool first,bool second){assert(!first&&second);return allyVisible;}
 bool GetQuestRewardStatus(uint32 q){return q==activeQuest&&rewarded;}uint32 GetQuestStatus(uint32 q){return q==activeQuest?questState:0;}
 QuestStatusData*getQuestStatus(uint32 q){quest.Status=questState;return q==activeQuest&&questState?&quest:nullptr;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 asset){assert(asset==103661);return q==activeQuest?marker:0;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32 type,uint32 asset)
 {
  assert(type==92&&(asset==46877||asset==46878||asset==47474));events.push_back(asset);
  if(onEvent)onEvent(asset);
  if(gm||!contact||!worlded||refused[asset])return;
  if(asset==46877&&scenarioFixture.step==0){scenarioFixture.achievement.data[45769].Counter=1;scenarioFixture.step=1;}
  if(scenarioFixture.step==1&&asset!=46877){scenarioFixture.achievement.data[asset==46878?45771:46175].Counter=1;if(scenarioFixture.achievement.data[45771].Counter&&scenarioFixture.achievement.data[46175].Counter)scenarioFixture.step=2;}
 }
};
struct ObjectAccessor
{
 static Player*FindPlayer(ObjectGuid key){auto i=players.find(key);return i==players.end()?nullptr:i->second;}
 static Creature*GetCreature(Unit const&actor,ObjectGuid key){return actor.GetMap()?actor.GetMap()->GetCreature(key):nullptr;}
 static Conversation*GetConversation(Unit const&actor,ObjectGuid key){if(!actor.GetMap())return nullptr;auto const&data=actor.GetMap()->conversations;auto i=data.find(key);return i==data.end()?nullptr:i->second;}
};
struct Pose{float x,y,z;};
struct Creature:Unit
{
 CreatureAI*i_AI=nullptr,*i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;DeathState state=ALIVE;
 ObjectGuid guid;Map*map=&world;uint32 entry=0,spawn=0,flags=0;bool worlded=true,preDelete=false,sightless=false,aggressive=true,visible=true,aura=false;
 std::vector<uint32>attacks;std::vector<Pose>homes;std::function<void()>onAttack;
 explicit Creature(uint32 id):guid{id,0x1122334411223344},entry(id){}
 ~Creature();Creature*ToCreature()override{return this;}ObjectGuid GetGUID()const override{return guid;}Map*GetMap()const override{return map;}
 uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return spawn;}uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return map->instance;}
 bool IsAlive()const override{return state==ALIVE;}DeathState getDeathState(){return state;}bool IsInWorld(){return worlded;}bool IsPreDelete(){return preDelete;}CreatureAI*AI()const{return i_AI;}
 bool HasUnitState(uint32 kind)const{return kind==UNIT_STATE_SIGHTLESS?sightless:false;}bool HasReactState(uint32 kind)const{assert(kind==REACT_AGGRESSIVE);return aggressive;}
 bool canSeeOrDetect(Unit const*,bool first,bool second)const{assert(!first&&second);return visible;}
 bool IsWithinDistInMap(Unit*target,float radius){auto*p=target->ToPlayer();assert(p);return map==p->map&&p->distance<=radius;}
 bool HasFlag(uint32,uint32 mask){return (flags&mask)!=0;}bool HasAura(uint32 id){assert(id==105419);return aura;}
 Creature*FindNearestCreature(uint32 id,float radius,bool alive=true){assert(alive&&(radius==100||radius==120||radius==250));for(auto const&item:map->actors)if(item.second->entry==id&&item.second->IsAlive())return item.second;return nullptr;}
 void SetHomePosition(float x,float y,float z,float orientation){assert(!orientation);homes.push_back({x,y,z});}
 InstanceScript*GetInstanceScript(){return map?map->script:nullptr;}bool AIM_Destroy();void Lifecycle(uint32);
};
struct SpellInfo{uint32 Id=0;};
struct CreatureAI
{
 Creature*me;bool passiveSight=false;
 explicit CreatureAI(Creature*actor):me(actor){}virtual~CreatureAI()=default;
 virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void MoveInLineOfSight(Unit*){}virtual void DoAction(int32){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual uint32 GetData(uint32)const{return 0;}
 bool CanSeeEvenInPassiveMode()const{return passiveSight;}
 virtual void AttackStart(Creature*target){me->attacks.push_back(target->entry);if(me->onAttack)me->onAttack();}
};
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return false;}void DoCast(uint32){}void DoMeleeAttackIfReady(){}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual~CreatureScript()=default;virtual CreatureAI*GetAI(Creature*)const{return nullptr;}};
struct EventMap{void Reset(){}void RescheduleEvent(uint32,uint32){}void Update(uint32){}uint32 ExecuteEvent(){return 0;}};
struct InstanceScript{virtual~InstanceScript()=default;virtual uint32 GetData(uint32)const{return 0;}virtual void SetData(uint32,uint32){}virtual ObjectGuid GetGuidData(uint32)const{return {};}};
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#include "AIDestroy.inc"
Creature::~Creature(){assert(!m_AI_locked);if(i_disabledAI!=i_AI)delete i_disabledAI;i_disabledAI=nullptr;AIM_Destroy();if(map)map->actors.erase(guid);}
int32 irand(int32 low,int32 high){assert(low<=0&&high>=0);return 0;}
void GetCreatureListWithEntryInGrid(std::list<Creature*>&result,Creature*actor,uint32 entry,float radius){assert(radius==90);for(auto const&item:actor->map->actors)if(item.second->entry==entry)result.push_back(item.second);}
enum class HighGuid{Conversation};struct Generator{uint64 next=1;uint64 Generate(){return next++;}}generator;
struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto*sObjectMgr=&objects;
struct Conversation
{
 struct Actor{uint64 GuidLow=0,GuidHight=0;};
 static inline std::vector<Conversation*>allocated;static inline std::vector<uint32>created;static inline std::map<uint32,bool>fails;
 static inline uint32 corruption=0,removed=0;static inline std::function<void(uint32)>onCreate;
 Map*map=nullptr;ObjectGuid guid,owner;uint32 id=0;bool worlded=false;std::vector<Actor>actors;
 Conversation(){allocated.push_back(this);}~Conversation(){if(map)map->conversations.erase(guid);allocated.erase(std::find(allocated.begin(),allocated.end(),this));}
 bool CreateConversation(uint64 key,uint32 entry,Unit*caster,void*,Unit&)
 {
  id=entry;created.push_back(entry);if(onCreate)onCreate(entry);if(fails[entry])return false;
  map=caster->GetMap();guid={key,0xFACEFACEFACEFACE};owner=caster->GetGUID();worlded=corruption!=6;map->conversations[guid]=this;
  if(entry==1386)
  {
   Creature*ally=nullptr,*source=nullptr;
   for(auto const&item:map->actors){if(item.second->entry==97166&&item.second->IsAlive())ally=item.second;if(item.second->entry==97167&&item.second->IsAlive())source=item.second;}
   if(!ally||!source){map->conversations.erase(guid);worlded=false;return false;}
   actors={{ally->guid.low,ally->guid.high},{source->guid.low,source->guid.high}};
   if(corruption==1)actors.clear();if(corruption==2)actors.resize(1);if(corruption==3)++actors[0].GuidHight;if(corruption==4)++actors[1].GuidHight;if(corruption==5)++actors[1].GuidLow;
  }
  if(corruption==7)map->conversations.erase(guid);
  return true;
 }
 bool IsInWorld(){return worlded;}ObjectGuid GetGUID(){return guid;}auto const&GetCreatures(){return actors;}
 Actor const*GetCreature(uint8 index){assert(index<actors.size());return &actors[index];}
 void Remove(){++removed;worlded=false;if(map)map->conversations.erase(guid);}
};
#include "QuestDelay.inc"
#include "Original.inc"
#include "Opening.inc"
#include "FirstWave.inc"
#include "Casts.inc"
#include "Actor.inc"
#include "BaselineActor.inc"
#include "NativeVisibility.inc"
struct Instance:InstanceScript
{
 WarriorOpening::State warriorOpening{&world};WarriorFirstWave::State warriorFirstWave{&world};
 uint32 GetData(uint32 type)const override{if(type==WarriorOpening::RequestData)return warriorOpening.Request();if(type==WarriorOpening::StartedData)return warriorOpening.started?warriorOpening.epoch:0;return 0;}
 void SetData(uint32 type,uint32 value)override{if(type==WarriorOpening::ClaimData)warriorOpening.Claim(value);}
 ObjectGuid GetGuidData(uint32 type)const override{return type==WarriorOpening::SourceData?warriorOpening.source:ObjectGuid{};}
 void Update(uint32 diff){warriorOpening.Update(diff);warriorFirstWave.Update(diff);}
};
struct Run
{
 Player player,other;Creature source{97167},ally{97166},enemy{97176},demon{102970},attacker1{102964},attacker2{102965},add1{102990},add2{102995};Instance instance;
 Run()
 {
  world=Map{};scenarioFixture=Scenario{};scenarioManager.missing=false;players.clear();warnings=errors=0;
  Conversation::created.clear();Conversation::fails.clear();Conversation::corruption=Conversation::removed=0;Conversation::onCreate={};assert(Conversation::allocated.empty());
  source.spawn=361094;ally.spawn=361091;demon.spawn=361138;other.guid.low=2;players[player.guid]=&player;world.players={{&player}};world.script=&instance;
  for(auto*actor:All()){world.actors[actor->guid]=actor;actor->i_AI=new CreatureAI(actor);}
  assert(source.AIM_Destroy());source.i_AI=npc_ragnvald_dragonborn{}.GetAI(&source);source.IsAIEnabled=true;instance.warriorOpening.Observe(&source);
 }
 ~Run(){while(!Conversation::allocated.empty())delete Conversation::allocated.back();}
 std::vector<Creature*>All(){return {&source,&ally,&enemy,&demon,&attacker1,&attacker2,&add1,&add2};}
 void AddOther(){players[other.guid]=&other;world.players.push_back({&other});}
 void Begin(){instance.Update(0);}
 void Tick(uint32 diff){for(auto*actor:All()){actor->m_Functions.Update(diff);actor->Lifecycle(diff);}instance.Update(diff);}
 void Replace(){assert(source.AIM_Destroy());source.i_AI=npc_ragnvald_dragonborn{}.GetAI(&source);source.IsAIEnabled=true;}
 auto&State(){return instance.warriorOpening;}
 void Done(){assert(State().started&&source.attacks==std::vector<uint32>({97176,97176}));assert(ally.attacks==std::vector<uint32>{97176});assert(attacker1.attacks==std::vector<uint32>{102970}&&attacker2.attacks==std::vector<uint32>{102970});}
};
int main()
{
 _set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;uint32 cases=0;
 {Run r;r.Begin();assert(r.player.events==std::vector<uint32>{46877});assert(Conversation::created==std::vector<uint32>({1386,770}));r.Tick(1999);assert(!r.State().started&&r.source.attacks.empty());r.Tick(1);r.Done();r.Tick(100000);r.Done();++cases;}
 for(uint32 failed:{1386u,770u}){Run r;Conversation::fails[failed]=true;r.Begin();if(failed==1386){r.Tick(100000);assert(r.source.attacks.empty()&&!r.State().started);Conversation::fails[failed]=false;r.Tick(1000);assert(r.source.attacks.empty());r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();}else{r.Tick(2000);r.Done();assert(!r.State().greeted);Conversation::fails[failed]=false;r.Tick(1000);assert(r.State().started&&r.State().greeted);}++cases;}
 {Run r;r.player.refused[46877]=true;r.Begin();r.Tick(100000);assert(!r.State().spoken&&r.source.attacks.empty()&&scenarioFixture.step==0);r.player.refused[46877]=false;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 for(uint32 corrupt=1;corrupt<=7;++corrupt){Run r;Conversation::corruption=corrupt;r.Begin();r.Tick(5000);assert(!r.State().spoken&&!r.State().started&&r.source.attacks.empty()&&Conversation::removed);Conversation::corruption=0;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 for(uint32 elapsed:{0u,1u,1999u}){Run r;r.Begin();r.Tick(elapsed);r.Replace();r.Tick(1000);assert(r.source.attacks.empty());r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();assert(std::count(Conversation::created.begin(),Conversation::created.end(),1386u)==1);++cases;}
 for(uint32 mode=0;mode<3;++mode){Run r;r.Begin();auto*native=r.source.i_AI;r.source.i_disabledAI=native;r.source.i_AI=mode==0?new CreatureAI(&r.source):mode==1?nullptr:npc_ragnvald_dragonborn{}.GetAI(&r.source);r.Tick(1000);r.Tick(1000);r.Done();r.Tick(10000);r.Done();++cases;}
 {Run r;r.Begin();r.source.onAttack=[&]{assert(r.source.m_AI_locked&&!r.source.AIM_Destroy());};r.Tick(2000);r.Done();++cases;}
 {Run r;r.Begin();r.source.i_AI->Reset();r.Tick(2000);r.Done();++cases;}
 for(uint32 cancel=0;cancel<8;++cancel){Map otherMap;Run r;r.Begin();if(cancel==0)r.player.quest.ScriptLifetime.reset();if(cancel==1)r.player.questState=0;if(cancel==2)r.player.marker=1;if(cancel==3)r.player.map=&otherMap;if(cancel==4)scenarioFixture.step=2;if(cancel==5)r.source.state=CORPSE;if(cancel==6)r.source.map=&otherMap;if(cancel==7){players.erase(r.player.guid);++r.player.guid.high;players[r.player.guid]=&r.player;}r.Tick(2000);assert(!r.State().started&&r.source.attacks.empty());++cases;}
 {Run r;r.player.distance=30;r.Begin();assert(r.State().greeted&&!r.State().pending&&r.player.events.empty());r.player.distance=15;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 {Run r;Unit pet;r.source.i_AI->MoveInLineOfSight(&pet);assert(r.player.events.empty()&&r.source.attacks.empty());r.Begin();r.Tick(2000);r.Done();++cases;}
 {Run r;r.Begin();r.player.alive=false;r.Tick(2000);r.Done();++cases;}
 {Run r;r.player.distance=30;r.Begin();r.demon.state=CORPSE;Unit pet;pet.owner=&r.player;r.instance.warriorFirstWave.Death(&r.demon,&pet);assert(scenarioFixture.step==0&&r.player.events.empty());r.player.distance=15;r.Tick(1000);assert(scenarioFixture.step==2&&r.player.events==std::vector<uint32>({46877,46878,47474}));r.Tick(2000);assert(r.source.attacks.empty()&&!r.State().started);++cases;}
 for(bool gm:{false,true}){Run r;assert(r.source.AIM_Destroy());r.source.i_AI=npc_ragnvald_dragonborn_baseline{}.GetAI(&r.source);r.source.IsAIEnabled=true;r.player.gm=gm;Conversation::fails[1386]=!gm;r.source.i_AI->MoveInLineOfSight(&r.player);r.source.Lifecycle(2000);assert(r.source.attacks==std::vector<uint32>({97176,97176}));if(gm)assert(scenarioFixture.step==0);std::cout<<"COUNTERFACTUAL pinned77ebf820e complete Ragnvald starts battle despite "<<(gm?"denied GM criteria":"failed1386 factory")<<"; repaired component cases retain admission/factory retries without battle.\n";++cases;}
 for(uint32 bad=0;bad<29;++bad){Map foreign;Run r;switch(bad){case 0:world.id=1;break;case 1:world.unload=true;break;case 2:scenarioFixture.id=775;break;case 3:scenarioFixture.id=961;break;case 4:scenarioFixture.step=2;break;case 5:scenarioManager.missing=true;break;case 6:r.source.entry=123;break;case 7:r.source.spawn=123;break;case 8:r.source.worlded=false;break;case 9:r.source.preDelete=true;break;case 10:r.source.state=CORPSE;break;case 11:r.player.cls=2;break;case 12:r.player.questState=0;break;case 13:r.player.questState=1;break;case 14:r.player.rewarded=true;break;case 15:r.player.marker=1;break;case 16:r.player.gm=true;break;case 17:r.player.worlded=false;break;case 18:r.player.contact=false;break;case 19:r.player.pending=true;break;case 20:r.player.phase=false;break;case 21:r.player.map=&foreign;break;case 22:r.player.alive=false;break;case 23:r.player.flight=true;break;case 24:r.source.sightless=true;break;case 25:r.source.aggressive=false;break;case 26:r.source.IsAIEnabled=false;break;case 27:r.source.visible=false;break;case 28:r.player.distance=41;break;}if(bad>=22&&bad<=27)assert(!shouldCallMoveInLineOfSight(&r.source,&r.player));r.Begin();r.Tick(10000);assert(r.player.events.empty()&&!r.State().spoken&&r.source.attacks.empty());++cases;}
 for(uint32 temporary=0;temporary<4;++temporary){Run r;r.Begin();if(temporary==0)r.player.contact=false;if(temporary==1)r.player.pending=true;if(temporary==2)r.player.phase=false;if(temporary==3)r.player.worlded=false;r.Tick(2000);assert(!r.State().started&&r.State().pending&&r.source.attacks.empty());r.player.contact=r.player.phase=r.player.worlded=true;r.player.pending=false;r.Tick(1000);r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();++cases;}
 {Run r;assert(shouldCallMoveInLineOfSight(&r.source,&r.player));r.source.aggressive=false;r.source.i_AI->passiveSight=true;assert(shouldCallMoveInLineOfSight(&r.source,&r.player));r.Begin();r.Tick(2000);r.Done();++cases;}
 {Run r;r.AddOther();r.player.gm=true;r.Begin();assert(r.player.events.empty()&&r.other.events==std::vector<uint32>{46877});auto found=std::find_if(Conversation::allocated.begin(),Conversation::allocated.end(),[](Conversation*c){return c->id==1386&&c->worlded;});assert(found!=Conversation::allocated.end()&&(*found)->owner==r.other.guid);r.Tick(2000);r.Done();++cases;}
 for(uint32 team:{67u,469u})for(uint32 quest:{38904u,42815u}){Run r;r.player.team=team;r.player.activeQuest=quest;r.Begin();r.Tick(2000);r.Done();++cases;}
 {Run r;auto original=std::make_unique<Player>();original->guid.low=9;players[original->guid]=original.get();world.players={{original.get()}};r.Begin();assert(r.State().spoken);players.erase(original->guid);world.players.clear();original.reset();r.Tick(2000);assert(!r.State().started&&r.source.attacks.empty());++cases;}
 {Run r;r.Begin();auto oldTicket=r.State().epoch;r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Tick(2000);assert(!r.State().started&&r.State().epoch!=oldTicket&&r.source.attacks.empty());r.Tick(2000);r.Done();assert(std::count(Conversation::created.begin(),Conversation::created.end(),1386u)==2);++cases;}
 {Run r;Conversation::fails[1386]=true;r.Begin();r.Tick(100000);Conversation::fails[1386]=false;r.Tick(100000);assert(r.source.attacks.empty());r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();++cases;}
 {Run r;r.Begin();r.enemy.state=r.demon.state=CORPSE;r.Tick(2000);assert(!r.State().started&&r.source.attacks.empty());r.enemy.state=r.demon.state=ALIVE;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 for(uint32 missing=0;missing<3;++missing){Run r;assert(r.source.AIM_Destroy());if(missing==1){r.source.i_AI=new CreatureAI(&r.source);r.source.IsAIEnabled=true;}if(missing==2)r.source.IsAIEnabled=true;r.Begin();r.Tick(1000);assert(r.player.events.empty()&&r.source.attacks.empty());r.Replace();r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 {Run r;Creature replacement{97167};r.Begin();replacement.spawn=361094;replacement.guid=r.source.guid;replacement.i_AI=npc_ragnvald_dragonborn{}.GetAI(&replacement);world.actors[replacement.guid]=&replacement;r.State().Observe(&replacement);r.source.Lifecycle(2000);assert(r.source.attacks.empty()&&!r.State().started);r.instance.Update(1000);replacement.Lifecycle(1999);assert(replacement.attacks.empty());replacement.Lifecycle(1);assert(r.State().started&&replacement.attacks==std::vector<uint32>({97176,97176}));++cases;}
 {Run r;Conversation::fails[770]=true;r.Begin();r.Tick(2000);r.Done();scenarioFixture.step=2;Conversation::fails[770]=false;r.Tick(1000);assert(!r.State().greeted);++cases;}
 {Run r;r.ally.spawn=123;r.Begin();assert(!r.State().spoken);r.Tick(5000);assert(r.source.attacks.empty());r.ally.spawn=361091;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 {Run r;auto*unrelated=new Conversation;unrelated->map=&world;unrelated->guid={999,0xFACEFACEFACEFACE};unrelated->worlded=true;world.conversations[unrelated->guid]=unrelated;Conversation::corruption=4;r.Begin();assert(unrelated->IsInWorld()&&ObjectAccessor::GetConversation(r.player,unrelated->guid)==unrelated&&!r.State().spoken);Conversation::corruption=0;r.Tick(1000);r.Tick(2000);r.Done();assert(unrelated->IsInWorld());++cases;}
 {Run r;r.Begin();auto ticket=r.State().epoch;scenarioFixture.step=0;scenarioFixture.achievement.data.clear();r.Tick(1000);assert(r.State().epoch!=ticket&&r.source.attacks.empty());r.Tick(1000);assert(r.source.attacks.empty());r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();++cases;}
 {Run r;r.Begin();r.source.state=CORPSE;r.Tick(2000);assert(!r.State().started&&r.source.attacks.empty());r.source.state=ALIVE;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 for(uint32 blocked=0;blocked<3;++blocked){Run r;if(blocked==0)r.ally.preDelete=true;if(blocked==1)r.ally.worlded=false;if(blocked==2)r.player.allyVisible=false;r.Begin();r.Tick(5000);assert(!r.State().spoken&&r.source.attacks.empty());r.ally.preDelete=false;r.ally.worlded=r.player.allyVisible=true;r.Tick(1000);r.Tick(2000);r.Done();++cases;}
 for(uint32 dialogue:{1386u,770u}){Run r;bool replaced=false;Conversation::onCreate=[&](uint32 id){if(id==dialogue&&!replaced){replaced=true;r.Replace();}};r.Begin();assert(replaced&&r.source.attacks.empty());if(dialogue==770)r.Tick(1000);r.Tick(1999);assert(r.source.attacks.empty());r.Tick(1);r.Done();++cases;}
 {Run r;bool replaced=false;r.player.onEvent=[&](uint32 id){if(id==46877&&!replaced){replaced=true;r.Replace();}};r.Begin();r.Tick(2000);r.Done();assert(replaced);++cases;}
 {Run r;Conversation::onCreate=[&](uint32 id){if(id==1386){r.player.quest.ScriptLifetime.reset();r.player.questState=0;}};r.Begin();r.Tick(10000);assert(!r.State().started&&r.source.attacks.empty());++cases;}
 std::cout<<"PASS "<<cases<<" opening ASAN component contexts: actual instance-owned opening/first-wave state, actual complete Ragnvald and original battle payload, native scheduler/quest helper/locked lifecycle/destruction/visibility predicate. Native criteria/conversation/map/player endpoints controlled; not whole-core/native Player or client traversal.\n";
}
