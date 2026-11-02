#include <algorithm>
#include <atomic>
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
struct ObjectGuid{uint64 low=0,high=0;bool operator==(ObjectGuid const&o)const{return low==o.low&&high==o.high;}bool operator!=(ObjectGuid const&o)const{return !(*this==o);}std::string ToString()const{return std::to_string(low)+":"+std::to_string(high);}};
#include "Functions.inc"
#include "DeathState.inc"
enum{TYPEID_UNIT=0,TYPEID_PLAYER=1,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_STATE_CASTING=1,EVENT_1=1,EVENT_2,EVENT_3,EVENT_4};
struct InstanceScript{uint32 GetData(uint32){return 0;}ObjectGuid GetGuidData(uint32){return {};}void SetData(uint32,uint32){}};
struct Creature;struct CreatureAI;struct Player;
struct Unit{FunctionProcessor m_Functions;virtual~Unit()=default;virtual uint32 GetTypeId(){return TYPEID_UNIT;}virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
#include "UnitTimer.inc"
};
struct Player:Unit{std::vector<uint32>credits;uint32 GetTypeId()override{return TYPEID_PLAYER;}Player*ToPlayer()override{return this;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92);credits.push_back(id);}};
struct SpellInfo{uint32 Id;};
struct Scenario{uint32 id=933,step=3;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}};
struct Scenarios{Scenario scenario;uint32 instanceId=42;bool missing=false;Scenario*GetScenario(uint32 instance){return missing||instance!=instanceId?nullptr:&scenario;}}scenarioManager;auto*sScenarioMgr=&scenarioManager;
struct EventMap{std::vector<std::pair<uint32,uint32>>events;void Reset(){events.clear();}void RescheduleEvent(uint32 id,uint32 delay){events.push_back({id,delay});}void Update(uint32){}uint32 ExecuteEvent(){return 0;}};
struct Pose{float x,y,z;};struct Motion{std::vector<Pose>points;void MovePoint(uint32 id,float x,float y,float z){assert(!id);points.push_back({x,y,z});}};
int32 irand(int32 low,int32 high){assert(low<=0&&high>=0);return 0;}
struct Creature:Unit{
 CreatureAI*i_AI=nullptr,*i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;DeathState state=ALIVE;uint32 entry=0,mapId=1500,instanceId=42;ObjectGuid guid;bool aura=false,inRange=true,worlded=true,preDelete=false;Motion motion;Player*nearest=nullptr;std::vector<Creature*>nearby;std::vector<uint32>attacks,casts,despawns;std::vector<uint8>talks;std::vector<Pose>homes;uint32 kills=0,actions=0;std::function<void()>onExternal;
 explicit Creature(uint32 id=0):entry(id),guid{id,0x1234567812345678}{}~Creature();Creature*ToCreature()override{return this;}CreatureAI*AI(){return i_AI;}InstanceScript*GetInstanceScript(){return nullptr;}ObjectGuid GetGUID(){return guid;}bool AIM_Destroy();void Lifecycle(uint32);DeathState getDeathState(){return state;}
 bool IsAlive(){return state==ALIVE;}bool IsInWorld(){return worlded;}bool IsPreDelete(){return preDelete;}bool HasUnitState(uint32 id){assert(id==UNIT_STATE_CASTING);return false;}bool HasAura(uint32 id){assert(id==105419);return aura;}bool IsWithinDistInMap(Unit*,float radius){assert(radius==15||radius==40);return inRange;}
 Creature*FindNearestCreature(uint32 id,float radius,bool alive=true){assert(alive&&(radius==100||radius==120||radius==200||radius==250));for(auto*p:nearby)if(p->entry==id&&p->IsAlive())return p;return nullptr;}
 Player*FindNearestPlayer(float radius){assert(radius==100);return nearest;}uint32 GetEntry(){return entry;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}float GetPositionX(){return 1;}float GetPositionY(){return 2;}
 Motion*GetMotionMaster(){return &motion;}void SetHomePosition(float x,float y,float z,float o){assert(!o);homes.push_back({x,y,z});}
 void Kill(Creature*p){assert(p==this);++kills;state=CORPSE;}void CastSpell(Creature*p,uint32 id){assert(p==this&&(id==205295||id==205296));casts.push_back(id);}
 void DespawnOrUnsummon(uint32 delay){assert(delay==4000);despawns.push_back(delay);}
};
struct CreatureAI{Creature*me;explicit CreatureAI(Creature*p):me(p){}virtual~CreatureAI()=default;virtual uint32 GetData(uint32)const{return 0;}virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void MoveInLineOfSight(Unit*){}virtual void DoAction(int32 id){assert(id==1);++me->actions;}virtual void UpdateAI(uint32){}virtual void UpdateLifecycle(uint32){}virtual void IsSummonedBy(Unit*){}virtual void SpellHitTarget(Unit*,SpellInfo const*){}
 void Talk(uint8,ObjectGuid={});void AttackStart(Creature*p){assert(p);me->attacks.push_back(p->entry);if(me->onExternal)me->onExternal();}void DoCast(uint32 id){me->casts.push_back(id);if(me->onExternal)me->onExternal();}bool UpdateVictim(){return false;}void DoMeleeAttackIfReady(){}
};
using ScriptedAI=CreatureAI;
struct CreatureScript{explicit CreatureScript(char const*){}virtual~CreatureScript()=default;virtual CreatureAI*GetAI(Creature*)const=0;};
Creature::~Creature(){if(i_disabledAI!=i_AI)delete i_disabledAI;delete i_AI;}
struct Text{void SendChat(Creature*p,uint8 id,ObjectGuid={}){p->talks.push_back(id);}}text;auto*sCreatureTextMgr=&text;
#include "Talk.inc"
#define ASSERT(condition,...) assert(condition)
#define TC_LOG_DEBUG(...) ((void)0)
uint32 diagnostics=0;
#define TC_LOG_ERROR(...) (++diagnostics)
#define TC_LOG_WARN(...) (++diagnostics)
#include "AIDestroy.inc"
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
void GetCreatureListWithEntryInGrid(std::list<Creature*>&out,Creature*p,uint32 entry,float radius){assert(radius==30||radius==90||radius==160);for(auto*q:p->nearby)if(q->entry==entry)out.push_back(q);}
struct ObjectAccessor{static Creature*GetCreature(Creature&source,ObjectGuid guid){for(auto*p:source.nearby)if(p->guid==guid)return p;return nullptr;}};
enum class HighGuid{Conversation};struct Generator{uint64 Generate(){return 1;}}generator;struct Objects{template<HighGuid>Generator*GetGenerator(){return &generator;}}objects;auto*sObjectMgr=&objects;
struct Conversation{static inline std::vector<uint32>created;static inline std::vector<Conversation*>allocated;static inline bool fails=false;Conversation(){allocated.push_back(this);}~Conversation(){allocated.erase(std::find(allocated.begin(),allocated.end(),this));}bool CreateConversation(uint64,uint32 id,Unit*,void*,Unit&){created.push_back(id);return !fails;}};
// Successful native object ownership is a controlled endpoint; retain its
// allocation until fixture cleanup, while failed factory allocations delete.
#include "Casts.inc"
#include "OpeningKeys.inc"
#include "Actors.inc"
struct Run{
 Creature actor,ally{97166},enemy{97176},demon{102970},attacker1{102964},attacker2{102965},add1{102990},add2{102995};Player player;std::vector<std::unique_ptr<Creature>>others;uint32 which;
 explicit Run(uint32 kind):actor(kind?103608:97167),which(kind){Conversation::created.clear();Conversation::fails=false;scenarioManager={};actor.nearest=&player;for(auto*p:{&ally,&enemy,&demon,&attacker1,&attacker2,&add1,&add2}){p->i_AI=new CreatureAI(p);actor.nearby.push_back(p);}actor.i_AI=NewAI();actor.nearby.push_back(&actor);for(auto*p:actor.nearby)if(p!=&actor)p->nearby=actor.nearby;}
 CreatureAI*NewAI(){return which?npc_danica_the_reclaimer{}.GetAI(&actor):npc_ragnvald_dragonborn{}.GetAI(&actor);}
 ~Run(){while(!Conversation::allocated.empty())delete Conversation::allocated.back();}
 void Begin(){if(which)actor.AI()->IsSummonedBy(&player);else actor.AI()->MoveInLineOfSight(&player);}
 void Tick(uint32 diff){actor.Lifecycle(diff);if(actor.state==ALIVE||actor.state==CORPSE)actor.m_Functions.Update(diff);}
 uint32 Delay(){return which?3000:2000;}
 uint32 Requests(){return static_cast<uint32>(actor.attacks.size()+actor.casts.size());}
 void Done(){if(which){assert(actor.casts==std::vector<uint32>{205287});assert(player.credits==std::vector<uint32>({49203,49262}));assert(Conversation::created==std::vector<uint32>{1405});}else{assert(actor.attacks==std::vector<uint32>(actor.IsAlive()?2:1,97176));assert(add1.attacks==std::vector<uint32>{97176}&&add2.attacks==std::vector<uint32>{97176}&&ally.attacks==std::vector<uint32>{97176});assert(attacker1.attacks==std::vector<uint32>{102970}&&attacker2.attacks==std::vector<uint32>{102970});assert(player.credits==std::vector<uint32>{46877});assert(actor.homes.size()==(actor.IsAlive()?2u:1u)&&actor.homes.back().x==-1988.32f&&actor.homes.back().y==-1231.07f&&actor.homes.back().z==4.99f);}}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);std::cout<<std::unitbuf;uint32 cases=0;
