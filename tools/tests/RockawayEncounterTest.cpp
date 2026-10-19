#include "EventMap.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <functional>
#include <optional>
#include "RockawayEvents.inc"

struct ObjectGuid { uint32 value=0; void Clear(){value=0;} bool IsEmpty()const{return !value;} bool operator<(ObjectGuid b)const{return value<b.value;} bool operator==(ObjectGuid b)const{return value==b.value;} };
using GuidSet=std::set<ObjectGuid>;
namespace WorldPackets{namespace Spells{
 struct ImmunityData{uint32 SchoolImmunities=0,Immunities=0;};
 struct SpellChannelStart{ObjectGuid CasterGUID;uint32 SpellID=0,SpellXSpellVisualID=0,ChannelDuration=0;std::optional<ImmunityData>InterruptImmunities;void*Write(){return nullptr;}};
 struct SpellChannelUpdate{ObjectGuid CasterGUID;uint32 TimeRemaining=0;void*Write(){return nullptr;}};
}}
struct Position {
 float x=0,y=0,z=0,o=0; float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}
};
struct Player;struct Creature;struct ScriptedAI;struct Quest;struct Group{bool raid=false;bool isRaidGroup()const{return raid;}};
struct Unit;
using WorldObject=Unit;
using GameObject=Unit;
struct SpellScript;
enum SpellEffIndex{EFFECT_0};
using ConditionList=std::vector<int>;
enum {TARGET_REFERENCE_TYPE_CASTER,TARGET_REFERENCE_TYPE_DEST};
enum {TARGET_CHECK_ENEMY,TARGET_CHECK_ALLY,TARGET_CHECK_PARTY,TARGET_CHECK_RAID,TARGET_CHECK_RAID_CLASS,TARGET_CHECK_SUMMON,TARGET_CHECK_ENTRY,TARGET_CHECK_DEFAULT};
enum {TARGET_OBJECT_TYPE_UNIT,TARGET_OBJECT_TYPE_GOBJ,TARGET_OBJECT_TYPE_DEST,TARGET_OBJECT_TYPE_OBJ_AND_DEST,TARGET_OBJECT_TYPE_CORPSE};
enum {SPELL_EFFECT_TELEPORT_L,SPELL_EFFECT_TELEPORT,TARGET_UNIT_NEARBY_ENTRY=38,SPELL_SCRIPT_HOOK_OBJECT_TARGET_SELECT};
enum{SPELL_ATTR1_IS_FISHING,UNIT_DYNAMIC_FIELD_CHANNEL_OBJECTS};
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_BAD_IMPLICIT_TARGETS};
struct SpellImplicitTargetInfo{int GetReferenceType()const{return TARGET_REFERENCE_TYPE_CASTER;}int GetCheckType()const{return TARGET_CHECK_ENTRY;}int GetObjectType()const{return TARGET_OBJECT_TYPE_UNIT;}};
struct SpellTargetPosition{float target_X=0,target_Y=0,target_Z=0,target_Orientation=0;uint32 target_mapId=0;};
struct SpellManager{SpellTargetPosition const*GetSpellTargetPosition(uint32){return nullptr;}}spellManager;
auto sSpellMgr=&spellManager;
struct NativeEffect{ConditionList*ImplicitTargetConditions=nullptr;};
struct CastTargets{Unit*unit=nullptr;Unit*GetUnitTarget(){return unit;}void SetUnitTarget(Unit*u){unit=u;}void SetDst(Unit const&){}void SetDst(float,float,float,float,int32=0){}};
enum CurrentSpellTypes{CURRENT_GENERIC_SPELL,CURRENT_CHANNELED_SPELL,CURRENT_AUTOREPEAT_SPELL,CURRENT_MAX_SPELL};
enum {SPELL_ATTR4_TRIGGERED=4,SPELL_INTERRUPT_FLAG_MOVEMENT=1,CHANNEL_INTERRUPT_FLAG_MOVE=1,UNIT_STATE_MOVE_IN_CASTING=2,SPELL_AURA_FIXATE=1};
struct ChannelInfo {
 uint32 Id=0,InterruptFlags=0;bool HasAttribute(int)const{return false;}
 NativeEffect effect;NativeEffect*Effects[1]{&effect};struct{bool RequiresSpellFocus=false;}CastingReq;
 float GetMaxRange(bool,Unit*,void*)const{return 100;}bool IsPositive()const{return true;}bool HasEffect(int)const{return false;}
 bool CanInterruptChannel(ChannelInfo*)const{return true;}uint32 CalcCastTime(Unit*)const{return 0;}
 bool HasChannelInterruptFlag(int)const{return false;}bool HasAura(int)const{return false;}
};
struct Spell {
 ChannelInfo info;ChannelInfo*m_spellInfo=&info;Unit*caster=nullptr;Spell**m_selfContainer=nullptr;
 CurrentSpellTypes GetCurrentContainer()const{return CURRENT_CHANNELED_SPELL;}
 Unit*GetCaster()const{return caster;}void SetReferencedFromCurrent(bool){}
 Unit*m_caster=nullptr,*focusObject=nullptr,*selected=nullptr,*searchResult=nullptr;CastTargets m_targets;
 bool finished=false;uint32 searches=0,hookCalls=0;std::vector<SpellScript*>m_loadedScripts;
 struct TargetInfo{ObjectGuid targetGUID;};struct GOTargetInfo{ObjectGuid targetGUID;};
 std::vector<std::unique_ptr<TargetInfo>>m_UniqueTargetInfo;std::vector<GOTargetInfo>m_UniqueGOTargetInfo;
 uint32 m_timer=0,m_SpellVisual=0;
 void SendChannelStart(uint32);void SendChannelUpdate(uint32);
 void SelectImplicitNearbyTargets(SpellEffIndex,SpellImplicitTargetInfo const&,uint32);
 void CallScriptObjectTargetSelectHandlers(WorldObject*&,SpellEffIndex);
 WorldObject*SearchNearbyTarget(float,int,int,ConditionList*){++searches;return searchResult;}
 void AddUnitTarget(Unit*u,uint32,bool=false,bool=false){selected=u;}void AddGOTarget(Unit*,uint32){}
 void SendCastResult(int){}void finish(bool){finished=true;}void SelectImplicitChainTargets(SpellEffIndex,SpellImplicitTargetInfo const&,WorldObject*,uint32){}
};
struct Map {uint32 GetDifficultyID()const{return 0;}} worldMap;
using DamageEffectType=int;
constexpr int REACT_PASSIVE=0,UNIT_STATE_CASTING=1,TEMPSUMMON_MANUAL_DESPAWN=0,QUEST_STATUS_INCOMPLETE=1;
constexpr int LootTemplates_Spell=12;
uint32 clockMs=0,nextGuid=1,failSummons=0;
std::set<uint32> failedSpells;
struct Unit:Position {
 ObjectGuid guid{nextGuid++}; uint32 entry=0,map=1220,phase=1,hp=100,healthRelative=100;bool alive=true,inWorld=true,visible=true,IsAIEnabled=true;
 Unit* owner=nullptr;std::set<uint32> auras;std::set<std::pair<uint32,uint32>> casterAuras;std::vector<uint32> casts;std::vector<ObjectGuid>channelObjects;
 Spell channel;Spell*m_currentSpells[CURRENT_MAX_SPELL]{};bool m_AutoRepeatFirstCast=false;uint32 states=0;
 void SetCurrentCastedSpell(Spell*);
 void AddUnitState(uint32 s){states|=s;}
 void InterruptSpell(CurrentSpellTypes type,bool=true,bool=true,uint32=0){if(auto s=m_currentSpells[type]){auras.erase(s->info.Id);m_currentSpells[type]=nullptr;if(s->m_caster)s->SendChannelUpdate(0);}states=0;}
 virtual ~Unit()=default;void Dispatch(Creature*creature);
 ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return entry;}uint32 GetMapId()const{return map;}
 Map*GetMap(){return &worldMap;}
 bool IsAlive()const{return alive;}bool IsCreature()const;Creature* ToCreature();
 Unit*ToUnit(){return this;}Unit*ToGameObject(){return this;}uint32 GetGUIDLow(){return guid.value;}bool isDead(){return !alive;}
 Player* GetCharmerOrOwnerPlayerOrPlayerItself();
 bool InSamePhase(Unit*p)const{return map==p->map&&phase==p->phase;}
 bool IsWithinDistInMap(Unit*p,float d,bool=true,bool=false)const{return InSamePhase(p)&&std::hypot(x-p->x,y-p->y)<=d;}
 uint32 GetHealth(Unit*)const{return healthRelative;}
 void SetFullHealth(){hp=healthRelative=100;alive=true;}
 void SetVisible(bool v){visible=v;}void setActive(bool){}void SetReactState(int){}void SetDisableGravity(bool){}
 void SetFacingToObject(Unit*){}void CombatStop(bool){}void CastStop(){for(uint32 i=0;i<CURRENT_MAX_SPELL;++i)InterruptSpell(CurrentSpellTypes(i));}
 bool HasUnitState(int state)const{return states&state;}bool HasAura(uint32 s)const{return auras.count(s);}
 bool HasAura(uint32 s,ObjectGuid c)const{return casterAuras.count({s,c.value});}
 uint32 channelSpellId=0,channelVisual=0,channelPackets=0;
 uint32 GetChannelSpellId()const{return channelSpellId;}
 void SetChannelSpellId(uint32 s){channelSpellId=s;}void SetChannelSpellXSpellVisualId(uint32 s){channelVisual=s;}
 void AddChannelObject(ObjectGuid g){channelObjects.push_back(g);}void ClearDynamicValue(int){channelObjects.clear();}
 uint32 GetSchoolImmunityMask(){return 0;}uint32 GetMechanicImmunityMask(){return 0;}void SendMessageToSet(void*,bool){++channelPackets;}
 std::vector<ObjectGuid>const&GetChannelObjects()const{return channelObjects;}
 void AddAura(uint32 id,Unit*target){if(!failedSpells.count(id))target->auras.insert(id);}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid c){if(!c.value){for(auto i=casterAuras.begin();i!=casterAuras.end();)if(i->first==id)i=casterAuras.erase(i);else ++i;}else casterAuras.erase({id,c.value});auras.erase(id);}
 void CastSpell(Unit*target,uint32 id,bool);void CastSpell(float x,float y,float z,uint32 id,bool);
 Position GetNearPosition(float d,float a){return {x+d*std::cos(a),y+d*std::sin(a),z,0};}
};
std::map<ObjectGuid,std::unique_ptr<Unit>> units;
struct Player:Unit {
 uint32 status=1,credits=0,fragments=0,errors=0;bool lostArmy=true,fullBags=false;Group*group=nullptr;
 uint32 GetQuestStatus(uint32)const{return status;}Group*GetGroup()const{return group;}
 bool CanRewardWorldQuest(Quest const*)const{return true;} // Actual guard runs in RockawayCompletionTest.
 bool HasQuestForItem(uint32)const{return lostArmy&&fragments<1000;}
 void AutoStoreLoot(uint32 id,int){assert(id==229113);if(fullBags)++errors;else fragments+=15;}
 void KilledMonsterCredit(uint32 id){assert(id==115168);++credits;status=2;}
};
struct TempSummon{Unit*owner;Unit*GetSummoner(){return owner;}ObjectGuid GetSummonerGUID(){return owner->GetGUID();}};
struct Creature:Unit {
 TempSummon temp{};TempSummon*ToTempSummon(){temp.owner=owner;return owner?&temp:nullptr;}void KillAllDelayedCombats(){}
 ScriptedAI*ai=nullptr;Unit*victim=nullptr;uint32 despawnTime=0,melees=0;std::vector<uint32>talks;std::vector<std::pair<uint32,uint32>>talkTimes;
 ScriptedAI*AI(){return ai;}Creature*SummonCreature(uint32,Position const&,int);
 void GetPlayerListInGrid(std::list<Player*>&list,float d){for(auto&v:units)if(auto p=dynamic_cast<Player*>(v.second.get()))if(p->inWorld&&IsWithinDistInMap(p,d))list.push_back(p);}
 void DespawnOrUnsummon(uint32 delay=0);
};
struct ScriptedAI {
 Creature*me;explicit ScriptedAI(Creature*c):me(c){c->ai=this;}virtual~ScriptedAI()=default;
 virtual void AttackStart(Unit*u){me->victim=u;}virtual void MoveInLineOfSight(Unit*){}virtual uint32 GetData(uint32)const{return 0;}
 virtual void Reset(){}virtual void SetGUID(ObjectGuid const&,int32){}virtual void UpdateAI(uint32){}
 virtual void JustDied(Unit*){}virtual void JustSummoned(Creature*){}virtual void SummonedCreatureDies(Creature*,Unit*){}virtual void SummonedCreatureDespawn(Creature*){}
 virtual void IsSummonedBy(Unit*){}virtual void DamageTaken(Unit*,uint32&,int){}virtual void DamageTakenAfterScaling(Unit*,uint32&,int){}
 void Talk(uint32 group){me->talks.push_back(group);me->talkTimes.emplace_back(group,clockMs);}bool UpdateVictim(){return me->victim&&me->victim->IsAlive();}void DoMeleeAttackIfReady(){++me->melees;}
};
struct SummonList:std::set<ObjectGuid> {
 Creature*me;explicit SummonList(Creature*c):me(c){}void Summon(Creature*c){insert(c->guid);}void Despawn(Creature*c){erase(c->guid);}
 void DespawnAll(){auto copy=*this;clear();for(auto g:copy){auto it=units.find(g);if(it!=units.end())static_cast<Creature*>(it->second.get())->DespawnOrUnsummon();}}
};
namespace ObjectAccessor {
 Creature*GetCreature(Unit&,ObjectGuid g){auto i=units.find(g);return i!=units.end()&&i->second->inWorld?dynamic_cast<Creature*>(i->second.get()):nullptr;}
 Player*GetPlayer(Unit&,ObjectGuid g){auto i=units.find(g);return i!=units.end()&&i->second->inWorld?dynamic_cast<Player*>(i->second.get()):nullptr;}
}
struct Quest{bool IsAllowedInRaid(uint32)const{return false;}};struct Rotation{uint32 ResetTime=10000;}rotation;
struct QuestStore {bool active=true;Quest quest;Quest*GetQuestTemplate(uint32){return &quest;}Rotation*GetWorldQuest(Quest*){return active?&rotation:nullptr;}}store;
auto sQuestDataStore=&store;
namespace GameTime{uint32 GetGameTime(){return clockMs/1000;}}
#define ASSERT assert
#define TC_LOG_DEBUG(...)
#include "RockawayCurrentSpell.inc"
#include "RockawayChannelFields.inc"
bool Unit::IsCreature()const{return dynamic_cast<Creature const*>(this)!=nullptr;}Creature*Unit::ToCreature(){return dynamic_cast<Creature*>(this);}
Player*Unit::GetCharmerOrOwnerPlayerOrPlayerItself(){if(auto p=dynamic_cast<Player*>(this))return p;return owner?owner->GetCharmerOrOwnerPlayerOrPlayerItself():nullptr;}

