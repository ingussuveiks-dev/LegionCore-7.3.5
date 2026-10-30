#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#include "RitualEnums.inc"
enum {CURRENT_CHANNELED_SPELL=1,EFFECT_0=0,AURA_EFFECT_HANDLE_REAL=1,DYNOBJ_AURA_TYPE=2,MAX_SPELL_EFFECTS=32,
 SPELL_GROUP_STACK_RULE_EXCLUSIVE=1,SPELL_GROUP_STACK_RULE_EXCLUSIVE_HIGHEST=2,SPELL_GROUP_STACK_RULE_EXCLUSIVE_FROM_SAME_CASTER=3};
enum SpellCastResult {SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS};using AuraEffectHandleModes=uint32;
uint32 warnings=0;
#define TC_LOG_WARN(...) (++warnings)
struct ObjectGuid {uint64 id=0;void Clear(){id=0;}bool operator==(ObjectGuid b)const{return id==b.id;}bool operator!=(ObjectGuid b)const{return id!=b.id;}explicit operator bool()const{return id!=0;}};
struct Vehicle{uint32 GetAvailableSeatCount(){assert(false);return 0;}};
struct SpellInfo {
 uint32 Id=204125,EffectMask=1;struct{uint32 SpellClassSet=0;}ClassOptions;
 struct{uint32 Attributes[16]{};}MiscData;SpellInfo*GetMisc(){return this;}
 struct Effect{uint32 TriggerSpell=0,ApplyAuraName=SPELL_AURA_DUMMY;}effect;
 Effect const*GetEffect(uint32,uint32)const{return &effect;}
 bool IsDifferentRankOf(SpellInfo const*)const{return false;}bool IsAuraExclusiveBySpecificWith(SpellInfo const*,bool)const{return false;}bool IsAuraExclusiveBySpecificPerCasterWith(SpellInfo const*)const{return false;}
 bool IsChanneled()const{return Id==201032;}bool HasAttribute(SpellAttr3 flag)const{return MiscData.Attributes[3]&flag;}bool HasAttribute(SpellAttr6 flag)const{return MiscData.Attributes[6]&flag;}bool HasAttribute(SpellAttr1 flag)const{return MiscData.Attributes[1]&flag;}
 bool _IsPositiveSpell()const{return true;}bool IsRankOf(SpellInfo const* other)const{return Id==other->Id;}bool IsMultiSlotAura()const{return false;}
}tortureInfo;
#include "TortureFix.inc"
struct SpellMgr{uint32 CheckSpellGroupStackRules(SpellInfo const*,SpellInfo const*){return 0;}}spellMgr;auto*sSpellMgr=&spellMgr;
struct Unit;struct Creature;
struct Aura {
 SpellInfo const*m_spellInfo=&tortureInfo;ObjectGuid caster;Unit*owner=nullptr;bool removed=false;uint32 m_diffMode=0;
 bool CanStackWith(Aura const*)const;SpellInfo const*GetSpellInfo()const{return m_spellInfo;}ObjectGuid GetCasterGUID()const{return caster;}uint32 GetType()const{return 0;}uint32 GetId()const{return m_spellInfo->Id;}
 bool IsPassive()const{return false;}bool IsArea()const{return false;}bool HasEffectType(uint32)const{return false;}ObjectGuid GetCastItemGUID()const{return {};}Unit*GetOwner()const{return owner;}void Remove(){removed=true;}
};
struct Spell{SpellInfo info;Spell(){info.Id=201032;}SpellInfo const*GetSpellInfo(){return &info;}};
struct Scenario{uint32 id=1037,step=1;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenarioState;
struct Manager{bool present=true;Scenario*GetScenario(uint32 instance){return present&&instance==42?&scenarioState:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct Unit{
 virtual~Unit()=default;virtual Creature*ToCreature(){return nullptr;}Unit*ToUnit(){return this;}Vehicle*GetVehicleKit(){assert(false);return nullptr;}
 ObjectGuid guid;uint32 mapId=1539,instance=42;int map=1;bool world=true,alive=true,combat=false,channelActive=true,channelAura=true,phase=true,los=true;float distance=4;Spell channel;Creature*nearby=nullptr;
 uint32 casts=0,removals=0;SpellCastResult result=SPELL_CAST_OK;std::vector<std::unique_ptr<Aura>> auras;
 ObjectGuid GetGUID(){return guid;}uint64 GetGUIDLow(){return guid.id;}uint32 GetMapId(){return mapId;}uint32 GetInstanceId(){return instance;}int GetMap(){return map;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}
 bool InSamePhase(Unit*){return phase;}bool IsWithinLOSInMap(Unit*){return los;}Spell*GetCurrentSpell(uint32 id){assert(id==CURRENT_CHANNELED_SPELL);return channelActive?&channel:nullptr;}
 Creature*FindNearestCreature(uint32 id,float range,bool living){assert(id==103144&&range==50&&living);return distance<=range?nearby:nullptr;}
 bool HasAura(uint32 id,ObjectGuid caster){if(id==201032)return caster==guid&&channelAura;assert(id==204125);for(auto const&a:auras)if(!a->removed&&a->caster==caster)return true;return false;}
 void RemoveAurasDueToSpell(uint32 id,ObjectGuid caster){assert(id==204125);for(auto const&a:auras)if(!a->removed&&a->caster==caster){a->Remove();++removals;}}
 SpellCastResult CastSpell(Unit*,uint32,bool);
};
struct Creature:Unit {uint32 entry=101875,spawn=370936;Creature*ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return spawn;}};
std::map<uint64,Creature*> actors;
struct ObjectAccessor{static Creature*GetCreature(Unit&owner,ObjectGuid guid){auto i=actors.find(guid.id);return i!=actors.end()&&i->second->map==owner.map&&i->second->world?i->second:nullptr;}};
#include "NativeAuraStack.inc"
SpellCastResult Unit::CastSpell(Unit*target,uint32 id,bool triggered){assert(id==204125&&triggered);++casts;if(result!=SPELL_CAST_OK)return result;if(target->HasAura(id,guid))return result;auto a=std::make_unique<Aura>();a->caster=guid;a->owner=target;for(auto const&old:target->auras)if(!old->removed&&!a->CanStackWith(old.get()))old->Remove();target->auras.push_back(std::move(a));return result;}
struct AuraEffect{};struct Hook{void operator+=(int){}};
struct AuraScript{
 Unit*unitOwner=nullptr,*auraCaster=nullptr;Aura*aura=nullptr;uint32 prevented=0;Hook OnEffectPeriodic,OnEffectUpdate,AfterEffectRemove;
 virtual~AuraScript()=default;virtual bool Load(){return true;}virtual void Register(){}Unit*GetUnitOwner(){return unitOwner;}Unit*GetCaster(){return auraCaster;}ObjectGuid GetCasterGUID(){return aura->caster;}Aura*GetAura(){return aura;}void PreventDefaultAction(){++prevented;}
};
#define PrepareAuraScript(...) public:
#include "NativeHookTypes.inc"
#define AuraEffectPeriodicFn(F,I,N) CheckPeriodicHook(&F,I,N)
#define AuraEffectUpdateFn(F,I,N) CheckUpdateHook(&F,I,N)
template<class C,class...Args> int CheckUpdateHook(void(C::*handler)(Args...),uint32 index,uint32 aura){static_assert(std::is_same_v<decltype(handler),NativeAuraEffectUpdateFnType<C>>);assert(index==0&&(aura==SPELL_AURA_DUMMY||aura==SPELL_AURA_PERIODIC_TRIGGER_SPELL));return 0;}
template<class C,class...Args> int CheckPeriodicHook(void(C::*handler)(Args...),uint32 index,uint32 aura){static_assert(std::is_same_v<decltype(handler),NativeAuraEffectPeriodicFnType<C>>);assert(index==0&&aura==SPELL_AURA_PERIODIC_TRIGGER_SPELL);return 0;}
template<class C,class...Args> int CheckRemoveHook(void(C::*handler)(Args...),uint32 index,uint32 aura,uint32 mode){static_assert(std::is_same_v<decltype(handler),NativeAuraEffectApplicationModeFnType<C>>);assert(index==0&&aura==SPELL_AURA_PERIODIC_TRIGGER_SPELL&&mode==AURA_EFFECT_HANDLE_REAL);return 0;}
#define AuraEffectRemoveFn(F,I,N,M) CheckRemoveHook(&F,I,N,M)
#include "../../src/server/scripts/Legion/ClassHalls/stromkar_ritual_torture.h"
struct Fixture{
 Creature target,source[3];Aura sourceAura[3];spell_stromkar_ritual_channel channels[3];
 Fixture(bool fixed=true){actors.clear();manager={};scenarioState={};warnings=0;tortureInfo={};if(fixed)ApplyTortureFix(&tortureInfo);target.entry=103144;target.spawn=370922;target.guid={9};actors[9]=&target;uint32 spawns[]={370936,370937,370943};for(uint32 i=0;i<3;++i){source[i].spawn=spawns[i];source[i].guid={i+1};source[i].nearby=&target;actors[i+1]=&source[i];sourceAura[i].caster=source[i].guid;channels[i].unitOwner=channels[i].auraCaster=&source[i];channels[i].aura=&sourceAura[i];}}
 uint32 Count(){return uint32(std::count_if(target.auras.begin(),target.auras.end(),[](auto const&a){return !a->removed;}));}
 void Start(){for(auto&c:channels){assert(c.Load());c.Register();c.Periodic(nullptr);assert(c.prevented==1);c.Update(500,nullptr);}}
};
int main(){_set_abort_behavior(0,_CALL_REPORTFAULT);uint32 cases=0;
 for(bool fixed:{false,true}){Fixture f(fixed);f.Start();assert(f.Count()==(fixed?3u:1u));for(uint32 i=0;i<3;++i)assert(f.source[i].casts==1);++cases;}
 for(uint32 stage:{0u,1u}){Fixture f;scenarioState.step=stage;f.Start();for(auto&c:f.channels)c.Update(500,nullptr);assert(f.Count()==3);for(auto&s:f.source)assert(s.casts==1);f.channels[1].Remove(nullptr,AURA_EFFECT_HANDLE_REAL);assert(f.Count()==2&&f.target.removals==1);f.channels[1].Remove(nullptr,AURA_EFFECT_HANDLE_REAL);assert(f.Count()==2&&f.target.removals==1);++cases;}
 for(uint32 invalid=0;invalid<21;++invalid){Fixture f;f.Start();auto&s=f.source[0];switch(invalid){case 0:s.world=false;break;case 1:s.alive=false;break;case 2:s.combat=true;break;case 3:s.mapId=1;break;case 4:s.entry=1;break;case 5:s.spawn=370935;break;case 6:s.channelActive=false;break;case 7:s.channel.info.Id=1;break;case 8:s.channelAura=false;break;case 9:manager.present=false;break;case 10:scenarioState.id=991;break;case 11:scenarioState.step=2;break;case 12:s.nearby=nullptr;break;case 13:f.target.spawn=1;break;case 14:f.target.world=false;break;case 15:f.target.alive=false;break;case 16:f.target.map=2;break;case 17:s.phase=false;break;case 18:s.los=false;break;case 19:s.distance=51;break;case 20:s.instance=43;break;}
  // Source-aura cleanup and independent receiver-aura cleanup must each work.
  spell_stromkar_ritual_torture receiver;receiver.unitOwner=&f.target;receiver.auraCaster=&s;receiver.aura=f.target.auras[0].get();receiver.Update(500,nullptr);assert(receiver.aura->removed);f.channels[0].Update(500,nullptr);assert(f.Count()==2&&s.casts==1);++cases;
 }
 {Fixture f;f.Start();spell_stromkar_ritual_torture receiver;receiver.unitOwner=&f.target;receiver.auraCaster=nullptr;receiver.aura=f.target.auras[0].get();receiver.Update(500,nullptr);assert(f.Count()==2);++cases;}
 {Fixture f;f.source[0].result=SPELL_FAILED_BAD_TARGETS;f.Start();assert(f.Count()==2&&warnings==1);f.channels[0].Update(500,nullptr);assert(warnings==1&&f.source[0].casts==2);f.source[0].result=SPELL_CAST_OK;f.channels[0].Update(500,nullptr);assert(f.Count()==3&&f.source[0].casts==3);++cases;}
 {Fixture f;f.source[0].spawn=370935;assert(!f.channels[0].Load());spell_stromkar_ritual_torture receiver;receiver.unitOwner=&f.target;receiver.auraCaster=&f.source[0];assert(!receiver.Load());++cases;}
 {Fixture f;f.Start();for(uint32 i=0;i<3;++i){spell_stromkar_ritual_torture receiver;receiver.unitOwner=&f.target;receiver.auraCaster=&f.source[i];receiver.aura=f.target.auras[i].get();assert(receiver.Load());receiver.Update(500,nullptr);assert(!receiver.aura->removed);}++cases;}
 std::cout<<"PASS "<<cases<<" native per-caster aura-stack/controller/cleanup/retry cases; world casts/target search and actual client chains remain separate services.\n";
}
