#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <cstdlib>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;
constexpr uint32 CLASS_MONK=10,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_BYTES_1=1,UNIT_STATE_STUNNED=1;
enum QuestStatus{QUEST_STATUS_NONE,QUEST_STATUS_COMPLETE,QUEST_STATUS_INCOMPLETE};
struct ObjectGuid{uint64 value=0;void Clear(){value=0;}bool IsEmpty()const{return !value;}bool operator==(ObjectGuid const&o)const{return value==o.value;}};
struct QuestStatusData{std::shared_ptr<uint8> ScriptLifetime;};
struct Progress{uint64 Counter=0;};struct Achievements{std::map<uint32,Progress> progress;auto const*GetCriteriaProgressMap()const{return &progress;}};
struct Scenario{uint32 id=1007,step=0;Achievements achievements;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}Achievements&GetAchievementMgr(){return achievements;}};
struct Manager{Scenario* current=nullptr;Scenario*GetScenario(uint32){return current;}}manager;auto*sScenarioMgr=&manager;
struct Unit;struct Creature;struct Player;
struct CreatureAI{virtual~CreatureAI()=default;virtual void HealReceived(Unit*,uint32&){}virtual void HealDone(Unit*,uint32&){}virtual void OnHealApplied(Unit*,uint32,int32){}virtual void UpdateLifecycle(uint32){}virtual void Reset(){}};
struct InstanceScript{bool CheckDamageManager()const{return false;}void UpdateDamageManager(uint64,int32,bool){assert(false);}};
struct Unit{
 virtual~Unit()=default;bool IsAIEnabled=false;CreatureAI* healerAI=nullptr;Player* owner=nullptr;int32 health=100,maximum=1000;
 virtual Creature*ToCreature(){return nullptr;}virtual Player*ToPlayer(){return nullptr;}virtual CreatureAI*GetAI(){return healerAI;}
 Player*GetCharmerOrOwnerPlayerOrPlayerItself();InstanceScript*GetInstanceScript(){return nullptr;}uint64 GetGUID()const{return 99;}
 int32 ModifyHealth(int32 amount,Unit*){int32 gain=std::min(maximum-health,amount);health+=gain;return gain;}
 int32 DealHeal(Unit*,uint32);
};
struct Player:Unit{
 ObjectGuid guid{42};void*map=reinterpret_cast<void*>(1);bool world=true,alive=true,contact=true,gm=false,teleport=false,phase=true,rewarded=false,hasQuest=true,accept=true;
 uint32 klass=CLASS_MONK,requests=0;QuestStatus status=QUEST_STATUS_INCOMPLETE;QuestStatusData quest;std::function<void()>onCredit;
 Player*ToPlayer()override{return this;}ObjectGuid GetGUID()const{return guid;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return teleport;}
 void*GetMap()const{return map;}bool InSamePhase(Creature*)const{return phase;}uint32 getClass()const{return klass;}bool GetQuestRewardStatus(uint32 id)const{assert(id==41003);return rewarded;}
 QuestStatus GetQuestStatus(uint32 id)const{assert(id==41003);return status;}QuestStatusData*getQuestStatus(uint32 id){assert(id==41003);return hasQuest?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32 id,uint64 delay,std::function<void()>&&fn){assert(id==41003&&!delay);if(!quest.ScriptLifetime)quest.ScriptLifetime=std::make_shared<uint8>();fn();}
 void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92&&event==48371);++requests;if(onCredit)onCredit();if(accept){manager.current->step=1;manager.current->achievements.progress[46576].Counter=1;}}
};
Player*Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){return ToPlayer()?ToPlayer():owner;}
std::map<uint64,Player*>players;unsigned warnings=0;
namespace ObjectAccessor{Player*GetPlayer(Creature&,ObjectGuid id){auto i=players.find(id.value);return i==players.end()?nullptr:i->second;}}
struct Motion{unsigned moves=0;void MovePoint(uint32 id,float x,float y,float z){assert(id==1&&x==-1020.62f&&y==-3135.73f&&z==26.91f);++moves;}};
struct Creature:Unit{
 CreatureAI*i_AI=nullptr,*i_disabledAI=nullptr;bool m_AI_locked=false,m_isUpdate=false,world=true,alive=true,predelete=false,deleted=false,scoped=false;
 uint32 entry=101881,spawn=362837,mapId=1541,poses=0,kits=0,cleared=0,removed=0;void*map=reinterpret_cast<void*>(1);Motion motion;
 Creature*ToCreature()override{return this;}CreatureAI*AI(){return i_AI;}CreatureAI*GetAI()override{return AI();}
 bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}bool IsPreDelete()const{return predelete;}bool IsDelete()const{return deleted;}bool HasQuestObjectScope()const{return scoped;}
 uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return spawn;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 7;}void*GetMap()const{return map;}
 float GetHealthPct()const{return 100.f*health/maximum;}void SetByteValue(uint32 field,uint32 slot,uint8 value){assert(field==1&&!slot&&!value);++poses;}
 void SetAnimKitId(uint32 value){assert(!value);++kits;}Motion*GetMotionMaster(){return &motion;}void SetWalk(bool value){assert(!value);}
 void ClearUnitState(uint32 state){assert(state==1);++cleared;}void RemoveAura(uint32 id){assert(id==204237);++removed;}
 void NotifyHealApplied(Unit*,uint32,int32);void Update(uint32);
};
#define TC_LOG_WARN(...) (++warnings)
#include "Notify.inc"
#include "DealHeal.inc"
#include "Tick.inc"
struct EventMap{void Reset(){}};
struct Taran:CreatureAI{
 Creature*me;bool _introHealDone=false,wait_heal=false;unsigned talk2=0;EventMap events;
 explicit Taran(Creature*actor):me(actor){}
 void Talk(uint32 id){assert(id==2);++talk2;}
#ifndef BASELINE
#include "Intro.inc"
#endif
#include "HealReceived.inc"
#include "Reset.inc"
};
struct Filter:CreatureAI{bool cancel=false;void HealDone(Unit*,uint32& amount)override{if(cancel)amount=0;}};
struct Test{
 Scenario scenario;Player player,peer;Unit pet,guardian;Creature actor;Taran ai;Filter filter;CreatureAI temporary;
 Test():ai(&actor){manager.current=&scenario;warnings=0;players.clear();players[42]=&player;peer.guid={43};players[43]=&peer;pet.owner=guardian.owner=&player;actor.IsAIEnabled=true;actor.i_AI=&ai;player.IsAIEnabled=true;player.healerAI=&filter;}
 void Heal(uint32 amount,Unit*source=nullptr){(source?source:&player)->DealHeal(&actor,amount);}void Tick(uint32 diff=0){actor.Update(diff);}
 void Done(uint32 requests=1){assert(ai._introHealDone&&player.requests==requests&&scenario.step==1&&scenario.achievements.progress[46576].Counter==1&&actor.motion.moves==1&&ai.talk2==1&&actor.poses==1&&actor.kits==1);}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);unsigned cases=0;
