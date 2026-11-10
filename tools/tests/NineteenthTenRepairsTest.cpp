#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <stdexcept>
#include <iostream>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using uint64=uint64_t;using ObjectGuid=uint64;
struct Position{};
struct Queue{std::vector<std::pair<uint32,std::function<void()>>>q;void Add(uint32 n,std::function<void()>f){q.emplace_back(n,f);}void Update(uint32 n){std::vector<std::function<void()>>v;for(auto i=q.begin();i!=q.end();)if(i->first<=n){v.push_back(i->second);i=q.erase(i);}else{i->first-=n;++i;}for(auto&f:v)f();}};
static uint32 bad=0,attempts=0;
struct Motion{template<class...T>void MovePoint(T...){}void Initialize(){}void MoveRandom(float){}void MoveFollow(void*,float,float){}};struct ObservedAI{template<class...T>void DoZoneInCombat(T...){}void Talk(uint32){}};struct Go{void SetGoState(uint32){}};
struct Unit{uint64 guid;bool alive=true,world=true,combat=false,player=false,teleport=false;uint32 mapId=1,instanceId=1,payload=0,control=0;Unit*owner=nullptr,*vehicle=nullptr;Queue q;Motion motion;ObservedAI ai;Unit(uint64 n):guid(n){}void Touch(){if(!world)++bad;}ObjectGuid GetGUID(){Touch();return guid;}bool IsAlive(){Touch();return alive;}bool IsInWorld(){return world;}bool IsPlayer(){return player;}Unit*ToPlayer(){return player?this:nullptr;}bool isInCombat(){Touch();return combat;}uint32 GetTeam(){return 469;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instanceId;}bool IsBeingTeleported(){return teleport;}Unit*GetAnyOwner(){return owner;}Unit*GetVehicleBase(){return vehicle;}void AddDelayedEvent(uint32 n,std::function<void()>f){q.Add(n,f);}void SetClientControl(Unit*,bool){++control;}void SetReactState(uint32){}Motion*GetMotionMaster(){Touch();return &motion;}ObservedAI*AI(){Touch();return &ai;}void CastSpell(Unit*,uint32,bool=false){Touch();++payload;}void CreateConversation(uint32){Touch();++payload;}void CompleteQuest(uint32){Touch();++payload;}template<class...T>void TeleportTo(T...){Touch();++payload;}void CombatStop(bool){Touch();++payload;}Unit&getHostileRefManager(){return *this;}void deleteReferences(){Touch();++payload;}void UpdateEntry(uint32){Touch();++payload;}Go*FindNearestGameObject(uint32,float){return nullptr;}template<class...T>void SetHomePosition(T...){}void RemoveFlag(uint32,uint32){}Unit*SelectNearestTarget(float){return nullptr;}};using Creature=Unit;using Player=Unit;
static std::map<uint64,Unit*>units;struct Map{Creature*GetCreature(ObjectGuid id){auto i=units.find(id);return i!=units.end()&&i->second->world?i->second:nullptr;}};
namespace ObjectAccessor{Creature*GetCreature(Unit&,ObjectGuid id){return Map{}.GetCreature(id);}}
struct Events{void DelayEvent(uint32,uint32){}};struct npc_escortAI{virtual void UpdateAI(uint32){}};
struct Base:npc_escortAI{Creature*me;Player*escort=nullptr;Map map,*instance=&map;bool pause=false,PlayerOn=false;uint32 pauseTimer=0xCDCDCDCD,freezeTimer=5000,chaseTimer=1000,infernoTimer=2000,infernoCount=0,starts=0;Events events;explicit Base(Creature*u):me(u){}void AddDelayedEvent(uint32 n,std::function<void()>f){me->q.Add(n,f);}Creature*DoSummonFlyer(uint32,Creature*,uint32,uint32,uint32,uint32){++attempts;return nullptr;}void Start(bool,bool,ObjectGuid){++starts;}Player*GetPlayerForEscort(){return escort;}};
static void Require(bool ok,char const*s){if(!ok)throw std::runtime_error(s);}
#include "Constants.inc"
#include "Selected.inc"
int main(){try{Unit me(1),p(2),other(3);p.player=true;units={{1,&me},{2,&p},{3,&other}};Frame f(&me);bool fixed=false;
#ifdef CASE_BROKENINTRO
p.mapId=1460;
#endif
#if defined(CASE_INFERNOPAUSE)
f.UpdateAI(1);fixed=f.pauseTimer==0xCDCDCDCD;
#ifndef COUNTERFACTUAL
f.infernoTimer=1;for(int i=0;i<3;++i)f.UpdateAI(2000);Require(attempts==3&&f.pause,"three authored casts enter pause");auto before=f.infernoTimer;f.UpdateAI(1);Require(f.infernoTimer==before,"inactive cast timer not decremented");f.UpdateAI(6000);Require(attempts==4&&!f.pause,"pause expiry resumes authored cast");
#endif
#elif defined(CASE_HODIRTIMER)
f.UpdateAI(1);fixed=f.pauseTimer==59999;
#elif defined(CASE_YMIRONWALK)
f.Reset();fixed=f.m_bIsWalking==0;f.m_bIsWalking=1;f.Reset();
#ifndef COUNTERFACTUAL
Require(!f.m_bIsWalking,"retry cancels interrupted walking state");
#endif
#elif defined(CASE_YMIRONPRISONER)||defined(CASE_OPERAASSISTANT)||defined(CASE_OPERABRUTE)
#if defined(CASE_YMIRONPRISONER)
f.Start(&other);
#elif defined(CASE_OPERAASSISTANT)
f.Start(&other,Position{});
#else
f.Start(&me,&other);
#endif
other.world=false;me.q.Update(20000);fixed=!bad;
#elif defined(CASE_VETIRRIDE)
me.owner=&p;f.escort=&p;p.vehicle=&me;f.PassengerBoarded(&p,0,false);fixed=!f.starts;
#ifndef COUNTERFACTUAL
Require(p.control==1&&!f.PlayerOn,"dismount restores player control without escort restart");f.PassengerBoarded(&p,0,true);f.UpdateAI(1);Require(f.starts==1&&p.control==2,"real board issues native mover handoff");p.vehicle=nullptr;f.PassengerBoarded(&p,0,true);f.UpdateAI(1);Require(p.control==2,"lost seat prevents stale mover handoff");f.Exit(&p);p.instanceId=2;p.q.Update(1000);Require(!p.payload,"extraction cannot overwrite foreign instance");
#endif
#else
f.Start(&p);p.instanceId=2;p.q.Update(50000);fixed=p.payload==0;
#ifndef COUNTERFACTUAL
p.instanceId=1;f.Start(&p);for(int i=0;i<4;++i)p.q.Update(50000);Require(p.payload>0,"original live instance retains all selected payloads");auto before=p.payload;f.Start(&p);p.world=false;p.q.Update(50000);Require(p.payload==before&&!bad,"world departure cancels pending payloads");
#endif
#endif
#ifdef COUNTERFACTUAL
Require(!fixed,"pinned control must reproduce selected defect");
#else
Require(fixed,"repaired selected contract");
#endif
std::cout<<"PASS selected timer/callback contract\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