#ifdef ROCKAWAY_BASELINE
#include "RockawayOldActors.inc"
#elif defined(ROCKAWAY_TIMING_BASELINE)
#include "RockawayTimingActors.inc"
#else
#include "RockawayActors.inc"
#endif

#ifndef ROCKAWAY_BASELINE
struct TargetHook{
 std::function<void(WorldObject*&)>fn;SpellEffIndex effect;
 bool IsEffectAffected(ChannelInfo*,SpellEffIndex e){return effect==e;}
 void Call(SpellScript*,WorldObject*&t){fn(t);}
};
template<class T>struct Hooks:std::vector<T>{void operator+=(T t){this->push_back(t);}};
struct SpellScript{
 Unit*caster=nullptr;int depth=0;Hooks<std::function<SpellCastResult()>>OnCheckCast;Hooks<TargetHook>OnObjectTargetSelect;
 virtual bool Load(){return true;}virtual void Register(){}Unit*GetCaster(){return caster;}
 void _PrepareScriptCall(int){++depth;}void _FinishScriptCall(){--depth;}
};
#define PrepareSpellScript(name) public:
#define SpellCheckCastFn(fn) [this](){return fn();}
#define SpellObjectTargetSelectFn(fn,eff,target) TargetHook{[this](WorldObject*&t){fn(t);},eff}
#include "RockawayChannel.inc"
#include "RockawayTargetHooks.inc"
#include "RockawayNearby.inc"
Unit*SelectChannel(Unit*caster,Unit*explicitTarget,Unit*nearest,bool&failed){
 spell_rockaway_channel script;script.caster=caster;Spell cast;cast.m_caster=caster;cast.info.Id=231433;cast.m_targets.unit=explicitTarget;cast.searchResult=nearest;
 if(script.Load()){script.Register();cast.m_loadedScripts.push_back(&script);for(auto const&check:script.OnCheckCast)if(check()!=SPELL_CAST_OK){failed=true;return nullptr;}}
 cast.SelectImplicitNearbyTargets(EFFECT_0,SpellImplicitTargetInfo{},1);assert(script.depth==0);failed=cast.finished;return cast.selected;
}
#endif

