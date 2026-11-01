#include <cassert>
#include <cstdint>
#include <map>
#include <list>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=uint32;
#include "Functions.inc"
#include "DeathState.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK=1,WAYPOINT_MOTION_TYPE=2,UNIT_STATE_STUNNED=4,UNIT_STATE_CASTING=8,MOTION_SLOT_IDLE=0,CRITERIA_TYPE_SCRIPT_EVENT_2=92};
float const PET_FOLLOW_ANGLE=1.57f,PET_FOLLOW_DIST=1.f;
uint32 urand(uint32 low,uint32){return low;}
struct ObjectGuid{uint64 low=0,high=0;bool operator==(ObjectGuid const& other)const{return low==other.low&&high==other.high;}};
struct Unit{virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}virtual float GetPositionZ()const{return 0;}};
struct Player:Unit{ObjectGuid guid{17,0x1234567800000011};std::vector<uint32> credits;uint32 GetTypeId()const override{return TYPEID_PLAYER;}ObjectGuid GetGUID()const{return guid;}void UpdateAchievementCriteria(uint32 type,uint32 id){assert(type==92);credits.push_back(id);}};
struct Motion{uint32 points=0,follows=0;std::vector<uint32> paths;void MovePoint(uint32 id,float,float,float){assert(id==0);++points;}void MovePath(uint32 id,bool){paths.push_back(id);}void MoveFollow(Unit*,float,float,uint32){++follows;}};
struct CreatureAI;
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;
 DeathState deathState=ALIVE;DeathState getDeathState(){return deathState;}
 FunctionProcessor functions;uint32 states=0,flags=UNIT_NPC_FLAG_SPELLCLICK,attacks=0,melee=0;bool near=true,victim=false;Motion motion;Player* nearest=nullptr;Creature* hait=nullptr;Creature* morvat=nullptr;Creature* li=nullptr;std::vector<Creature*> adds;
 std::vector<std::pair<uint32,ObjectGuid>> talks;std::function<void()> onTalk;
 CreatureAI* AI(){return i_AI;}bool AIM_Destroy();void Lifecycle(uint32 diff);
 void RemoveFlag(uint32,uint32 flag){flags&=~flag;}
 void SetHomePosition(float,float,float,float){}float GetPositionX()const{return 1;}float GetPositionY()const{return 1;}float GetPositionZ()const override{return 1;}float GetOrientation()const{return 0;}
 bool IsWithinDistInMap(Unit*,float)const{return near;}bool HasUnitState(uint32 flag)const{return (states&flag)!=0;}
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float){return nearest;}Creature* FindNearestCreature(uint32 id,float,bool){return id==97778?hait:id==97811?morvat:id==98074?li:nullptr;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* actor,uint32 id,float){assert(id==98074);out.insert(out.end(),actor->adds.begin(),actor->adds.end());}
struct CreatureAI{
 Creature* me;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;virtual void Reset(){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void SetData(uint32,uint32){}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}
 void Talk(uint8 id,ObjectGuid WhisperGuid={});void AttackStart(Unit*){++me->attacks;}
};
struct CreatureTextMgrEndpoint{void SendChat(Creature* source,uint8 id,ObjectGuid guid={}){source->talks.emplace_back(id,guid);if(source->onTalk)source->onTalk();}};
CreatureTextMgrEndpoint textMgrEndpoint;auto* const sCreatureTextMgr=&textMgrEndpoint;
#include "TalkForward.inc"
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return me->victim;}void DoMeleeAttackIfReady(){++me->melee;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
#include "Actor.inc"
using Chen=npc_monk_chen_porter::npc_monk_chen_porterAI;
struct Run{
 Creature actor,hait,morvat,li,add;Player player;CreatureAI temporary{&actor},haitAI{&hait},liAI{&li},addAI{&add};
 Run(){actor.i_AI=new Chen(&actor);actor.nearest=&player;actor.hait=&hait;actor.morvat=&morvat;actor.li=&li;actor.adds={&li,&add};hait.i_AI=&haitAI;li.i_AI=&liAI;add.i_AI=&addAI;}
 ~Run(){if(actor.i_disabledAI&&actor.i_disabledAI!=actor.i_AI)delete actor.i_disabledAI;actor.i_disabledAI=nullptr;if(actor.i_AI!=&temporary)delete actor.i_AI;}
 Chen* Content(){return static_cast<Chen*>(actor.i_disabledAI?actor.i_disabledAI:actor.i_AI);}
 void Begin(uint32 path){if(path==0)Content()->MovementInform(WAYPOINT_MOTION_TYPE,5);else Content()->SetData(1,1);}
 void Tick(uint32 diff,bool combat=true){actor.Lifecycle(diff);actor.functions.Update(diff);if(combat&&actor.i_AI)actor.i_AI->UpdateAI(diff);}
 void Suspend(bool current=true,bool alias=false){auto old=actor.i_AI;actor.i_disabledAI=old;actor.i_AI=alias?old:current?&temporary:nullptr;}
};
void AssertFinished(Run const& r,uint32 path){
 if(path==0){assert(r.actor.talks.size()==2&&r.actor.talks[0].first==7&&r.actor.talks[1].first==8&&r.actor.talks[1].second==r.player.guid);assert(r.hait.talks.size()==4);for(uint32 i=0;i<4;++i){assert(r.hait.talks[i].first==10+i);if(i)assert(r.hait.talks[i].second==r.player.guid);}assert(r.player.credits==std::vector<uint32>{46947});assert(r.hait.motion.paths==std::vector<uint32>{9859006}&&r.actor.motion.points==1&&r.actor.motion.paths.empty());}
 else{assert(r.actor.talks.size()==3&&r.actor.talks[0].first==3&&r.actor.talks[1].first==4&&r.actor.talks[2].first==5&&r.actor.talks[2].second==r.player.guid);assert(r.player.credits==std::vector<uint32>{46956});assert(r.actor.motion.paths==std::vector<uint32>{9859005});assert(r.li.motion.follows==2&&r.add.motion.follows==1);assert(r.li.talks.size()==1&&r.li.talks[0].first==0&&r.add.talks.size()==1&&r.hait.talks.empty());}
}
int main(){
#ifdef BASELINE
 for(uint32 path=0;path<2;++path){Run r;r.Begin(path);assert(r.actor.functions.SizeQueue()==(path==0?6u:3u));assert(r.actor.AIM_Destroy());assert(r.actor.functions.SizeQueue()==(path==0?6u:3u));}
 std::cout<<"PASS EXPECTED BROKEN dc4d75908 counterfactual: nine Chen callbacks persist in Unit queue after actual AIM_Destroy; four capture raw Player and five text calls dereference target AI. Dangling/null callbacks deliberately not invoked, no fabricated runtime crash.\n";
#else
 uint32 cases=0;
 for(uint32 path=0;path<2;++path){
  for(uint32 mode=0;mode<10;++mode){Run r;r.Begin(path);assert(r.actor.functions.SizeQueue()==0&&r.Content()->_presentationFunctions.SizeQueue()==(path==0?6u:3u));
   if(mode==1)r.actor.states|=UNIT_STATE_STUNNED;if(mode==2)r.actor.states|=UNIT_STATE_CASTING;if(mode==3)r.Suspend();if(mode==4)r.Suspend(false);if(mode==5)r.Suspend(true,true);if(mode==6)r.Content()->Reset();if(mode==7)r.actor.victim=true;
   if(mode==9)r.Tick(20000);else for(uint32 time=0;time<16000;time+=1000)r.Tick(1000,mode!=8);
   AssertFinished(r,path);r.Tick(20000);AssertFinished(r,path);++cases;
  }
  for(uint32 elapsed:{0u,1u,2999u}){Run r;r.Begin(path);r.Tick(elapsed,false);auto actorTalks=r.actor.talks.size(),haitTalks=r.hait.talks.size();auto credits=r.player.credits.size();assert(r.actor.AIM_Destroy());r.Tick(30000,false);assert(r.actor.talks.size()==actorTalks&&r.hait.talks.size()==haitTalks&&r.player.credits.size()==credits&&r.actor.motion.paths.empty()&&!r.actor.motion.points&&r.hait.motion.paths.empty());r.actor.i_AI=new Chen(&r.actor);r.Tick(30000);assert(r.actor.talks.size()==actorTalks);++cases;}
  {Run r;r.Begin(path);uint32 other=0;r.actor.AddDelayedEvent(3000,[&]{++other;});assert(r.actor.AIM_Destroy());r.Tick(30000,false);assert(other==1&&r.actor.talks.size()==1&&r.hait.talks.empty());++cases;}
  {Run r;r.Begin(path);r.actor.onTalk=[&]{assert(r.actor.m_AI_locked&&!r.actor.AIM_Destroy());};r.Tick(20000);r.actor.onTalk={};AssertFinished(r,path);++cases;}
  {Run r;r.Begin(path);assert(r.actor.AIM_Destroy());r.actor.talks.clear();r.player.credits.clear();r.actor.i_AI=new Chen(&r.actor);r.Tick(20000);assert(r.actor.talks.empty());r.Begin(path);r.Tick(20000);AssertFinished(r,path);++cases;}
  {Run r;r.Begin(path);r.Begin(path);assert(r.actor.functions.SizeQueue()==0&&r.Content()->_presentationFunctions.SizeQueue()==(path==0?12u:6u));auto talks=r.actor.talks.size(),credits=r.player.credits.size();assert(r.actor.AIM_Destroy());r.Tick(30000,false);assert(r.actor.talks.size()==talks&&r.player.credits.size()==credits&&r.hait.talks.empty()&&r.actor.motion.paths.empty());++cases;}
  for(auto state:{ALIVE,CORPSE,DEAD,JUST_DIED,JUST_RESPAWNED}){Run r;r.Begin(path);auto before=r.actor.talks.size();r.actor.deathState=state;r.Tick(20000);if(state==ALIVE||state==CORPSE)AssertFinished(r,path);else{assert(r.actor.talks.size()==before&&r.hait.talks.empty()&&r.actor.motion.paths.empty());r.actor.deathState=ALIVE;r.Tick(20000);AssertFinished(r,path);}++cases;}
 }
 // Exact original timing and stable ordering of the two distinct15s lines.
 {Run r;r.Begin(0);r.Tick(2999);assert(r.hait.talks.empty());r.Tick(1);assert(r.hait.talks.size()==1);r.Tick(3999);assert(r.actor.talks.size()==1);r.Tick(1);assert(r.actor.talks.size()==2);r.Tick(3999);assert(r.hait.talks.size()==1);r.Tick(1);assert(r.hait.talks.size()==2);r.Tick(3999);assert(r.hait.talks.size()==2&&r.hait.motion.paths.empty());r.Tick(1);assert(r.hait.talks.size()==4&&r.hait.talks[2].first==12&&r.hait.talks[3].first==13&&r.hait.motion.paths.size()==1);r.Tick(999);assert(!r.actor.motion.points);r.Tick(1);AssertFinished(r,0);++cases;}
 {Run r;r.Begin(1);r.Tick(3999);assert(r.actor.talks.size()==1);r.Tick(1);assert(r.actor.talks.size()==2);r.Tick(999);assert(!r.li.motion.follows);r.Tick(1);assert(r.li.motion.follows==1);r.Tick(2999);assert(r.actor.motion.paths.empty());r.Tick(1);AssertFinished(r,1);++cases;}
 for(bool replacement:{false,true}){Run r;auto transient=std::make_unique<Player>();transient->guid={0xabcdef0000000042,0x9876543200000044};auto guid=transient->guid;r.actor.nearest=transient.get();r.Content()->MovementInform(WAYPOINT_MOTION_TYPE,5);assert(transient->credits==std::vector<uint32>{46947});transient.reset();r.actor.nearest=replacement?&r.player:nullptr;r.Tick(20000);assert(r.actor.talks.back().first==8&&r.actor.talks.back().second==guid);for(uint32 i=1;i<4;++i)assert(r.hait.talks[i].second==guid);assert(r.player.credits.empty()&&r.hait.motion.paths.size()==1&&r.actor.motion.points==1);++cases;}
 for(bool missing:{false,true}){Run r;r.Begin(0);if(missing)r.actor.hait=nullptr;else r.hait.i_AI=nullptr;r.Tick(20000);if(missing)assert(r.hait.talks.empty()&&r.actor.talks.size()==2&&r.actor.motion.points==1&&r.hait.motion.paths.empty());else AssertFinished(r,0);++cases;}
 {Run r;r.Begin(0);r.hait.i_AI=nullptr;r.Tick(11000);assert(r.hait.talks.size()==2&&r.hait.talks[0].first==10&&r.hait.talks[1].first==11);r.hait.i_AI=&r.haitAI;r.Tick(5000);AssertFinished(r,0);++cases;}
 {Run r;r.Begin(0);r.Tick(11000);auto oldTalks=r.hait.talks.size();Creature newHait;CreatureAI newAI{&newHait};newHait.i_AI=&newAI;r.actor.hait=&newHait;r.Tick(5000);assert(r.hait.talks.size()==oldTalks&&newHait.talks.size()==2&&newHait.motion.paths.size()==1);++cases;}
 {Run r;r.Begin(1);r.li.i_AI=nullptr;r.add.i_AI=nullptr;r.Tick(20000);AssertFinished(r,1);++cases;}
 {Run r;r.Begin(1);r.actor.adds.clear();r.actor.li=nullptr;r.Tick(20000);assert(r.actor.motion.paths.size()==1&&r.actor.talks.size()==3);++cases;}
 for(uint32 path=0;path<2;++path){Run r;r.actor.nearest=nullptr;r.Begin(path);r.Tick(20000);assert(r.player.credits.empty());if(path==0)assert(r.actor.talks.size()==1&&r.hait.talks.size()==1&&r.hait.motion.paths.empty()&&r.actor.motion.points==1);else assert(r.actor.talks.size()==2&&r.actor.motion.paths.size()==1);++cases;}
 {Run r;Unit pet;r.Content()->MoveInLineOfSight(&pet);assert(!r.Content()->_introDone&&!r.actor.attacks);r.Content()->MoveInLineOfSight(&r.player);r.Content()->MoveInLineOfSight(&r.player);assert(r.actor.attacks==1&&r.actor.talks.size()==1&&r.actor.talks[0].first==2);++cases;}
 {Run r;r.Content()->MovementInform(WAYPOINT_MOTION_TYPE,2);r.Content()->MovementInform(WAYPOINT_MOTION_TYPE,4);assert(r.actor.talks.size()==1&&r.actor.talks[0].first==6&&!(r.hait.flags&1));++cases;}
 {Run r;r.Content()->SetData(2,1);r.Content()->SetData(1,2);r.Tick(20000);assert(r.actor.talks.empty()&&r.player.credits.empty());uint32 damage=0xffffffff;r.Content()->DamageTaken(&r.player,damage,0);assert(!damage);++cases;}
 std::cout<<"PASS "<<cases<<" actual full Chen/native FunctionProcessor+locked lifecycle+AIM_Destroy+actual CreatureAI Talk forwarding ASAN contexts: nine timers/exact15s ordering, original full128-bit player targets survive destruction/replacement, AI destroy/recreate/retry/stun/cast/suspend/alias/late/no-victim clocks, missing/replaced target and all original text with null/current AI, independent route/follow continuation, unrelated Unit work/native damage/LOS/motion endpoints. World/network/credit/movement completion endpoints controlled; no actual client/full943 or admission certification.\n";
#endif
}