#ifdef BASELINE
 {Test t;t.actor.health=900;t.Heal(50);assert(t.actor.health==950&&!t.player.requests&&!t.actor.motion.moves);++cases;}
 {Test t;t.actor.health=950;t.filter.cancel=true;t.Heal(50);assert(t.actor.health==950&&t.player.requests==1&&t.actor.motion.moves==1);++cases;}
 {Test t;t.actor.health=950;t.player.accept=false;t.Heal(50);assert(t.ai._introHealDone&&t.actor.motion.moves==1&&t.scenario.step==0);t.player.accept=true;t.Heal(1);assert(t.player.requests==1);++cases;}
 {Test t;t.actor.health=950;t.Heal(1,&t.pet);assert(t.ai._introHealDone&&!t.player.requests&&t.actor.motion.moves==1);++cases;}
 std::cout<<"PASS COUNTERFACTUAL pinned3ba6d8c6f "<<cases<<" native applied-heal pipelines: crossing95 misses opening; cancelled heal at95 starts it; rejected credit consumes latch; pet healer loses credit. Controlled health/criteria/world sinks, not passing gameplay.\n";
#else
 for(unsigned actorMode=0;actorMode<5;++actorMode)for(unsigned source=0;source<3;++source)for(unsigned amount:{849u,850u,1000u}){
  Test t;if(actorMode==1){t.actor.i_AI=&t.temporary;t.actor.i_disabledAI=&t.ai;}if(actorMode==2)t.actor.i_disabledAI=&t.ai;if(actorMode==3){t.actor.i_AI=nullptr;t.actor.i_disabledAI=&t.ai;t.actor.IsAIEnabled=false;}if(actorMode==4)t.actor.i_AI=nullptr,t.actor.IsAIEnabled=false;
  t.Heal(amount,source==1?&t.pet:source==2?&t.guardian:nullptr);assert(!t.player.requests);t.Tick();bool done=actorMode!=4&&amount>=850;
  if(done)t.Done();else assert(!t.player.requests&&!t.actor.motion.moves);
  for(unsigned i=0;i<3;++i){t.Heal(1);t.Tick(1000);}if(actorMode!=4)t.Done();else assert(!t.player.requests);++cases;
 }
 for(unsigned mode=0;mode<3;++mode){Test t;t.actor.health=950;t.filter.cancel=mode==0;t.Heal(mode==1?0:1);t.Tick();if(mode==2)t.Done();else assert(!t.player.requests&&!t.actor.motion.moves);++cases;}
 for(unsigned denial=0;denial<22;++denial){Test t;switch(denial){case 0:t.actor.world=false;break;case 1:t.actor.alive=false;break;case 2:t.actor.predelete=true;break;case 3:t.actor.scoped=true;break;case 4:++t.actor.entry;break;case 5:++t.actor.spawn;break;case 6:++t.actor.mapId;break;case 7:++t.scenario.id;break;case 8:t.scenario.step=1;break;case 9:t.player.world=false;break;case 10:t.player.alive=false;break;case 11:t.player.contact=false;break;case 12:t.player.gm=true;break;case 13:t.player.teleport=true;break;case 14:t.player.map=nullptr;break;case 15:t.player.phase=false;break;case 16:t.player.klass=1;break;case 17:t.player.rewarded=true;break;case 18:t.player.status=QUEST_STATUS_NONE;break;case 19:t.player.hasQuest=false;break;case 20:manager.current=nullptr;break;case 21:t.actor.health=899;break;}t.Heal(50);t.Tick();assert(!t.player.requests&&!t.actor.motion.moves);++cases;}
 {Test t;t.player.status=QUEST_STATUS_COMPLETE;t.Heal(850);t.Tick();t.Done();++cases;}
 {Test t;t.player.accept=false;t.Heal(850);t.Tick();assert(t.player.requests==1&&!t.ai._introHealDone&&!t.actor.motion.moves);t.Tick(999);assert(t.player.requests==1);t.player.accept=true;t.Tick(1);t.Done(2);++cases;}
 {Test t;t.player.accept=false;t.Heal(850);t.Tick();for(unsigned i=1;i<30;++i)t.Tick(1000);assert(t.player.requests==30&&warnings==1&&!t.actor.motion.moves&&t.ai._introHealer.IsEmpty());t.Tick(100000);assert(t.player.requests==30);t.player.accept=true;t.Heal(1);t.Tick();assert(t.player.requests==31&&t.actor.motion.moves==1);++cases;}
 for(unsigned revoke=0;revoke<8;++revoke){Test t;t.Heal(850);switch(revoke){case 0:t.player.quest.ScriptLifetime.reset();break;case 1:t.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:t.player.status=QUEST_STATUS_NONE;break;case 3:players.erase(42);break;case 4:t.player.map=nullptr;break;case 5:t.actor.health=949;break;case 6:t.ai.Reset();break;case 7:t.scenario.step=2;break;}t.Tick();assert(!t.player.requests&&!t.actor.motion.moves&&t.ai._introHealer.IsEmpty());++cases;}
 for(unsigned revoke=0;revoke<5;++revoke){Test t;t.player.onCredit=[&]{switch(revoke){case 0:t.player.quest.ScriptLifetime.reset();break;case 1:t.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:t.player.map=nullptr;break;case 3:t.ai.Reset();break;case 4:t.Heal(1);t.Tick();break;}};t.Heal(850);t.Tick();assert(t.player.requests==1);if(revoke==4)t.Done();else assert(!t.actor.motion.moves&&!t.ai._introHealDone);++cases;}
 for(unsigned revoke=0;revoke<8;++revoke){Test t;t.player.onCredit=[&]{switch(revoke){case 0:t.player.phase=false;break;case 1:t.player.rewarded=true;break;case 2:t.actor.world=false;break;case 3:t.actor.scoped=true;break;case 4:++t.actor.entry;break;case 5:++t.actor.spawn;break;case 6:++t.actor.mapId;break;case 7:t.player.hasQuest=false;break;}};t.Heal(850);t.Tick();assert(t.player.requests==1&&!t.actor.motion.moves&&!t.ai._introHealDone);++cases;}
 for(unsigned partial=0;partial<2;++partial){Test t;t.player.accept=false;t.player.onCredit=[&]{if(partial)t.scenario.step=1;else t.scenario.achievements.progress[46576].Counter=1;};t.Heal(850);t.Tick();assert(!t.actor.motion.moves&&!t.ai._introHealDone);t.scenario.step=1;t.scenario.achievements.progress[46576].Counter=1;t.player.onCredit={};t.Tick(1000);t.Done();++cases;}
 {Test t;t.Heal(850);t.actor.health=950;t.Heal(1,&t.peer);assert(t.ai._introHealer==t.player.guid);t.Tick();t.Done();assert(!t.peer.requests);++cases;}
 std::cout<<"PASS "<<cases<<" actual Taran opening/Reset helpers with native DealHeal/NotifyHealApplied/Creature lifecycle prefix under ASAN: crossing95, cancelled/zero/overheal, original Player/pet/guardian, current+suspended/alias/missing AI, exact real actor/map/scenario/active quest admission, finite accepted48371/46576 retry, first healer ownership and reentrant/abandon/reset/retake/map/health revocation. Controlled health/criteria/geometry/motion, no client or later combat/pickup proof.\n";
#endif
}
