#pragma once
// External endpoints for complete production callbacks, deliberately not a core emulator.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int32=int32_t;using int8=int8_t;
#include "Labels.inc"
#define M_PI 3.14159265358979323846
#define RAND(a,b,c) (a)
static uint32 urand(uint32 low,uint32){return low;}static float frand(float low,float){return low;}
static void Require(bool yes,char const*why){if(!yes)throw std::runtime_error(why);}
struct ObjectGuid{uint64 value=0;static ObjectGuid const Empty;ObjectGuid()=default;ObjectGuid(uint64 v):value(v){}void Clear(){value=0;}bool IsEmpty()const{return !value;}explicit operator bool()const{return value!=0;}friend bool operator<(ObjectGuid a,ObjectGuid b){return a.value<b.value;}friend bool operator==(ObjectGuid a,ObjectGuid b){return a.value==b.value;}friend bool operator!=(ObjectGuid a,ObjectGuid b){return !(a==b);}};ObjectGuid const ObjectGuid::Empty{};
struct Position{float m_positionX=0,m_positionY=0,m_positionZ=0,m_orientation=0;float GetPositionX()const{return m_positionX;}float GetPositionY()const{return m_positionY;}float GetPositionZ()const{return m_positionZ;}float GetOrientation()const{return m_orientation;}};
struct Unit;struct Player;struct Creature;struct GameObject;struct MockAI;struct Vehicle;
static std::map<ObjectGuid,Unit*>actors;static int talks=0;static uint64 lastSpeaker=0;
struct Unit{
 ObjectGuid guid;Position pos{0,0,0,0};int map=1,kind=TYPEID_UNIT,credits=0,removedAuras=0,casts=0;uint32 lastSpell=0;uint64 lastTarget=0,health=1000000,maxHealth=1000000;float scale=1;bool alive=true,inWorld=true;Unit*owner=nullptr;std::list<Unit*>auraTargets;
 explicit Unit(uint64 id):guid(id){actors[guid]=this;}virtual ~Unit(){actors.erase(guid);}ObjectGuid GetGUID()const{return guid;}int GetTypeId()const{return kind;}Player*ToPlayer();Creature*ToCreature();bool IsAlive()const{return alive;}bool IsInWorld()const{return inWorld;}float GetPositionX()const{return pos.m_positionX;}float GetPositionY()const{return pos.m_positionY;}float GetPositionZ()const{return pos.m_positionZ;}float GetOrientation()const{return pos.m_orientation;}Position GetPosition()const{return pos;}
 float GetExactDist(Unit const*p)const{return std::abs(pos.m_positionX-p->pos.m_positionX);}float GetDistance(Unit const*p)const{return GetExactDist(p);}float GetDistance(Position const&)const{return 0;}Position GetNearPosition(float distance,float){auto p=pos;p.m_positionX+=distance;return p;}bool IsWithinMeleeRange(Unit*,float){return true;}
 uint64 GetHealth()const{return health;}uint64 GetMaxHealth()const{return maxHealth;}void SetHealth(uint64 v){health=v;}float GetHealthPct(){return float(health)*100/maxHealth;}bool HealthBelowPct(float pct){return GetHealthPct()<pct;}float GetFloatValue(uint32){return scale;}void SetObjectScale(float v){scale=v;}void RemoveAurasDueToSpell(uint32){++removedAuras;}bool HasAura(uint32){return false;}void ClearUnitState(uint32){}void AddUnitState(uint32){}void SetTargetGUID(ObjectGuid){}Unit*GetOwner(){return owner;}void TargetsWhoHasMyAuras(std::list<Unit*>&result,std::vector<uint32>const&){result=auraTargets;}void AddAura(uint32,Unit*){}
 void CastSpell(Unit*p,uint32 spell,bool=false){++casts;lastSpell=spell;lastTarget=p?p->guid.value:0;}void CastSpell(float,float,float,uint32 spell,bool){++casts;lastSpell=spell;}void KilledMonsterCredit(uint32){++credits;}void KilledMonsterCredit(uint32,ObjectGuid){++credits;}void GroupEventHappens(uint32,Unit*){++credits;}uint32 GetPower(uint32){return 0;}void SetPower(uint32,uint32){}
};
struct Player:Unit{explicit Player(uint64 id):Unit(id){kind=TYPEID_PLAYER;}};
struct Motion{int points=0;void MovePoint(uint32,Position const&){++points;}void MovePoint(uint32,float,float,float){++points;}void Clear(){}};
struct MockAI{int attacks=0,actions=0;uint64 target=0;void AttackStart(Unit*p){++attacks;target=p->GetGUID().value;}void Talk(uint32){++talks;}void DoAction(uint32){++actions;}};
struct Vehicle{int removals=0;std::function<void()> onRemove;void RemoveAllPassengers(){++removals;if(onRemove)onRemove();}};
struct Creature:Unit{
 Motion motion;MockAI ai;Vehicle vehicle;std::map<uint32,Creature*>nearest;Creature*summoned=nullptr;Player*nearestPlayer=nullptr;Unit*victim=nullptr;bool allowVictim=true;int despawns=0,disappears=0,summonAttempts=0;uint32 entry=299;
 explicit Creature(uint64 id):Unit(id){}MockAI*AI(){volatile auto identity=guid.value;(void)identity;return &ai;}Motion*GetMotionMaster(){return &motion;}Vehicle*GetVehicleKit(){return &vehicle;}void SetFlag(uint32,uint32){}void RemoveFlag(uint32,uint32){}void SetReactState(uint32){}void SetVisible(bool){}void SetDisableGravity(bool){volatile auto id=guid.value;(void)id;}void SetControlled(bool,uint32){}void SetFacingTo(Unit*p){lastTarget=p->GetGUID().value;}bool HasUnitState(uint32){return false;}void StopAttack(bool=true){}void AttackStop(){}Position GetHomePosition(){return pos;}Unit*getVictim(){return victim;}uint32 GetEntry(){return entry;}Creature*FindNearestCreature(uint32 e,float){auto it=nearest.find(e);return it==nearest.end()?nullptr:it->second;}Player*FindNearestPlayer(float){return nearestPlayer;}template<class...T>Creature*SummonCreature(T...){++summonAttempts;return summoned;}void DespawnOrUnsummon(uint32=0){++despawns;}void DisappearAndDie(){++disappears;alive=false;}void AddThreat(Unit*,float){}
};
Player*Unit::ToPlayer(){return kind==TYPEID_PLAYER?static_cast<Player*>(this):nullptr;}Creature*Unit::ToCreature(){return kind==TYPEID_UNIT?static_cast<Creature*>(this):nullptr;}
struct GameObject:Unit{int destroyed=0,loot=0;explicit GameObject(uint64 id):Unit(id){}void GetPlayerListInGrid(std::list<Player*>&list,float){list.clear();}void DestroyForPlayer(Player*p){volatile auto id=p->GetGUID().value;(void)id;++destroyed;}void SetLootState(uint32){++loot;}};
namespace ObjectAccessor{static Unit*GetUnit(Unit&me,ObjectGuid id){auto it=actors.find(id);return it!=actors.end()&&it->second->inWorld&&it->second->map==me.map?it->second:nullptr;}static Creature*GetCreature(Unit&me,ObjectGuid id){auto u=GetUnit(me,id);return u?u->ToCreature():nullptr;}static Player*GetPlayer(Unit&me,ObjectGuid id){auto u=GetUnit(me,id);return u?u->ToPlayer():nullptr;}}
static Creature*GetClosestCreatureWithEntry(Creature*me,uint32 e,float d){return me->FindNearestCreature(e,d);}
static void DoScriptText(uint32,Creature*speaker,Unit* =nullptr){if(speaker){lastSpeaker=speaker->GetGUID().value;++talks;}}
struct EventMap{uint64 now=0;std::map<uint32,uint64>due;void Reset(){now=0;due.clear();}void Update(uint32 diff){now+=diff;}void ScheduleEvent(uint32 id,uint32 delay){due[id]=now+delay;}void RescheduleEvent(uint32 id,uint32 delay){ScheduleEvent(id,delay);}void CancelEvent(uint32 id){due.erase(id);}void DelayEvents(uint32 delay){for(auto&v:due)v.second+=delay;}uint64 GetNextEventTime(uint32 id){auto it=due.find(id);return it==due.end()?now+100000:it->second;}uint64 GetTimer(){return now;}uint32 ExecuteEvent(){auto it=std::min_element(due.begin(),due.end(),[](auto const&a,auto const&b){return a.second<b.second;});if(it==due.end()||it->second>now)return 0;auto id=it->first;due.erase(it);return id;}};
struct SpellInfo{uint32 Id=0;};
struct SummonList{void Summon(Creature*){}template<class...T>void DoAction(T...){};};struct EntryCheckPredicate{explicit EntryCheckPredicate(uint32){}};
struct BossAI{Creature*me;EventMap events;SummonList summons;bool heroic=true;explicit BossAI(Creature*c):me(c){}template<class...T>Unit* SelectTarget(T...){return me->victim;}bool UpdateVictim(){return me->allowVictim;}bool IsHeroic(){return heroic;}void EnterEvadeMode(){}void DoMeleeAttackIfReady(){}void AttackStart(Unit*p){me->AI()->AttackStart(p);}void Talk(uint32){++talks;}void DoZoneInCombat(Creature*,float){}void DoCast(Unit*p,uint32 spell,bool=false){me->CastSpell(p,spell);}void DoCast(uint32 spell){me->CastSpell(me,spell);}void DoCastVictim(uint32 spell){me->CastSpell(me->victim,spell);}void DoCastAOE(uint32 spell,bool=false){DoCast(spell);}};
struct GameObjectAI{GameObject*go;explicit GameObjectAI(GameObject*g):go(g){}};
