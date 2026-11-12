#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
namespace TenthActor {
using uint8=uint8_t;using uint32=uint32_t;
#include "TenthActorLabels.inc"
static void Require(bool b,char const*s){if(!b)throw std::runtime_error(s);}
struct ObjectGuid{uint64_t id=0;void Clear(){id=0;}static ObjectGuid Empty;};inline ObjectGuid ObjectGuid::Empty{};
struct Position{float GetPositionX()const{return 1;}float GetPositionY()const{return 2;}float GetPositionZ()const{return 3;}float GetOrientation()const{return 0;}};static Position summonPositions[4];
struct EventMap{std::map<uint32,uint32>events;void Reset(){events.clear();}void Update(uint32){}uint32 ExecuteEvent(){return 0;}void RescheduleEvent(uint32 id,uint32 delay){events[id]=delay;}};
struct Unit{virtual~Unit()=default;uint32 GetTypeId(){return TYPEID_PLAYER;}};struct InstanceScript{};struct Creature;
static std::map<uint64_t,std::unique_ptr<Creature>>registry;static uint64_t next=1;static bool heroic=false,failSpawn=false;static uint32 diagnostics=0;
struct Motion{void MoveTargetedHome(){}void MoveChase(Unit*){}};
struct Actions{std::vector<uint32>actions;void DoAction(uint32 id){actions.push_back(id);}};
struct Creature:Unit{ObjectGuid guid{next++};uint32 entry=0;bool dead=false,live=true;std::set<uint32>auras;InstanceScript instance;Motion motion;Actions actions;Creature*SummonCreature(uint32 id,Position const&,uint32){if(failSpawn)return nullptr;auto p=std::make_unique<Creature>();p->entry=id;auto ptr=p.get();registry.emplace(p->guid.id,std::move(p));return ptr;}ObjectGuid GetGUID(){return guid;}InstanceScript*GetInstanceScript(){return &instance;}Motion*GetMotionMaster(){return &motion;}void RemoveAllAuras(){auras.clear();}bool isDead(){return dead;}void Respawn(){dead=false;}void RemoveAura(uint32 id){auras.erase(id);}void NearTeleportTo(float,float,float,float){}bool HasAura(uint32 id){return auras.count(id)!=0;}void CastSpell(Unit*,uint32 id,bool=false){auras.insert(id);}void CastStop(){}Unit*getVictim(){return this;}Actions*AI(){return &actions;}Actions*GetAI(){return &actions;}void DespawnOrUnsummon(){live=false;}};
namespace ObjectAccessor{static Creature*GetCreature(Creature&,ObjectGuid id){auto i=registry.find(id.id);return i==registry.end()||!i->second->live?nullptr:i->second.get();}}
struct ScriptedAI{Creature*me;uint32 evades=0;explicit ScriptedAI(Creature*p):me(p){}virtual~ScriptedAI()=default;virtual void Reset(){}virtual void EnterCombat(Unit*){}virtual void UpdateAI(uint32){}virtual void JustDied(Unit*){}virtual void KilledUnit(Unit*){}virtual void JustReachedHome(){}void EnterEvadeMode(){++evades;Reset();}bool UpdateVictim(){return true;}Unit*SelectTarget(uint32,uint32,float,bool){return me;}void DoCast(Unit*,uint32){}void DoCast(uint32){}void DoMeleeAttackIfReady(){}void Talk(uint32){}};
#define RAID_MODE(a,b) (heroic?(b):(a))
#define TC_LOG_ERROR(...) (++diagnostics)
#include "TenthActorSelected.inc"
#undef TC_LOG_ERROR
#undef RAID_MODE
static void Run(){for(bool h:{false,true}){heroic=h;registry.clear();Creature boss;boss_corla_herald_of_twilightAI ai(&boss);ai.Reset();uint32 pairs=h?3:2;Require(registry.size()==pairs*2,"Corla retains native normal and heroic channel counts");auto first=std::find_if(registry.begin(),registry.end(),[](auto const&p){return p.second->entry==NPC_TWILIGHT_ZEALOT_CORLA;});registry.erase(first);ai.Reset();Require(std::count_if(registry.begin(),registry.end(),[](auto const&p){return p.second->live;})==pairs*2,"Corla replaces a missing zealot and its old owned trigger");ai.Reset();Require(std::count_if(registry.begin(),registry.end(),[](auto const&p){return p.second->live;})==pairs*2,"Repeated reset cannot duplicate live channel actors");for(auto&p:registry)p.second->dead=true;ai.Reset();for(auto&p:registry)if(p.second->live)Require(!p.second->dead,"Tracked dead channel actor respawns");ai.EnterCombat(&boss);Require(ai.events.events[EVENT_DARK_COMMAND]==20000,"Original dark-command timing retained");ai.JustDied(&boss);ai.JustDied(&boss);Require(std::none_of(registry.begin(),registry.end(),[](auto const&p){return p.second->live;}),"Repeated death cleans both owned actor families");ai.Reset();registry.clear();failSpawn=true;auto before=diagnostics;ai.EnterCombat(&boss);Require(ai.evades==1&&diagnostics>=before+1,"Failed owned spawn emits diagnostics and refuses combat");failSpawn=false;ai.Reset();Require(std::count_if(registry.begin(),registry.end(),[](auto const&p){return p.second->live;})==pairs*2,"Later reset retries failed channel creation");}registry.clear();}
}
