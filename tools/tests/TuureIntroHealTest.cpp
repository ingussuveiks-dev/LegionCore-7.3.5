#include <cassert>
#include <cstdint>
#include <memory>
#include <map>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int64=int64_t;
enum {CLASS_PRIEST=5,QUEST_STATUS_NONE=0,QUEST_STATUS_COMPLETE=1,QUEST_STATUS_INCOMPLETE=3,CRITERIA_TYPE_SCRIPT_EVENT_2=92,UNIT_FIELD_BYTES_1=1,UNIT_STATE_STUNNED=8};
#include "Functions.inc"
struct Creature;struct Player;struct Unit;
struct ObjectGuid {uint64 high=0,low=0;bool IsPlayer()const{return high==1;}bool IsEmpty()const{return !high&&!low;}void Clear(){high=low=0;}bool operator==(ObjectGuid v)const{return high==v.high&&low==v.low;}explicit operator bool()const{return !IsEmpty();}operator uint64()const{return low;}};
struct Map {};
struct CreatureAI {virtual ~CreatureAI()=default;virtual void HealReceived(Unit*,uint32&){};virtual void HealDone(Unit*,uint32&){};virtual void Reset(){};virtual void UpdateLifecycle(uint32){};};
struct InstanceScript {bool CheckDamageManager()const{return false;}void UpdateDamageManager(ObjectGuid,int32,bool){};};
namespace WorldPackets::Combat {struct HealthUpdate {ObjectGuid Guid;uint64 Health=0;int Write(){return 0;}};}
struct Unit {
 virtual ~Unit()=default;uint64 health=1,maximum=1000;bool IsAIEnabled=false,alive=true,world=true,phase=true;Map* map=nullptr;ObjectGuid guid{2,1},owner,charmer;CreatureAI* healAI=nullptr;FunctionProcessor functions;
 virtual Player const* ToPlayer()const{return nullptr;}Player* ToPlayer(){return const_cast<Player*>(static_cast<Unit const*>(this)->ToPlayer());}
 virtual Creature const* ToCreature()const{return nullptr;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;
 ObjectGuid GetCharmerGUID()const{return charmer;}ObjectGuid GetOwnerGUID()const{return owner;}
#include "OwnerGuid.inc"
 ObjectGuid GetGUID()const{return guid;}Map* GetMap()const{return map;}bool IsInWorld()const{return world;}bool IsAlive()const{return alive;}
 bool InSamePhase(Unit* u)const{return phase&&u->phase;}uint64 GetHealth(Unit* =nullptr)const{return health;}uint64 GetMaxHealth(Unit* =nullptr)const{return maximum;}
 float GetHealthPct()const{return maximum?100.f*float(health)/float(maximum):0;}
 void SetHealth(uint64 v,uint32){health=v;}void SetHealthScal(uint64 v,Unit*,uint32){health=v;}
 virtual CreatureAI* GetAI(){return healAI;}InstanceScript* GetInstanceScript(){return nullptr;}
 void AddDelayedEvent(uint64 ms,std::function<void()>&& f){functions.AddDelayedEvent(ms,std::move(f));}
 int64 ModifyHealth(int64,Unit* =nullptr,uint32=0);int32 DealHeal(Unit*,uint32);
};
struct QuestStatusData {uint32 Status=3;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit {
 QuestStatusData quest;bool record=true,contact=true,pending=false,rewarded=false,near=true;uint32 classId=5,credits=0;std::function<void()> onCredit;
 Player(){guid={1,1};}Player const* ToPlayer()const override{return this;}
 bool CanContact()const{return contact;}bool HasPendingTeleport()const{return pending;}uint32 getClass()const{return classId;}
 bool IsWithinDistInMap(Unit*,float d)const{assert(d==100);return near;}bool GetQuestRewardStatus(uint32 q)const{assert(q==41993);return rewarded;}
 uint32 GetQuestStatus(uint32 q)const{assert(q==41993);return record?quest.Status:0;}
 QuestStatusData* getQuestStatus(uint32 q){assert(q==41993);return record?&quest:nullptr;}
 void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);
 void UpdateAchievementCriteria(uint32 type,uint32 asset){assert(type==92&&asset==50563);++credits;if(onCredit)onCredit();}
 template<class Packet>void SendDirectMessage(Packet const&){}
};
struct Creature:Unit {
 bool preDelete=false,m_AI_locked=false;uint32 entry=106134,mapId=1489,moves=0,poses=0,animations=0,removals=0;CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
 Creature(){IsAIEnabled=true;}
 bool IsPreDelete()const{return preDelete;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return 7;}
 Creature const* ToCreature()const override{return this;}CreatureAI* GetAI()override{return primary;}CreatureAI* AI(){return primary;}
 void ClearUnitState(uint32){}void RemoveAura(uint32 id){assert(id==204237);++removals;}
 void SetByteValue(uint32 field,uint32 offset,uint32 value){assert(field==1&&!offset&&!value);++poses;}void SetAnimKitId(uint32 id){assert(!id);++animations;}
 Creature* GetMotionMaster(){return this;}void MovePoint(uint32 id,float x,float y,float z){assert(id==1&&x==-13.92f&&y==1175.70f&&z==-66.31f);++moves;}
 void Tick(){CreatureAI* i_AI=primary;uint32 diff=0;
#include "Lifecycle.inc"
 }
};
struct ObjectAccessor {inline static std::vector<Player*> players;static Player* GetPlayer(Unit const& source,ObjectGuid id){for(auto p:players)if(p->guid==id&&p->map==source.map&&p->world)return p;return nullptr;}};
#include "Owner.inc"
#include "Quest.inc"
#include "ModifyHealth.inc"
#include "DealHeal.inc"
struct Scenario {uint32 id=1085,step=0;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}} scenario;
struct ScenarioMgr {Scenario* active=&scenario;Scenario* GetScenario(uint32 id){assert(id==7);return active;}} manager;
auto sScenarioMgr=&manager;
#include "Policy.inc"
struct EventMap {void Reset(){}};
struct Boros:CreatureAI {
 Creature* me;bool _introHealDone=false,wait_heal=false,_pendingHealRecovery=false;ObjectGuid _introHealer;std::weak_ptr<uint8> _introAttempt;EventMap events;uint32 talks=0;
 explicit Boros(Creature* c):me(c){c->primary=this;}void Talk(uint32 id){assert(id==2);++talks;}
#include "Methods.inc"
};
struct HealerAI:CreatureAI {bool cancel=false;void HealDone(Unit*,uint32& amount)override{if(cancel)amount=0;}};
struct Run {Map map,other;Player player,stranger;Unit pet,guardian,npc;Creature actor;std::unique_ptr<Boros> ai;
 Run(){scenario={};manager.active=&scenario;player.map=stranger.map=pet.map=guardian.map=npc.map=actor.map=&map;stranger.guid={1,2};pet.owner=guardian.owner=player.guid;actor.health=500;ObjectAccessor::players={&player,&stranger};ai=std::make_unique<Boros>(&actor);}
};
int main(){
#ifdef BASELINE
 {Run r;assert(r.player.DealHeal(&r.actor,450)==450&&r.actor.health==950&&!r.ai->_introHealDone&&!r.player.credits);r.actor.Tick();assert(!r.player.credits);r.player.DealHeal(&r.actor,1);assert(r.player.credits==1&&r.actor.moves==1);}
 {Run r;r.actor.health=950;r.npc.DealHeal(&r.actor,1);assert(r.ai->_introHealDone&&r.actor.moves==1&&!r.player.credits);r.player.DealHeal(&r.actor,1);assert(!r.player.credits);}
 std::cout<<"PASS labelled df07f265 counterfactual: native HealReceived precedes ModifyHealth, so actual95pct crossing needs another heal; NPC heal at95 consumes transition without50563 and prevents later player recovery.\n";
#else
 uint32 variants=0;
 for(uint32 status:{3u,1u})for(int source=0;source<3;++source)for(int mode=0;mode<4;++mode){Run r;r.player.quest.Status=status;Unit* healer[]={&r.player,&r.pet,&r.guardian};assert(healer[source]->DealHeal(&r.actor,450)==450&&r.actor.health==950&&!r.ai->_introHealDone&&!r.player.credits);
  CreatureAI temporary;if(mode==1){r.actor.primary=&temporary;r.actor.i_disabledAI=r.ai.get();}if(mode==2)r.actor.i_disabledAI=r.ai.get();if(mode==3){r.actor.primary=nullptr;r.actor.i_disabledAI=r.ai.get();}
  r.player.onCredit=[&]{assert(r.ai->_introHealDone&&r.ai->_introHealer.IsEmpty()&&r.actor.m_AI_locked);uint32 amount=1;r.ai->HealReceived(&r.player,amount);};
  r.actor.Tick();assert(r.player.credits==1&&!r.stranger.credits&&r.ai->talks==1&&r.actor.moves==1&&r.actor.poses==1&&r.actor.animations==1);r.actor.Tick();assert(r.player.credits==1&&r.actor.moves==1);++variants;
 }
 for(int invalid=0;invalid<19;++invalid){Run r;r.player.DealHeal(&r.actor,450);switch(invalid){case 0:r.player.contact=false;break;case 1:r.player.alive=false;break;case 2:r.player.world=false;break;case 3:r.player.pending=true;break;case 4:r.player.classId=4;break;case 5:r.player.quest.Status=0;break;case 6:r.player.rewarded=true;break;case 7:r.player.phase=false;break;case 8:r.player.near=false;break;case 9:r.player.map=&r.other;break;case 10:r.actor.mapId=1220;break;case 11:r.actor.entry=106137;break;case 12:r.actor.alive=false;break;case 13:r.actor.world=false;break;case 14:r.actor.preDelete=true;break;case 15:scenario.id=972;break;case 16:scenario.step=1;break;case 17:manager.active=nullptr;break;case 18:r.player.record=false;break;}
  r.actor.Tick();assert(!r.player.credits&&!r.ai->_introHealDone&&!r.actor.moves&&r.ai->_introHealer.IsEmpty());}
 {Run r;r.player.DealHeal(&r.actor,449);r.actor.Tick();assert(!r.ai->_introHealDone&&r.ai->_introHealer.IsEmpty());r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);}
 {Run r;r.npc.DealHeal(&r.actor,450);r.actor.Tick();r.npc.DealHeal(&r.actor,1);r.actor.Tick();assert(!r.ai->_introHealDone);r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);}
 {Run r;r.player.DealHeal(&r.actor,450);auto held=r.player.quest.ScriptLifetime;r.player.quest={};r.actor.Tick();assert(!r.player.credits&&!r.ai->_introHealDone);r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1&&r.player.quest.ScriptLifetime!=held);}
 {Run r;r.player.DealHeal(&r.actor,450);r.ai->Reset();r.actor.Tick();assert(!r.player.credits);r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);r.ai->Reset();r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);}
 {Run r;r.player.DealHeal(&r.actor,450);r.ai.reset();r.ai=std::make_unique<Boros>(&r.actor);r.actor.Tick();assert(!r.player.credits);r.player.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);}
 {Run r;auto healer=std::make_unique<Player>();healer->map=&r.map;healer->guid={1,99};ObjectAccessor::players={healer.get(),&r.stranger};healer->DealHeal(&r.actor,450);healer.reset();ObjectAccessor::players={&r.stranger};r.actor.Tick();assert(!r.stranger.credits&&!r.actor.moves);}
 {Run r;r.player.DealHeal(&r.actor,450);Player replacement;replacement.map=&r.map;replacement.guid={3,1};ObjectAccessor::players={&replacement};r.actor.Tick();assert(!r.actor.moves&&!replacement.credits);}
 {Run r;r.player.DealHeal(&r.actor,450);Player replacement;replacement.map=&r.map;ObjectAccessor::players={&replacement};r.actor.Tick();assert(!r.actor.moves&&!replacement.credits);}
 for(bool cancel:{false,true}){Run r;HealerAI ai;ai.cancel=cancel;r.player.IsAIEnabled=true;r.player.healAI=&ai;r.player.DealHeal(&r.actor,cancel?450:0);r.actor.Tick();assert(r.actor.health==500&&!r.player.credits&&!r.ai->_introHealDone);}
 {Run r;r.player.DealHeal(&r.actor,450);r.actor.health=940;r.actor.Tick();assert(!r.player.credits);r.player.DealHeal(&r.actor,10);r.actor.Tick();assert(r.player.credits==1);}
 {Run r;uint32 amount=450;r.ai->HealReceived(nullptr,amount);r.actor.health=950;r.actor.Tick();assert(!r.actor.moves&&!r.player.credits);}
 {Run r;r.pet.owner={1,99};r.pet.DealHeal(&r.actor,450);r.actor.Tick();assert(!r.actor.moves);r.pet.owner=r.player.guid;r.pet.DealHeal(&r.actor,1);r.actor.Tick();assert(r.player.credits==1);}
 std::cout<<"PASS full actual Boros HealReceived/Reset/Cancel/Complete/lifecycle and native DealHeal/ModifyHealth/owner/quest lease/FunctionProcessor/lifecycle dispatch: "<<variants<<" status/player/pet/guardian/primary-disabled variants,19 revalidation denials,950 boundary/zero/cancelled/subthreshold/intervening damage, NPC/missing owner, abandon-reaccept with held old lease, Reset/retry/replaced-freed AI/recipient, fullGUID and reentrant exactly-once50563. Health/registry/motion/packet endpoints are adapters; actual first-heal/follow/client remains pending.\n";
#endif
}
