#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>
using uint8=uint8_t;using uint32=uint32_t;using SpellEffIndex=int;using SpellCastResult=int;
enum {QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1,SPELL_FAILED_BAD_TARGETS=1,SPELL_CAST_OK=0,EFFECT_0=0,SPELL_EFFECT_DUMMY=3,TARGET_UNIT_NEARBY_ENTRY=38};
struct ObjectGuid {uint32 value=0;bool operator==(ObjectGuid b)const{return value==b.value;}};
struct Unit;struct Player;struct Creature;
struct WorldObject {virtual ~WorldObject()=default;virtual Player* ToPlayer(){return nullptr;}virtual Creature* ToCreature(){return nullptr;}bool world=true,alive=true;void* map=nullptr;uint32 entry=0;ObjectGuid guid;bool IsInWorld(){return world;}bool IsAlive(){return alive;}void* GetMap(){return map;}uint32 GetEntry(){return entry;}ObjectGuid GetGUID(){return guid;}};
struct Unit:WorldObject {std::vector<uint32> casts,auras;Unit* last=nullptr;bool castFailed=false,auraFailed=false;bool HasAura(uint32 spell){return std::find(auras.begin(),auras.end(),spell)!=auras.end();}void CastSpell(Unit* target,uint32 spell,bool triggered){assert(triggered);if(!castFailed){casts.push_back(spell);last=target;}}void AddAura(uint32 spell,Unit* target){if(!auraFailed){target->auras.push_back(spell);last=target;}}};
struct Creature:Unit {uint32 spawn=0;Creature* ToCreature()override{return this;}uint32 GetDBTableGUIDLow(){return spawn;}};
struct Player:Unit {Player* ToPlayer()override{return this;}uint32 mapId=1220,zone=7541,instance=0,status=3,objective=0;bool phase=true;struct Status{std::shared_ptr<uint8> ScriptLifetime;}state;
 uint32 GetMapId(){return mapId;}uint32 GetZoneId(){return zone;}uint32 GetInstanceId(){return instance;}uint32 GetQuestStatus(uint32 q){assert(q==39060);return status;}bool InSamePhase(Unit*){return phase;}uint32 GetQuestObjectiveData(uint32 q,uint32 e){assert(q==39060&&e>=94340&&e<=94342);return objective;}template<class F>void AddQuestDelayedEvent(uint32 q,uint32 delay,F){assert(q==39060&&!delay);if(!state.ScriptLifetime)state.ScriptLifetime=std::make_shared<uint8>(0);}Status* getQuestStatus(uint32 q){assert(q==39060);return &state;}};
struct Hook{void operator+=(int){}};
struct SpellScript {Unit* caster=nullptr,*target=nullptr,*hit=nullptr;Hook OnEffectHitTarget,OnCheckCast,OnObjectTargetSelect;Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}Unit* GetHitUnit(){return hit;}virtual void Register(){}};
#define PrepareSpellScript(x) public:
#define SpellEffectFn(...) 1
#define SpellCheckCastFn(...) 1
#define SpellObjectTargetSelectFn(...) 1
#include "GreywatchVisual.inc"
int main(){
 int map=0;Player p;p.map=&map;Creature actors[3];uint32 spawns[]{292386,292849,292844};
 for(unsigned i=0;i<3;++i){auto& c=actors[i];c.map=&map;c.entry=94340+i;c.spawn=spawns[i];c.guid={100+i};
  spell_stormheim_blight_torch torch;torch.caster=&p;torch.hit=&c;auto before=p.casts.size();torch.Hit(0);assert(p.casts.size()==before+1&&p.casts.back()==186746&&p.last==&c);
  spell_stormheim_blight_throw missile;missile.caster=&p;missile.target=&c;missile.hit=&c;assert(missile.Check()==SPELL_CAST_OK);WorldObject* nearby=&p;missile.Select(nearby);assert(nearby==&c);missile.Hit(0);assert(c.auras.back()==186747&&p.auras.empty());
  p.objective=1;torch.Hit(0);assert(p.casts.size()==before+1);p.objective=0;
 }
 auto& c=actors[0];c.auras.clear();spell_stormheim_blight_throw delayed;delayed.caster=&p;delayed.target=delayed.hit=&c;assert(delayed.Check()==SPELL_CAST_OK);auto before=c.auras.size();p.status=QUEST_STATUS_COMPLETE;delayed.Hit(0);assert(c.auras.size()==before+1); // Final745 credit can complete before the missile arrives.
 p.status=0;delayed.Hit(0);assert(c.auras.size()==before+1);p.status=QUEST_STATUS_INCOMPLETE;p.state.ScriptLifetime.reset();p.AddQuestDelayedEvent(39060,0,[]{});delayed.Hit(0);assert(c.auras.size()==before+1); // Abandon/reaccept is a new lifetime.
 WorldObject* nearby=&p;delayed.Select(nearby);assert(!nearby);
 spell_stormheim_blight_throw missile;missile.caster=&p;missile.target=missile.hit=&c;assert(missile.Check()==SPELL_CAST_OK);missile.hit=&actors[1];missile.Hit(0);assert(c.auras.size()==before+1);missile.hit=&c;
 auto old=c.spawn;c.spawn=1;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);c.spawn=old;
 p.phase=false;missile.Hit(0);assert(c.auras.size()==before+1);p.phase=true;
 p.mapId=1;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);p.mapId=1220;p.instance=1;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);p.instance=0;
 c.alive=false;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);c.alive=true;c.world=false;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);c.world=true;
 int otherMap=0;c.map=&otherMap;assert(missile.Check()==SPELL_FAILED_BAD_TARGETS);c.map=&map;
 Player other;other.map=&map;spell_stormheim_blight_throw independent;independent.caster=&other;independent.target=independent.hit=&c;assert(independent.Check()==SPELL_CAST_OK);independent.Hit(0);assert(c.auras.size()==before+1); // Public fire is not stacked/refreshed by another player.
 p.castFailed=true;spell_stormheim_blight_torch torch;torch.caster=&p;torch.hit=&c;auto casts=p.casts.size();torch.Hit(0);assert(p.casts.size()==casts);p.castFailed=false;
 assert(missile.Check()==SPELL_CAST_OK);c.auras.clear();p.auraFailed=true;before=c.auras.size();missile.Hit(0);assert(c.auras.size()==before); // Optional AddAura failure has no progression side effect.
 std::cout<<"PASS: extracted native torch/throw handlers select all3 exact DB actors, prevent closer-player retargeting, use native747 aura only on impact, preserve final-credit timing, reject stale quest lifetimes/map/phase/spawn/death and tolerate cosmetic failures. Cast engine, aura expiry and packets/rendering are fixtures.\n";
}
