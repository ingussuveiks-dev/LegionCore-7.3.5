// Extracted complete methods/callbacks with controlled external endpoints.
// This ASAN fixture does not represent whole raid/client traversal.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/server/game/Chat/CommandIntegerArgument.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using uint64=uint64_t;
static constexpr uint32 UNIT_STATE_EVADE=1,REACT_AGGRESSIVE=2,CURRENT_CHANNELED_SPELL=3;
struct Unit;struct Session;using WorldObject=Unit;using Player=Unit;using Creature=Unit;using GameObject=Unit;
enum class GossipOptionNpc{SpiritHealer,BattleMaster};
struct Menus{void ClearMenus(){}};
using SpellCastResult=uint32;
struct AuraEffect{float amount=10;float GetAmount()const{return amount;}};
struct Vehicle{Unit*passenger=nullptr;Unit*GetPassenger(uint32){return passenger;}};
struct Position{float x=0,y=0,z=0,o=0;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}void Relocate(float a,float b,float c,float d){x=a;y=b;z=c;o=d;}};
struct ObjectGuid{static ObjectGuid Empty;};ObjectGuid ObjectGuid::Empty;
enum class HighGuid{AreaTrigger};
struct Generator{uint64 Generate(){return 1;}};struct ObjectMgr{Generator generator;template<HighGuid>Generator*GetGenerator(){return &generator;}}objectMgr;auto sObjectMgr=&objectMgr;
struct AreaTrigger;static AreaTrigger*createdArea=nullptr;static bool areaSuccess=false;static uint32 areaDeletes=0;
struct AreaTrigger{uint32 scales=0;AreaTrigger(){createdArea=this;}~AreaTrigger(){++areaDeletes;if(createdArea==this)createdArea=nullptr;}template<class...T>bool CreateAreaTrigger(T...){return areaSuccess;}void SetSphereScale(float,uint32,bool){++scales;}};
struct Queue{std::vector<std::function<void()>> functions;std::vector<uint64> delays;void Add(uint64 n,std::function<void()>f){delays.push_back(n);functions.push_back(std::move(f));}void Drain(){auto pending=std::move(functions);functions.clear();for(auto&f:pending)f();}};
struct CreatureGroup{std::map<Creature*,uint32> members;auto&GetMembers(){return members;}};
struct InstanceScript{uint32 stage=0;uint32 getScenarionStep(){return stage;}uint64 GetGuidData(uint32){return 1;}};
struct Map{bool mythic=false;uint32 players=20;void LoadGrid(float,float){}bool IsMythicRaid(){return mythic;}uint32 GetPlayersCountExceptGMs(){return players;}Creature*GetCreature(uint64);};
struct Motion{Unit*owner;void MoveRandom(float);template<class...T>void MovePoint(T...);void Clear();};
static Unit*selected=nullptr;
struct Unit{uint32 kind=2,role=0,calls=0,last=0,entry=299,mapId=0;float distance=0,z=0,health=10000,lastAmount=0;Vehicle*vehicle=nullptr;bool combat=false,IsAIEnabled=true,gm=true;Menus menus;Menus*PlayerTalkClass=&menus;Queue queue;Map map;InstanceScript*instance=nullptr;CreatureGroup*formation=nullptr;Unit*owner=nullptr,*victim=nullptr;std::list<uint32> auras;Motion motion{this};
 bool isGameMaster(){return gm;}Session*GetSession(){return nullptr;}ObjectGuid GetGUID(){return {};}void CLOSE_GOSSIP_MENU(){}template<class...T>void ADD_GOSSIP_ITEM_EXTENDED(T...){}template<class...T>void ADD_GOSSIP_ITEM(T...){}template<class...T>void SEND_GOSSIP_MENU(T...){}void MovePosition(Position&,float,float){}
 uint32 GetMapId(){return mapId;}float GetPositionX(){return 0;}float GetPositionY(){return 0;}float GetOrientation(){return 0;}Position GetPosition(){return {};}
 uint32 GetEntry(){return entry;}Vehicle*GetVehicleKit(){return vehicle;}float CountPctFromCurHealth(float pct){return health*pct/100;}void CastCustomSpell(Unit*,uint32 n,float const*amount,int,int,bool){last=n;lastAmount=*amount;++calls;}
 bool IsPlayer(){return kind==1;}Player*ToPlayer(){return IsPlayer()?this:nullptr;}Creature*ToCreature(){return kind==2?this:nullptr;}bool HasAura(uint32 id){return std::find(auras.begin(),auras.end(),id)!=auras.end();}uint32 GetSpecializationRole(){return role;}Map*GetMap(){return &map;}void AddDelayedEvent(uint64 n,std::function<void()>f){queue.Add(n,std::move(f));}void SetReactState(uint32 v){last=v;++calls;}Motion*GetMotionMaster(){return &motion;}void InterruptSpell(uint32 v){last=v;++calls;}void RemoveAura(uint32 v){last=v;++calls;}Unit*GetAnyOwner(){return owner;}Unit*AI(){return this;}void DoAction(uint32 v){last=v;++calls;}void Talk(uint32 v){last=v;++calls;}void ZoneTalk(uint32 v){last=v;++calls;}bool isInCombat(){return combat;}void ClearUnitState(uint32){++calls;}InstanceScript*GetInstanceScript(){return instance;}Creature*FindNearestCreature(uint32,float,bool=true){return selected;}CreatureGroup*GetFormation(){return formation;}Unit*getVictim(){return victim;}float GetDistance(Unit*){return distance;}float GetPositionZ(){return z;}void SetFacingToObject(Unit*){++calls;}void CastSpell(Unit*,uint32 id,bool=false){last=id;++calls;}void AttackStart(Unit*){++calls;}void DespawnOrUnsummon(uint32 n){last=n;++calls;}void setActive(bool){}void SetControlled(bool,uint32){}template<class...T>Creature*SummonCreature(uint32 n,float,float,float,float,uint32,uint32 despawn){last=despawn;++calls;return nullptr;}
};
Creature*Map::GetCreature(uint64){return selected;}void Motion::MoveRandom(float){++owner->calls;}template<class...T>void Motion::MovePoint(T...){++owner->calls;}void Motion::Clear(){++owner->calls;}
static void GetCreatureListWithEntryInGrid(std::list<Creature*>&v,Unit*,uint32,float){if(selected)v.push_back(selected);}
static float frand(float a,float){return a;}
struct Base{Unit*me;Unit*go;explicit Base(Unit*u):me(u),go(u){}void DoCast(uint32 n){me->CastSpell(me,n);}void AttackStart(Unit*u){me->AttackStart(u);}};
using GameObjectAI=Base;
using ScriptedAI=Base;
struct Session{Unit*player;Unit*GetPlayer(){return player;}};
struct ChatHandler{Session*session=nullptr;bool sent=false;uint32 messages=0;explicit ChatHandler(Session*s=nullptr):session(s){}Session*GetSession(){return session;}void SendSysMessage(uint32){++messages;}void SetSentErrorMessage(bool b){sent=b;}void PSendSysMessage(char const*,uint32){++messages;}};
struct CreatureScript{explicit CreatureScript(char const*){}};
static float rand_norm(){return 0.5f;}
struct Hook{template<class T>void operator+=(T){}};
using SpellEffIndex=uint32;
struct SpellScript{Unit*caster=nullptr,*hit=nullptr;Hook OnObjectAreaTargetSelect,OnEffectHitTarget,OnCheckCast;Unit*GetCaster(){return caster;}Unit*GetHitUnit(){return hit;}Unit*GetExplTargetUnit(){return hit;}Unit*GetTarget(){return hit;}};
#define PrepareSpellScript(T) public:
#define SpellObjectAreaTargetSelectFn(...) 0
#define SpellEffectFn(...) 0
#define SpellCheckCastFn(...) 0
namespace Trinity{struct UnitSortDistance{bool near;Unit*caster;UnitSortDistance(bool n,Unit*c):near(n),caster(c){}bool operator()(Unit*a,Unit*b){return a->distance<b->distance;}};namespace Containers{template<class T>void RandomResizeList(std::list<T>&v,uint32 n){if(v.size()>n)v.resize(n);}}}
#include "Selected.inc"
static void Require(bool ok,char const*message){if(!ok)throw std::runtime_error(message);}
template<class T>static void Life(){Unit actor,target;InstanceScript instance;actor.instance=&instance;selected=&target;{auto ai=std::make_unique<T>(&actor);ai->Start();Require(!actor.queue.functions.empty(),"Native source callback not queued");ai.reset();actor.queue.Drain();Require(!actor.calls&&!target.calls,"Retired AI callback executed");}actor.queue.delays.clear();{T live(&actor);live.Start();actor.queue.Drain();Require(actor.calls+target.calls>0,"Live callback payload was lost");target.queue.Drain();}selected=nullptr;}
static void Run(uint32 which){
 Unit caster,pet,object,first,second,third;first.kind=second.kind=third.kind=1;object.kind=3;std::list<WorldObject*> targets={nullptr,&pet,&object,&first,&second,&third};
 if(which==0){Case0::Selected s;s.caster=&caster;second.auras.push_back(Case0::SPELL_ENFLAME_CORRUPTION_MARK);s.FilterTargets(targets);Require(targets.size()==2&&std::find(targets.begin(),targets.end(),&second)==targets.end(),"Corruption must retain eligible players only with native cap/marks");}
 if(which==1){Case1::Selected s;s.caster=&caster;first.role=Case1::ROLES_HEALER;second.role=third.role=Case1::ROLES_DPS;s.FilterTargets(targets);Require(targets.size()==2&&targets.front()==&first,"Molten Touch must retain one eligible healer and one DPS");auto previous=targets;s.FilterTargets(targets);Require(targets==previous,"Other spell effects must reuse the selected targets");}
 if(which==2){Case2::Selected s;s.caster=&caster;first.auras.push_back(Case2::SPELL_SHADOWTOUCHED);third.auras.push_back(Case2::SPELL_SHADOWTOUCHED);first.distance=1;third.distance=2;s.FilterTargets(targets);Require(targets.size()==1&&targets.front()==&first,"Shadowscar must retain native marked-player nearest target");}
 if(which==3){Case3::Selected s;s.caster=&caster;Require(s.CheckElevation()==Case3::SPELL_FAILED_BAD_TARGETS,"Missing passenger target must be rejected before dereference");caster.entry=542172;first.entry=542169;s.hit=&first;Require(s.CheckElevation()==Case3::SPELL_CAST_OK,"Native correct passenger admission changed");Vehicle vehicle;vehicle.passenger=&second;caster.vehicle=&vehicle;Require(s.CheckElevation()==Case3::SPELL_FAILED_NOT_READY,"Occupied seat must still reject the cast");caster.vehicle=nullptr;first.entry=299;Require(s.CheckElevation()==Case3::SPELL_FAILED_BAD_TARGETS,"Wrong passenger entry must remain rejected");}
 if(which==4){Case4::Selected s;s.hit=&first;AuraEffect effect;s.HandleEffectPeriodic(&effect);Require(!caster.calls,"Missing Swarm caster must not manufacture damage/healing");s.caster=&caster;first.health=1000;s.HandleEffectPeriodic(&effect);Require(caster.calls==2&&caster.lastAmount==250,"Native Swarm250 minimum damage/heal must remain");first.health=10000;s.HandleEffectPeriodic(&effect);Require(caster.calls==4&&caster.lastAmount==1000,"Native Swarm current-health damage/heal must remain");}
 if(which==5){for(auto id:{0u,1u,1220u,999u}){Unit actor;actor.mapId=id;areaSuccess=false;auto deleted=areaDeletes;{Case5::Selected script(&actor);}Require(!createdArea&&areaDeletes==deleted+1,"Failed area creation must release the object exactly once");Require(actor.calls==(id==999?0u:2u),"Independent Snowglobe map visuals must survive a missing area trigger");areaSuccess=true;{Case5::Selected script(&actor);}Require(createdArea&&createdArea->scales==uint32(id==0||id==1),"Valid Snowglobe retains native map-specific sphere scaling");delete createdArea;}}
 if(which==6){areaSuccess=false;Session session{&first};ChatHandler handler;handler.session=&session;Require(!Case6::Selected::HandleAreaTriggerCommand(&handler,"12198")&&handler.sent&&handler.messages==1&&!createdArea,"Failed area trigger command must report failure and preserve its visible diagnostic");areaSuccess=true;handler.sent=false;Require(Case6::Selected::HandleAreaTriggerCommand(&handler,"12198")&&!handler.sent&&createdArea,"Successful area trigger command must retain success");delete createdArea;handler.session=nullptr;Require(!Case6::Selected::HandleAreaTriggerCommand(&handler,"12198")&&handler.sent,"Existing console/missing-player guard must remain");}
 if(which==7)Life<Case7::Selected>();
 if(which==8){Unit gate,actor;InstanceScript instance;gate.instance=&instance;selected=&actor;Case8::Selected s(&gate);s.OnStateChanged(0,nullptr);Require(gate.queue.functions.empty()&&!s.eventDone,"No triggering unit cannot claim the sequence");s.OnStateChanged(0,&actor);s.OnStateChanged(0,&actor);Require(s.eventDone&&gate.queue.functions.size()==3&&actor.calls==1,"Gate activation must queue the authored sequence exactly once");selected=nullptr;}
 if(which==9){Unit creature,gm;auto script=std::make_unique<Case9::Selected>();Require(script->OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"2")&&script->OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,3,"1234"),"Configure actual stress tool");Require(script->OnGossipSelect(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,1),"Actual stress spawn admission");Require(script->OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,3,"9876"),"Later configuration change");script.reset();creature.queue.Drain();Require(creature.last==1234&&creature.calls==2,"Queued GM spawn must retain chosen settings without the retired script");
 Case9::Selected live;gm.gm=false;Require(!live.OnGossipSelect(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,1)&&!live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"3"),"Stale GM menu must not authorize a non-GM");gm.gm=true;Require(!live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,4,"0")&&!live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"-1")&&!live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"3junk")&&!live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,1,"4"),"Invalid settings cannot create zero-group hangs,wrap or invalid native placement range");Require(live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"3")&&live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,4,"300"),"Finite large grouping admission");creature.queue.delays.clear();live.OnGossipSelect(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,1);Require(creature.queue.functions.size()==3&&creature.queue.delays==std::vector<uint64>{10,15,20},"Grouping must stop at the exact requested count with original spawn cadence");creature.queue.Drain();Require(Case9::SelectedDelay(UINT32_MAX-1)==(uint64(UINT32_MAX)+1)*5,"Source spawn delay must not wrap at32 bits");Require(live.OnGossipSelectCode(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,2,"0"),"Zero-count no-op remains valid");live.OnGossipSelect(&gm,&creature,Case9::GOSSIP_SENDER_MAIN,1);Require(creature.queue.functions.empty(),"Zero count cannot enqueue stress work");}
}
int main(int argc,char**argv){try{Require(argc==2,"Select a case");auto n=uint32(std::stoul(argv[1]));Run(n);std::cout<<"PASS complete source first-batch case="<<n<<";controlled endpoints;ASAN\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