ScriptedAI*MakeAI(Creature*c){switch(c->entry){case 115133:return new npc_rockaway_vision(c);case 115132:return new npc_rockaway_ritualist(c);case 116187:return new npc_rockaway_tentacle(c);default:return new npc_rockaway_slime(c);}}
Creature*Spawn(uint32 entry,Unit*owner,Position pos={}){auto c=std::make_unique<Creature>();c->entry=entry;c->owner=owner;static_cast<Position&>(*c)=pos;auto ptr=c.get();units.emplace(c->guid,std::move(c));MakeAI(ptr);if(owner){ptr->AI()->IsSummonedBy(owner);owner->ToCreature()->AI()->JustSummoned(ptr);}return ptr;}
Creature*Creature::SummonCreature(uint32 e,Position const&p,int){if(failSummons){--failSummons;return nullptr;}return Spawn(e,this,p);}
void Creature::DespawnOrUnsummon(uint32 delay){if(delay){despawnTime=clockMs+delay;return;}if(!inWorld)return;inWorld=false;if(owner&&owner->IsCreature())owner->ToCreature()->AI()->SummonedCreatureDespawn(this);}
void Unit::CastSpell(Unit*target,uint32 id,bool){casts.push_back(id);if(failedSpells.count(id))return;
#ifndef ROCKAWAY_BASELINE
 if(id==231433){bool failed=false;target=SelectChannel(this,target,target,failed);if(failed||!target)return;target->casterAuras.insert({id,guid.value});}
#endif
 if(id==231328||id==231333||id==231433){channel.info.Id=id;channel.caster=channel.m_caster=this;SetCurrentCastedSpell(&channel);channel.m_UniqueTargetInfo.clear();channel.m_UniqueTargetInfo.push_back(std::make_unique<Spell::TargetInfo>(Spell::TargetInfo{target->GetGUID()}));channel.SendChannelStart(uint32(-1));if(id!=231433)auras.insert(id);}if(id==231329)target->auras.insert(id);if(id==231296)Spawn(116187,this,*target);}
