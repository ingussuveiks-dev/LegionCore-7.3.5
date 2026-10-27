#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
enum{SPELL_CAST_OK=0,CLASS_WARRIOR=1,ALLIANCE=469,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,EFFECT_0=0,SPELL_AURA_ACTIVATE_SCENE=430,AURA_EFFECT_HANDLE_REAL=1,UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=1,UNIT_FLAG_IMMUNE_TO_NPC=2,UNIT_FLAG_NOT_ATTACKABLE_1=4,UNIT_STATE_CASTING=1,EVENT_1=1,EVENT_2,EVENT_3,EVENT_4,EVENT_5,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
using DamageEffectType=uint32;using AuraEffectHandleModes=uint32;
static uint32 warnings=0;static std::vector<std::string> order;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid{uint64 id=0;bool operator==(ObjectGuid g)const{return id==g.id;}bool operator!=(ObjectGuid g)const{return id!=g.id;}bool operator<(ObjectGuid g)const{return id<g.id;}void Clear(){id=0;}static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Player;struct Creature;struct CreatureAI;struct Map;using TempSummon=Creature;
struct QuestStatusData{uint32 Status=3;std::shared_ptr<uint8>ScriptLifetime;};
struct Scheduler{uint64 now=0;struct Task{uint64 when;std::function<void()>run;};std::vector<Task>tasks;void AddDelayedEvent(uint64 ms,std::function<void()>&&fn){tasks.push_back({now+ms,std::move(fn)});}void Advance(uint32 ms){now+=ms;for(;;){auto i=std::find_if(tasks.begin(),tasks.end(),[&](Task const&t){return t.when<=now;});if(i==tasks.end())break;auto f=std::move(i->run);tasks.erase(i);f();}}};
struct Unit:Scheduler{Player*owner=nullptr;virtual~Unit()=default;virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}virtual ObjectGuid GetGUID()const{return {};}Player*GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner;}virtual uint32 CastSpell(Unit*,uint32,bool=false);void Kill(Unit*);uint32 Apply(Creature*,uint32);};
struct Ref{Player*player;Player*getSource()const{return player;}};
struct Map{uint32 id=1500,instance=42;bool unload=false;std::vector<Ref>players;std::map<ObjectGuid,Creature*>actors;auto const&GetPlayers(){return players;}bool IsMapUnload(){return unload;}Creature*GetCreature(ObjectGuid id){auto i=actors.find(id);return i==actors.end()?nullptr:i->second;}}world;
struct Progress{uint64 Counter=0;};struct Achievement{std::map<uint32,Progress>data;auto const*GetCriteriaProgressMap(){return &data;}};
struct Scenario{uint32 id=933,step=4;Achievement achievement;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}Achievement&GetAchievementMgr(){return achievement;}}scenario;
struct Scenarios{bool missing=false;Scenario*GetScenario(uint32 id){return missing||id!=42?nullptr:&scenario;}}scenarioManager;auto*sScenarioMgr=&scenarioManager;
struct Maps{Map*FindMap(uint32 map,uint32 instance){return map==world.id&&instance==world.instance?&world:nullptr;}}maps;auto*sMapMgr=&maps;
struct Motion{uint32 moves=0;void MovePoint(uint32,float x,float y,float z){assert(std::abs(x+2092.75f)<.01f&&std::abs(y+1180.69f)<.01f&&z==31);++moves;}};
struct InstanceScript{void OnCreatureDamageTaken(Creature*,Unit*,uint32&){};};
struct Creature:Unit{ObjectGuid guid{100},summoner;uint32 entry=92608,spawn=361167,instance=42,flags=0;uint64 hp=1000,max=1000,targetMax=1000;float factor=1;Map*map=&world;bool alive=true,worlded=true,active=false,victim=false,casting=false,IsAIEnabled=true,temp=false;CreatureAI*ai=nullptr;CreatureAI*i_disabledAI=nullptr;Motion motion;std::set<ObjectGuid>hidden;
 Creature*ToCreature()override{return this;}ObjectGuid GetGUID()const override{return guid;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return spawn;}uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return instance;}Map*GetMap(){return map;}bool IsAlive(){return alive;}bool IsInWorld(){return worlded;}CreatureAI*AI(){return ai;}CreatureAI*GetAI(){return ai;}InstanceScript*GetInstanceScript(){return nullptr;}
 bool isActiveObject(){return active;}void setActive(bool b){active=b;}void SetFlag(uint32,uint32 f){flags|=f;}void RemoveFlag(uint32,uint32 f){flags&=~f;}bool HasFlag(uint32,uint32 f){return (flags&f)!=0;}bool HasUnitState(uint32){return casting;}uint64 GetHealth(Unit* =nullptr){uint64 scaled=hp*targetMax/max;return hp&&!scaled?1:scaled;}uint64 CountPctFromMaxHealth(uint32 pct,Unit*){return targetMax*pct/100;}bool HealthBelowPct(uint32 pct){return hp*100<max*pct;}float GetHealthMultiplierForTarget(Unit*){return factor;}
 Motion*GetMotionMaster(){return &motion;}Creature*FindNearestCreature(uint32,float,bool=true);Player*FindNearestPlayer(float);TempSummon*ToTempSummon(){return temp?this:nullptr;}ObjectGuid GetSummonerGUID(){return summoner;}
 bool ShouldHideFor(ObjectGuid g){return hidden.count(g)!=0;}void AddToHideList(ObjectGuid g){hidden.insert(g);}void RemoveFromHideList(ObjectGuid g){hidden.erase(g);}void UpdateObjectVisibility(){order.push_back(hidden.empty()?"show":"hide");}
};
static Creature*boss=nullptr,*ally=nullptr,*valkyr=nullptr;static std::function<void(Creature*,Unit*)>instanceDeath;
struct Player:Unit{ObjectGuid guid{1};Map*map=&world;uint32 cls=1,questId=38904,status=3,marker=0,credits=0,criteria=0,teleports=0,voices=0,casts=0,team=67,m_scenarioLoginId=0,m_scenarioLoginInstance=0;struct Resume{uint32 scenario=0;}m_scenarioResume{933};bool worlded=true,contact=true,gm=false,pending=false,phase=true,rewarded=false,castFails=false,creditFails=false,eventFails=false,teleportFails=false;QuestStatusData quest;std::set<uint32>auras;
 Player*ToPlayer()override{return this;}ObjectGuid GetGUID()const override{return guid;}uint32 GetMapId(){return map->id;}uint32 GetInstanceId(){return map->instance;}Map*GetMap(){return map;}uint32 getClass(){return cls;}uint32 GetTeam(){return team;}bool IsInWorld(){return worlded;}bool CanContact(){return contact;}bool isGameMaster(){return gm;}bool HasPendingTeleport(){return pending;}bool InSamePhase(Creature*){return phase;}bool GetQuestRewardStatus(uint32 q){return q==questId&&rewarded;}bool IsQuestRewarded(uint32 q){return GetQuestRewardStatus(q);}uint32 GetQuestStatus(uint32 q){return q==questId?status:0;}QuestStatusData*getQuestStatus(uint32 q){quest.Status=status;return q==questId&&status?&quest:nullptr;}
 uint32 GetQuestObjectiveData(uint32 q,uint32 id){assert(id==103661);return q==questId?marker:0;}void KilledMonsterCredit(uint32 id){assert(id==103661);++credits;if(!creditFails){marker=1;status=1;}}void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==46881);++criteria;if(!eventFails){scenario.achievement.data[45777].Counter=1;scenario.step=5;}}
 bool HasAura(uint32 id,ObjectGuid){return auras.count(id)!=0;}void RemoveAurasDueToSpell(uint32 id,ObjectGuid){auras.erase(id);order.push_back("clean"+std::to_string(id));if(id==185673&&valkyr)valkyr->worlded=false;}void RemoveAura(uint32 id){auras.erase(id);}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 uint32 CastSpell(Unit*,uint32,bool=false)override;bool CreateConversation(uint32 id){assert(id==826);++voices;return true;}Creature*FindNearestCreature(uint32 id,float,bool=true);
 bool TeleportTo(uint32 map,float x,float y,float z,float o){assert(map==1479&&std::abs(x-1071.81f)<.01f&&std::abs(y-7224.32f)<.01f&&std::abs(z-97.891f)<.01f&&std::abs(o-3.15048f)<.001f);++teleports;order.push_back("teleport");return !teleportFails;}void SetScenarioId(uint32){}bool ExitCompletedBrokenShoreLogin();
};
static std::map<ObjectGuid,Player*>players;
struct ObjectAccessor{static Player*FindPlayer(ObjectGuid g){auto i=players.find(g);return i==players.end()?nullptr:i->second;}};
Creature*Creature::FindNearestCreature(uint32 id,float,bool){return id==97166?ally:nullptr;}Player*Creature::FindNearestPlayer(float){return players.empty()?nullptr:players.begin()->second;}
Creature*Player::FindNearestCreature(uint32 id,float,bool){return id==92608?boss:id==93818&&valkyr&&valkyr->worlded?valkyr:nullptr;}
uint32 Player::CastSpell(Unit*target,uint32 id,bool){assert(target==this&&id==191979);++casts;if(castFails)return 1;auras.insert(185627);auras.insert(185673);auras.insert(185674);if(valkyr){valkyr->temp=true;valkyr->summoner=guid;valkyr->worlded=true;}return 0;}
uint32 Unit::CastSpell(Unit*,uint32 id,bool){if(id==185714)order.push_back("resurrection");return 0;}
struct CreatureAI{Creature*me;explicit CreatureAI(Creature*c):me(c){c->ai=this;}virtual~CreatureAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void DoAction(int32){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void JustDied(Unit*){}virtual void UpdateAI(uint32){}virtual void AttackStart(Unit*){me->victim=true;}};
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;uint32 spellCasts=0;bool UpdateVictim(){return me->victim;}void DoCast(uint32){++spellCasts;}void DoMeleeAttackIfReady(){}void EnterEvadeMode(){Reset();me->hp=me->max;me->victim=false;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual~CreatureScript()=default;virtual CreatureAI*GetAI(Creature*)const{return nullptr;}};
struct EventMap{uint64 now=0;std::map<uint32,uint64>events;void Reset(){events.clear();}void RescheduleEvent(uint32 id,uint32 delay){events[id]=now+delay;}void Update(uint32 diff){now+=diff;}uint32 ExecuteEvent(){for(auto i=events.begin();i!=events.end();++i)if(i->second<=now){uint32 id=i->first;events.erase(i);return id;}return 0;}};
enum class HighGuid{Conversation};struct Generator{uint32 Generate(){return 1;}}generator;struct ObjectMgr{template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto*sObjectMgr=&objects;
struct Conversation{bool CreateConversation(uint32,uint32,Unit*,void*,Unit&){return false;}};
struct Position{float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetPositionZ(){return 0;}float GetOrientation(){return 0;}};struct SpellTargetPosition{uint32 target_mapId=0;float target_X=0,target_Y=0,target_Z=0,target_Orientation=0;};struct SpellMgr{SpellTargetPosition const*GetSpellTargetPosition(uint32){return nullptr;}}spellMgr;auto*sSpellMgr=&spellMgr;struct MapManager{static bool IsValidMapCoord(uint32,float,float,float,float){return true;}};
struct AuraEffect{};struct Hook{void operator+=(int){}};struct AuraScript{Unit*owner=nullptr;Unit*GetUnitOwner(){return owner;}Hook OnEffectApply,AfterEffectRemove;virtual void Register(){}};
#define PrepareAuraScript(name) public:
#define AuraEffectApplyFn(...) 0
#define AuraEffectRemoveFn(...) 0
#include "QuestDelay.inc"
#include "Finale.inc"
#include "Scene.inc"
#include "Actor.inc"
#define npc_malgalor npc_malgalor_baseline
#include "Baseline.inc"
#undef npc_malgalor
struct Instance{void CreatureDies(Creature*,Unit*);
};
#include "InstanceDeath.inc"
#include "LoginExit.inc"
void Unit::Kill(Unit*victim){auto*c=victim->ToCreature();assert(c);if(!c->alive)return;c->alive=false;c->hp=0;if(c->ai&&c->IsAIEnabled)c->ai->JustDied(this);if(instanceDeath)instanceDeath(c,this);}
uint32 Unit::Apply(Creature*victim,uint32 damage){DamageEffectType damagetype=0;if(victim->ai&&victim->IsAIEnabled)victim->ai->DamageTaken(this,damage,damagetype);uint64 health=victim->GetHealth(this);
#include "PostScale.inc"
 if(damage>=health)Kill(victim);else victim->hp=(health-damage)*victim->max/victim->targetMax;return damage;}
struct Run{Player player,other;Creature source,friendNpc,rescue;ScriptedAI friendAI;Instance instance;std::unique_ptr<npc_malgalor::npc_malgalorAI>ai;
 Run():friendAI(&friendNpc){warnings=0;order.clear();players.clear();world=Map{};scenario=Scenario{};scenarioManager.missing=false;boss=&source;ally=&friendNpc;valkyr=&rescue;friendNpc.entry=97166;rescue.entry=93818;rescue.worlded=false;player.guid={1};other.guid={2};players[player.guid]=&player;world.players={{&player}};world.actors[source.guid]=&source;ai=std::make_unique<npc_malgalor::npc_malgalorAI>(&source);ai->Reset();source.victim=true;instanceDeath=[this](Creature*c,Unit*u){instance.CreatureDies(c,u);};}
 ~Run(){instanceDeath={};}void Ready(){ai->DoAction(0);ai->UpdateAI(30000);assert(!source.flags);}void Finish(Unit*attacker=nullptr){if(!attacker)attacker=&player;Ready();uint32 hit=uint32(source.GetHealth(attacker)+1);attacker->Apply(&source,hit);assert(ai->defeating&&source.alive);source.victim=false;ai->UpdateAI(5499);assert(source.alive);ai->UpdateAI(1);assert(!source.alive&&player.marker&&player.credits==1&&player.criteria==1);}
};
int main(){
 {Run r;npc_malgalor_baseline::npc_malgalorAI old(&r.source);old.Reset();r.source.flags=0;r.source.ai=&old;instanceDeath={};r.player.Apply(&r.source,1001);assert(!r.source.alive&&!r.player.marker&&!r.player.criteria);std::cout<<"COUNTERFACTUAL actual old AI permits a fresh one-hit death and never requests46881/103661.\n";}
 {Run r;npc_malgalor_baseline::npc_malgalorAI old(&r.source);old.Reset();r.source.flags=0;r.source.ai=&old;instanceDeath={};r.source.hp=100;r.player.Apply(&r.source,1);r.source.victim=false;old.UpdateAI(60000);assert(r.source.alive&&!r.player.marker&&old.outro);std::cout<<"COUNTERFACTUAL old low-health outro stops its5500ms event behind UpdateVictim after losing target.\n";}
 for(uint32 quest:{38904u,42815u})for(uint64 max:{1ull,100ull,1000ull,1000000ull})for(uint32 kind:{0u,1u,2u,3u})for(uint32 health:{100u,21u}){Run r;r.player.questId=quest;r.source.targetMax=max;r.source.hp=r.source.max*health/100;Unit pet,guardian;pet.owner=&r.player;guardian.owner=&r.player;Unit*attacker=kind==0?static_cast<Unit*>(&r.player):kind==1?&pet:kind==2?&guardian:static_cast<Unit*>(&r.friendNpc);r.Finish(attacker);assert(!warnings);r.ai->JustDied(attacker);r.instance.CreatureDies(&r.source,attacker);assert(r.player.credits==1&&r.player.criteria==1);r.player.Advance(4000);assert(r.player.voices==1);r.player.Advance(4000);assert(r.player.teleports==1&&r.player.auras.empty());auto res=std::find(order.begin(),order.end(),"resurrection"),clean=std::find(order.begin(),order.end(),"clean185673"),tele=std::find(order.begin(),order.end(),"teleport");assert(res<clean&&clean<tele);}
 {Run r;r.Ready();r.source.factor=10;r.source.targetMax=100;r.player.Apply(&r.source,1500);assert(r.ai->defeating&&r.source.hp==200);}
 {Run r;r.Ready();r.player.Apply(&r.source,10);r.player.Apply(&r.source,790);assert(!r.ai->defeating&&r.source.hp==200);r.player.Apply(&r.source,1);assert(r.ai->defeating&&!r.player.marker);r.player.Apply(&r.source,100000);assert(r.source.alive&&!r.player.marker);r.source.victim=false;r.ai->UpdateAI(5500);assert(r.player.marker);}
 {Run r;r.ai->DoAction(0);r.ai->DoAction(0);r.source.victim=false;r.ai->UpdateAI(24000);assert(r.source.motion.moves==1&&(r.source.flags&7)==7);r.ai->UpdateAI(6000);assert(!(r.source.flags&7)&&r.source.victim);}
 {Run r;r.ai->DoAction(0);r.ai->UpdateAI(1000);r.ai->Reset();r.ai->UpdateAI(30000);assert(!(r.source.flags&7)&&r.source.motion.moves==1);}
 {Run r;r.Finish();r.player.status=0;r.player.quest.ScriptLifetime.reset();r.player.Advance(8000);assert(!r.player.teleports&&r.player.auras.empty());}
 {Run r;r.Ready();r.player.Apply(&r.source,1001);r.player.quest.ScriptLifetime.reset();r.player.status=3;r.ai->UpdateAI(5500);assert(r.source.alive&&!r.player.marker&&r.source.hp==r.source.max);}
 {Run r;r.other.guid={2};players[r.other.guid]=&r.other;world.players.push_back({&r.other});r.Finish();assert(r.other.marker&&r.other.credits==1&&r.other.criteria==0);}
 for(uint32 bad=0;bad<6;++bad){Run r;r.other.guid={2};players[r.other.guid]=&r.other;world.players.push_back({&r.other});if(bad==0)r.other.cls=2;if(bad==1)r.other.status=0;if(bad==2)r.other.phase=false;if(bad==3)r.other.gm=true;if(bad==4)r.other.pending=true;if(bad==5)r.other.contact=false;r.Finish();assert(!r.other.marker);}
 {Run r;r.Ready();r.source.IsAIEnabled=false;r.player.Kill(&r.source);assert(r.player.marker&&r.player.criteria==1);}
 {Run r;r.Ready();r.player.Kill(&r.source);assert(warnings==1&&r.player.marker);}
 {Run r;r.Finish();r.player.teleportFails=true;r.player.Advance(8000);assert(r.player.teleports==1&&warnings==1&&r.player.marker);r.player.teleportFails=false;r.player.Advance(2000);assert(r.player.teleports==2);}
 {Run r;r.player.castFails=true;r.Finish();assert(warnings==1&&r.player.auras.empty());r.player.Advance(8000);assert(r.player.teleports==1);}
 {Run r;r.Finish();Map elsewhere;elsewhere.id=1220;r.player.map=&elsewhere;r.player.Advance(8000);assert(!r.player.teleports&&r.player.auras.empty());}
 {Run r;r.player.auras.insert(185627);r.Finish();r.player.Advance(8000);assert(r.player.auras==std::set<uint32>({185627}));}
 {Run r;r.player.Kill(&r.source);assert(!r.player.marker);}
 {Run r;r.Ready();r.source.spawn=123;r.player.Kill(&r.source);assert(!r.player.marker);}
 {Run r;r.Ready();scenario.id=775;r.player.Kill(&r.source);assert(!r.player.marker);}
 {Run r;r.Finish();r.player.tasks.clear();r.player.quest.ScriptLifetime.reset();r.player.auras={185627,185673,185674};assert(r.player.ExitCompletedBrokenShoreLogin()&&r.player.teleports==1&&r.player.auras.empty());}
 for(uint32 bad=0;bad<4;++bad){Run r;r.player.status=1;r.player.marker=1;if(bad==0)r.player.cls=2;if(bad==1)r.player.marker=0;if(bad==2)r.player.status=3;if(bad==3)r.player.m_scenarioLoginId=961;assert(!r.player.ExitCompletedBrokenShoreLogin()&&!r.player.teleports);}
 {Run r;spell_warrior_malgalor_arrival scene;scene.owner=&r.player;scene.Apply(nullptr,1);scene.Apply(nullptr,1);assert(r.source.hidden.size()==1&&order==std::vector<std::string>({"hide"}));Map otherMap;otherMap.id=1220;r.player.map=&otherMap;scene.Removed(nullptr,1);scene.Removed(nullptr,1);assert(r.source.hidden.empty()&&order.back()=="show");}
 {Run r;r.source.AddToHideList(r.player.guid);spell_warrior_malgalor_arrival scene;scene.owner=&r.player;scene.Apply(nullptr,1);scene.Removed(nullptr,1);assert(r.source.ShouldHideFor(r.player.guid));}
 {Run r;spell_warrior_malgalor_arrival scene;scene.owner=&r.player;scenario.id=775;scene.Apply(nullptr,1);assert(r.source.hidden.empty());}
 {Run r;spell_warrior_malgalor_arrival scene;scene.owner=&r.player;r.source.spawn=1;scene.Apply(nullptr,1);assert(r.source.hidden.empty()&&warnings==1);}
 std::cout<<"PASS actual Malgalor AI/finale/instance-death/quest-delay/login-exit/scene hooks:64 faction/target-health/player-pet-guardian-ally one-hit paths, final-scale clamp, victimless5500ms, native24/30s intro/reset, exactly-once/group/foreign members, abandon/reaccept, missing AI, direct death, teleport retry, persisted victory recovery, per-viewer scene hide/cleanup/foreign hide retention. Native endpoints remain controlled; client traversal separate.\n";
}
