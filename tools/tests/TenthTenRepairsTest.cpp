#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using AuraEffectHandleModes=int;
#if defined(CASE_AVATAR)||defined(CASE_GULDAN)||defined(CASE_ELISANDE)||defined(CASE_AUGUR)||defined(CASE_LICHKING)||defined(CASE_KAATHAR)
#include "EventProcessor.inc"
struct Creature{EventProcessor m_Events;};
#include "Selected.inc"
struct State{bool held=true,finished=false;uint32 destroyed=0,premature=0,aborts=0,executes=0,effects=0;};
struct Held:BasicEvent{State&s;Held(State&x):s(x){}~Held(){++s.destroyed;s.premature+=s.held;}bool IsDeletable()const override{return!s.held;}bool Execute(uint64,uint32)override{++s.executes;return true;}void Abort(uint64)override{++s.aborts;if(!s.finished){s.finished=true;++s.effects;}}};
struct Once:BasicEvent{unsigned&calls;Once(unsigned&n):calls(n){}bool Execute(uint64,uint32)override{++calls;return true;}};
int main(){for(auto abort:selected){State state;Creature actor;actor.m_Events.AddEvent(new Held(state),1);abort(&actor);
#ifdef COUNTERFACTUAL
 assert(state.destroyed==1&&state.premature==1&&state.aborts==1&&!state.executes);
#else
 assert(!state.destroyed&&state.aborts==1&&!state.executes);actor.m_Events.Update(1);assert(!state.destroyed&&!state.executes);unsigned calls=0;actor.m_Events.AddEvent(new Once(calls),actor.m_Events.CalculateTime(1));actor.m_Events.Update(1);assert(calls==1&&!state.destroyed);state.held=false;actor.m_Events.Update(1);assert(state.destroyed==1&&!state.premature&&!state.executes&&state.effects==1);actor.m_Events.Update(1);assert(state.destroyed==1);
#endif
 }std::cout<<"PASS exact encounter abort statements on whole native EventProcessor underASAN;retained/current references,new scheduling,one abort/one destruction;full AI sequence separate\n";}
