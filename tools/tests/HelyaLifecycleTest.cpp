#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;using DamageEffectType=int;
constexpr int IN_PROGRESS=1,DONE=3,NOT_STARTED=0,FAIL=2,UNIT_MOD_HEALTH=0,TOTAL_PCT=0;
constexpr int UNIT_FIELD_FLAGS=1,UNIT_FLAG_IMMUNE_TO_PC=256,UNIT_FLAG_IMMUNE_TO_NPC=512,UNIT_FLAG_NOT_ATTACKABLE_1=128;
constexpr int UNIT_STATE_ROOT=1,REACT_AGGRESSIVE=2,REACT_PASSIVE=0,POWER_ENERGY=0,POWER_ALTERNATE=1;
constexpr int CRITERIA_TYPE_COMPLETE_DUNGEON_ENCOUNTER=165,SAY_INTRO_3_FIXTURE=13,ACTION_2=2;
#define TC_LOG_DEBUG(...) ((void)0)
template<class T,class U>T CalculatePct(T n,U pct){return T(n*pct/100);}
struct Position{float x,y,z,o;};
struct ObjectGuid{uint64 value=0;void Clear(){value=0;}bool operator<(ObjectGuid const& b)const{return value<b.value;}};
#include "TrialHeader.inc"
#include "HelyaEnums.inc"
struct Creature;struct TempSummon;struct InstanceScript;
struct WorldObject{virtual ~WorldObject()=default;};
struct Unit:WorldObject{
 uint64 health=1000,maximum=1000;bool IsAIEnabled=true;
 virtual Creature* ToCreature(){return nullptr;}virtual Creature const* ToCreature()const{return nullptr;}
 uint64 GetHealth()const{return health;}uint64 GetMaxHealth()const{return maximum;}uint64 GetHealth(Unit*)const;uint64 GetMaxHealth(Unit*)const;
 float GetHealthPct()const{return maximum?float(health)*100/float(maximum):0;}
 uint8 GetLevelForTarget(WorldObject const*)const{return 100;}float GetModifierValue(int,int)const{return 1;}
 uint64 CountPctFromMaxHealth(int pct)const{return CalculatePct(maximum,pct);}bool HealthBelowPct(int pct){return health<CountPctFromMaxHealth(pct);}
 uint32 Deal(Creature*,uint32);void NativeDeath(Creature*);
};
struct Player:Unit{};
struct AI{virtual void DamageTaken(Unit*,uint32&,DamageEffectType){}virtual void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void SummonedCreatureDies(Creature*,Unit*){}virtual void Reset(){}};
using CreatureAI=AI;
struct GameObject{unsigned respawn=0;void SetRespawnTime(uint32 n){respawn=n;}};
struct MapFixture{std::vector<std::pair<uint32,Position>> spawned;void SummonCreature(uint32 id,Position p){spawned.push_back({id,p});}GameObject* GetGameObject(ObjectGuid){return nullptr;}};
struct InstanceScript{
 int state=IN_PROGRESS;unsigned credits=0;MapFixture map;MapFixture* instance=&map;
 ObjectGuid HelyaGUID,GoLightRay;std::vector<ObjectGuid> GoLightRayGUID;
 int GetBossState(int id){assert(id==Data::BossIDs::HelyaID);return state;}
 void OnCreatureDamageTaken(Creature*,Unit*,uint32&){}
 void DoUpdateAchievementCriteria(int type,uint32 asset){assert(type==165&&asset==2008);++credits;}
 // Summon admission/idempotence is covered by the full native instance probe;
 // this extracted fixture retains the authored scene entries and positions.
 void EnsureVictoryActor(uint8,uint32 entry,Position const& pos){instance->SummonCreature(entry,pos);}
#include "HelyaVictoryActors.inc"
 void HelyaDone(){
#include "HelyaDone.inc"
 }
 void OnHelyaCreate(Creature* creature);
};
struct CreatureLevelStat{uint64 healthMax=1000;};
struct Creature:Unit{
 ::AI* i_disabledAI=nullptr;
 bool scalable=false,alive=true,casting=false;CreatureLevelStat stat;::AI* ai=nullptr;InstanceScript* instance=nullptr;uint32 entry=114537,faction=16,flags=0,groups=0,stops=0,react=REACT_AGGRESSIVE;ObjectGuid guid;
 uint64 now=0;std::multimap<uint64,std::function<void()>> callbacks;std::set<uint32> auras;
 Creature* ToCreature()override{return this;}Creature const* ToCreature()const override{return this;}virtual TempSummon* ToTempSummon(){return nullptr;}
 bool HasScalableLevels()const{return scalable;}CreatureLevelStat const* GetScaleLevelStat(uint8){return &stat;}uint8 getLevel()const{return 110;}uint64 GetCreateHealth()const{return maximum;}uint64 GetMaxHealthByLevel(uint8)const{return stat.healthMax;}
 float GetHealthMultiplierForTarget(WorldObject const*)const;InstanceScript* GetInstanceScript(){return instance;}::AI* AI(){return ai;}
 ObjectGuid GetGUID(){return guid;}uint32 GetEntry(){return entry;}void SetHealth(uint64 n){health=n;}
 void SetInCombatWith(Unit* a){ai->EnterCombat(a);}uint32 getFaction(){return faction;}bool HasFlag(int,uint32 n){return flags&n;}void RemoveFlag(int,uint32 n){flags&=~n;}
 void CastStop(){casting=false;++stops;}void AttackStop(){}void SetReactState(int n){react=n;}void SetPower(int,int){}void SetMaxPower(int,int){}void AddUnitState(int){}
 void RemoveAllAuras(){auras.clear();}void RemoveAura(uint32 id){auras.erase(id);}void RemoveAllAreaObjects(){}void KillAllDelayedCombats(){callbacks.clear();}
 void SummonCreatureGroup(int n){assert(n==3);++groups;}
 void AddDelayedEvent(uint32 ms,std::function<void()> f){callbacks.emplace(now+ms,std::move(f));}void AddDelayedCombat(uint32 ms,std::function<void()> f){AddDelayedEvent(ms,std::move(f));}
 void Tick(uint64 ms){now+=ms;while(!callbacks.empty()&&callbacks.begin()->first<=now){auto f=std::move(callbacks.begin()->second);callbacks.erase(callbacks.begin());f();}}
};
struct TempSummon:Creature{Unit* summoner=nullptr;TempSummon* ToTempSummon()override{return this;}Unit* GetSummoner(){return summoner;}};
void InstanceScript::OnHelyaCreate(Creature* creature){switch(creature->GetEntry()){
#include "HelyaCreate.inc"
 default:break;}}
