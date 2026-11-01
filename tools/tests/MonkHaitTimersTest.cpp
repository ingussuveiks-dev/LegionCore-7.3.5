#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Functions.inc"
#include "DeathState.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_FIELD_NPC_FLAGS,UNIT_FIELD_FLAGS,UNIT_NPC_FLAG_SPELLCLICK=1,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,UNIT_FLAG_NOT_ATTACKABLE_1=128,POINT_MOTION_TYPE=8,WAYPOINT_MOTION_TYPE=2,UNIT_STATE_STUNNED=4,UNIT_STATE_CASTING=8,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
uint32 const cinematicFlags=256|512|128;
struct ObjectGuid{uint64 low=0,high=0;bool operator==(ObjectGuid const& v)const{return low==v.low&&high==v.high;}bool operator<(ObjectGuid const& v)const{return high==v.high?low<v.low:high<v.high;}std::string ToString()const{return std::to_string(high)+":"+std::to_string(low);}};
struct Player;struct Creature;struct CreatureAI;
std::map<ObjectGuid,Creature*> world;uint64 nextGuid=1;uint32 expectedFaults=0;
struct Unit{virtual ~Unit()=default;virtual Player* ToPlayer(){return nullptr;}};
struct Player:Unit{ObjectGuid guid{1,0xabcdef1234567890};std::vector<uint32> credits,kills;Player* ToPlayer()override{return this;}ObjectGuid GetGUID()const{return guid;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92);credits.push_back(id);}void KilledMonsterCredit(uint32 id){kills.push_back(id);}};
struct Motion{std::vector<uint32> points;uint32 paths=0;void MovePoint(uint32 id,float,float,float){points.push_back(id);}void MovePath(uint32 id,bool){assert(id==9859006);++paths;}};
struct Vehicle{uint32 removals=0;bool passenger=false;void RemoveAllPassengers(){++removals;passenger=false;}};
struct GameObject{uint32 uses=0;void UseDoorOrButton(uint32,bool,Creature*){++uses;}};
struct Creature:Unit{
 ObjectGuid guid{nextGuid++,0xf130101400000001};uint32 map=1014;bool alive=true,near=true,castAccepted=true,walk=false,hasVehicle=true,victim=false;
 DeathState deathState=ALIVE;DeathState getDeathState(){return deathState;}
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;FunctionProcessor functions;
 Player* nearest=nullptr;Creature* chuan=nullptr;Creature* demon=nullptr;GameObject* door=nullptr;Motion motion;Vehicle vehicle;uint32 npcflags=0,flags=0,states=0,attacks=0,melee=0;ObjectGuid attacked;
 std::vector<std::pair<uint32,ObjectGuid>> talks;std::vector<uint32> casts;std::function<void()> onTalk;
 Creature(){world.emplace(guid,this);}~Creature();CreatureAI* AI(){return i_AI;}bool AIM_Destroy();void Lifecycle(uint32 diff);
 ObjectGuid GetGUID()const{return guid;}bool IsAlive()const{return alive;}
 void SetFlag(uint32 field,uint32 value){(field==UNIT_FIELD_NPC_FLAGS?npcflags:flags)|=value;}void RemoveFlag(uint32 field,uint32 value){(field==UNIT_FIELD_NPC_FLAGS?npcflags:flags)&=~value;}
 void SetHomePosition(float,float,float,float){}float GetPositionX()const{return 1;}float GetPositionY()const{return 1;}float GetPositionZ()const{return 1;}float GetOrientation()const{return 0;}
 bool HasUnitState(uint32 state)const{return (states&state)!=0;}void SetWalk(bool value){walk=value;}Motion* GetMotionMaster(){return &motion;}Vehicle* GetVehicleKit(){return hasVehicle?&vehicle:nullptr;}
 Player* FindNearestPlayer(float){return nearest;}Creature* FindNearestCreature(uint32 id,float,bool){return id==98001?chuan:id==98011?demon:nullptr;}GameObject* FindNearestGameObject(uint32 id,float){assert(id==244682);return door;}
 void CastSpell(Creature* target,uint32 id){casts.push_back(id);if(id==227140&&castAccepted)target->vehicle.passenger=true;}void CastSpell(float,float,float,uint32 id){casts.push_back(id);}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};
