#include <limits>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=int;
constexpr int IN_PROGRESS=1,DONE=3,FAIL=2,UNIT_MOD_HEALTH=0,TOTAL_PCT=0;
constexpr int UNIT_FIELD_FLAGS=1,UNIT_FIELD_NPC_FLAGS=2,UNIT_FIELD_FLAGS_2=3;
constexpr int UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_NOT_ATTACKABLE_1=128,UNIT_NPC_FLAG_GOSSIP=1,UNIT_NPC_FLAG_QUESTGIVER=2;
constexpr int UNIT_FLAG2_REGENERATE_POWER=1,UNIT_FLAG2_DISABLE_TURN=2,UNIT_FLAG2_UNK6=4;
constexpr int CRITERIA_TYPE_COMPLETE_DUNGEON_ENCOUNTER=165,CRITERIA_TYPE_DUNGEON_ENCOUNTER_COUNTER=181;
constexpr int SAY_DEFEATED=1,PHASE_1=1,PHASE_2=2,PHASE_3=3,ACTION_1=1,ACTION_2=2;
constexpr int SpellRevivify=228171,SpellHornOfValour=228012,SpellHornOfValourPlayer=228013,ENCOUNTER_FRAME_DISENGAGE=1,ENCOUNTER_FRAME_ENGAGE=2,SpellHoverModeAnimState=90766;
namespace Data {namespace BossIDs {constexpr int OdynID=0;}namespace DataMisc {constexpr int OdynTestOfAges=1,OdynEventComplete1=2,OdynEventComplete2=3;}}
#define TC_LOG_DEBUG(...) ((void)0)
template<class T>T CalculatePct(T n,int pct){return T(n*pct/100);}
struct Creature;struct InstanceScript;struct SpellInfo {uint32 Id;};
struct WorldObject {virtual ~WorldObject()=default;};
struct Unit:WorldObject {
 uint64 health=1000,maximum=1000;
 virtual Creature* ToCreature(){return nullptr;}virtual Creature const* ToCreature()const{return nullptr;}
 uint64 GetHealth()const{return health;}uint64 GetMaxHealth()const{return maximum;}
 uint64 GetHealth(Unit*)const;uint64 GetMaxHealth(Unit*)const;
 float GetHealthPct()const{return maximum?float(health)*100/float(maximum):0;}
 uint8 GetLevelForTarget(WorldObject const*)const{return 100;}float GetModifierValue(int,int)const{return 1;}
 uint64 CountPctFromMaxHealth(int n,Unit* target=nullptr)const{return CalculatePct(GetMaxHealth(target),n);}
 bool HealthBelowPct(int n)const{return health<CountPctFromMaxHealth(n);}bool HealthAbovePct(int n)const{return health>CountPctFromMaxHealth(n);}
 uint32 Deal(Creature*,uint32);
};
struct Player:Unit {};
struct AI {
 virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}
 virtual void SpellFinishCast(SpellInfo const*){}virtual void DoAction(int){}
};
using CreatureAI=AI;
struct InstanceScript {
 int state=IN_PROGRESS;std::map<int,int> data;unsigned achievements=0,counters=0,disengages=0,engages=0;
 int GetBossState(int id){assert(id==Data::BossIDs::OdynID);return state;}
 int GetData(int id){return data[id];}void SetData(int id,int n){data[id]=n;}
 void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
 void SendEncounterUnit(int type,Creature*){if(type==ENCOUNTER_FRAME_DISENGAGE)++disengages;else ++engages;}
 void DoUpdateAchievementCriteria(int type,uint32 asset,int,int,Creature*){assert(asset==1958);if(type==165)++achievements;else{assert(type==181);++counters;}}
};
struct CreatureLevelStat {uint64 healthMax=1000;};
struct Creature:Unit {
 ::AI* i_disabledAI=nullptr;
 CreatureLevelStat stat;bool scalable=true,IsAIEnabled=true,alive=true;::AI* ai=nullptr;InstanceScript* instance=nullptr;Creature* owner=nullptr;
 unsigned auras=0,home=0,teleports=0,stopped=0,faction=16;std::map<int,uint32> flags;
 Creature* ToCreature()override{return this;}Creature const* ToCreature()const override{return this;}
 bool HasScalableLevels()const{return scalable;}CreatureLevelStat const* GetScaleLevelStat(uint8){return &stat;}
 uint8 getLevel()const{return 110;}uint64 GetCreateHealth()const{return maximum;}uint64 GetMaxHealthByLevel(uint8)const{return stat.healthMax;}
 float GetHealthMultiplierForTarget(WorldObject const*)const;
 InstanceScript* GetInstanceScript(){return instance;}::AI* AI(){return ai;}::AI* GetAI(){return ai;}Creature* GetAnyOwner(){return owner;}
 uint32 GetEntry(){return 114263;}void SetHealth(uint64 n){health=n;}void RemoveAllAuras(){++auras;}
 void setFaction(int n){faction=n;}uint32 getFaction(){return faction;}
 void SetFlag(int field,uint32 n){flags[field]|=n;}void RemoveFlag(int field,uint32 n){flags[field]&=~n;}
 void RemoveAura(int){}Player* FindNearestPlayer(float,bool){return nullptr;}template<class F>void AddDelayedEvent(uint32,F f){f();}
 struct Motion {void MoveJump(float,float,float,float,float,int,float){}} motion;Motion* GetMotionMaster(){return &motion;}
 void SetHomePosition(float,float,float,float){++home;}void NearTeleportTo(float,float,float,float){++teleports;}void CastStop(){++stopped;}
};
#include "NativeHealth.inc"
#include "NativeMultiplier.inc"
#include "TrialClamp.inc"
struct OdynProbe:AI {
 Creature* me;InstanceScript* instance;bool encounterComplete=false,SecondPhaseSay=false;uint8 eventPhase=PHASE_2;
 unsigned deaths=0,evades=0,talks=0,despawns=0,debuffs=0,phases=0,actions=0;
 OdynProbe(Creature& c,InstanceScript& i):me(&c),instance(&i){c.ai=this;c.instance=&i;}
 void _JustDied(){++deaths;instance->state=DONE;}void EnterEvadeMode(){++evades;}
 void Talk(int id){assert(id==SAY_DEFEATED);++talks;}void DespawnAllSummons(){++despawns;}void RemoveDebuffsOnPlayer(){++debuffs;}
 void StartEvents(int p){assert(p==PHASE_3);++phases;eventPhase=p;}
 void DoAction(int a)override{assert(a==ACTION_1);++actions;}
#include "TrialOdynDamage.inc"
};
struct OldOdynProbe:OdynProbe {
 using OdynProbe::OdynProbe;void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{}
#include "OldTrialOdynDamage.inc"
};
struct DefenderProbe:AI {
 Creature* me;InstanceScript* instance;bool event1=false,event2=false,heroic=false;unsigned casts=0;
 DefenderProbe(Creature& c,InstanceScript& i):me(&c),instance(&i){c.ai=this;c.instance=&i;}
 void AttackStart(Unit*){}bool IsHeroicPlusRaid(){return heroic;}void DoCast(int spell){assert(spell==SpellRevivify||spell==SpellHornOfValourPlayer);++casts;}
};
struct HymdallProbe:DefenderProbe {using DefenderProbe::DefenderProbe;
#include "HymdallDamage.inc"
#include "HymdallFinish.inc"
#include "HymdallReturn.inc"
};
struct HyrjaProbe:DefenderProbe {using DefenderProbe::DefenderProbe;
#include "HyrjaDamage.inc"
#include "HyrjaFinish.inc"
#include "HyrjaReturn.inc"
};
struct OldHymdallProbe:HymdallProbe {using HymdallProbe::HymdallProbe;void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{}
#include "OldHymdallDamage.inc"
};
struct OldHyrjaProbe:HyrjaProbe {using HyrjaProbe::HyrjaProbe;void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{}
#include "OldHyrjaDamage.inc"
};
uint32 Unit::Deal(Creature* victim,uint32 damage){
 DamageEffectType damagetype=0;victim->AI()->DamageTaken(this,damage,damagetype);uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
 if(health<=damage)victim->alive=false;
 return damage;
}
template<class Probe>void DefenderCases(int own,int other){
 for(bool heroic:{false,true})for(bool pet:{false,true})for(uint64 scale:{1000ull,1000000ull}){
  Player p;Unit guardian;Unit& attacker=pet?guardian:static_cast<Unit&>(p);Creature c,owner;c.health=c.maximum=1000000000;c.stat.healthMax=scale;
  InstanceScript i;OdynProbe boss(owner,i);c.owner=&owner;Probe probe(c,i);probe.heroic=heroic;
  uint32 raw=std::max<uint32>(1,uint32(2.0*scale*c.GetHealthMultiplierForTarget(&attacker)));
  assert(!attacker.Deal(&c,raw)&&c.alive&&c.health==c.CountPctFromMaxHealth(25)&&probe.event1&&probe.casts==1&&i.GetData(own)==DONE&&boss.actions==1);
  assert(!attacker.Deal(&c,raw)&&probe.casts==1&&boss.actions==1&&c.alive);
  SpellInfo revivify{SpellRevivify};probe.SpellFinishCast(&revivify);assert(!probe.event1&&i.GetData(own)==FAIL);
  c.health=c.maximum;assert(!attacker.Deal(&c,raw)&&probe.casts==2&&i.GetData(own)==DONE&&boss.actions==2);
  i.SetData(other,DONE);probe.SpellFinishCast(&revivify);assert(i.GetData(own)==DONE);
 }
 for(int pct:{25,85}){Player p;Creature c;c.scalable=false;InstanceScript i;Probe probe(c,i);probe.heroic=true;if(pct==85)i.SetData(Data::DataMisc::OdynTestOfAges,DONE);
  assert(p.Deal(&c,1000-pct*10-1)==uint32(1000-pct*10-1)&&!probe.event1&&!probe.event2);
  assert(!p.Deal(&c,1000-pct*10)&&c.alive&&c.GetHealthPct()==pct);
  if(pct==25)assert(probe.event1&&probe.casts==1);else assert(probe.event2&&c.faction==35&&c.teleports==1&&c.stopped==1&&i.disengages==1);
 }
 for(bool pet:{false,true}){Player p;Unit guardian;Unit& attacker=pet?guardian:static_cast<Unit&>(p);Creature c;c.health=c.maximum=1000000000;c.stat.healthMax=1000;
  InstanceScript i;i.SetData(Data::DataMisc::OdynTestOfAges,DONE);Probe probe(c,i);probe.heroic=true;
  assert(!attacker.Deal(&c,1)&&probe.event2&&c.alive&&c.health==c.CountPctFromMaxHealth(85)&&c.faction==35&&i.disengages==1);
  assert(!attacker.Deal(&c,1)&&i.disengages==1&&c.alive);
  probe.DoAction(ACTION_2);assert(!probe.event2&&c.faction==16&&i.engages==1);c.health=c.maximum;
  assert(!attacker.Deal(&c,1)&&probe.event2&&i.disengages==2&&c.alive);
 }
 {Player p;Creature c;c.scalable=false;InstanceScript i;Probe probe(c,i);uint32 d=0;probe.DamageTakenAfterScaling(&p,d,0);assert(!probe.event1);
  c.faction=35;d=2000;probe.DamageTakenAfterScaling(&p,d,0);assert(d==2000&&!probe.event1);c.faction=16;i.state=DONE;probe.DamageTakenAfterScaling(&p,d,0);assert(!probe.event1);}
}
template<class Probe>void OldScaledDeath(){
 Player p;Creature c;c.health=c.maximum=1000000000;c.stat.healthMax=1000;InstanceScript i;Probe probe(c,i);
 assert(p.Deal(&c,1)>1000&&!c.alive&&i.state==IN_PROGRESS);
}
int main(){
 {Player p;Creature c;InstanceScript i;OldOdynProbe old(c,i);assert(!p.Deal(&c,2000)&&!old.encounterComplete&&i.state==IN_PROGRESS&&c.alive);}
 {Player p;Creature c;InstanceScript i;OldHymdallProbe old(c,i);assert(!p.Deal(&c,2000)&&!old.event1&&i.GetData(Data::DataMisc::OdynEventComplete1)!=DONE);}
 {Player p;Creature c;InstanceScript i;OldHyrjaProbe old(c,i);assert(!p.Deal(&c,2000)&&!old.event1&&i.GetData(Data::DataMisc::OdynEventComplete2)!=DONE);}
 OldScaledDeath<OldOdynProbe>();OldScaledDeath<OldHymdallProbe>();OldScaledDeath<OldHyrjaProbe>();
 for(bool pet:{false,true})for(uint64 scale:{1000ull,1000000ull}){
  Player p;Unit guardian;Unit& attacker=pet?guardian:static_cast<Unit&>(p);Creature c;c.health=c.maximum=1000000000;c.stat.healthMax=scale;InstanceScript i;OdynProbe probe(c,i);
  assert(c.GetHealth(&attacker)==scale&&c.GetMaxHealth(&attacker)==scale);
  uint32 raw=std::max<uint32>(1,uint32(2.0*scale*c.GetHealthMultiplierForTarget(&attacker)));
  auto dealt=attacker.Deal(&c,raw);assert(!dealt&&c.alive&&c.health==c.CountPctFromMaxHealth(10)&&probe.encounterComplete&&i.state==DONE);
  assert(probe.deaths==1&&probe.evades==1&&probe.talks==1&&probe.despawns==1&&probe.debuffs==1&&!probe.phases);
  assert(c.faction==35&&c.flags[UNIT_FIELD_NPC_FLAGS]==3&&i.achievements==1&&i.counters==1);
  for(int n=0;n<4;++n)assert(!attacker.Deal(&c,raw));assert(i.achievements==1&&probe.deaths==1);
 }
 {Player p;Creature c;c.scalable=false;InstanceScript i;OdynProbe probe(c,i);assert(p.Deal(&c,449)==449&&!probe.encounterComplete&&!probe.phases);assert(p.Deal(&c,450)==450&&probe.phases==1);assert(!p.Deal(&c,900)&&probe.encounterComplete&&probe.phases==1);}
 {Player p;Creature c;c.health=c.maximum=1000;c.stat.healthMax=1000000;InstanceScript i;OdynProbe probe(c,i);assert(p.Deal(&c,1000000)==1000&&!probe.encounterComplete&&!probe.phases&&c.alive);}
 {Player p;Creature c;c.scalable=false;c.health=101;InstanceScript i;OdynProbe probe(c,i);assert(!p.Deal(&c,1)&&probe.encounterComplete&&c.health==100);}
 for(int state:{0,FAIL,DONE}){Player p;Creature c;InstanceScript i;i.state=state;OdynProbe probe(c,i);uint32 d=2000;probe.DamageTakenAfterScaling(&p,d,0);assert(d==2000&&!probe.encounterComplete);}
 {Player p;Creature c;InstanceScript i;OdynProbe probe(c,i);probe.eventPhase=PHASE_1;uint32 d=2000;probe.DamageTakenAfterScaling(&p,d,0);assert(d==2000&&!probe.encounterComplete);probe.eventPhase=PHASE_2;d=0;probe.DamageTakenAfterScaling(&p,d,0);assert(!probe.encounterComplete);}
 DefenderCases<HymdallProbe>(Data::DataMisc::OdynEventComplete1,Data::DataMisc::OdynEventComplete2);
 DefenderCases<HyrjaProbe>(Data::DataMisc::OdynEventComplete2,Data::DataMisc::OdynEventComplete1);
 std::cout<<"PASS: three old lethal-hit stalls reproduced; production Odyn10%, defender25%/85% hooks through native final scaling and target-health getters; player/pet, repeated hits, Revivify retry/both-defeated, flags and1958 once-only. Instance persistence/combat/map services are fixtures; client pending.\n";
}