#ifdef BASELINE
 for(uint32 kind:{0u,1u})for(uint32 elapsed:{0u,1u,999u}){Run r(kind);r.Begin();r.Tick(elapsed);assert(r.actor.m_Functions.Size()+r.actor.m_Functions.SizeQueue()==1);assert(r.actor.AIM_Destroy());assert(r.actor.m_Functions.Size()+r.actor.m_Functions.SizeQueue()==1);r.actor.m_Functions.KillAllFunctions();r.actor.m_Functions.Update(0);++cases;}
 std::cout<<"PASS EXPECTED BROKEN0ab5e6990: "<<cases<<" raw2/3s Unit callbacks survive native AIM_Destroy. Invalid captures discarded without execution; no fabricated crash.\n";
#else
 for(uint32 kind:{0u,1u}){
  for(uint32 mode=0;mode<5;++mode){Run r(kind);r.Begin();assert(!r.actor.m_Functions.SizeQueue());if(mode){r.actor.i_disabledAI=r.actor.i_AI;r.actor.i_AI=mode==1?new CreatureAI(&r.actor):mode==2?nullptr:r.actor.i_disabledAI;r.actor.IsAIEnabled=mode!=4;}r.Tick(r.Delay()-1);assert(!r.Requests());r.Tick(1);r.Done();r.Tick(10000);r.Done();++cases;}
  for(uint32 elapsed:{0u,1u,999u}){Run r(kind);r.Begin();r.Tick(elapsed);auto credit=r.player.credits;assert(r.actor.AIM_Destroy());uint32 independent=0;r.actor.AddDelayedEvent(1,[&]{++independent;});r.Tick(10000);assert(!r.Requests()&&r.player.credits==credit&&independent==1);r.actor.i_AI=r.NewAI();r.Tick(10000);assert(!r.Requests()&&r.player.credits==credit);++cases;}
  for(auto state:{ALIVE,CORPSE,DEAD,JUST_DIED,JUST_RESPAWNED}){Run r(kind);r.Begin();r.actor.state=state;r.Tick(r.Delay());if(state==ALIVE||state==CORPSE)r.Done();else{assert(!r.Requests());r.actor.state=ALIVE;r.Tick(r.Delay());r.Done();}++cases;}
  {Run r(kind);r.Begin();r.actor.onExternal=[&]{assert(r.actor.m_AI_locked&&!r.actor.AIM_Destroy());};r.Tick(r.Delay());r.actor.onExternal={};r.Done();++cases;}
  {Run r(kind);r.Begin();r.actor.AI()->Reset();r.Tick(r.Delay());r.Done();++cases;}
  {Run r(kind);r.Begin();r.Tick(100000);r.Done();++cases;}
 }
 using Ragnvald=npc_ragnvald_dragonborn::npc_ragnvald_dragonbornAI;
 for(uint32 recover:{0u,1u,5u,10u}){Run r(0);diagnostics=0;assert(r.ally.AIM_Destroy());r.Begin();r.Tick(2000);assert(r.ally.attacks.empty()&&r.ally.m_Functions.SizeQueue()==1);assert(r.actor.AIM_Destroy());for(uint32 i=1;i<=recover;++i)r.ally.m_Functions.Update(500);assert(r.ally.attacks.empty());r.ally.i_AI=new CreatureAI(&r.ally);r.ally.m_Functions.Update(500);if(recover==10){assert(r.ally.attacks.empty()&&diagnostics==1);}else{assert(r.ally.attacks==std::vector<uint32>{97176}&&!diagnostics);}r.ally.m_Functions.Update(10000);assert(r.ally.attacks.size()==(recover==10?0u:1u));++cases;}
 for(uint32 bad=0;bad<5;++bad){Run r(0);diagnostics=0;assert(r.ally.AIM_Destroy());Ragnvald::AttackBattleTarget(&r.ally,r.enemy.GetGUID());assert(r.ally.m_Functions.SizeQueue()==1);if(bad==0)r.ally.state=CORPSE;if(bad==1)r.enemy.state=CORPSE;if(bad==2)r.ally.nearby.clear();if(bad==3)++r.enemy.guid.high;if(bad==4)++r.enemy.guid.low;r.ally.i_AI=new CreatureAI(&r.ally);r.ally.m_Functions.Update(500);assert(r.ally.attacks.empty()&&!diagnostics&&!r.ally.m_Functions.Size()&&!r.ally.m_Functions.SizeQueue());++cases;}
 {Run r(0);diagnostics=0;assert(r.attacker1.AIM_Destroy());assert(r.attacker2.AIM_Destroy());r.Begin();r.Tick(2000);assert(r.attacker1.attacks.empty()&&r.attacker2.attacks.empty());r.attacker1.i_AI=new CreatureAI(&r.attacker1);r.attacker2.i_AI=new CreatureAI(&r.attacker2);r.attacker1.m_Functions.Update(499);r.attacker2.m_Functions.Update(499);assert(r.attacker1.attacks.empty()&&r.attacker2.attacks.empty());r.attacker1.m_Functions.Update(1);r.attacker2.m_Functions.Update(1);assert(r.attacker1.attacks==std::vector<uint32>{102970}&&r.attacker2.attacks==std::vector<uint32>{102970}&&!diagnostics);++cases;}
 {Run r(0);Unit pet;r.actor.AI()->MoveInLineOfSight(&pet);assert(r.player.credits.empty()&&Conversation::created.empty());r.actor.inRange=false;r.Begin();assert(r.player.credits.empty()&&Conversation::created.empty());++cases;}
 {Run r(0);r.add1.state=CORPSE;r.add2.aura=true;r.Begin();r.Tick(2000);assert(r.add1.attacks.empty()&&r.add1.homes.empty()&&r.add2.attacks.empty()&&r.add2.homes.empty());++cases;}
 {Run r(1);auto next=std::make_unique<Creature>(97167);next->i_AI=npc_ragnvald_dragonborn{}.GetAI(next.get());r.actor.nearby.push_back(next.get());r.Begin();r.Tick(3000);SpellInfo spell{205287};r.actor.AI()->SpellHitTarget(&r.attacker1,&spell);r.actor.AI()->SpellHitTarget(&r.attacker2,&spell);assert(r.attacker1.casts==std::vector<uint32>{205295}&&r.attacker2.casts==std::vector<uint32>{205296});r.actor.AI()->UpdateAI(1999);assert(r.actor.despawns.empty()&&Conversation::created==std::vector<uint32>{1405});r.actor.AI()->UpdateAI(1);assert(r.actor.despawns==std::vector<uint32>{4000}&&Conversation::created==std::vector<uint32>({1405,1412}));r.actor.AI()->UpdateAI(10000);assert(Conversation::created==std::vector<uint32>({1405,1412})&&r.actor.despawns.size()==1);++cases;}
 std::cout<<"PASS "<<cases<<" ASAN contexts: pinned77ebf820e Ragnvald legacy clock body and current complete Danica; actual new Ragnvald admission/queue belongs to Test-WarriorOpening, native FunctionProcessor/Unit queue/Creature locked current+suspended lifecycle/AIM_Destroy/Talk. Exact2/3s deadlines, ALIVE/CORPSE clocks, AI deletion/replacement, reentrant native deletion rejection, reset persistence and independent Unit work retained. Full128-bit original-target recovery on ally-owned500ms queue survives source-AI deletion; current-AI recovery/no retargeting/dead/missing/bounded exhaustion and native Danica spellhit/2s departure covered. One deliberately exhausted helper diagnostic is an expected failing outcome paired with recovered attacks, not successful combat. World/cast/conversation/motion/credit endpoints controlled; immediate shared foreign-AI/admission/factory-cast failure recovery and full933/client remain separate.\n";
#endif
}
