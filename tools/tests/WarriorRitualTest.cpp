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
enum{QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CLASS_WARRIOR=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_STATE_CASTING=1,UNIT_FIELD_FLAGS=1,UNIT_FIELD_CREATED_BY_SPELL=2,UNIT_FLAG_IMMUNE_TO_PC=1,UNIT_FLAG_IMMUNE_TO_NPC=2,UNIT_FLAG_NOT_ATTACKABLE_1=4,TEMPSUMMON_MANUAL_DESPAWN=8};
enum SpellCastResult{SPELL_CAST_OK=0,SPELL_FAILED_ERROR=1};
enum DeathState{ALIVE,CORPSE};
static uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
#define ASSERT(value,...) assert(value)
struct ObjectGuid
{
 uint64 low=0,high=0;bool operator==(ObjectGuid g)const{return low==g.low&&high==g.high;}bool operator!=(ObjectGuid g)const{return !(*this==g);}bool operator<(ObjectGuid g)const{return high==g.high?low<g.low:high<g.high;}
 bool IsEmpty()const{return !low&&!high;}void Clear(){low=high=0;}uint64 GetLowPart()const{return low;}uint64 GetHighPart()const{return high;}
};
#include "Functions.inc"
struct Map;struct Player;struct Creature;struct TempSummon;struct CreatureAI;struct InstanceScript;struct Conversation;
struct Unit
{
 FunctionProcessor m_Functions;Player*owner=nullptr;virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}virtual ObjectGuid GetGUID()const{return {};}
 virtual Map*GetMap()const{return nullptr;}Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner;}
 #include "UnitTimer.inc"
};
struct Ref{Player*player;Player*getSource()const{return player;}};
struct Map
{
 uint32 id=1500,instance=42;bool unload=false;InstanceScript*script=nullptr;std::vector<Ref>players;std::map<ObjectGuid,Creature*>actors;std::map<ObjectGuid,Conversation*>conversations;
 uint32 GetId(){return id;}uint32 GetInstanceId(){return instance;}bool IsMapUnload(){return unload;}auto const&GetPlayers(){return players;}Creature*GetCreature(ObjectGuid key){auto i=actors.find(key);return i==actors.end()?nullptr:i->second;}
}world;
struct Criterion{uint64 Counter=0;};struct Achievement{std::map<uint32,Criterion>data;auto const*GetCriteriaProgressMap(){return &data;}};
struct Scenario{uint32 id=933,step=0;Achievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Achievement&GetAchievementMgr(){return achievement;}}scenarioFixture;
struct Scenarios{bool missing=false;Scenario*GetScenario(uint32 key){return missing||key!=world.instance?nullptr:&scenarioFixture;}}scenarioManager;auto*sScenarioMgr=&scenarioManager;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8>ScriptLifetime;};static std::map<ObjectGuid,Player*>players;
struct Player:Unit
{
 ObjectGuid guid{1,0xAABBCCDDAABBCCDD};Map*map=&world;uint32 cls=1,activeQuest=38904,questState=3,marker=0,team=67;bool contact=true,worlded=true,gm=false,pending=false,phase=true,rewarded=false;QuestStatusData quest;
 std::vector<uint32>events;std::map<uint32,bool>refused;std::function<void(uint32)>onEvent;
 Player*ToPlayer()override{return this;}ObjectGuid GetGUID()const override{return guid;}Map*GetMap()const override{return map;}uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return map->instance;}uint32 getClass(){return cls;}
 bool IsInWorld(){return worlded;}bool CanContact(){return contact;}bool isGameMaster(){return gm;}bool HasPendingTeleport(){return pending;}bool InSamePhase(Creature*){return phase;}
 bool GetQuestRewardStatus(uint32 q){return q==activeQuest&&rewarded;}uint32 GetQuestStatus(uint32 q){return q==activeQuest?questState:0;}QuestStatusData*getQuestStatus(uint32 q){quest.Status=questState;return q==activeQuest&&questState?&quest:nullptr;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 asset){assert(asset==103661);return q==activeQuest?marker:0;}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&(asset==49203||asset==49262));events.push_back(asset);if(onEvent)onEvent(asset);if(gm||!contact||!worlded||refused[asset]||scenarioFixture.step!=2)return;scenarioFixture.achievement.data[asset==49203?46912:46929].Counter=1;if(scenarioFixture.achievement.data[46912].Counter&&scenarioFixture.achievement.data[46929].Counter)scenarioFixture.step=3;}
};
struct ObjectAccessor
{
 static Player*FindPlayer(ObjectGuid key){auto i=players.find(key);return i==players.end()?nullptr:i->second;}static Creature*GetCreature(Unit const&actor,ObjectGuid key){return actor.GetMap()?actor.GetMap()->GetCreature(key):nullptr;}
 static Conversation*GetConversation(Unit const&actor,ObjectGuid key){if(!actor.GetMap())return nullptr;auto const&data=actor.GetMap()->conversations;auto i=data.find(key);return i==data.end()?nullptr:i->second;}
};
struct Motion{std::vector<std::vector<float>>moves;void MovePoint(uint32 point,float x,float y,float z){assert(!point);moves.push_back({x,y,z});}};
struct Creature:Unit
{
 ObjectGuid guid;Map*map=&world;uint32 entry=0,spawn=0,flags=0,createdSpell=0;bool worlded=true,preDelete=false,alive=true,casting=false,aura=false,phase=true;CreatureAI*ai=nullptr;bool IsAIEnabled=true;
 Motion motion;uint32 melee=0;std::vector<uint32>despawns,casts;std::map<uint32,bool>refused;std::function<SpellCastResult(uint32)>onCast;std::function<void()>onKill;float range=0;
 explicit Creature(uint32 id):guid{id,0x1122334411223344},entry(id){}virtual~Creature();Creature*ToCreature()override{return this;}ObjectGuid GetGUID()const override{return guid;}Map*GetMap()const override{return map;}virtual TempSummon*ToTempSummon(){return nullptr;}
 uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return spawn;}uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return map->instance;}bool IsAlive(){return alive;}bool IsInWorld(){return worlded;}bool IsPreDelete(){return preDelete;}CreatureAI*AI(){return ai;}
 DeathState getDeathState(){return alive?ALIVE:CORPSE;}bool HasFlag(uint32,uint32 mask){return (flags&mask)!=0;}uint32 GetUInt32Value(uint32 key){assert(key==UNIT_FIELD_CREATED_BY_SPELL);return createdSpell;}bool HasUnitState(uint32 key){assert(key==UNIT_STATE_CASTING);return casting;}bool HasAura(uint32 key){assert(key==105419);return aura;}
 bool InSamePhase(Creature*actor){return phase&&actor->phase;}bool IsWithinDistInMap(Creature*target,float radius){return target->map==map&&target->range<=radius;}
 Motion*GetMotionMaster(){return &motion;}float GetPositionX(){return 5;}float GetPositionY(){return 6;}void DespawnOrUnsummon(uint32 delay=0){despawns.push_back(delay);if(!delay)worlded=false;}void Kill(Creature*target){assert(target==this);alive=false;if(onKill)onKill();}
 InstanceScript*GetInstanceScript(){return map?map->script:nullptr;}SpellCastResult CastSpell(Creature*target,uint32 id,bool triggered=false){assert(target==this&&!triggered&&(id==205287||id==205295||id==205296));casts.push_back(id);if(refused[id])return SPELL_FAILED_ERROR;return onCast?onCast(id):SPELL_CAST_OK;}
 Player*FindNearestPlayer(float radius){assert(radius==100);return map->players.empty()?nullptr:map->players.front().player;}Creature*FindNearestCreature(uint32 id,float radius,bool live=true){for(auto const&item:map->actors)if(item.second->entry==id&&item.second->range<=radius&&item.second->alive==live)return item.second;return nullptr;}
 Creature*SummonCreature(uint32,float,float,float,float,uint32,uint32);
};
struct TempSummon:Creature{ObjectGuid summoner;using Creature::Creature;TempSummon*ToTempSummon()override{return this;}ObjectGuid GetSummonerGUID(){return summoner;}};
struct SpellInfo{uint32 Id=0;};struct CreatureAI
{
 Creature*me;explicit CreatureAI(Creature*actor):me(actor){}virtual~CreatureAI()=default;virtual uint32 GetData(uint32)const{return 0;}virtual void SpellHitTarget(Unit*,SpellInfo const*){}virtual void DoAction(int32){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual void IsSummonedBy(Unit*){}void DoCast(uint32 id){me->CastSpell(me,id,false);}
};
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;void UpdateAI(uint32)override;bool UpdateVictim(){return true;}void DoMeleeAttackIfReady(){++me->melee;}};
#include "BaseUpdate.inc"
struct CreatureScript{explicit CreatureScript(char const*){}virtual~CreatureScript()=default;virtual CreatureAI*GetAI(Creature*)const{return nullptr;}};
struct InstanceScript{virtual~InstanceScript()=default;virtual ObjectGuid GetGuidData(uint32)const{return {};}virtual void SetGuidData(uint32,ObjectGuid){}};
Creature::~Creature(){delete ai;if(map){auto i=map->actors.find(guid);if(i!=map->actors.end()&&i->second==this)map->actors.erase(i);}}
int32 irand(int32 low,int32 high){assert(low<=0&&high>=0);return 0;}void GetCreatureListWithEntryInGrid(std::list<Creature*>&result,Creature*actor,uint32 entry,float radius){for(auto const&item:actor->map->actors)if(item.second->entry==entry&&item.second->worlded&&item.second->range<=radius)result.push_back(item.second);}
struct Texts{uint32 calls=0;void SendChat(Creature*actor,uint32 group){assert(actor->entry==97166&&!group);++calls;}}texts;auto*sCreatureTextMgr=&texts;
enum class HighGuid{Conversation};struct Generator{uint64 next=1;uint64 Generate(){return next++;}}generator;struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto*sObjectMgr=&objects;
struct Conversation
{
 struct Actor{uint64 GuidLow=0,GuidHight=0;};static inline std::vector<Conversation*>allocated;static inline uint32 created=0,removed=0,corruption=0;static inline bool fail=false;static inline std::function<void()>onCreate;
 Map*map=nullptr;ObjectGuid guid;bool worlded=false;std::vector<Actor>actors;
 Conversation(){allocated.push_back(this);}~Conversation(){if(map)map->conversations.erase(guid);allocated.erase(std::find(allocated.begin(),allocated.end(),this));}
 bool CreateConversation(uint64 key,uint32 entry,Unit*caster,void*,Unit&){assert(entry==1405);++created;if(onCreate)onCreate();if(fail)return false;map=caster->GetMap();guid={key,0xFACEFACEFACEFACE};worlded=corruption!=4;map->conversations[guid]=this;auto owner=caster->GetGUID();actors={{owner.low,owner.high}};for(auto const&item:map->actors)if(item.second->entry==97167){actors.push_back({item.first.low,item.first.high});break;}if(corruption==1)actors.clear();if(corruption==2)++actors[0].GuidLow;if(corruption==3)++actors[0].GuidHight;if(corruption==5)map->conversations.erase(guid);return true;}
 bool IsInWorld(){return worlded;}ObjectGuid GetGUID(){return guid;}auto const&GetCreatures(){return actors;}Actor const*GetCreature(uint8 index){assert(index<actors.size());return &actors[index];}void Remove(){++removed;worlded=false;if(map)map->conversations.erase(guid);}
};
#include "QuestDelay.inc"
#include "Original.inc"
namespace WarriorOpening{constexpr uint32 AIStatusData=93320,Idle=1,Queued=2;}
#include "Ritual.inc"
#include "Danica.inc"
struct npc_ragnvald_dragonborn{struct npc_ragnvald_dragonbornAI{static void ResumeBattle(Creature*){}};};
#include "BaselineDanica.inc"
struct RagnvaldAI:CreatureAI{using CreatureAI::CreatureAI;uint32 actions=0;uint32 GetData(uint32 key)const override{return key==WarriorOpening::AIStatusData?1:0;}void DoAction(int32 value)override{assert(value==1&&scenarioFixture.step==3);++actions;}};
struct Instance:InstanceScript
{
 WarriorRitual::State ritual{&world};ObjectGuid GetGuidData(uint32 key)const override{return key==WarriorRitual::DanicaData?ritual.danica:ObjectGuid{};}void SetGuidData(uint32 key,ObjectGuid value)override{if(key==WarriorRitual::HitData)ritual.Hit(value);}
};
static std::function<Creature*()>onSummon;
Creature*Creature::SummonCreature(uint32 id,float x,float y,float z,float o,uint32 type,uint32 duration){assert(entry==102970&&id==103608&&x==-2001&&y==-1255&&z==37&&!o&&type==8&&!duration);return onSummon?onSummon():nullptr;}
struct Run
{
 Player player,other;Creature demon{102970},first{102964},second{102965},ragnvald{97167},ally{97166},enemy{97176},nextEnemy{98257};TempSummon danica{103608};Instance instance;
 std::vector<std::unique_ptr<TempSummon>>ghosts;uint32 remaining=0,firstHits=0,secondHits=0;bool missingFirst=false,missingSecond=false;std::function<void(uint32)>onGhost;
 Run()
 {
  world=Map{};scenarioFixture=Scenario{};scenarioManager.missing=false;players.clear();warnings=0;texts.calls=0;Conversation::created=Conversation::removed=Conversation::corruption=0;Conversation::fail=false;Conversation::onCreate={};assert(Conversation::allocated.empty());world.script=&instance;
  world.players={{&player}};players[player.guid]=&player;other.guid.low=2;players[other.guid]=&other;demon.spawn=361138;demon.alive=false;first.spawn=361098;second.spawn=361099;ragnvald.spawn=361094;ally.spawn=361091;nextEnemy.spawn=361151;danica.summoner=demon.guid;
  for(Creature*actor:std::initializer_list<Creature*>{&demon,&first,&second,&ragnvald,&ally,&enemy,&nextEnemy,&danica}){world.actors[actor->guid]=actor;instance.ritual.Observe(actor);}
  ragnvald.ai=new RagnvaldAI(&ragnvald);danica.ai=new npc_danica_the_reclaimer::npc_danica_the_reclaimerAI(&danica);
  danica.onCast=[this](uint32 id){assert(id==205287);remaining=5000;danica.casting=true;return SPELL_CAST_OK;};
  first.onCast=[this](uint32 id){assert(id==205295);++firstHits;if(!missingFirst)Spawn(0);return SPELL_CAST_OK;};second.onCast=[this](uint32 id){assert(id==205296);++secondHits;if(!missingSecond)Spawn(1);return SPELL_CAST_OK;};onSummon=[this](){world.actors[danica.guid]=&danica;danica.worlded=true;instance.ritual.Observe(&danica);return &danica;};
 }
 ~Run(){onSummon={};Conversation::onCreate={};while(!Conversation::allocated.empty())delete Conversation::allocated.back();}
 void Spawn(uint32 index){auto actor=std::make_unique<TempSummon>(103688-index);actor->guid.low=100000+ghosts.size();actor->summoner=index?second.guid:first.guid;actor->createdSpell=205295+index;assert(!world.actors.count(actor->guid));world.actors[actor->guid]=actor.get();instance.ritual.Observe(actor.get());ghosts.push_back(std::move(actor));if(onGhost)onGhost(index);}
 void Death(Unit*killer=nullptr){instance.ritual.Death(&demon,killer);}void Phase2(){scenarioFixture.step=2;scenarioFixture.achievement.data[45771].Counter=1;scenarioFixture.achievement.data[46175].Counter=1;}
 void Hit(Unit*target,uint32 id=205287){SpellInfo info{id};if(danica.ai)danica.ai->SpellHitTarget(target,&info);}
 void Tick(uint32 diff){if(remaining){if(diff>=remaining){remaining=0;danica.casting=false;Hit(&danica);Hit(&ally);Hit(&first);Hit(&second);}else remaining-=diff;}instance.ritual.Update(diff);}
 void Begin(){Death(&player);Phase2();Tick(0);Tick(2999);assert(danica.casts.empty());Tick(1);assert(remaining==5000);}void Complete(){Begin();Tick(4999);assert(player.events.empty());Tick(1);assert(scenarioFixture.step==3);Tick(1999);assert(!instance.ritual.departed);Tick(1);assert(instance.ritual.resumed);}
};
int main()
{
 uint32 checks=0;
 {Run r;r.danica.ai->UpdateAI(1000);assert(!r.danica.melee);static_cast<ScriptedAI*>(r.danica.ai)->ScriptedAI::UpdateAI(1000);assert(r.danica.melee==1);++checks;}
 {Run r;Conversation::fail=true;r.Phase2();npc_danica_the_reclaimer_baseline::npc_danica_the_reclaimer_baselineAI old(&r.danica);old.IsSummonedBy(&r.demon);old.UpdateLifecycle(3000);assert(r.remaining==5000&&r.ghosts.empty()&&scenarioFixture.step==3&&r.player.events.size()==2);SpellInfo info{205287};old.SpellHitTarget(&r.danica,&info);old.UpdateAI(2000);assert(!r.danica.despawns.empty()&&r.ghosts.empty());std::cout<<"COUNTERFACTUAL pinned1531780a2 complete Danica grants both events despite failed1405 and before normal5s cast completion, then departs after an unrelated self hit with no spirits. Repaired current component cases require both actual owned native receipts.\n";++checks;}
 {Run r;r.Complete();assert(r.firstHits==1&&r.secondHits==1&&r.ghosts.size()==2&&texts.calls==1&&Conversation::created==1);assert((r.player.events==std::vector<uint32>{49203,49262}));assert(r.ghosts[0]->despawns==std::vector<uint32>{4000}&&r.danica.despawns==std::vector<uint32>{4000});assert(static_cast<RagnvaldAI*>(r.ragnvald.ai)->actions==1);r.Tick(100000);assert(static_cast<RagnvaldAI*>(r.ragnvald.ai)->actions==1);++checks;}
 for(uint32 quest:{38904u,42815u})for(uint32 owner:{0u,1u,2u}){Run r;r.player.activeQuest=quest;r.player.team=quest==38904?67:469;Unit pet;pet.owner=&r.player;r.Death(owner==0?static_cast<Unit*>(&r.player):owner==1?&pet:nullptr);assert(r.instance.ritual.members.front().player==r.player.guid);r.Phase2();r.Tick(0);r.Tick(3000);r.Tick(5000);assert(r.player.events.size()==2);++checks;}
 {Run r;r.Death();r.Tick(100000);assert(!r.instance.ritual.prepared&&r.first.alive&&r.second.alive&&r.danica.casts.empty()&&r.player.events.empty());r.Phase2();r.Tick(0);assert(r.first.alive==false);r.Tick(3000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 for(uint32 i=0;i<2;++i){Run r;r.Begin();r.Hit(&r.danica);r.Hit(&r.ally);r.Hit(i?static_cast<Unit*>(&r.first):static_cast<Unit*>(&r.second),205295);assert(r.ghosts.empty()&&r.player.events.empty()&&!r.instance.ritual.departed);++checks;}
 for(uint32 i=0;i<2;++i){Run r;if(i)r.second.refused[205296]=true;else r.first.refused[205295]=true;r.Begin();r.Tick(5000);assert(r.player.events.empty()&&r.ghosts.size()==1);auto accepted=i?r.firstHits:r.secondHits;r.first.refused.clear();r.second.refused.clear();r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3&&r.ghosts.size()==2&&(i?r.firstHits:r.secondHits)==accepted);++checks;}
 for(uint32 i=0;i<2;++i){Run r;if(i)r.missingSecond=true;else r.missingFirst=true;r.Begin();r.Tick(5000);assert(r.player.events.empty()&&r.ghosts.size()==1);r.missingFirst=r.missingSecond=false;r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3&&r.ghosts.size()==2);++checks;}
 for(uint32 corruption=1;corruption<=5;++corruption){Run r;Conversation::corruption=corruption;r.Death();r.Phase2();r.Tick(0);r.Tick(3000);assert(r.danica.casts.empty()&&r.player.events.empty()&&!r.instance.ritual.spoken);Conversation::corruption=0;r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;Conversation::fail=true;r.Death();r.Phase2();r.Tick(0);r.Tick(3000);assert(r.danica.casts.empty());Conversation::fail=false;r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.danica.refused[205287]=true;r.Death();r.Phase2();r.Tick(0);r.Tick(3000);assert(!r.instance.ritual.castAccepted&&r.player.events.empty());r.danica.refused.clear();r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3&&Conversation::created==1);++checks;}
 {Run r;r.Begin();r.Tick(2500);r.remaining=0;r.danica.casting=false;r.Tick(1000);assert(r.remaining==5000&&r.player.events.empty());r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.Begin();r.Tick(5000);auto ghosts=r.ghosts.size();r.Hit(&r.first);r.Hit(&r.second);r.Tick(100000);assert(r.ghosts.size()==ghosts&&r.player.events.size()==2&&r.instance.ritual.resumed);++checks;}
 for(uint32 i=0;i<2;++i){Run r;r.player.refused[i?49262:49203]=true;r.Begin();r.Tick(5000);assert(scenarioFixture.step==2&&!r.instance.ritual.departed);auto firstEvents=std::count(r.player.events.begin(),r.player.events.end(),49203u);r.player.refused.clear();r.Tick(2000);assert(scenarioFixture.step==3);if(i)assert(std::count(r.player.events.begin(),r.player.events.end(),49203u)==firstEvents);++checks;}
 for(uint32 key=0;key<11;++key){Run r;if(key==0)r.player.gm=true;if(key==1)r.player.cls=2;if(key==2)r.player.questState=0;if(key==3)r.player.questState=1;if(key==4)r.player.rewarded=true;if(key==5)r.player.marker=1;if(key==6)r.player.phase=false;if(key==7)r.player.pending=true;if(key==8)r.player.contact=false;if(key==9)r.player.worlded=false;if(key==10)r.demon.flags=UNIT_FLAG_IMMUNE_TO_PC;r.Death();assert(!r.instance.ritual.pending);r.Phase2();r.Tick(100000);assert(r.player.events.empty()&&r.danica.casts.empty());++checks;}
 for(uint32 key=0;key<5;++key){Run r;r.Begin();if(key==0)r.player.questState=0;if(key==1)r.player.quest.ScriptLifetime=std::make_shared<uint8>(0);if(key==2)players.erase(r.player.guid);if(key==3)scenarioFixture.id=775;if(key==4)world.unload=true;r.Tick(5000);assert(r.player.events.empty()&&!r.instance.ritual.pending&&!r.danica.worlded);++checks;}
 {Run r;r.Begin();delete r.danica.ai;r.danica.ai=new npc_danica_the_reclaimer::npc_danica_the_reclaimerAI(&r.danica);r.Tick(5000);assert(scenarioFixture.step==3&&r.ghosts.size()==2);++checks;}
 {Run r;r.Begin();delete r.danica.ai;r.danica.ai=nullptr;r.Tick(5000);assert(r.player.events.empty());r.danica.ai=new npc_danica_the_reclaimer::npc_danica_the_reclaimerAI(&r.danica);r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.Begin();r.onGhost=[&](uint32 index){if(!index){delete r.danica.ai;r.danica.ai=nullptr;}};r.Tick(5000);assert(r.ghosts.size()==1&&r.player.events.empty());r.onGhost={};r.danica.ai=new npc_danica_the_reclaimer::npc_danica_the_reclaimerAI(&r.danica);r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3&&r.ghosts.size()==2&&r.firstHits==1);++checks;}
 {Run r;r.Begin();r.player.onEvent=[&](uint32 asset){if(asset==49203)r.player.questState=0;};r.Tick(5000);assert(r.player.events==std::vector<uint32>{49203});r.Tick(2000);assert(!r.instance.ritual.pending&&!r.instance.ritual.departed);++checks;}
 {Run r;Conversation::onCreate=[&](){r.player.questState=0;};r.Death();r.Phase2();r.Tick(0);r.Tick(3000);assert(r.danica.casts.empty());r.Tick(1000);assert(!r.instance.ritual.pending);++checks;}
 {Run r;r.instance.ritual.danica.Clear();world.actors.erase(r.danica.guid);r.Death();r.Phase2();r.Tick(0);assert(r.instance.ritual.prepared&&r.instance.ritual.danica==r.danica.guid);r.Tick(3000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.instance.ritual.danica.Clear();world.actors.erase(r.danica.guid);onSummon={};r.Death();world.actors.erase(r.demon.guid);r.Phase2();r.Tick(100000);assert(r.player.events.empty()&&r.instance.ritual.pending&&!r.instance.ritual.prepared);++checks;}
 {Run r;r.Begin();r.Tick(5000);delete r.ragnvald.ai;r.ragnvald.ai=nullptr;r.Tick(2000);assert(r.instance.ritual.departed&&!r.instance.ritual.resumed);r.ragnvald.ai=new RagnvaldAI(&r.ragnvald);r.Tick(100000);assert(r.instance.ritual.resumed&&static_cast<RagnvaldAI*>(r.ragnvald.ai)->actions==1);++checks;}
 {Run r;r.Death();r.Phase2();r.Tick(100000);assert(r.instance.ritual.prepared&&r.danica.casts.empty());r.Tick(2999);assert(r.danica.casts.empty());r.Tick(1);r.Tick(5000);assert(scenarioFixture.step==3&&!r.instance.ritual.departed);r.Tick(1999);assert(!r.instance.ritual.departed);r.Tick(1);assert(r.instance.ritual.resumed);++checks;}
 {Run r;r.Begin();r.Tick(100000);assert(scenarioFixture.step==3&&!r.instance.ritual.departed);r.Tick(1999);assert(!r.instance.ritual.departed);r.Tick(1);assert(r.instance.ritual.resumed);++checks;}
 for(uint32 key=0;key<5;++key){Run r;r.Begin();if(key==0)r.first.spawn=1;if(key==1)r.first.alive=true;if(key==2)r.first.preDelete=true;if(key==3)r.first.worlded=false;if(key==4){r.instance.ritual.corpses[0].high++;}r.Tick(5000);assert(r.ghosts.size()==1&&r.player.events.empty());++checks;}
 for(uint32 key=0;key<5;++key){Run r;r.Begin();r.first.onCast=[&](uint32){r.Spawn(0);auto*ghost=r.ghosts.back().get();r.instance.ritual.candidates[0].Clear();if(key==0)ghost->summoner.high++;if(key==1)ghost->createdSpell=205296;if(key==2)ghost->entry=103687;if(key==3)ghost->worlded=false;if(key==4)ghost->preDelete=true;r.instance.ritual.Observe(ghost);return SPELL_CAST_OK;};r.Tick(5000);assert(r.instance.ritual.spirits[0].IsEmpty()&&r.player.events.empty());++checks;}
 {Run r;r.Begin();r.first.onCast=[&](uint32){r.Spawn(0);return SPELL_FAILED_ERROR;};r.Tick(5000);assert(r.instance.ritual.spirits[0].IsEmpty()&&!r.ghosts[0]->worlded&&r.player.events.empty());++checks;}
 for(uint32 key=0;key<3;++key){Run r;r.Begin();if(key==0)r.danica.summoner.high++;if(key==1)r.danica.createdSpell=205287;if(key==2)r.danica.preDelete=true;r.Tick(5000);assert(r.player.events.empty()&&r.ghosts.empty());++checks;}
 {Run r;r.Begin();r.Hit(&r.first);r.Hit(&r.first);assert(r.firstHits==1&&r.secondHits==0&&r.player.events.empty());r.Hit(&r.second);r.Hit(&r.second);r.Tick(0);assert(r.firstHits==1&&r.secondHits==1&&scenarioFixture.step==3);++checks;}
 {Run r;world.players={{&r.other},{&r.player}};Unit guardian;guardian.owner=&r.player;r.Death(&guardian);assert(r.instance.ritual.members.size()==2&&r.instance.ritual.members[0].player==r.player.guid);r.player.gm=true;r.Phase2();r.Tick(0);r.Tick(3000);r.Tick(5000);assert(r.player.events.empty()&&r.other.events.size()==2);++checks;}
 for(uint32 key=0;key<3;++key){Run r;r.Begin();if(key==0)r.player.contact=false;if(key==1)r.player.pending=true;if(key==2)r.player.phase=false;r.Tick(5000);assert(r.player.events.empty()&&r.instance.ritual.pending);r.player.contact=true;r.player.pending=false;r.player.phase=true;r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.Begin();r.Tick(5000);scenarioFixture.step=0;r.Tick(1);assert(!r.instance.ritual.pending&&!r.danica.worlded&&r.ghosts[0]->despawns.back()==0);++checks;}
 {Run r;r.Begin();r.player.onEvent=[&](uint32){r.instance.ritual.Cancel();};r.Tick(5000);assert(!r.instance.ritual.pending&&r.player.events.size()==1);++checks;}
 {Run r;r.Begin();r.onGhost=[&](uint32){r.instance.ritual.Cancel();};r.Tick(5000);assert(!r.instance.ritual.pending&&r.player.events.empty()&&!r.ghosts[0]->worlded);++checks;}
 {Run r;r.Death();auto attempt=r.player.quest.ScriptLifetime;r.instance.ritual.Cancel();r.player.quest.ScriptLifetime=std::make_shared<uint8>(uint8{0});r.instance.ritual.Observe(&r.danica);r.Death();r.Phase2();r.Tick(0);r.Tick(3000);r.Tick(5000);assert(scenarioFixture.step==2&&r.player.events.empty());++checks;}
 {Run r;r.first.range=99;r.second.range=107;r.Death();r.Phase2();r.Tick(0);assert(r.first.alive&&r.second.alive);r.first.range=72;r.second.range=79;r.Tick(3000);assert(!r.first.alive&&!r.second.alive&&r.remaining==5000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 for(uint32 key=0;key<4;++key){Run r;r.Begin();r.Tick(5000);if(key==0)r.nextEnemy.worlded=false;if(key==1)r.nextEnemy.alive=false;if(key==2)r.nextEnemy.spawn=1;if(key==3)r.nextEnemy.range=251;r.Tick(2000);assert(r.instance.ritual.departed&&!r.instance.ritual.resumed);r.nextEnemy.worlded=r.nextEnemy.alive=true;r.nextEnemy.spawn=361151;r.nextEnemy.range=249;r.Tick(1000);assert(r.instance.ritual.resumed&&static_cast<RagnvaldAI*>(r.ragnvald.ai)->actions==1);++checks;}
 for(uint32 key=0;key<2;++key){Run r;if(key==0)r.ally.range=101;else r.ally.phase=false;r.Begin();assert(texts.calls==0);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 {Run r;r.ragnvald.phase=false;r.Death();r.Phase2();r.Tick(0);r.Tick(3000);assert(r.danica.casts.empty()&&!r.instance.ritual.spoken);r.ragnvald.phase=true;r.Tick(1000);r.Tick(5000);assert(scenarioFixture.step==3);++checks;}
 std::cout<<"PASS "<<checks<<" actual ritual state/Danica bridge ASAN components: genuine original death/weak quests, normal cast/real owned spirit acknowledgements, two native leaves, exact3s+5s+2s modeled endpoints, missing-only retry/AI replacement and cleanup. Controlled Map/Player/Spell/Conversation endpoints; actual native cast/catalog separate, full933/client pending.\n";
}
