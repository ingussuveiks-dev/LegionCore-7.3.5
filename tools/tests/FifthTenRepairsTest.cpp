// Complete production methods/event branches. Controlled world/cast endpoints.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using uint8=uint8_t;using int32=int32_t;using ObjectGuid=int;using GuidList=std::list<ObjectGuid>;
enum{EFFECT_0=0,EFFECT_1=1,EFFECT_2=2,EFFECT_3=3,BASE_ATTACK=0,SPELL_MISS_NONE=0,TYPEID_PLAYER=4,DIFFICULTY_HEROIC=2,TEMPSUMMON_CORPSE_TIMED_DESPAWN=1,SPELL_AURA_SCHOOL_ABSORB=69,SPELL_EFFECT_DUMMY=3,TARGET_UNIT_CASTER_AREA_RAID=56,DevotionAura=210320,SPELL_SHAMAN_T21_RESTORATION_2P=251764,SPELL_SHAMAN_T21_RAINFALL=252154,SoulFragment1=204255,SoulFragment2=203795,SoulFragment3=204062,TRIGGERED_FULL_MASK=1,TRIGGERED_CASTED_BY_AREATRIGGER=2,SPELL_ENQUBATION=240347,SPELL_HARDENED_SHELL=240315,SPELL_ENQUBATION_GREEN=242031,UNIT_FIELD_FLAGS=0,UNIT_FLAG_NOT_SELECTABLE=1,UNIT_FLAG_IMMUNE_TO_PC=2,UNIT_FLAG_NOT_ATTACKABLE_1=4,SPELL_INTRO_FIELD_OF_FIRE=252614,EVENT_HATCHING=4,EVENT_HATCHING_CLEAN_UP=5,EVENT_ZAO_ATTACK=12,SPELL_SHOOT_SUN=112084,EVENT_JUMP=1004};
using SpellEffIndex=int;using AuraEffectHandleModes=int;using TriggerCastFlags=int;
bool high=true;std::vector<uint32>rngBounds;
uint32 urand(uint32 low,uint32 top){assert(low<=top);rngBounds.push_back(top);return high?top:low;}int TestRand(){return 17;}float frand(float a,float){return a;}
struct Position{
 float m_positionX,m_positionY,m_positionZ,m_orientation;static float NormalizeOrientation(float o){return o;}
#include "PositionCtor.inc"
 float GetPositionX()const{return m_positionX;}float GetPositionY()const{return m_positionY;}float GetPositionZ()const{return m_positionZ;}float GetOrientation()const{return m_orientation;}
};
struct JumpArrivalCastArgs{};namespace Movement{struct SpellEffectExtraData{};}
struct Jump{float x,y,z,o,xy,vertical;uint32 id;bool facing;}jump;
struct Unit;struct Player;struct Creature;struct AreaTrigger;struct AuraEffect;struct Aura;struct Instance;
std::map<int,Unit*> registry;
struct Effect{float BasePoints=25,BonusCoefficientFromAP=2;float CalcValue(Unit* =nullptr)const{return BasePoints;}};
struct SpellInfo{Effect e[4];Effect*Effects[4]={&e[0],&e[1],&e[2],&e[3]};};
struct AuraEffect{float amount=10;float GetAmount()const{return amount;}void SetAmount(float f){amount=f;}};
struct Unit{
 virtual~Unit()=default;bool alive=true,player=false,friendly=true;int guid=0;uint64 hp=50,maxhp=100;float distance=1,ap=100;std::vector<uint32>casts;std::vector<int>castTargets;std::array<ObjectGuid,18> m_SummonSlot{};std::map<int,ObjectGuid>slots;std::list<AreaTrigger*> fragments;Instance*map=nullptr;
 virtual Player*ToPlayer(){return nullptr;}bool HasAura(uint32){return false;}bool IsPlayer(){return player;}bool IsAlive(){return alive;}uint32 GetTypeId(){return IsPlayer()?TYPEID_PLAYER:3;}uint64 GetHealth(){return hp;}uint64 GetMaxHealth(){return maxhp;}float GetHealthPct(){return 100.f*float(hp)/float(maxhp);}void SetHealth(uint64 n){hp=std::min(n,maxhp);}int GetGUID(){return guid;}float GetTotalAttackPowerValue(int){return ap;}void GetAreaObjectList(std::list<AreaTrigger*>&l,std::vector<uint32>const&){l=fragments;}float GetDistance(Unit*){return distance;}float GetDistance(AreaTrigger*);float GetPositionX(){return 1;}float GetPositionY(){return 2;}float GetPositionZ(){return 3;}
 void RemoveAurasDueToSpell(uint32,int){removed=true;}bool removed=false;float amount=0;AuraEffect effect;bool hasEffect=true;AuraEffect*GetAuraEffect(uint32,int,int=0){return hasEffect?&effect:nullptr;}
 void CastCustomSpell(Unit*t,uint32,float*v,void*,void*,int){t->effect.SetAmount(*v);t->hasEffect=true;}
 void CastSpell(Unit*t,uint32 id,bool=false,void* =nullptr,AuraEffect const* =nullptr){casts.push_back(id);castTargets.push_back(t->guid);}void CastSpellDelay(Unit*t,uint32 id,bool,uint32 delay){assert(delay==100);CastSpell(t,id);}
 Instance*GetMap(){return map;}Instance*GetInstanceScript(){return map;}Creature*SummonCreature(uint32,Position const&);static Unit*GetUnit(Unit&,int id){auto i=registry.find(id);return i==registry.end()?nullptr:i->second;}
};
using WorldObject=Unit;
struct Player:Unit{Player(){player=true;}Player*ToPlayer()override{return this;}};
struct ModelAI{void EnterCombat(Unit*){}};
struct Creature:Unit{ModelAI ai;unsigned despawns=0;std::vector<uint32>summoned;ModelAI*AI(){return &ai;}Unit*getVictim(){return this;}void Respawn(bool){alive=true;}void RemoveFlag(int,int){}void DespawnOrUnsummon(uint32){++despawns;}Creature*SummonCreature(uint32 id,float,float,float,float,int,int){summoned.push_back(id);return this;}};
using TempSummon=Creature;
struct Instance{Instance*instance=this;Creature*GetCreature(int id){auto i=registry.find(id);return i==registry.end()?nullptr:dynamic_cast<Creature*>(i->second);}};
// Avoid dereferencing fake context: actual registry lookup is keyed by GUID.
namespace ObjectAccessor{Unit*GetUnit(Unit&,int id){auto i=registry.find(id);return i==registry.end()?nullptr:i->second;}Unit*GetUnit(AreaTrigger&,int id){auto i=registry.find(id);return i==registry.end()?nullptr:i->second;}}
Creature*Unit::SummonCreature(uint32 id,Position const&p){assert(id==73400);volatile float x=p.GetPositionX();(void)x;static Creature c;c.guid=90;registry[90]=&c;return &c;}
struct AreaTrigger{SpellInfo info;SpellInfo*GetSpellInfo(){return &info;}unsigned actions=0;float distance=0;void CastAction(){++actions;}};float Unit::GetDistance(AreaTrigger*a){return a->distance;}
struct TargetInfo{int targetGUID;int missCondition=0,effectMask=1;};using TargetInfoPtr=std::shared_ptr<TargetInfo>;
struct Spell{std::vector<TargetInfoPtr>hits;std::vector<TargetInfoPtr>*GetUniqueTargetInfo(){return &hits;}};
struct Hook{template<class T>void operator+=(T){}};
struct SpellScript{Unit*caster=nullptr;Position*dest=nullptr;Spell spell;Unit*GetCaster(){return caster;}Position const*GetExplTargetDest(){return dest;}Spell*GetSpell(){return &spell;}Hook OnObjectAreaTargetSelect,AfterCast,OnCast;virtual~SpellScript()=default;virtual void Register(){}};
struct DamageInfo{uint32 GetDamage(){return 1;}};struct Aura{AuraEffect effect;AuraEffect*GetEffect(int){return &effect;}};
struct AuraScript{Unit*caster=nullptr;SpellInfo info;Aura aura;Unit*GetCaster(){return caster;}SpellInfo*GetSpellInfo(){return &info;}Aura*GetAura(){return &aura;}Hook DoEffectCalcAmount,OnEffectAbsorb;virtual~AuraScript()=default;virtual void Register(){}};
struct SpellScriptLoader{SpellScriptLoader(char const*){}virtual~SpellScriptLoader()=default;virtual SpellScript*GetSpellScript()const{return nullptr;}virtual AuraScript*GetAuraScript()const{return nullptr;}};
#define PrepareSpellScript(x) public:
#define PrepareAuraScript(x) public:
#define SpellCastFn(x) &x
#define SpellObjectAreaTargetSelectFn(x,...) &x
#define AuraEffectCalcAmountFn(x,...) &x
#define AuraEffectAbsorbFn(x,...) &x
void GetTier21HealingRainTargets(Unit*,std::list<Player*>&){}
struct spell_pal_at_aura_of_sacrifice{AreaTrigger*at;uint32 spellId=210372;int casterGUID=9;spell_pal_at_aura_of_sacrifice(AreaTrigger*a):at(a){}bool CheckTarget(Unit*,Unit*t){return t->friendly;}virtual void InitVariables(Unit*){}virtual void SetAuraMastery(bool){}virtual void HandleAuras(Unit*,GuidList*){}virtual void HandlePlayerCount(Unit*,GuidList*){}};
struct Events{std::vector<std::pair<uint32,uint32>>calls;void RescheduleEvent(uint32 id,uint32 delay){calls.emplace_back(id,delay);}};
uint32 const eggsCasts[3]={242030,242027,SPELL_ENQUBATION_GREEN};
std::map<uint32,uint32>npc_books={{0,121384},{1,121364},{2,121392}};
namespace Trinity{namespace Containers{template<class C>void RandomResizeList(C&c,size_t n){c.resize(n);}}}
struct Probe{Creature*me;Instance*instance;Events events;std::map<int,std::vector<int>>eggs;std::vector<int>wireframesGuids;GuidList introGuids,suns;std::unordered_map<int,bool>Stomach_Map;uint32 timerIntroEvent2=1;int GetDifficultyID(){return 23;}void DoCast(uint32 id){me->casts.push_back(id);}void DoCast(Unit*t,uint32 id,bool b){me->CastSpell(t,id,b);}void Run();Unit*SelectRandomNotStomach();};
#define rand TestRand
#include "Selected.inc"
#undef rand
void Check(bool v,char const*msg){if(!v){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){Creature creature,egg0,egg1;Instance instance;creature.guid=1;creature.map=&instance;egg0.guid=2;egg1.guid=3;registry={{1,&creature},{2,&egg0},{3,&egg1}};
#if defined(CASE_JUMP)
 MotionMaster m;m.MoveJump(4244.69f,-428.03f,259.35f,20,25);
#ifdef COUNTERFACTUAL
 Check(jump.y==0&&jump.z==0&&jump.xy==-428.03f&&jump.vertical==259.35f&&jump.id==20&&jump.facing,"original implicit Position corrupts coordinates,speeds,id,facing");
#else
 Check(jump.x==4244.69f&&jump.y==-428.03f&&jump.z==259.35f&&jump.xy==20&&jump.vertical==25&&jump.id==EVENT_JUMP&&!jump.facing,"legacy XYZ contract restored");m.MoveJump(Position(1,2,3,4),5,6,7,true);Check(jump.o==4&&jump.id==7&&jump.facing,"explicit Position/orientation contract retained");m.MoveJump(1,2,3,4,5,6,7,true);Check(jump.o==4&&jump.xy==5&&jump.vertical==6&&jump.id==7,"six-coordinate native contract retained");
#endif
#elif defined(CASE_HARJATAN)
 Probe p{&creature,&instance};for(int i=0;i<4;++i)p.eggs[i]={2,3};p.Run();Check(egg1.casts.size()==4&&egg0.casts.empty(),"highest valid egg chosen in both non-green clusters");p.eggs.clear();p.Run();Check(p.events.calls.back().second==28000,"empty clusters retain authored cleanup timer");
#elif defined(CASE_ANTORUS)
 Probe p{&creature,&instance};p.wireframesGuids={2,3};p.introGuids={1};p.Run();Check(egg1.casts.size()==1&&egg0.casts.empty(),"last protector participates");p.wireframesGuids.clear();p.timerIntroEvent2=1;p.Run();Check(p.timerIntroEvent2==7000,"empty protector list preserves timer");
#elif defined(CASE_BOOKS)
 Probe p{&creature,&instance};p.Run();
#ifdef COUNTERFACTUAL
 Check(creature.summoned==std::vector<uint32>{0,0},"original selects missing map key and fails value-as-key erase");
#else
 Check(creature.summoned==std::vector<uint32>{121392,121364},"two distinct valid entries at high bound");creature.summoned.clear();high=false;p.Run();Check(creature.summoned==std::vector<uint32>{121384,121364},"two distinct entries at low bound");Check(npc_books.size()==3,"source book pool unmodified");
#endif
#elif defined(CASE_ZAO)
 Probe p{&creature,&instance};p.suns={2,3};p.Run();
#ifdef COUNTERFACTUAL
 Check(creature.casts.empty()&&p.suns.size()==2,"original inclusive upper bound wastes attack on empty GUID");
#else
 Check(creature.castTargets==std::vector<int>{3},"last sun can be shot");p.suns={2};p.Run();Check(creature.castTargets.back()==2,"singleton always selected");registry.erase(2);p.Run();Check(p.suns.empty(),"missing actor removed,normal reschedule preserved");
#endif
#elif defined(CASE_CTHUN)
 Probe p{&creature,&instance};p.Stomach_Map={{2,false},{3,false},{1,true}};auto target=p.SelectRandomNotStomach();
#ifdef COUNTERFACTUAL
 std::vector<int>order;for(auto x:p.Stomach_Map)if(!x.second)order.push_back(x.first);Check(target==registry[order.front()]&&target!=registry[order.back()],"original always excludes last of two eligible candidates");Check(rngBounds.empty(),"original modulo excludes final candidate");
#else
 std::vector<int>order;for(auto x:p.Stomach_Map)if(!x.second)order.push_back(x.first);Check(target==registry[order.back()],"last outside-stomach candidate included");p.Stomach_Map={{2,true},{3,false}};Check(p.SelectRandomNotStomach()==&egg1,"singleton preserved");p.Stomach_Map={{2,true}};Check(!p.SelectRandomNotStomach(),"no outside target");
#endif
#elif defined(CASE_SPIRIT)
 Player a,b;Unit pet;a.guid=4;b.guid=5;pet.guid=6;registry[4]=&a;registry[5]=&b;registry[6]=&pet;a.hp=20;b.hp=80;spell_sha_spirit_link::spell_sha_spirit_link_SpellScript s;s.caster=&creature;s.spell.hits={std::make_shared<TargetInfo>(TargetInfo{4}),std::make_shared<TargetInfo>(TargetInfo{5}),std::make_shared<TargetInfo>(TargetInfo{99})};s.HandleAfterCast();
#ifdef COUNTERFACTUAL
 Check(a.hp==33&&b.hp==33,"original stale target dilutes health");
#else
 Check(a.hp==50&&b.hp==50,"only resolved living successful players define average");a.hp=20;b.hp=80;pet.hp=1;b.alive=false;s.spell.hits.push_back(std::make_shared<TargetInfo>(TargetInfo{6}));s.HandleAfterCast();Check(a.hp==20&&b.hp==80&&pet.hp==1,"no dead-player health or pet progress");b.alive=true;a.maxhp=uint64(1)<<35;b.maxhp=uint64(1)<<36;a.hp=a.maxhp/4;b.hp=b.maxhp*3/4;s.spell.hits.resize(2);s.HandleAfterCast();Check(a.hp==a.maxhp/2&&b.hp==b.maxhp/2,"native64bit health preserved");a.hp=20;a.maxhp=100;b.hp=80;b.maxhp=100;s.spell.hits[1]->missCondition=1;s.HandleAfterCast();Check(a.hp==20&&b.hp==80,"missed target excluded");s.spell.hits[0]->effectMask=0;s.HandleAfterCast();Check(a.hp==20,"zero eligible no division");
#endif
#elif defined(CASE_BARRIER)
 spell_dh_soul_barrier::spell_dh_soul_barrier_AuraScript s;bool recalc=true;float amount=17;s.CalculateAmount(nullptr,amount,recalc);Check(amount==17,"missing original caster retains native incoming amount");Player p;s.caster=&p;AreaTrigger near,far;near.distance=25;far.distance=26;p.fragments={&near,&far};s.CalculateAmount(nullptr,amount,recalc);Check(amount==400&&near.actions==1&&far.actions==0,"original AP and fragment radius/consumption retained");
#elif defined(CASE_RAIN)
 spell_sha_healing_rain_SpellScript s;s.caster=&creature;s.HandleOnCast();Check(registry.count(90)==0,"missing destination leaves existing summon untouched");Position dest(1,2,3,0);s.dest=&dest;s.HandleOnCast();Check(registry.count(90)==1,"valid destination keeps native summon");
#elif defined(CASE_DEVOTION)
 AreaTrigger at;spell_pal_at_devotion_aura s(&at);s.baseAmount=-20;GuidList ids={2,3,99};s.HandleAuras(&creature,&ids);
#ifdef COUNTERFACTUAL
 Check(ids.size()==3&&std::abs(egg0.effect.amount-(-20.f/3))<0.01,"original missing recipient dilutes aura");ids.clear();s.HandleAuras(&creature,&ids);Check(!std::isfinite(s.curAmount),"original empty division nonfinite");
#else
 Check(ids.size()==2&&egg0.effect.amount==-10,"resolved recipient divisor");s.lastPlayerCount=2;egg1.alive=false;s.HandlePlayerCount(&creature,&ids);Check(ids.size()==1&&egg0.effect.amount==-20&&egg1.removed,"death cleanup recalculates survivors");s.SetAuraMastery(true);egg1.alive=true;ids.push_back(3);s.HandleAuras(&creature,&ids);Check(egg0.effect.amount==-20&&egg1.effect.amount==-20,"mastery full amount preserved");egg1.friendly=false;s.HandleAuras(&creature,&ids);Check(ids.size()==1,"native friendly check retained");ids.clear();s.HandleAuras(&creature,&ids);Check(s.curAmount==0&&std::isfinite(s.curAmount),"empty set finite");
#endif
#endif
#ifdef COUNTERFACTUAL
 std::cout<<"COUNTERFACTUAL original defect demonstrated;not ordinary successful gameplay\n";
#else
 std::cout<<"PASS extracted complete changed methods/event branches under ASAN;world/cast endpoints modeled,not actual client traversal\n";
#endif
}