#else
struct Creature;struct Unit;struct Player;using ObjectGuid=uint64;
constexpr uint32 EFFECT_0=0,EFFECT_1=1,SPELL_AURA_PERIODIC_DUMMY=1,AURA_EFFECT_HANDLE_REAL=1,POWER_ENERGY=3,UNIT_FIELD_FLAGS=0,UNIT_FLAG_NON_ATTACKABLE=1,UNIT_FLAG_NOT_SELECTABLE=2,UNIT_FLAG_IMMUNE_TO_PC=4;
struct AuraEffect{};struct Hook{void operator+=(int){}};
#define PrepareAuraScript(x) public:
#define AuraEffectApplyFn(...) 0
#define AuraEffectRemoveFn(...) 0
#define AuraEffectPeriodicFn(...) 0
unsigned sentinelReads=0;struct Ref{Player*p=nullptr;Player*getSource(){return p;}};
struct PlayerList{std::vector<Ref>refs;Ref empty;struct Iter{PlayerList*list;size_t index;Ref*operator->(){if(index>=list->refs.size()){++sentinelReads;return &list->empty;}return &list->refs[index];}};bool isEmpty()const{return refs.empty();}Iter begin()const{return {const_cast<PlayerList*>(this),0};}};
struct Map{using PlayerList=::PlayerList;PlayerList players;PlayerList const&GetPlayers(){return players;}}map;
std::map<ObjectGuid,Creature*>world;
struct AI{uint32 actions=0;void DoAction(uint32){++actions;}};
struct Unit{bool creature=true,inWorld=true,dead=false;ObjectGuid id=1;uint32 powerType=0,maxPower=0,power=0,kills=0,casts=0,staleCasts=0,facing=0;std::vector<ObjectGuid>castTargets;AI ai;virtual~Unit()=default;Creature*ToCreature(){return creature?reinterpret_cast<Creature*>(this):nullptr;}uint32 GetPower(uint32 type){return type==powerType?power:0;}void SetPowerType(uint32 type){powerType=type;}void SetMaxPower(uint32 type,uint32 value){if(type==powerType)maxPower=value;}void SetPower(uint32 type,uint32 p){if(type==powerType)power=std::min(p,maxPower);}ObjectGuid GetGUID(){return id;}Map*GetMap(){return &map;}void Kill(Unit*t,bool){if(!t->dead){++kills;t->dead=true;castTargets.push_back(t->id);}}void CastSpell(Unit*t,uint32 id,...){if(id==117624){t=this;dead=true;}++casts;castTargets.push_back(t->id);staleCasts+=!t->inWorld;}void SetFacingToObject(Unit*){++facing;}void GetCreatureListWithEntryInGrid(std::list<Creature*>&out,uint32,float){for(auto const&p:world)out.push_back(p.second);}AI*GetAI(){return &ai;}void RemoveFlag(uint32,uint32){}};
struct Creature:Unit{::AI*AI(){return &ai;}};struct Player:Unit{Player(){creature=false;}};
namespace ObjectAccessor{Creature*GetCreature(Unit&,ObjectGuid id){auto i=world.find(id);return i==world.end()?nullptr:i->second;}}
struct AuraScript{Unit*caster=nullptr;Creature*me=nullptr;Hook AfterEffectApply,AfterEffectRemove,OnEffectPeriodic;virtual~AuraScript()=default;virtual void Register(){}Unit*GetCaster(){return caster;}};
#include "Constants.inc"
#include "Selected.inc"
void Check(bool v,char const*why){if(!v){std::cerr<<why<<'\n';std::abort();}}
int main(){Creature actor;Selected script;script.caster=&actor;script.me=&actor;
#if defined(CASE_FOLNUNA)
 SelectedSetup(&actor);uint32 expected=0;for(unsigned n=0;n<56;++n){expected+=script.energy[n];script.Tick(nullptr);}
#ifdef COUNTERFACTUAL
 Check(actor.power==0&&expected==100&&script.i==0,"original reset cannot access energy and curve never advances");
#else
 Check(actor.power==100&&script.i==0,"all56 authored increments sum100 andwrap");actor.power=0;script.Tick(nullptr);Check(actor.power==1&&script.i==1,"nextcycle starts original first increment");script.caster=nullptr;script.Tick(nullptr);Check(script.i==1,"missingcaster cannotconsume next increment");Player nonCreature;script.caster=&nonCreature;script.Tick(nullptr);Check(script.i==1,"nonCreature cannotconsume curve");
#endif
#elif defined(CASE_DREADSPRAY)
 auto target=new Creature;target->id=20;world[20]=target;script.OnApply(nullptr,0);world.erase(20);target->inWorld=false;
#ifdef COUNTERFACTUAL
 script.OnTick(nullptr);Check(actor.staleCasts==1,"original persisted raw target used after worldremoval");delete target;
#else
 delete target;script.OnTick(nullptr);Check(!actor.casts&&script.dreadspraytriggers.empty(),"deleted target GUID consumed withoutUAF/cast");Creature a,b;a.id=21;b.id=22;world={{21,&a},{22,&b}};script.OnApply(nullptr,0);script.OnTick(nullptr);script.OnTick(nullptr);Check(actor.castTargets==std::vector<ObjectGuid>({22,21}),"original LIFO actor order");script.OnTick(nullptr);Check(actor.casts==2,"empty sequence no duplicate cast");script.OnApply(nullptr,0);script.OnApply(nullptr,0);Check(script.dreadspraytriggers.size()==2,"newapply clears previous cache");script.caster=nullptr;script.OnRemove(nullptr,0);Check(script.dreadspraytriggers.empty(),"own cache cleared even missingcaster");
#endif
#elif defined(CASE_FORGE)
 script.OnTick(nullptr);
#ifdef COUNTERFACTUAL
 Check(sentinelReads==1,"old empty player iterator dereferenced");
#else
 Check(!sentinelReads&&actor.casts==1,"emptymap preserves only nativecosmetic");Player player;player.id=50;map.players.refs.push_back({&player});script.OnTick(nullptr);Check(actor.casts==3&&actor.castTargets.back()==50,"presentoriginal participant charged pluscosmetic");script.caster=nullptr;script.OnTick(nullptr);Check(actor.casts==3,"missingcaster no casts");
#endif
#elif defined(CASE_TECTUS)
 script.Finish();
#ifdef COUNTERFACTUAL
 Check(sentinelReads==1,"old delayed finisher dereferences end when lastplayerleft");Player player;map.players.refs.push_back({&player});script.Finish();Check(player.dead&&!actor.dead,"native117624 self-targets and kills original player instead of Tectus");
#else
 Check(!sentinelReads,"empty queued finisher safe");Player player;map.players.refs.push_back({&player});script.Finish();Check(player.kills==1&&player.castTargets.back()==actor.id&&!player.dead&&actor.dead,"existing firstparticipant kills only actual boss");script.Finish();Check(player.kills==1,"nativeKill exactlyonce refusal");map.players.refs.clear();script.Finish();Check(player.kills==1,"repeatemptycallback creates no progression/caster");
#endif
#endif
 std::cout<<"PASS whole selected aura callback/class or exact queued body;controlled grid/player endpoints;native/client separate\n";}
#endif
