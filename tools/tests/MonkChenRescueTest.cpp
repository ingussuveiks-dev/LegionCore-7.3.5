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
#include <cstdlib>
#include <cmath>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using DamageEffectType=uint32;
#include "Functions.inc"
#include "DeathState.inc"
enum{TYPEID_UNIT,TYPEID_PLAYER,UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_SPELLCLICK=1,WAYPOINT_MOTION_TYPE=2,POINT_MOTION_TYPE=8,IDLE_MOTION_TYPE=0,UNIT_STATE_ROAMING_MOVE=16,UNIT_STATE_STUNNED=4,UNIT_STATE_CASTING=8,MOTION_SLOT_IDLE=0,CRITERIA_TYPE_SCRIPT_EVENT_2=92,CLASS_MONK=10,QUEST_STATUS_NONE=0,QUEST_STATUS_INCOMPLETE=3,MAX_QUEST_LOG_SIZE=25};
uint32 clockNow=0;uint32 getMSTime(){return clockNow;}
#include "Elapsed.inc"
#include "WaypointFields.inc"
using WaypointPath=std::vector<WaypointData>;
#include "PathBase.inc"
struct WaypointMgr{WaypointPath data,haitData;bool available=true,haitAvailable=true;WaypointMgr(){Reset();}void Reset(){available=haitAvailable=true;haitData.assign(9,WaypointData{});for(uint32 i=0;i<9;++i)haitData[i].id=i+1;haitData.back().x=3939.18f;haitData.back().y=1838.15f;haitData.back().z=904.334f;data.assign(6,WaypointData{});for(uint32 i=0;i<6;++i)data[i].id=i+1;data.back().x=3881.59f;data.back().y=1726.28f;data.back().z=932.669f;}WaypointPath const* GetPathScript(uint32 id){return available&&id==9859005?&data:haitAvailable&&id==9859006?&haitData:nullptr;}} waypointMgr;auto sWaypointMgr=&waypointMgr;
namespace Movement{struct MoveSpline{bool done=true;bool Finalized()const{return done;}};}
struct MovementGenerator{virtual ~MovementGenerator()=default;virtual uint32 GetMovementGeneratorType()const{return IDLE_MOTION_TYPE;}};
struct Creature;
template<class T> class WaypointMovementGenerator;
template<> class WaypointMovementGenerator<Creature>:public MovementGenerator,public PathMovementBase<Creature,WaypointPath const*>{
 public:uint32 _pathId=9859005;struct Wait{void Reset(uint32){}} _nextMoveTime;
 WaypointMovementGenerator(){_path=&waypointMgr.data;}
 uint32 GetMovementGeneratorType()const override{return WAYPOINT_MOTION_TYPE;}
 #include "PathId.inc"
 void Node(uint32 n){_currentNode=n;}void Bind(WaypointPath const* p){_path=p;}void OnArrived(Creature&);void MovementInform(Creature&);
};
float const PET_FOLLOW_ANGLE=1.57f,PET_FOLLOW_DIST=1.f;
uint32 urand(uint32 low,uint32){return low;}
struct ObjectGuid{uint64 low=0,high=0;bool operator==(ObjectGuid const& o)const{return low==o.low&&high==o.high;}bool operator!=(ObjectGuid const& o)const{return !(*this==o);}bool operator<(ObjectGuid const&o)const{return high<o.high||(high==o.high&&low<o.low);}bool IsEmpty()const{return !low&&!high;}void Clear(){low=high=0;}bool IsPlayer()const{return high==1;}};
struct Player;struct Creature;struct CreatureAI;struct InstanceScript;
struct CriteriaTreeEntry{uint32 ID,CriteriaID;};
uint32 const nativeRoots[]={45900,45841,45902,45904,45906,45908,45910};
struct DB2Manager{std::map<uint32,std::vector<CriteriaTreeEntry const*>> children;CriteriaTreeEntry leaves[7]={{45901,29068},{45842,29046},{45903,29069},{45905,29047},{45907,29070},{45909,29071},{45911,29072}};DB2Manager(){for(uint32 i=0;i<7;++i)children[nativeRoots[i]]={&leaves[i]};}auto GetCriteriaTreeList(uint32 tree){auto f=children.find(tree);return f==children.end()?nullptr:&f->second;}} sDB2Manager;
struct Scenario{uint32 id=943,step=3,currentTree=45904,_criteriaTreeForUpdate=0;struct Progress{uint64 Counter=0;};std::map<uint32,Progress> progress;uint32 GetScenarioId()const{return id;}uint32 GetCurrentStep()const{return step;}Scenario& GetAchievementMgr(){return *this;}auto GetCriteriaProgressMap(){return &progress;}bool CanUpdateCriteria(uint32,uint32=0)const;};
#include "CanCriteria.inc"
struct ScenarioMgr{Scenario* current=nullptr;Scenario* GetScenario(uint32){return current;}} manager;auto sScenarioMgr=&manager;
struct Map{uint32 id=1014,instance=7;bool unloading=false;std::map<ObjectGuid,Creature*> creatures;struct Ref{Player* player;Player* getSource()const{return player;}};std::vector<Ref> players;InstanceScript* script=nullptr;std::function<void(uint32)> onUpdate;void Update(uint32 diff){if(onUpdate)onUpdate(diff);}void ScriptsStart(uint32,uint32,Creature*,void*){}bool IsMapUnload()const{return unloading;}uint32 GetId()const{return id;}uint32 GetInstanceId()const{return instance;}Creature* GetCreature(ObjectGuid guid){auto f=creatures.find(guid);return f==creatures.end()?nullptr:f->second;}auto const& GetPlayers(){return players;}};
struct InstanceMap:Map{InstanceScript* i_data=nullptr;void Update(uint32);};
struct Unit{Map* map=nullptr;ObjectGuid guid,owner;virtual ~Unit()=default;virtual uint32 GetTypeId()const{return TYPEID_UNIT;}virtual float GetPositionZ()const{return 0;}virtual Player const* ToPlayer()const{return nullptr;}virtual Player* ToPlayer(){return nullptr;}Unit* ToUnit(){return this;}Map* GetMap()const{return map;}ObjectGuid GetGUID()const{return guid;}ObjectGuid GetCharmerOrOwnerGUID()const{return owner;}Player* GetCharmerOrOwnerPlayerOrPlayerItself()const;void DispatchDeath(Creature*);};
struct QuestStatusData{uint32 Status=QUEST_STATUS_INCOMPLETE;std::shared_ptr<uint8> ScriptLifetime;};
struct Player:Unit{uint32 type=CLASS_MONK,status=QUEST_STATUS_INCOMPLETE,slot=0;bool world=true,contact=true,gm=false,teleport=false,reward=false,phase=true,accept=true;float distance=10;QuestStatusData quest;FunctionProcessor functions;std::vector<uint32> credits;std::function<void()> onCredit;uint32 GetTypeId()const override{return TYPEID_PLAYER;}Player const* ToPlayer()const override{return this;}Player* ToPlayer() override{return this;}uint32 getClass()const{return type;}uint32 GetQuestStatus(uint32)const{return status;}bool GetQuestRewardStatus(uint32)const{return reward;}uint32 FindQuestSlot(uint32)const{return slot;}auto getQuestStatus(uint32){return status?&quest:nullptr;}bool IsInWorld()const{return world;}bool CanContact()const{return contact;}bool isGameMaster()const{return gm;}bool HasPendingTeleport()const{return teleport;}bool InSamePhase(Creature*)const{return phase;}void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}void AddQuestDelayedEvent(uint32,uint64,std::function<void()>&&);void UpdateAchievementCriteria(uint32 type,uint32 id,uint64=0,uint64=0,Unit* =nullptr,bool=false){assert(type==92);credits.push_back(id);auto scenario=manager.current;if(scenario&&scenario->step<7)scenario->currentTree=nativeRoots[scenario->step];if(accept&&id==46956&&scenario&&scenario->CanUpdateCriteria(45905)){scenario->progress[29047].Counter=1;scenario->step=4;}if(accept&&id==46947&&scenario&&scenario->CanUpdateCriteria(45907)){scenario->progress[29070].Counter=1;scenario->step=5;}if(accept&&id==46951&&scenario&&scenario->CanUpdateCriteria(45909)){scenario->progress[29071].Counter=1;scenario->step=6;}if(onCredit)onCredit();}};
namespace ObjectAccessor{std::map<ObjectGuid,Unit*> units;std::map<ObjectGuid,Player*> players;Player* FindPlayer(ObjectGuid guid){auto f=players.find(guid);return f==players.end()?nullptr:f->second;}Player* GetPlayer(Unit const& unit,ObjectGuid guid){auto player=FindPlayer(guid);return player&&player->map==unit.map?player:nullptr;}}
namespace ObjectAccessor{Unit* GetUnit(Unit const& caller,ObjectGuid guid){auto p=GetPlayer(caller,guid);if(p)return p;auto f=units.find(guid);return f!=units.end()&&f->second->map==caller.map?f->second:nullptr;}}
#include "Owner.inc"
#include "QuestDelay.inc"
struct Motion{uint32 points=0,follows=0,requests=0;bool install=true;std::vector<uint32> paths;WaypointMovementGenerator<Creature> waypoint;MovementGenerator* idle=nullptr;MovementGenerator* current=nullptr;Movement::MoveSpline* spline=nullptr;std::function<void()> onPoint;void MovePoint(uint32,float,float,float){++points;idle=current=nullptr;if(onPoint)onPoint();}void MovePath(uint32 id,bool){++requests;if(!install)return;paths.push_back(id);waypoint._pathId=id;waypoint.Bind(sWaypointMgr->GetPathScript(id));waypoint.Node(0);idle=current=&waypoint;if(spline)spline->done=false;}std::function<void()> onFollow;Unit* followTarget=nullptr;float followDistance=0,followAngle=0;uint32 followSlot=0;void MoveFollow(Unit* target,float distance,float angle,uint32 slot){++follows;followTarget=target;followDistance=distance;followAngle=angle;followSlot=slot;if(onFollow)onFollow();}MovementGenerator* GetMotionSlot(uint32){return idle;}bool empty()const{return !current;}MovementGenerator* top(){return current;}void Retire(){idle=current=nullptr;}};
struct Creature:Unit{
 CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;bool m_AI_locked=false,IsAIEnabled=true;
 DeathState deathState=ALIVE;DeathState getDeathState(){return deathState;}
 uint32 entry=103148,spawn=362201,states=0,flags=UNIT_NPC_FLAG_SPELLCLICK,attacks=0,melee=0;bool world=true,predelete=false,hidden=false,near=true,phaseVisible=true,victim=false;Motion motion;Player* nearest=nullptr;Creature* hait=nullptr;Creature* morvat=nullptr;Creature* li=nullptr;std::vector<Creature*> adds;FunctionProcessor functions;
 std::vector<std::pair<uint32,ObjectGuid>> talks;std::function<void()> onTalk;
 Movement::MoveSpline spline;Movement::MoveSpline* movespline=&spline;float x=1,y=1,z=1,distance=0;uint32 waypointID=0;
 Creature(){motion.spline=&spline;}void UpdateWaypointID(uint32 id){waypointID=id;}uint32 GetCurrentWaypointID()const{return waypointID;}void ClearUnitState(uint32 flag){states&=~flag;}
 bool IsWithinDist3d(float dx,float dy,float dz,float range,bool)const{return std::sqrt((x-dx)*(x-dx)+(y-dy)*(y-dy)+(z-dz)*(z-dz))<=range;}
 CreatureAI* AI(){return i_AI;}CreatureAI* GetAI(){return i_AI;}CreatureAI* GetDisabledAI(){return i_disabledAI;}bool AIM_Destroy();void Lifecycle(uint32 diff);void KillAllDelayedCombats(){}
 uint32 GetMapId()const{return map?map->id:0;}uint32 GetInstanceId()const{return map?map->instance:0;}uint32 GetEntry()const{return entry;}uint32 GetDBTableGUIDLow()const{return spawn;}bool IsInWorld()const{return world;}bool IsPreDelete()const{return predelete;}bool IsAlive()const{return deathState==ALIVE;}bool ShouldHideFor(ObjectGuid)const{return hidden;}InstanceScript* GetInstanceScript(){return map?map->script:nullptr;}
 void RemoveFlag(uint32,uint32 flag){flags&=~flag;}void SetHomePosition(float,float,float,float){}float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const override{return z;}float GetOrientation()const{return 0;}
 bool InSamePhase(Creature* other)const{return phaseVisible&&other&&other->phaseVisible;}bool IsWithinDistInMap(Unit* unit,float range,bool=true)const{if(!near)return false;if(unit->GetTypeId()==TYPEID_PLAYER)return static_cast<Player*>(unit)->distance<=range;auto other=dynamic_cast<Creature*>(unit);return !other||other->distance<=range;}bool HasUnitState(uint32 flag)const{return (states&flag)!=0;}
 Motion* GetMotionMaster(){return &motion;}Player* FindNearestPlayer(float){return nearest;}Creature* FindNearestCreature(uint32 id,float,bool){return id==97778?hait:id==97811?morvat:id==98074?li:nullptr;}
 void AddDelayedEvent(uint64 delay,std::function<void()>&& f){functions.AddDelayedEvent(delay,std::move(f));}
};
void GetCreatureListWithEntryInGrid(std::list<Creature*>& out,Creature* actor,uint32 id,float){assert(id==98074);out.insert(out.end(),actor->adds.begin(),actor->adds.end());}
struct CreatureAI{Creature* me;explicit CreatureAI(Creature* actor):me(actor){}virtual ~CreatureAI()=default;virtual void Reset(){}virtual void UpdateLifecycle(uint32){}virtual void UpdateAI(uint32){}virtual void MovementInform(uint32,uint32){}virtual void MoveInLineOfSight(Unit*){}virtual void SetData(uint32,uint32){}virtual uint32 GetData(uint32)const{return 0;}virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void JustDied(Unit*){}void LastWPReached(){}void Talk(uint8 id,ObjectGuid WhisperGuid={});void AttackStart(Unit*){++me->attacks;}};
using UnitAI=CreatureAI;
struct TextMgr{void SendChat(Creature* actor,uint8 id,ObjectGuid guid={}){actor->talks.emplace_back(id,guid);if(actor->onTalk)actor->onTalk();}} text;auto sCreatureTextMgr=&text;
#include "TalkForward.inc"
void Creature::Lifecycle(uint32 diff){
#include "Lifecycle.inc"
}
#define TC_LOG_DEBUG(...) ((void)0)
#define ASSERT(condition,...) assert(condition)
#include "AIDestroy.inc"
bool roll_chance_i(uint32){return true;}uint32 sWaypointScripts=0;
#include "WaypointArrival.inc"
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ScriptedAI:CreatureAI{using CreatureAI::CreatureAI;bool UpdateVictim(){return me->victim;}void DoMeleeAttackIfReady(){++me->melee;}};
struct CreatureScript{explicit CreatureScript(char const*){}virtual ~CreatureScript()=default;virtual CreatureAI* GetAI(Creature*)const=0;};
struct InstanceScript{Map* instance;uint32 baseCreates=0,forScript=0;explicit InstanceScript(Map* m):instance(m){}virtual ~InstanceScript()=default;virtual void OnCreatureCreate(Creature*){++baseCreates;}virtual void CreatureDies(Creature*,Unit*){}void CreatureDiesForScript(Creature*,Unit*){++forScript;}virtual ObjectGuid GetGuidData(uint32)const{return {555,999};}virtual void SetGuidData(uint32,ObjectGuid){}virtual void Update(uint32){}void UpdateForScript(uint32){}};
#include "InstanceUpdate.inc"
#include "Rescue.inc"
#include "Jorvinax.inc"
#include "Presentation.inc"
#include "Actor.inc"
#include "BaselineActor.inc"
#include "Instance.inc"
void Unit::DispatchDeath(Creature* creature){
#include "NativeDeath.inc"
}
using Chen=npc_monk_chen_porter::npc_monk_chen_porterAI;
struct Root:CreatureAI{Creature* chen;uint32 calls=0;Root(Creature* m,Creature* c):CreatureAI(m),chen(c){}void JustDied(Unit*)override{++calls;if(chen&&chen->AI())chen->AI()->SetData(1,1);}};
struct Run{
 InstanceMap map;Scenario scenario;instance_monk_intro instance{&map};Creature chen,source,hait,li,add,kid3,kid4;Player player,other;Unit pet,guardian;Root root{&source,&chen};CreatureAI haitAI{&hait},temporary{&chen};
 Run(){clockNow=0;waypointMgr.Reset();ObjectAccessor::players.clear();manager.current=&scenario;map.script=&instance;chen.map=source.map=hait.map=player.map=other.map=pet.map=guardian.map=&map;chen.guid={21,2};source.guid={22,2};hait.guid={23,2};hait.entry=97778;hait.spawn=362271;player.guid={17,1};other.guid={18,1};pet.owner=guardian.owner=player.guid;source.entry=97811;source.spawn=362229;chen.i_AI=new Chen(&chen);source.i_AI=&root;chen.nearest=&player;chen.hait=&hait;chen.morvat=&source;chen.li=&li;li.map=add.map=kid3.map=kid4.map=&map;li.entry=add.entry=kid3.entry=kid4.entry=98074;li.spawn=362259;add.spawn=362263;kid3.spawn=362264;kid4.spawn=362265;li.guid={31,2};add.guid={32,2};kid3.guid={33,2};kid4.guid={34,2};chen.adds={&li,&add,&kid3,&kid4};hait.i_AI=&haitAI;map.creatures={{chen.guid,&chen},{source.guid,&source},{hait.guid,&hait}};map.creatures[li.guid]=&li;map.creatures[add.guid]=&add;map.creatures[kid3.guid]=&kid3;map.creatures[kid4.guid]=&kid4;map.players={{&player}};ObjectAccessor::players={{player.guid,&player},{other.guid,&other}};instance.OnCreatureCreate(&chen);instance.OnCreatureCreate(&source);instance.OnCreatureCreate(&hait);}
 ~Run(){delete chen.i_AI;if(chen.i_disabledAI&&chen.i_disabledAI!=chen.i_AI&&chen.i_disabledAI!=&temporary)delete chen.i_disabledAI;manager.current=nullptr;ObjectAccessor::players.clear();}
 auto& state(){return instance.chenRescue;}Chen* actor(){return static_cast<Chen*>(chen.i_disabledAI?chen.i_disabledAI:chen.i_AI);}
 void Death(Unit* killer=nullptr){source.deathState=CORPSE;Unit ally;ally.map=&map;(killer?killer:&ally)->DispatchDeath(&source);}
 void Tick(uint32 diff){clockNow+=diff;map.i_data=&instance;map.onUpdate=[this](uint32 elapsed){chen.Lifecycle(elapsed);chen.functions.Update(elapsed);player.functions.Update(elapsed);};map.Update(diff);}
 void Started(){assert(state().started&&!state().pending&&state().claim.IsEmpty()&&actor()->_rescueStarted);assert(scenario.step==4&&scenario.progress[29047].Counter==1);assert(chen.talks.size()==1&&chen.talks[0].first==3);assert(actor()->_presentationFunctions.SizeQueue()+actor()->_presentationFunctions.Size()==0);}
};
uint32 RunRescueCases(){
 _set_abort_behavior(0,_CALL_REPORTFAULT);
 // Pinned complete predecessor, not a hand-written alternative implementation.
 {Run r;r.scenario.step=2;delete r.chen.i_AI;r.chen.i_AI=new npc_monk_chen_porter_baseline::npc_monk_chen_porter_baselineAI(&r.chen);r.chen.i_AI->SetData(1,1);r.chen.Lifecycle(8000);assert(r.scenario.step==2&&!r.scenario.progress[29047].Counter&&r.chen.motion.paths==std::vector<uint32>{9859005});std::cout<<"COUNTERFACTUAL pinned6b6fae complete Chen: early46956 rejected by controlled current-tree endpoint but original escort starts; no actual client reproduction claimed.\n";}
 {Run r;r.other.status=QUEST_STATUS_NONE;r.chen.nearest=&r.other;delete r.chen.i_AI;r.chen.i_AI=new npc_monk_chen_porter_baseline::npc_monk_chen_porter_baselineAI(&r.chen);r.chen.i_AI->SetData(1,1);assert(r.player.credits.empty()&&r.other.credits==std::vector<uint32>{46956});}
 uint32 cases=0;
 {Run r;for(uint32 step=0;step<7;++step){r.scenario.currentTree=nativeRoots[step];assert(r.scenario.CanUpdateCriteria(45905)==(step==3));}r.scenario.currentTree=45906;r.scenario._criteriaTreeForUpdate=45904;assert(r.scenario.CanUpdateCriteria(45905));r.scenario.currentTree=0;r.scenario._criteriaTreeForUpdate=0;assert(!r.scenario.CanUpdateCriteria(45905));++cases;}
 for(uint32 killer=0;killer<5;++killer)for(uint32 step=0;step<=3;++step){Run r;r.scenario.step=step;Unit* unit=killer==0?static_cast<Unit*>(&r.player):killer==1?&r.pet:killer==2?&r.guardian:killer==3?static_cast<Unit*>(&r.chen):nullptr;r.Death(unit);if(step<3){assert(r.state().pending&&!r.state().started&&r.player.credits.empty()&&r.chen.talks.empty());r.Tick(100000);assert(r.player.credits.empty());r.scenario.step=3;r.Tick(0);}r.Started();r.Death(unit);r.actor()->SetData(1,1);r.Tick(2000);assert(r.player.credits==std::vector<uint32>{46956});r.Tick(5999);assert(r.chen.motion.paths.empty());r.Tick(1);assert(r.chen.motion.paths==std::vector<uint32>{9859005});++cases;}
 for(uint32 failure=0;failure<17;++failure){Run r;switch(failure){case 0:r.player.type=1;break;case 1:r.player.status=0;break;case 2:r.player.reward=true;break;case 3:r.player.slot=25;break;case 4:r.player.gm=true;break;case 5:r.player.world=false;break;case 6:r.player.contact=false;break;case 7:r.player.teleport=true;break;case 8:r.player.phase=false;break;case 9:r.map.id=2;break;case 10:r.scenario.id=933;break;case 11:r.scenario.step=4;break;case 12:r.source.spawn=1;break;case 13:r.source.entry=1;break;case 14:r.source.predelete=true;break;case 15:r.source.world=false;break;case 16:r.map.creatures.erase(r.source.guid);break;}if(failure==9){r.source.deathState=CORPSE;r.state().Death(&r.source,&r.player);}else r.Death();r.Tick(20000);assert(!r.state().started&&r.player.credits.empty()&&r.chen.talks.empty());++cases;}
 for(uint32 missing=0;missing<8;++missing){Run r;CreatureAI* saved=r.chen.i_AI;switch(missing){case 0:r.chen.i_AI=nullptr;break;case 1:r.chen.world=false;break;case 2:r.chen.predelete=true;break;case 3:r.chen.deathState=CORPSE;break;case 4:r.chen.hidden=true;break;case 5:r.player.distance=101;break;case 6:r.map.creatures.erase(r.chen.guid);break;case 7:r.chen.spawn=362134;break;}if(missing==7){r.source.deathState=CORPSE;r.state().Death(&r.source,&r.pet);}else r.Death(&r.pet);assert(r.state().pending&&!r.state().started&&r.player.credits.empty());r.chen.i_AI=saved;r.chen.world=true;r.chen.predelete=false;r.chen.deathState=ALIVE;r.chen.hidden=false;r.player.distance=100;r.map.creatures[r.chen.guid]=&r.chen;r.chen.spawn=362201;r.Tick(1999);assert(!r.state().started);r.Tick(1);r.Started();++cases;}
 {Run r;r.state().chen.Clear();r.map.creatures.erase(r.chen.guid);r.Death();assert(r.state().pending);r.map.creatures[r.chen.guid]=&r.chen;r.instance.OnCreatureCreate(&r.chen);r.Tick(2000);r.Started();++cases;}
 {Run r;r.source.IsAIEnabled=false;r.Death(&r.guardian);r.Started();assert(r.root.calls==0&&r.instance.forScript==1);++cases;}
 {Run r;delete r.chen.i_AI;r.chen.i_AI=nullptr;r.scenario.step=1;r.Death();r.chen.i_AI=new Chen(&r.chen);r.map.creatures.erase(r.source.guid);r.scenario.step=3;r.Tick(2000);r.Started();++cases;}
 {Run r;r.player.accept=false;r.Death();assert(r.state().pending&&r.scenario.step==3&&r.chen.talks.empty());for(uint32 i=0;i<100;++i)r.actor()->SetData(1,1);assert(r.player.credits.size()==1);r.Tick(1999);assert(r.player.credits.size()==1);r.player.accept=true;r.Tick(1);r.Started();assert(r.player.credits.size()==2);++cases;}
 {Run r;r.other.gm=true;r.chen.nearest=&r.other;r.map.players.insert(r.map.players.begin(),{&r.other});r.Death(&r.pet);r.Started();assert(r.other.credits.empty()&&r.player.credits==std::vector<uint32>{46956});++cases;}
 {Run r;r.map.players={{&r.other},{&r.player}};r.other.quest.ScriptLifetime=std::make_shared<uint8>();r.Death(&r.guardian);r.Started();assert(r.player.credits==std::vector<uint32>{46956}&&r.other.credits.empty());++cases;}
 for(uint32 cancel=0;cancel<7;++cancel){Run r;r.scenario.step=2;r.Death();auto old=r.player.quest.ScriptLifetime;switch(cancel){case 0:r.player.quest.ScriptLifetime.reset();break;case 1:r.player.quest.ScriptLifetime=std::make_shared<uint8>();break;case 2:r.player.map=nullptr;break;case 3:r.map.unloading=true;break;case 4:r.scenario.id=999;break;case 5:r.scenario.step=0;break;case 6:r.player.status=0;break;}r.Tick(2000);assert(!r.state().pending&&!r.state().started);r.player.map=&r.map;r.player.status=3;r.map.unloading=false;r.scenario.id=943;r.scenario.step=3;r.player.quest.ScriptLifetime=std::make_shared<uint8>();r.Death();r.Tick(2000);assert(r.player.credits.empty()&&r.chen.talks.empty());++cases;}
 for(uint32 reentry=0;reentry<6;++reentry){Run r;r.player.onCredit=[&]{if(reentry==0)r.state().Update(0);if(reentry==1)r.state().Cancel();if(reentry==2){delete r.chen.i_AI;r.chen.i_AI=new Chen(&r.chen);}if(reentry==3)manager.current=nullptr;if(reentry==4){r.scenario.step=2;}if(reentry==5)r.player.quest.ScriptLifetime=std::make_shared<uint8>();};r.Death();r.player.onCredit={};if(reentry==0||reentry==2)r.Started();else assert(!r.state().started&&r.chen.talks.empty());++cases;}
 {Run r;r.scenario.step=1;r.Death();assert(r.chen.AIM_Destroy());r.chen.i_AI=new Chen(&r.chen);r.scenario.step=3;r.Tick(0);r.Started();++cases;}
 {Run r;r.chen.i_disabledAI=r.chen.i_AI;r.chen.i_AI=nullptr;r.Death();r.Started();r.Tick(8000);assert(r.chen.motion.paths==std::vector<uint32>{9859005});++cases;}
 {Run r;r.actor()->SetData(1,1);r.Tick(20000);assert(r.player.credits.empty()&&r.chen.talks.empty());r.source.deathState=ALIVE;r.state().Death(&r.source,&r.pet);assert(!r.state().pending);++cases;}
 {Run r;r.chen.spawn=362134;r.actor()->SetData(1,1);assert(r.player.credits==std::vector<uint32>{46956}&&r.chen.talks.size()==1);++cases;}
 {Run r;assert(r.instance.baseCreates==3&&r.instance.GetGuidData(99)==(ObjectGuid{555,999}));++cases;}
 std::cout<<"PASS "<<cases<<" actual State/current full Chen/instance/native owner/death-order/quest-delay/FunctionProcessor/lifecycle ASAN contexts: early prerequisites, player/pet/guardian/ally kills, native46956 receipt before one original escort, retries, full source/actor identity, original weak attempts, reentrant cancellation and unchanged base/other actor behavior. Criteria/map/visibility/network/movement endpoints controlled; later arrival46947/recovery and client/full943/all36 remain separate. Expected modeled rejection warnings="<<warnings<<".\n";
 return cases;
}
#ifndef MONK_CHEN_ARRIVAL_TEST
int main(){RunRescueCases();}
#endif