void Unit::CastSpell(float px,float py,float pz,uint32 id,bool){casts.push_back(id);if(!failedSpells.count(id)&&id==231354)Spawn(116195,this,{px,py,pz,0});}
void Unit::Dispatch(Creature*creature){
#include "RockawayDeath.inc"
}
void Tick(uint32 ms){for(uint32 t=0;t<ms;t+=100){auto dt=std::min(100u,ms-t);clockMs+=dt;std::vector<Creature*>copy;for(auto&v:units)if(auto c=dynamic_cast<Creature*>(v.second.get()))if(c->inWorld)copy.push_back(c);for(auto c:copy)if(c->inWorld&&c->alive)c->AI()->UpdateAI(dt);for(auto c:copy)if(c->inWorld&&c->despawnTime&&clockMs>=c->despawnTime)c->DespawnOrUnsummon();}}
Player*NewPlayer(Position p){auto v=std::make_unique<Player>();static_cast<Position&>(*v)=p;auto result=v.get();units.emplace(v->guid,std::move(v));return result;}
struct Run {
 Creature*vision; npc_rockaway_vision*ai;Player*player;Unit pet;
 Run(){units.clear();clockMs=0;store.active=true;rotation.ResetTime=10000;failSummons=0;failedSpells.clear();vision=Spawn(115133,nullptr,{6229.235f,4951.045f,0,0});ai=static_cast<npc_rockaway_vision*>(vision->AI());ai->Reset();player=NewPlayer(*vision);pet.owner=player;Tick(100);assert(ai->ready);}
 void Hit(uint32 slot,Unit*killer=nullptr,bool kill=false){auto c=ObjectAccessor::GetCreature(*vision,ai->slots[slot]);assert(c&&c->alive);uint32 damage=kill?100:1;if(c->IsAIEnabled)c->AI()->DamageTaken(killer?killer:player,damage,0);if(kill&&damage){c->alive=false;(killer?killer:player)->Dispatch(c);}}
};

