#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>
using uint32=uint32_t;using int32=int32_t;using SpellEffIndex=uint32;
enum {SPELL_EFFECT_HANDLE_HIT_TARGET=1,SPELL_EFFECT_HANDLE_HIT=2,TARGET_FLAG_UNIT_MASK=1,TARGET_FLAG_GAMEOBJECT_MASK=2,PHASE_UPDATE_FLAG_ZONE_UPDATE=1,CONFIG_GROUP_XP_DISTANCE=0};
#include "EventTypes.inc"
#define TC_LOG_DEBUG(...) ((void)0)
struct ObjectGuid{static ObjectGuid const Empty;};ObjectGuid const ObjectGuid::Empty{};
struct Map{bool dungeon=false;uint32 starts=0;void* source=nullptr;void* target=nullptr;bool IsDungeon()const{return dungeon;}void ScriptsStart(int,uint32 id,void* from,void* to){assert(id==49193);++starts;source=from;target=to;}};
struct WorldObject{bool world=true;uint32 mapId=1480,instance=42;float distance=0;Map* map=nullptr;bool IsInWorld()const{return world;}bool IsCreature()const{return false;}ObjectGuid GetGUID()const{return{};}Map* GetMap()const{return map;}uint32 GetMapId()const{return mapId;}uint32 GetInstanceId()const{return instance;}bool InInstance()const{return true;}float GetDistance(WorldObject const* other)const{return other->distance;}};
struct Player;struct ZoneScript{uint32 requests=0;void ProcessEvent(WorldObject* target,uint32 id){assert(!target&&id==49193);++requests;}};
struct InstanceScript:ZoneScript{};
struct Unit:WorldObject{virtual bool IsPlayer()const{return false;}virtual Player* ToPlayer(){return nullptr;}ZoneScript* zone=nullptr;InstanceScript* script=nullptr;ZoneScript* GetZoneScript(){return zone;}InstanceScript* GetInstanceScript(){return script;}};
struct GroupReference{Player* member;GroupReference* following=nullptr;Player* getSource(){return member;}GroupReference* next(){return following;}};
struct Group{GroupReference* first;GroupReference* GetFirstMember(){return first;}};
struct World{float getFloatConfig(int id){assert(id==CONFIG_GROUP_XP_DISTANCE);return 50;}} world;auto sWorld=&world;
struct Phase{uint32 updates=0;void RemoveUpdateFlag(int flag){assert(flag==PHASE_UPDATE_FLAG_ZONE_UPDATE);++updates;}};
struct Player:Unit{bool alive=true,corpse=false,talked=false,eligible=true;uint32 credits=0;Group* currentGroup=nullptr;Phase phase;std::vector<uint32> events;std::vector<std::function<void()>> pending;
 bool IsPlayer()const override{return true;}Player* ToPlayer()override{return this;}bool IsAlive()const{return alive;}WorldObject const* GetCorpse()const{return corpse?this:nullptr;}Group* GetGroup(){return currentGroup;}bool TalkedToCreature(uint32 id,ObjectGuid){assert(id==94390);return talked;}
 void KilledMonsterCredit(uint32 id,ObjectGuid){assert(id==94390);if(eligible)++credits;} // quest storage endpoint is controlled
 bool IsAtGroupRewardDistance(WorldObject const*)const;void RewardPlayerAndGroupAtEvent(uint32,WorldObject*);Phase& GetPhaseMgr(){return phase;}
 void AddDelayedEvent(uint32 delay,std::function<void()> callback){assert(delay==100);pending.push_back(callback);}void Flush(){auto work=pending;pending.clear();for(auto& callback:work)callback();}
 void UpdateAchievementCriteria(CriteriaTypes type,uint32 id,uint32 count){assert((type==CRITERIA_TYPE_SCRIPT_EVENT||type==CRITERIA_TYPE_SCRIPT_EVENT_2)&&id==49193&&count==1);events.push_back(type);}
};
constexpr int sEventScripts=0;
struct Effect{int32 MiscValue=94390;uint32 mask=0;uint32 GetProvidedTargetMask()const{return mask;}};
struct SpellInfo{uint32 Id=197092;Effect effect;Effect const* GetEffect(SpellEffIndex,uint32){return &effect;}};
struct Spell{uint32 effectHandleMode=SPELL_EFFECT_HANDLE_HIT_TARGET,m_diffMode=0;Unit* m_caster=nullptr;Unit* unitTarget=nullptr;WorldObject* gameObjTarget=nullptr;WorldObject* focusObject=nullptr;SpellInfo* m_spellInfo=nullptr;void EffectKillCredit(SpellEffIndex);void EffectSendEvent(SpellEffIndex);void EffectUpdatePlayerPhase(SpellEffIndex);};
#include "GroupEvent.inc"
#include "GroupRange.inc"
#include "KillCredit.inc"
#include "SendEvent.inc"
#include "UpdatePhase.inc"
int main(){Map map;Player owner,near,dead,released,far,otherMap,otherInstance,inactive,offline;for(auto p:{&owner,&near,&dead,&released,&far,&otherMap,&otherInstance,&inactive,&offline})p->map=&map;
 dead.alive=false;released.alive=false;released.corpse=true;far.distance=50.01f;otherMap.mapId=1;otherInstance.instance=43;inactive.eligible=false;offline.world=false;
 GroupReference refs[]={{&owner},{&near},{&dead},{&released},{&far},{&otherMap},{&otherInstance},{&inactive},{&offline},{nullptr}};for(unsigned i=0;i<9;++i)refs[i].following=&refs[i+1];Group group{refs};owner.currentGroup=&group;
 SpellInfo info;Spell spell;spell.m_caster=&owner;spell.unitTarget=&owner;spell.m_spellInfo=&info;spell.EffectKillCredit(0);
 assert(owner.credits==1&&near.credits==1&&dead.credits==1&&released.credits==0&&far.credits==0&&otherMap.credits==0&&otherInstance.credits==0&&inactive.credits==0&&offline.credits==0);
 map.dungeon=true;spell.EffectKillCredit(0);assert(far.credits==1&&otherMap.credits==0&&otherInstance.credits==0);map.dungeon=false;
 owner.talked=true;auto prior=owner.credits;spell.EffectKillCredit(0);assert(owner.credits==prior);owner.talked=false;owner.world=false;spell.EffectKillCredit(0);assert(owner.credits==prior);owner.world=true;
 Unit pet;spell.unitTarget=&pet;spell.EffectKillCredit(0);spell.unitTarget=nullptr;spell.EffectKillCredit(0);assert(owner.credits==prior);
 spell.unitTarget=&owner;spell.EffectUpdatePlayerPhase(2);assert(owner.phase.updates==0&&owner.pending.size()==1);owner.Flush();assert(owner.phase.updates==1);spell.unitTarget=&pet;spell.EffectUpdatePlayerPhase(2);assert(owner.pending.empty());
 info.effect.MiscValue=49193;spell.effectHandleMode=SPELL_EFFECT_HANDLE_HIT;spell.unitTarget=nullptr;InstanceScript instance;owner.script=&instance;spell.EffectSendEvent(1);assert(instance.requests==1&&map.starts==1&&map.source==&owner&&!map.target&&owner.events==std::vector<uint32>({73,92}));
 ZoneScript zone;owner.zone=&zone;spell.EffectSendEvent(1);assert(zone.requests==1&&instance.requests==1&&map.starts==2&&owner.events.size()==4);
 info.effect.mask=TARGET_FLAG_UNIT_MASK;spell.EffectSendEvent(1);assert(map.starts==2&&owner.events.size()==4);info.effect.mask=0;spell.effectHandleMode=0;spell.EffectSendEvent(1);spell.EffectKillCredit(0);spell.EffectUpdatePlayerPhase(2);assert(map.starts==2&&owner.pending.empty());
 std::cout<<"PASS actual197092 native134 group/range/instance/life/quest-endpoint delegation,61 targetless49193 zone/instance/map73+92 and mask guards,167 delayed100ms phase update. Quest counters, world/conversation/equipment acceptance remain separate endpoints/client checks.\n";
}