#include "NativeHealth.inc"
#include "NativeMultiplier.inc"
struct HelyaProbe:AI{
 Creature* me;InstanceScript* instance;uint32 berserk=0,_checkEvadeTimer=0,AOEdmg=0,count=0;uint8 trashDiedCount=0,trashPhase2Count=0,eventPhase=PHASE_1;
 bool phase2=false,phase3=false,HelyaIntro=false,checkrephase=false,FuryOfTheMaw=false,FuryOfTheMawTrash=false;ObjectGuid playerGuid;std::set<ObjectGuid> defeatedGrippingTentacles;
 bool heroic=false,mythic=false,failCasts=false;unsigned deaths=0,starts=0,despawns=0,debuffs=0,talks=0;std::vector<uint32> casts;std::vector<int> spoken;
 struct Events{std::map<int,uint32> scheduled;void Reset(){scheduled.clear();}void RescheduleEvent(int e,uint32 t){scheduled[e]=t;}} events;
 HelyaProbe(Creature& c,InstanceScript& i):me(&c),instance(&i){c.ai=this;c.instance=&i;}
 bool IsMythicRaid(){return mythic;}bool IsHeroicRaid(){return heroic&&!mythic;}bool IsHeroicPlusRaid(){return heroic||mythic;}bool IsLfrRaid(){return !heroic&&!mythic;}bool IsNormalRaid(){return false;}
 void DoCast(uint32 spell){casts.push_back(spell);if(!failCasts)me->auras.insert(spell);}void Talk(int line){++talks;spoken.push_back(line);}void DoAction(int a){assert(a==ACTION_2);}
 void DespawnAllSummons(){++despawns;}void RemoveAllDebaffsOnPlayer(){++debuffs;}
 void _Reset(){instance->state=NOT_STARTED;me->SetHealth(me->maximum);me->callbacks.clear();}
 void _EnterCombat(){++starts;instance->state=IN_PROGRESS;}
 void _JustDied(){++deaths;if(instance->state!=DONE){instance->state=DONE;instance->HelyaDone();}}
#include "HelyaReset.inc"
#include "HelyaEnter.inc"
#include "HelyaStart.inc"
#include "HelyaDamage.inc"
#include "HelyaSummonDeath.inc"
#include "HelyaDeath.inc"
};
struct OldHelyaProbe:HelyaProbe{using HelyaProbe::HelyaProbe;void DamageTakenAfterScaling(Unit*,uint32&,DamageEffectType)override{}
#include "OldHelyaDamage.inc"
};
void Unit::NativeDeath(Creature* creature){
#include "NativeDeath.inc"
}
uint32 Unit::Deal(Creature* victim,uint32 damage){
 if(!victim->alive)return 0;
 DamageEffectType damagetype=0;if(victim->IsAIEnabled)victim->AI()->DamageTaken(this,damage,damagetype);uint64 health=victim->GetHealth(this);
#include "NativeScaledHook.inc"
 if(health<=damage){victim->alive=false;NativeDeath(victim);}
 return damage;
}
void KillTentacle(Unit& killer,Creature& boss,HelyaProbe& probe,uint64 id){TempSummon t;t.entry=114900;t.guid.value=id;t.summoner=&boss;t.instance=probe.instance;t.IsAIEnabled=false;assert(killer.Deal(&t,2000)==2000&&!t.alive);probe.SummonedCreatureDies(&t,&killer);}
int main(){
 {Player p;Creature c;InstanceScript i;OldHelyaProbe old(c,i);assert(p.Deal(&c,2000)==2000&&!c.alive&&!old.phase2&&i.credits==1);}
 {Player p;Creature c;c.scalable=true;c.health=c.maximum=1000000000;c.stat.healthMax=1000;InstanceScript i;OldHelyaProbe old(c,i);assert(p.Deal(&c,1)>1000&&!c.alive&&!old.phase2&&i.credits==1);}
 for(bool pet:{false,true})for(bool heroic:{false,true})for(bool failedCast:{false,true})for(uint64 scale:{1000ull,1000000ull}){
  Player p;Unit guardian;Unit& attacker=pet?guardian:static_cast<Unit&>(p);Creature c;c.scalable=true;c.health=c.maximum=1000000000;c.stat.healthMax=scale;InstanceScript i;HelyaProbe probe(c,i);probe.heroic=heroic;
  probe.failCasts=failedCast;uint32 raw=std::max<uint32>(1,uint32(2.0*scale*c.GetHealthMultiplierForTarget(&attacker)));
  c.casting=true;assert(!attacker.Deal(&c,raw)&&c.alive&&probe.phase2&&probe.eventPhase==PHASE_2&&c.health==650000000&&c.groups==1&&!c.casting&&!i.credits);
  for(int n=0;n<3;++n)assert(!attacker.Deal(&c,raw));assert(c.groups==1);
  Creature unrelated;unrelated.entry=114901;probe.SummonedCreatureDies(&unrelated,nullptr);assert(!probe.trashPhase2Count);
  for(uint64 n=1;n<=8;++n){KillTentacle(attacker,c,probe,n);assert(probe.trashPhase2Count==n&&c.health==650000000-n*20000000&&probe.eventPhase==PHASE_2);}
  KillTentacle(attacker,c,probe,9);assert(probe.trashPhase2Count==9&&c.health==470000000&&probe.eventPhase==PHASE_3&&!i.credits);
  uint32 allowed=1;probe.DamageTakenAfterScaling(&attacker,allowed,0);assert(allowed==1);c.Tick(5000);assert(std::find(probe.spoken.begin(),probe.spoken.end(),SAY_PHASE_2)==probe.spoken.end());c.Tick(1500);assert(!probe.checkrephase);
  assert(attacker.Deal(&c,raw)>0&&!c.alive&&probe.deaths==1&&i.credits==1&&i.state==DONE);
  assert(i.map.spawned.size()==9&&i.map.spawned.front().first==116760);assert(!attacker.Deal(&c,raw)&&i.credits==1);
 }
 {Player p;Creature c;InstanceScript i;HelyaProbe probe(c,i);assert(p.Deal(&c,349)==349&&!probe.phase2);assert(!p.Deal(&c,350)&&c.health==650&&probe.phase2);KillTentacle(p,c,probe,101);assert(probe.defeatedGrippingTentacles.size()==1&&probe.trashPhase2Count==1);probe.Reset();assert(!probe.phase2&&probe.defeatedGrippingTentacles.empty()&&!probe.trashPhase2Count&&c.health==1000);probe.EnterCombat(&p);assert(!p.Deal(&c,2000));KillTentacle(p,c,probe,101);assert(probe.trashPhase2Count==1&&c.health==630);}
 {Player p;Creature c;c.scalable=true;c.stat.healthMax=1000000;InstanceScript i;HelyaProbe probe(c,i);assert(p.Deal(&c,1000000)==1000&&c.alive&&!probe.phase2);}
 for(int state:{NOT_STARTED,FAIL}){Player p;Creature c;InstanceScript i;i.state=state;HelyaProbe probe(c,i);probe.eventPhase=0;assert(!p.Deal(&c,2000)&&probe.starts==1&&probe.phase2&&c.alive);}
 for(int mode=0;mode<4;++mode){Player p;Creature c;InstanceScript i;i.state=NOT_STARTED;HelyaProbe probe(c,i);probe.eventPhase=0;if(mode==0)c.flags=UNIT_FLAG_IMMUNE_TO_PC;if(mode==1)c.faction=35;if(mode==2)i.state=DONE;uint32 d=mode==3?0:2000;probe.DamageTakenAfterScaling(&p,d,0);assert(!probe.starts&&!probe.phase2);}
 {Creature c;InstanceScript i;i.state=DONE;i.OnHelyaCreate(&c);assert(i.map.spawned.size()==9&&i.map.spawned.front().first==116760);i.state=NOT_STARTED;i.map.spawned.clear();i.OnHelyaCreate(&c);assert(i.map.spawned.empty());}
 std::cout<<"PASS: baseline premature kills; production65% crossing, native scaling and summon-death dispatch, nine unique2% wounds/47% return, repeated/null/pet/first-hit/retry guards, actual phase/reset/death and instance post-victory spawn branches. Spell/timer/map/health-application services are fixtures; client pending.\n";
}