struct AuraEffect{};using AuraEffectHandleModes=int;
constexpr int AURA_REMOVE_BY_EXPIRE=1,AURA_REMOVE_BY_ENEMY_SPELL=2;
struct App{int mode=1;int GetRemoveMode(){return mode;}};
struct TaintContext{
 Unit*caster;Unit*target;App app;Unit*GetCaster(){return caster;}Unit*GetTarget(){return target;}App*GetTargetApplication(){return &app;}
#include "RockawayTaint.inc"
};
#ifdef ROCKAWAY_BASELINE
int main(){
 Run r;assert(r.vision->HasUnitState(UNIT_STATE_CASTING));r.Hit(0);Tick(25000);
 for(uint32 id:{231296u,231351u,231329u})assert(std::count(r.vision->casts.begin(),r.vision->casts.end(),id)==0);
 std::cout<<"PASS: pinned old AI reproduces the infinite decorative-channel attack lock through native SetCurrentCastedSpell.\n";
}
#elif defined(ROCKAWAY_TIMING_BASELINE)
int main(){
 Run r;for(uint32 i=0;i<4;++i)r.Hit(i,&r.pet,true);auto finalAt=clockMs;
 assert(r.ai->finishTime==6000);Tick(5999);assert(!r.player->credits);Tick(1);
 assert(r.player->credits==1&&clockMs-finalAt==6000);
 std::cout<<"PASS: pinned516fc1f1 native finale dispatches credit at6000ms, before77496's decoded6771.474ms clip ends.\n";
}
#else
int main(){
 // Execute the complete native target38 resolver and object-target hook
 // dispatcher, with grid search/target insertion as explicit service fixtures.
 {Run r;auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);
  assert(c->casts.empty()); // Summon callback precedes slot admission.
  Tick(100);assert(r.vision->casterAuras.size()==4);
  for(auto g:r.ai->slots){auto a=ObjectAccessor::GetCreature(*r.vision,g);assert(a->GetChannelSpellId()==231433&&a->channelObjects.size()==1&&a->channelObjects[0]==r.vision->guid&&r.vision->HasAura(231433,g));}
  auto n=c->casts.size();Tick(4000);assert(c->casts.size()==n);
  auto other=Spawn(115133,nullptr,*c);bool failed=false;
  assert(SelectChannel(c,other,r.player,failed)==r.vision&&!failed); // Closer bystander and wrong explicit image.
  assert(SelectChannel(c,r.vision,other,failed)==r.vision&&!failed); // Another image wins grid search.
  assert(!SelectChannel(c,r.vision,nullptr,failed)&&failed); // Native search failure precedes the hook.
  auto unscoped=Spawn(116353,nullptr,*c);assert(SelectChannel(unscoped,r.vision,r.player,failed)==r.player&&!failed);
  auto unslotted=Spawn(115132,r.vision,*c);assert(!SelectChannel(unslotted,r.vision,r.vision,failed)&&failed);
  auto reject=[&](){bool f=false;assert(!SelectChannel(c,r.vision,r.player,f)&&f);};
  r.ai->ready=false;reject();r.ai->ready=true;r.ai->outro=true;reject();r.ai->outro=false;
  r.ai->cooldown=1;reject();r.ai->cooldown=0;r.ai->resetting=true;reject();r.ai->resetting=false;
  c->phase=2;reject();c->phase=1;c->x+=200;reject();c->x-=200;
  r.vision->alive=false;reject();r.vision->alive=true;c->alive=false;reject();c->alive=true;
  store.active=false;reject();store.active=true;
  // Revalidate inside the actual target hook if ownership changes after Check.
  spell_rockaway_channel script;script.caster=c;script.Register();assert(script.Check()==SPELL_CAST_OK);
  r.ai->dead[0]=true;Spell race;race.m_caster=c;race.m_targets.unit=r.vision;race.searchResult=r.player;race.m_loadedScripts={&script};
  race.SelectImplicitNearbyTargets(EFFECT_0,SpellImplicitTargetInfo{},1);assert(race.finished&&!race.selected&&script.depth==0);r.ai->dead[0]=false;
 }
 {Run r;Tick(100);auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);auto n=c->casts.size();
  r.vision->RemoveAurasDueToSpell(231433,c->guid);Tick(1000);assert(c->casts.size()==n+1&&r.vision->HasAura(231433,c->guid));
  c->channelObjects={r.player->guid};Tick(1000);assert(c->channelObjects[0]==r.vision->guid);
  c->CastStop();failedSpells.insert(231433);Tick(1000);assert(!c->GetChannelSpellId());failedSpells.erase(231433);Tick(1000);assert(c->GetChannelSpellId()==231433);
  r.Hit(0,&r.pet,true);assert(r.vision->casterAuras.size()==3&&!c->GetChannelSpellId()&&c->channelObjects.empty());
  for(uint32 i=1;i<4;++i)assert(r.vision->HasAura(231433,r.ai->slots[i]));
  ObjectGuid unrelated{99999};r.vision->casterAuras.insert({231433,unrelated.value});r.ai->ClearRun();assert(r.vision->casterAuras.size()==1&&r.vision->HasAura(231433,unrelated));
  r.ai->ClearRun();assert(r.vision->HasAura(231433,unrelated)); // Empty slots must not remove every caster's aura.
 }
 {Run r;Tick(100);auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);c->IsAIEnabled=false;r.Hit(0,&r.pet,true);assert(!r.vision->HasAura(231433,c->guid)&&!c->GetChannelSpellId());}
 {Run r;Tick(100);auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);r.ai->outro=true;Tick(1000);assert(!c->GetChannelSpellId()&&!r.vision->HasAura(231433,c->guid));}
 {Run r;Tick(100);assert(!r.vision->HasAura(231328)&&!r.vision->HasUnitState(UNIT_STATE_CASTING));
  auto ritualist=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);assert(r.vision->HasAura(231433,ritualist->guid)&&ritualist->HasUnitState(UNIT_STATE_CASTING));
  r.Hit(0,&r.pet,true);assert(!r.vision->HasAura(231433,ritualist->guid)&&!ritualist->HasUnitState(UNIT_STATE_CASTING));
  Tick(16000);for(uint32 id:{231296u,231351u,231329u})assert(std::count(r.vision->casts.begin(),r.vision->casts.end(),id)>0);
  r.ai->ClearRun();assert(!r.vision->HasAura(231328));}
 {Run r;r.vision->CastSpell(r.vision,231328,true);r.Hit(0);Tick(11000);
  assert(std::count(r.vision->casts.begin(),r.vision->casts.end(),231296)==0);
  r.vision->CastStop();r.vision->AddAura(231328,r.vision);Tick(200);
  assert(std::count(r.vision->casts.begin(),r.vision->casts.end(),231296)==1&&std::count(r.vision->casts.begin(),r.vision->casts.end(),231351)==1);}
 // Every death order, including a pet's first-hit killing blow; no premature or duplicate credit.
 for(uint32 combatMs:{0u,10000u}){Run r;r.Hit(0);auto ritual=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[0]);
  assert(r.vision->talkTimes.size()==1&&r.vision->talkTimes[0].first==1&&ritual->talkTimes.size()==1&&ritual->talkTimes[0].first==1);
  r.Hit(0);assert(r.vision->talkTimes.size()==1&&ritual->talkTimes.size()==1);Tick(combatMs);
  for(uint32 i=0;i<4;++i)r.Hit(i,&r.pet,true);
  assert(r.vision->talkTimes.size()==2&&r.vision->talkTimes.back().first==4&&ritual->talkTimes.size()==2&&ritual->talkTimes.back().first==2);
  auto finalAt=clockMs;Tick(6999);assert(!r.player->credits);Tick(1);assert(r.player->credits==1&&clockMs-finalAt==7000&&!r.vision->visible);
  std::cout<<"TIMING combat_ms="<<combatMs<<" opening_to_final_ms="<<r.vision->talkTimes[1].second-r.vision->talkTimes[0].second
   <<" ritual_aggro_to_death_ms="<<ritual->talkTimes[1].second-ritual->talkTimes[0].second<<" final_to_credit_ms="<<clockMs-finalAt<<'\n';
 }
 std::array<uint32,4>order{0,1,2,3};uint32 orders=0;
 do{Run r;Tick(100);assert(r.vision->casterAuras.size()==4);for(uint32 j=0;j<4;++j){r.Hit(order[j],&r.pet,true);assert(r.player->credits==0);if(j<3)assert(!r.ai->outro);}assert(r.ai->outro);Tick(6900);assert(!r.player->credits);Tick(100);assert(r.player->credits==1&&r.player->fragments==15&&!r.vision->visible);r.ai->Finish();Tick(1000);assert(r.player->credits==1);++orders;}while(std::next_permutation(order.begin(),order.end()));assert(orders==24);
 {Run r;uint32 damage=1000000;r.vision->healthRelative=900;r.ai->DamageTakenAfterScaling(&r.pet,damage,0);assert(damage==899&&r.ai->started&&!r.ai->outro);Tick(6000);assert(!r.player->credits);}
 {Run r;store.active=false;Tick(100);assert(!r.ai->enabled&&!r.vision->visible);for(auto g:r.ai->slots)assert(!g.value);store.active=true;Tick(100);assert(r.ai->ready&&r.vision->visible);}
 {Run r;rotation.ResetTime=1;Tick(1000);assert(!r.ai->enabled&&!r.vision->visible);}
 {Run r;Group group;r.player->group=&group;auto ally=NewPlayer(*r.vision);ally->group=&group;auto observer=NewPlayer(*r.vision);auto wrong=NewPlayer(*r.vision);wrong->phase=2;wrong->group=&group;auto done=NewPlayer(*r.vision);done->status=2;done->group=&group;auto dead=NewPlayer(*r.vision);dead->alive=false;dead->group=&group;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(ally->credits==1&&observer->credits==0&&wrong->credits==0&&done->credits==0&&dead->credits==0);}
 {Run r;r.player->lostArmy=false;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(r.player->credits==1&&!r.player->fragments);}
 {Run r;r.player->fragments=1000;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(r.player->credits==1&&r.player->fragments==1000);}
 {Run r;r.player->fragments=999;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(r.player->fragments==1014);}
 {Run r;r.player->fullBags=true;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(r.player->credits==1&&!r.player->fragments&&r.player->errors==1);}
 {Run r;r.Hit(0,nullptr,true);auto old=r.ai->slots[1];ObjectAccessor::GetCreature(*r.vision,old)->DespawnOrUnsummon();assert(!r.ai->started&&r.ai->cooldown);auto stale=Spawn(116195,r.vision,*r.player);assert(!stale->inWorld);Tick(10100);assert(r.ai->ready&&!r.ai->dead[0]&&r.ai->slots[1].value!=old.value);}
 {Run r;r.ai->ClearRun();failSummons=1;r.ai->EnsureRitualists();assert(!r.ai->ready);uint32 d=100;auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[1]);c->AI()->DamageTaken(r.player,d,0);assert(d==0);r.ai->EnsureRitualists();assert(r.ai->ready);}
 {Run r;r.Hit(0);r.player->x+=200;Tick(10100);assert(!r.ai->started&&!r.vision->visible);Tick(10000);assert(r.ai->ready);}
 {Run r;for(uint32 id:{231296u,231351u,231354u,231329u})failedSpells.insert(id);r.Hit(0);Tick(25000);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(r.player->credits==1);}
 {Run r;r.Hit(0);Tick(11000);uint32 slimes=0,slams=0;for(auto&v:units)if(auto c=dynamic_cast<Creature*>(v.second.get())){if(c->entry==116195){++slimes;assert(c->victim==r.player);}if(c->entry==116187){slams+=std::count(c->casts.begin(),c->casts.end(),231298);uint32 damage=999;c->AI()->DamageTakenAfterScaling(r.player,damage,0);assert(damage==0);}}assert(slimes==5&&slams==1);}
 {Run r;for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);store.active=false;Tick(7000);assert(!r.player->credits&&!r.player->fragments);}
 {Run r;for(uint32 i=0;i<4;++i){r.Hit(i,nullptr,true);auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[i]);r.ai->SummonedCreatureDies(c,nullptr);}Tick(7000);assert(r.player->credits==1);Tick(30100);assert(r.ai->ready&&!r.ai->started&&!r.ai->dead[0]);}
 {Run r;r.Hit(0);TaintContext a{r.vision,r.player,{1}};a.OnRemove(nullptr,0);assert(r.vision->casts.back()==232681);auto n=r.vision->casts.size();a.app.mode=0;a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n);a.app.mode=2;a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n+1);r.ai->ClearRun();a.OnRemove(nullptr,0);assert(r.vision->casts.size()==n+1);a.caster=nullptr;a.OnRemove(nullptr,0);}
 {Run r;for(uint32 i=0;i<4;++i){auto c=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[i]);c->IsAIEnabled=false;r.Hit(i,&r.pet,true);}Tick(7000);assert(r.player->credits==1);}
 {Run r;auto rogue=Spawn(115132,r.vision,*r.player);r.ai->SummonedCreatureDies(rogue,&r.pet);assert(!r.ai->dead[0]&&!r.ai->outro);r.Hit(0);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);r.player->status=0;Tick(7000);assert(!r.player->credits&&!r.player->fragments);}
 {Run r;auto corpse=ObjectAccessor::GetCreature(*r.vision,r.ai->slots[3]);for(uint32 i=0;i<4;++i)r.Hit(i,nullptr,true);Tick(7000);assert(corpse->inWorld&&!corpse->alive);Tick(60100);assert(!corpse->inWorld&&r.ai->ready&&!r.ai->outro);}
 {Run r;Group raid;raid.raid=true;r.player->group=&raid;for(uint32 i=0;i<4;++i)r.Hit(i,&r.pet,true);Tick(7000);assert(!r.player->credits&&!r.player->fragments);}
 std::cout<<"PASS: production AI/hooks; full native target38 resolver, object-hook dispatcher, SendChannelStart/Update, EventMap and spell-container handler. Exact owner vs bystander/other image, scope, readiness/race failures, retry, four per-caster beams, death/reset/empty-slot cleanup,24 death orders and continuing attacks. Grid search, Spell preparation, aura application and packet serialization are fixtures; client playback remains pending.\n";
}
#endif