namespace ObjectAccessor{Creature* GetCreature(Creature const& source,ObjectGuid guid){auto at=world.find(guid);return at!=world.end()&&at->second->map==source.map?at->second:nullptr;}}
struct CreatureAI{
 Creature* me;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;virtual void Reset(){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void SetData(uint32,uint32){}virtual void DoAction(int32){}virtual void OnSpellClick(Unit*){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 void Talk(uint8 id,ObjectGuid WhisperGuid={});virtual void AttackStart(Unit* victim){assert(victim);auto target=static_cast<Creature*>(victim);me->attacked=target->guid;++me->attacks;}
};
Creature::~Creature(){world.erase(guid);if(i_disabledAI&&i_disabledAI!=i_AI)delete i_disabledAI;delete i_AI;}
struct CreatureTextMgrEndpoint{void SendChat(Creature* source,uint8 id,ObjectGuid guid={}){source->talks.emplace_back(id,guid);if(source->onTalk)source->onTalk();}};
CreatureTextMgrEndpoint textEndpoint;auto* const sCreatureTextMgr=&textEndpoint;
#include "TalkForward.inc"
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#define TC_LOG_DEBUG(...) ((void)0)
#define TC_LOG_ERROR(...) (++expectedFaults)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return me->victim;}void DoMeleeAttackIfReady(){++me->melee;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "Actor.inc"
using Hait=npc_master_hait::npc_master_haitAI;
struct Run{
 std::unique_ptr<Creature> hait=std::make_unique<Creature>(),chuan=std::make_unique<Creature>(),demon=std::make_unique<Creature>();GameObject door;Player player;
 Run(){hait->i_AI=new Hait(hait.get());chuan->i_AI=new CreatureAI(chuan.get());demon->i_AI=new CreatureAI(demon.get());hait->nearest=&player;hait->chuan=chuan.get();chuan->demon=demon.get();chuan->door=&door;}
 Hait* Content(){return static_cast<Hait*>(hait->i_disabledAI?hait->i_disabledAI:hait->i_AI);}
 void HaitTick(uint32 diff,bool combat=true){hait->Lifecycle(diff);hait->functions.Update(diff);if(combat&&hait->i_AI)hait->i_AI->UpdateAI(diff);}
 void ChuanTick(uint32 diff){if(chuan)chuan->functions.Update(diff);}void DemonTick(uint32 diff){if(demon)demon->functions.Update(diff);}
 void Begin(uint32 path){if(path==0)Content()->SetData(1,1);else if(path==1)Content()->MovementInform(POINT_MOTION_TYPE,1);else Content()->DoAction(1);}
 // Each native Unit clock is deliberately separate; no invented global-clock
 // assumption when one actor schedules work during another actor's update.
 void Launch(){Content()->DoAction(1);HaitTick(10000);ChuanTick(0);DemonTick(0);assert(demon->flags&cinematicFlags);assert(door.uses==1&&chuan->walk);}
 void Suspend(bool current=true,bool alias=false){auto old=hait->i_AI;hait->i_disabledAI=old;hait->i_AI=alias?old:current?new CreatureAI(hait.get()):nullptr;}
};
void AssertSelfDone(Run const& r,uint32 path){
 if(path==0){assert(r.hait->talks.size()==2&&r.hait->talks[0].first==5&&r.hait->talks[1].first==6);assert(r.hait->motion.points==std::vector<uint32>{1});assert(r.player.credits==std::vector<uint32>{46868});}
 if(path==1){assert(r.hait->talks.size()==3&&r.hait->talks[0].first==7&&r.hait->talks[1].first==8&&r.hait->talks[2].first==9&&r.hait->talks[2].second==r.player.guid);assert(r.player.credits.empty());}
 if(path==2){assert(r.hait->talks.size()==4);for(uint32 i=0;i<4;++i)assert(r.hait->talks[i].first==i);assert(r.hait->casts==std::vector<uint32>{193053});assert(r.player.credits.empty());}
}
int main(){
#ifdef BASELINE
 for(uint32 path=0;path<3;++path){Run r;r.Begin(path);uint32 pending=path==0?1:path==1?2:4;assert(r.hait->functions.SizeQueue()==pending);assert(r.hait->AIM_Destroy());assert(r.hait->functions.SizeQueue()==pending);}
 {Run r;r.Launch();assert(r.chuan->functions.Size()==3&&r.demon->functions.Size()==1);assert(r.hait->AIM_Destroy());assert(r.chuan->functions.Size()==3);}
 std::cout<<"PASS EXPECTED BROKEN ad164392f: seven selfAI callbacks survive AIM_Destroy in Unit queue; cross-actor20s captures Chuan pointer and Chuan-owned23s retains foreign Demon/HaitAI. Dangling callbacks deliberately not executed; no fabricated runtime crash.\n";
#else
 uint32 cases=0;
 for(uint32 path=0;path<3;++path){
  for(uint32 mode=0;mode<10;++mode){Run r;r.Begin(path);assert(r.hait->functions.SizeQueue()==0&&r.Content()->_presentationFunctions.SizeQueue()==(path==0?1u:path==1?2u:4u));
   if(mode==1)r.hait->states|=UNIT_STATE_STUNNED;if(mode==2)r.hait->states|=UNIT_STATE_CASTING;if(mode==3)r.Suspend();if(mode==4)r.Suspend(false);if(mode==5)r.Suspend(true,true);if(mode==6)r.Content()->Reset();if(mode==7)r.hait->victim=true;
   if(mode==9)r.HaitTick(11000);else for(uint32 t=0;t<10000;t+=1000)r.HaitTick(1000,mode!=8);AssertSelfDone(r,path);r.HaitTick(10000);AssertSelfDone(r,path);++cases;
  }
  for(uint32 elapsed:{0u,1u,999u}){Run r;r.Begin(path);r.HaitTick(elapsed,false);auto count=r.hait->talks.size();assert(r.hait->AIM_Destroy());r.HaitTick(30000,false);assert(r.hait->talks.size()==count&&r.hait->casts.empty());r.hait->i_AI=new Hait(r.hait.get());r.HaitTick(30000);assert(r.hait->talks.size()==count);++cases;}
  {Run r;r.Begin(path);r.hait->onTalk=[&]{assert(r.hait->m_AI_locked&&!r.hait->AIM_Destroy());};r.HaitTick(10000);r.hait->onTalk={};AssertSelfDone(r,path);++cases;}
  {Run r;r.Begin(path);r.Begin(path);assert(r.hait->AIM_Destroy());auto count=r.hait->talks.size();r.HaitTick(30000,false);assert(r.hait->talks.size()==count&&r.hait->casts.empty());++cases;}
  for(auto state:{ALIVE,CORPSE,DEAD,JUST_DIED,JUST_RESPAWNED}){Run r;r.Begin(path);auto before=r.hait->talks.size();r.hait->deathState=state;r.HaitTick(10000);if(state==ALIVE||state==CORPSE)AssertSelfDone(r,path);else{assert(r.hait->talks.size()==before&&r.hait->casts.empty());r.hait->deathState=ALIVE;r.HaitTick(10000);AssertSelfDone(r,path);}++cases;}
 }
 // Original successful physical requests, exact distinct20/23 clocks,
 // passenger removal and three native flag bits, with unrelated bits retained.
 {Run r;r.Launch();r.demon->flags|=0x1000000;r.DemonTick(19999);assert(r.chuan->casts.empty());r.DemonTick(1);assert(r.chuan->casts==std::vector<uint32>{227140}&&r.demon->casts==std::vector<uint32>{193268}&&r.demon->vehicle.passenger);r.DemonTick(2999);assert(!r.demon->vehicle.removals);r.DemonTick(1);assert(r.demon->vehicle.removals==1&&!r.demon->vehicle.passenger&&r.demon->flags==0x1000000&&r.demon->attacks==1&&r.demon->attacked==r.hait->guid);assert(r.chuan->casts==std::vector<uint32>({227140,185627})&&r.chuan->flags&256&&r.hait->talks.back().first==4);r.DemonTick(30000);assert(r.demon->attacks==1&&r.demon->vehicle.removals==1&&r.player.credits.empty());++cases;}
 for(uint32 missing=0;missing<4;++missing){Run r;r.Launch();auto oldHait=r.hait->guid;if(missing==0){r.hait->chuan=nullptr;r.chuan.reset();}if(missing==1)r.hait.reset();if(missing==2)assert(r.hait->AIM_Destroy());if(missing==3){r.hait->map=999;r.chuan->map=999;}r.DemonTick(23000);assert(!(r.demon->flags&cinematicFlags)&&r.demon->vehicle.removals==1&&!r.demon->vehicle.passenger);assert(r.demon->attacks==((missing==1||missing==3)?0u:1u));if(r.demon->attacks)assert(r.demon->attacked==oldHait);++cases;}
 {Run r;r.Launch();auto old=r.chuan->guid;r.hait->chuan=nullptr;r.chuan.reset();auto replacement=std::make_unique<Creature>();replacement->i_AI=new CreatureAI(replacement.get());r.hait->chuan=replacement.get();r.DemonTick(23000);assert(replacement->guid.high==old.high&&replacement->guid.low!=old.low&&replacement->casts.empty()&&!replacement->flags&&r.demon->attacks==1&&r.demon->vehicle.removals==1);++cases;}
 {Run r;r.Launch();auto old=r.hait->guid;r.hait.reset();auto replacement=std::make_unique<Creature>();replacement->i_AI=new Hait(replacement.get());r.DemonTick(23000);assert(replacement->guid.high==old.high&&replacement->guid.low!=old.low&&replacement->talks.empty()&&!r.demon->attacks&&!(r.demon->flags&cinematicFlags));++cases;}
 for(bool accepted:{false,true}){Run r;r.Launch();r.chuan->castAccepted=accepted;r.DemonTick(23000);assert(r.demon->casts==std::vector<uint32>{193268}&&r.chuan->casts==std::vector<uint32>({227140,185627})&&r.demon->vehicle.removals==1&&!r.demon->vehicle.passenger&&r.demon->attacks==1);++cases;}
 {Run r;r.Launch();r.demon->hasVehicle=false;r.DemonTick(23000);assert(!r.demon->vehicle.removals&&!(r.demon->flags&cinematicFlags)&&r.demon->attacks==1);++cases;}
 for(bool deadHait:{false,true}){Run r;r.Launch();(deadHait?r.hait:r.demon)->alive=false;r.DemonTick(23000);assert(!(r.demon->flags&cinematicFlags)&&r.demon->vehicle.removals==1&&!r.demon->attacks);++cases;}
 // Current AI may be temporarily absent; cleanup is independent and native
 // AttackStart is retried against only the original living actor.
 for(uint32 restore:{0u,1u,5u,10u}){Run r;r.Launch();assert(r.demon->AIM_Destroy());r.DemonTick(23000);assert(!r.demon->attacks&&!(r.demon->flags&cinematicFlags)&&r.demon->vehicle.removals==1);if(!restore)r.demon->i_AI=new CreatureAI(r.demon.get());for(uint32 attempt=1;attempt<=10;++attempt){if(attempt==restore)r.demon->i_AI=new CreatureAI(r.demon.get());r.DemonTick(499);assert(r.demon->attacks==(attempt>std::max(1u,restore)?1u:0u));r.DemonTick(1);}assert(r.demon->attacks==1);r.DemonTick(10000);assert(r.demon->attacks==1&&!expectedFaults);++cases;}
 {Run r;r.Launch();assert(r.demon->AIM_Destroy());r.DemonTick(23000);for(uint32 attempt=0;attempt<9;++attempt)r.DemonTick(500);assert(!expectedFaults);r.DemonTick(499);assert(!expectedFaults);r.DemonTick(1);assert(expectedFaults==1&&!r.demon->attacks&&!(r.demon->flags&cinematicFlags)&&r.demon->vehicle.removals==1);r.DemonTick(10000);assert(expectedFaults==1);expectedFaults=0;std::cout<<"EXPECTED injected no-AI fault: exactly one finite10-retry diagnostic, cleanup still completed; not a successful ordinary attack.\n";++cases;}
 {Run r;r.Launch();assert(r.demon->AIM_Destroy());r.DemonTick(23000);r.hait.reset();r.DemonTick(5000);assert(!r.demon->attacks&&!expectedFaults);++cases;}
 {Run r;r.Launch();assert(r.demon->AIM_Destroy());r.DemonTick(23000);auto old=r.demon->guid;r.demon.reset();auto replacement=std::make_unique<Creature>();replacement->i_AI=new CreatureAI(replacement.get());replacement->functions.Update(10000);assert(replacement->guid.low!=old.low&&!replacement->attacks&&replacement->casts.empty()&&!expectedFaults);++cases;}
 // Five Chuan callbacks are Unit-self work and survive only that Creature,
 // retaining their native text when its own AI is unavailable.
 {Run r;r.Begin(0);assert(r.chuan->AIM_Destroy());r.ChuanTick(1999);assert(r.chuan->talks.empty());r.ChuanTick(1);assert(r.chuan->talks.size()==1);r.ChuanTick(2000);assert(r.chuan->talks.size()==2);r.ChuanTick(3000);assert(r.chuan->talks.size()==3&&r.chuan->talks[0].first==3&&r.chuan->talks[2].first==5);++cases;}
 {Run r;r.Launch();assert(r.chuan->AIM_Destroy());r.ChuanTick(7999);assert(r.chuan->talks.empty());r.ChuanTick(1);assert(r.chuan->talks.size()==1&&r.chuan->talks[0].first==0);r.ChuanTick(7000);assert(r.chuan->talks.size()==2&&r.chuan->talks[1].first==1);++cases;}
 {Run r;r.Begin(0);r.hait->chuan=nullptr;r.chuan.reset();r.HaitTick(10000);AssertSelfDone(r,0);++cases;}
 {Run r;r.hait->chuan=nullptr;r.Begin(2);r.HaitTick(10000);AssertSelfDone(r,2);assert(r.demon->functions.SizeQueue()==0);++cases;}
 {Run r;r.chuan->demon=nullptr;r.Begin(2);r.HaitTick(10000);AssertSelfDone(r,2);assert(r.chuan->functions.SizeQueue()==2);++cases;}
 {Run r;r.hait->nearest=nullptr;r.Begin(1);r.HaitTick(6000);assert(r.hait->talks.size()==2);++cases;}
 {Run r;r.Content()->MovementInform(WAYPOINT_MOTION_TYPE,5);r.Content()->MovementInform(WAYPOINT_MOTION_TYPE,8);assert(r.hait->talks.size()==2&&r.hait->talks[0].first==14&&r.hait->talks[1].first==15&&r.player.credits==std::vector<uint32>{46951});++cases;}
 {Run r;r.Content()->SetData(2,1);r.Content()->DoAction(2);r.HaitTick(30000);assert(r.hait->talks.empty());uint32 damage=0xffffffff;r.Content()->DamageTaken(&r.player,damage,0);assert(!damage);++cases;}
 assert(!expectedFaults);
 std::cout<<"PASS "<<cases<<" actual full Hait/native FunctionProcessor+Talk+locked lifecycle+AIM_Destroy ASAN contexts: seven owned AI timers/five Unit-self texts, full foreign GUIDs, actual Demon-owned20/23 cleanup despite missing/replaced/dead/cross-map actors, failed boarding request/missing vehicle, native AttackStart retry/recovery/finite expected failure, destroy/recreate/suspend/alias/stun/late/repeated roots. Per-Unit clocks explicit; world/vehicle/casts/credit endpoints controlled, no client/full943/admission claim.\n";
#endif
}
