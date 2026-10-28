#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;using SpellEffIndex=uint32;
enum{CURRENT_CHANNELED_SPELL=1,EFFECT_0=0,SPELL_EFFECT_DUMMY=3};
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS};
struct Position{float x=0,y=0,z=0;void Relocate(Position const&p){*this=p;}};using WorldLocation=Position;
struct SpellInfo{uint32 Id=201032;};
struct Unit;struct Creature;
struct Spell{SpellInfo info;SpellInfo const*GetSpellInfo(){return &info;}struct{Unit*unit=nullptr;void SetUnitTarget(Unit*u){unit=u;}}m_targets;};
struct Scenario{uint32 id=1037,step=1;uint32 GetScenarioId(){return id;}uint32 GetCurrentStep(){return step;}}scenario;
struct Manager{bool present=true;Scenario*GetScenario(uint32 id){return present&&id==42?&scenario:nullptr;}}manager;auto*sScenarioMgr=&manager;
struct Unit:Position{virtual~Unit()=default;virtual Creature*ToCreature(){return nullptr;}uint32 mapid=1539,instance=42,guid=1;int map=1;bool world=true,alive=true,combat=false,aura=true,channelActive=true,phase=true,los=true;Spell channel;Creature*nearby=nullptr;
 uint32 GetMapId(){return mapid;}uint32 GetInstanceId(){return instance;}int GetMap(){return map;}uint32 GetGUID(){return guid;}bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool isInCombat(){return combat;}bool HasAura(uint32 id,uint32 owner){assert(id==201032&&owner==guid);return aura;}Spell*GetCurrentSpell(uint32){return channelActive?&channel:nullptr;}bool InSamePhase(Unit*){return phase;}bool IsWithinLOSInMap(Unit*){return los;}Creature*FindNearestCreature(uint32 id,float range,bool live){assert(id==103144&&range==50&&live);return nearby;}};
struct Creature:Unit{uint32 entry=101875,db=370936;Creature*ToCreature()override{return this;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return db;}};
struct Hook{void operator+=(int){}};
struct SpellScript{Unit*caster=nullptr;Spell spell;WorldLocation dest;bool noDest=false,finished=false;Hook OnCheckCast,OnEffectLaunch;virtual~SpellScript()=default;virtual bool Load(){return true;}virtual void Register(){}Unit*GetCaster(){return caster;}WorldLocation*GetHitDest(){return noDest?nullptr:&dest;}void SetExplTargetDest(WorldLocation&p){dest=p;}Spell*GetSpell(){return &spell;}void FinishCast(SpellCastResult r){assert(r==SPELL_FAILED_BAD_TARGETS);finished=true;}};
#define PrepareSpellScript(...) public:
#define SpellCheckCastFn(...) 0
#define SpellEffectFn(...) 0
#include "../../src/server/scripts/Legion/ClassHalls/stromkar_ritual_beam.h"
struct Fixture{Creature caster,receiver;spell_stromkar_ritual_beam script;Fixture(){manager={};scenario={};receiver.db=370922;receiver.entry=103144;receiver.x=2131.32f;receiver.y=2423.56f;receiver.z=122.278f;caster.nearby=&receiver;script.caster=&caster;script.dest.Relocate(caster);}};
int main(){
 for(uint32 id:{370936u,370937u,370943u})for(uint32 step:{0u,1u}){Fixture f;f.caster.db=id;scenario.step=step;assert(f.script.Load()&&f.script.Check()==SPELL_CAST_OK);assert(f.script.dest.x==f.caster.x);/* pinned native periodic self target / destination106 counterfactual */f.script.Launch(0);assert(!f.script.finished&&f.script.dest.x==f.receiver.x&&f.script.dest.y==f.receiver.y&&f.script.dest.z==f.receiver.z&&f.script.spell.m_targets.unit==&f.receiver);f.receiver.x+=2;f.script.Launch(0);assert(f.script.dest.x==f.receiver.x);}
 for(uint32 bad=0;bad<17;++bad){Fixture f;switch(bad){case 0:f.caster.world=false;break;case 1:f.caster.alive=false;break;case 2:f.caster.combat=true;break;case 3:manager.present=false;break;case 4:scenario.id=991;break;case 5:scenario.step=2;break;case 6:f.caster.aura=false;break;case 7:f.caster.channelActive=false;break;case 8:f.caster.channel.info.Id=1;break;case 9:f.caster.nearby=nullptr;break;case 10:f.receiver.db=1;break;case 11:f.receiver.world=false;break;case 12:f.receiver.alive=false;break;case 13:f.receiver.map=2;break;case 14:f.caster.phase=false;break;case 15:f.caster.los=false;break;case 16:f.caster.instance=43;break;}assert(f.script.Check()==SPELL_FAILED_BAD_TARGETS);f.script.Launch(0);assert(f.script.finished&&!f.script.spell.m_targets.unit);}
 for(uint32 bad=0;bad<4;++bad){Fixture f;Unit pet;if(bad==0)f.caster.entry=1;if(bad==1)f.caster.mapid=1;if(bad==2)f.caster.db=370642;/* Priest */if(bad==3)f.script.caster=&pet;assert(!f.script.Load());}
 {Fixture f;assert(f.script.Check()==SPELL_CAST_OK);f.receiver.alive=false;f.script.Launch(0);assert(f.script.finished&&!f.script.spell.m_targets.unit);}
 {Fixture f;f.script.noDest=true;f.script.Launch(0);assert(f.script.finished);}
 std::cout<<"PASS actual ritual beam script: three native actors/two stages, real receiver position and GUID,17 refusals, target loss/retry and Priest/foreign isolation. Cast/collision/packet services controlled; client beam still requires retest.\n";
}
