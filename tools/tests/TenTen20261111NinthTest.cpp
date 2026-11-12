// Complete production methods; minimal controlled battle/DB2/session endpoints.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <cstdlib>
using uint8=uint8_t;using int8=int8_t;using uint32=uint32_t;using int32=int32_t;
constexpr int MAX_PETBATTLE_TEAM=2,MAX_PETBATTLE_SLOTS=3,PETBATTLE_TEAM_1=0,PETBATTLE_TEAM_2=1,PETBATTLE_NULL_ID=0,PETBATTLE_NULL_SLOT=-1,MAX_PETBATTLE_ABILITIES=3,MAX_PETBATTLE_EFFECT_TYPES=235;
enum Labels {PETBATTLE_STATUS_CREATION=1,PETBATTLE_STATUS_RUNNING,PETBATTLE_STATUS_FINISHED,PETBATTLE_ROUND_RUNNING,PETBATTLE_ROUND_FINISHED,PETBATTLE_ROUND_RESULT_NONE,PETBATTLE_ROUND_RESULT_NORMAL,PETBATTLE_EVENT_BUFF_CHANGE,PETBATTLE_EVENT_BUFF_APPLY,PETBATTLE_EVENT_PET_SWAP,PETBATTLE_EVENT_FLAG_SKIP_TURN,PETBATTLE_EVENT_AURA_PROCESSING_BEGIN,PETBATTLE_EVENT_AURA_PROCESSING_END,PETBATTLE_EVENT_SET_SPEED,PETBATTLE_CAST_TRIGGER_ALL,PETBATTLE_CAST_TRIGGER_NONE,PETBATTLE_ABILITY_TURN0_PROC_ON_NONE,PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_OUT,PETBATTLE_ABILITY_TURN0_PROC_ON_SWAP_IN,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_START,PETBATTLE_ABILITY_TURN0_PROC_ON_ROUND_END,PETBATTLE_ABILITY_TURN0_PROC_ON_TURN,BATTLEPET_ABILITY_FLAG_TRIGGER=256};
enum States { BATTLEPET_STATE_swapOutLock,BATTLEPET_STATE_Mechanic_IsWebbed,BATTLEPET_STATE_swapInLock,BATTLEPET_STATE_Add_FlatDamageDealt,BATTLEPET_STATE_Add_FlatDamageTaken,BATTLEPET_STATE_Stat_Power,BATTLEPET_STATE_Mod_DamageDealtPercent,BATTLEPET_STATE_Mod_DamageTakenPercent,BATTLEPET_STATE_Mod_PetType_ID,BATTLEPET_STATE_Mod_PetTypeDamageDealtPercent,BATTLEPET_STATE_Mod_PetTypeDamageTakenPercent,BATTLEPET_STATE_Stat_CritChance,BATTLEPET_STATE_Passive_Beast,BATTLEPET_STATE_Passive_Magic,BATTLEPET_STATE_untargettable,BATTLEPET_STATE_Mod_HealingDealtPercent,BATTLEPET_STATE_Mod_HealingTakenPercent,BATTLEPET_STATE_Condition_DidDamageThisRound,BATTLEPET_STATE_Condition_WasDamagedThisTurn,BATTLEPET_STATE_Passive_Humanoid,BATTLEPET_STATE_Passive_Dragon };
enum Types { BATTLEPET_PETTYPE_HUMANOID,BATTLEPET_PETTYPE_DRAGONKIN,BATTLEPET_PETTYPE_FLYING,BATTLEPET_PETTYPE_UNDEAD,BATTLEPET_PETTYPE_CRITTER,BATTLEPET_PETTYPE_MAGIC,BATTLEPET_PETTYPE_ELEMENTAL,BATTLEPET_PETTYPE_BEAST,BATTLEPET_PETTYPE_AQUATIC,BATTLEPET_PETTYPE_MECHANICAL };
enum PetBattleAbilityImplicitTarget {PETBATTLE_TARGET_NONE,PETBATTLE_TARGET_CASTER,PETBATTLE_TARGET_TARGET,PETBATTLE_TARGET_CASTER_TEAM,PETBATTLE_TARGET_CASTER_TEAM_0,PETBATTLE_TARGET_CASTER_TEAM_1,PETBATTLE_TARGET_CASTER_TEAM_2,PETBATTLE_TARGET_TARGET_TEAM,PETBATTLE_TARGET_TARGET_TEAM_0,PETBATTLE_TARGET_TARGET_TEAM_1,PETBATTLE_TARGET_TARGET_TEAM_2,PETBATTLE_TARGET_ALL,PETBATTLE_TARGET_CASTER_TEAM_UNCONDITIONAL,PETBATTLE_TARGET_HEAD};
constexpr uint32 PETBATTLE_EVENT_FLAG_STRONG=1024,PETBATTLE_EVENT_FLAG_WEAK=2048,PETBATTLE_EVENT_FLAG_CRITICAL=4,PETBATTLE_EVENT_FLAG_DODGE=16;
template<class T,class U> auto CalculatePct(T x,U p){return T(x*p/100);}
static int lastRoll;static bool roll_chance_i(int v){lastRoll=v;return v>=100;}
static unsigned errorLogs;
#define TC_LOG_ERROR(...) (++errorLogs)
struct BattlePetSpeciesEntry{uint32 PetTypeEnum=0;};
struct BattlePetAbilityEntry{uint32 Flags=0;};
struct BattlePetAbilityTurnEntry{uint32 BattlePetAbilityID=0;uint8 OrderIndex=1;uint32 TurnTypeEnum=0,EventTypeEnum=0,ID=0;};
struct BattlePetAbilityEffectEntry{uint32 BattlePetEffectPropertiesID=0,ID=0,BattlePetAbilityTurnID=0;uint8 OrderIndex=1;int Param[6]{};};
template<class T>struct Store:std::vector<T const*>{T const*LookupEntry(uint32 id)const{return id&&!this->empty()?this->front():nullptr;}};
static Store<BattlePetSpeciesEntry>sBattlePetSpeciesStore;static Store<BattlePetAbilityEntry>sBattlePetAbilityStore;static Store<BattlePetAbilityTurnEntry>sBattlePetAbilityTurnStore;static Store<BattlePetAbilityEffectEntry>sBattlePetAbilityEffectStore;
struct DamageMod{float Humanoid=1,Dragonkin=1,Flying=1,Undead=1,Critter=1,Magic=1,Elemental=1,Beast=1,Aquatic=1,Mechanical=1;};
struct DamageTable{DamageMod value;DamageMod const*GetRow(int){return &value;}}sBattlePetTypeDamageModTable;
struct PetBattleEvent{uint32 BuffTurn=0,slot=0;PetBattleEvent(uint32=0,int=0,uint32=0,uint32=0,uint32=0,uint32=0,uint32=0){};void UpdateBuff(uint32,uint32 s,uint32,int,int){slot=s;}void UpdateFrontPet(int=-1){}void UpdateSpeed(int,int){}};
struct PetBattle;struct PetBattleAura;struct PetBattleAbilityEffect;
struct BattlePetInstance{uint32 TeamID=0,ID=0,Species=0;int Health=100,States[64]{},Cooldowns[3]{-1,-1,-1};bool attack=true;bool IsAlive(){return Health>0;}bool CanAttack(){return attack;}int GetMaxHealth(){return 100;}int GetSpeed(){return 10;}};
struct PetBattleTeam{PetBattle*PetBattleInstance=nullptr;std::shared_ptr<BattlePetInstance>TeamPets[3];int TeamPetCount=0,ActivePetID=0;uint32 ActiveAbilityId=0,activeAbilityTurn=0,activeAbilityTurnMax=0;bool Ready=false,isRun=false,finish=false;int updates=0,PlayerGuid=0;bool CanSwap(int8 replacementPet=PETBATTLE_NULL_SLOT);bool HasPendingMultiTurnCast(){return false;}bool Update(){++updates;return finish;}void DoCasts(uint32=0){};};
struct Timer{void Update(uint32){}bool Passed(){return true;}void Reset(){}};
struct Session{void SendPetBattleRoundResult(PetBattle*){}};struct Player{Session session;Session*GetSession(){return &session;}};
namespace ObjectAccessor{static Player*GetObjectInWorld(int,Player*){return nullptr;}}
struct PetBattle{std::shared_ptr<BattlePetInstance>Pets[6];PetBattleTeam*Teams[2];std::list<PetBattleAura*>PetAuras;std::list<PetBattleEvent>RoundEvents;std::vector<std::pair<uint32,uint32>>RoundPetSpeedUpdate;std::vector<int>PetXDied;std::map<int,bool>FightedPets;uint32 RoundTurn=0,BattleStatus=PETBATTLE_STATUS_RUNNING,RoundStatus=PETBATTLE_ROUND_FINISHED,RoundResult=PETBATTLE_ROUND_RESULT_NONE,RoundFirstTeamCasting=0,Turn=0;Timer m_UpdateTimer;int apply=0,expire=0;
PetBattle(){for(int t=0;t<2;++t){Teams[t]=new PetBattleTeam;Teams[t]->PetBattleInstance=this;Teams[t]->ActivePetID=t*3;}}~PetBattle();
bool AddAura(uint32,uint32,uint32,int32,int32,uint32,uint32);void Update(uint32);void SwapPet(uint32,int32,bool=false);void ProceedRound();int Cast(uint32,uint32,uint32,uint32,uint32){return 0;}uint32 GetFirstAttackingTeam(){return 0;}};
struct PetBattleAura{uint32 AbilityID=0,TriggerId=0,CasterPetID=0,TargetPetID=0,ID=0;int Duration=1,MaxDuration=1,Turn=1;bool Expired=false;void Apply(PetBattle*b){++b->apply;}void Expire(PetBattle*b){Expired=true;++b->expire;}void Process(PetBattle*);};
PetBattle::~PetBattle(){for(auto p:Teams)delete p;for(auto p:PetAuras)delete p;}
struct PetBattleAbilityTurn{uint32 ChainFailure=0;};
struct PetBattleAbilityEffect{BattlePetAbilityEffectEntry const*EffectInfo=nullptr;PetBattleAbilityTurn*AbilityTurn=nullptr;PetBattle*PetBattleInstance=nullptr;uint32 AbilityID=0,Caster=0,Target=0,Flags=0;bool StopChain=false,IsTriggered=false,ReportFailAsImmune=false;std::vector<uint32>Targets;static inline std::vector<uint32>hits;static inline bool failFirst=false;
bool AddTarget(uint32);bool AddTarget(PetBattleAbilityImplicitTarget);void SelectTargets();bool Execute();int32 CalculateDamage(int32);bool HandleHealPercent();int GetPetType(){return 0;}int GetState(uint32 p,uint32 s){return PetBattleInstance->Pets[p]->States[s];}int GetHealth(uint32 p){return PetBattleInstance->Pets[p]->Health;}int GetMaxHealth(uint32){return 100;}uint32 GetActiveOpponent(){return PetBattleInstance->Teams[!PetBattleInstance->Pets[Caster]->TeamID]->ActivePetID;}void CalculateHit(int){}int CalculateHeal(int n){return n;}bool Heal(uint32 p,int v){PetBattleInstance->Pets[p]->Health+=v;return v>0;}bool HandleDummy(){hits.push_back(Target);PetBattleInstance->RoundEvents.emplace_back();return !failFirst||Target!=0;}};
struct Handler{bool(PetBattleAbilityEffect::*Handle)()=nullptr;PetBattleAbilityImplicitTarget ImplicitTarget=PETBATTLE_TARGET_NONE;};static Handler Handlers[MAX_PETBATTLE_EFFECT_TYPES];
#include "Selected.inc"
static void Fill(PetBattle&b,int count=3){for(int t=0;t<2;++t)for(int j=0;j<count;++j){auto p=std::make_shared<BattlePetInstance>();p->ID=t*3+j;p->TeamID=t;b.Pets[p->ID]=p;b.Teams[t]->TeamPets[j]=p;b.Teams[t]->TeamPetCount=count;}}
static void check(bool x){if(!x)std::exit(1);}
int main(int argc,char**argv){int c=argc>1?atoi(argv[1]):0;PetBattle b;Fill(b);BattlePetAbilityEffectEntry data;data.BattlePetEffectPropertiesID=1;Handlers[1]={&PetBattleAbilityEffect::HandleDummy,PETBATTLE_TARGET_TARGET_TEAM};PetBattleAbilityEffect e;e.PetBattleInstance=&b;e.EffectInfo=&data;
switch(c){
case 0:{for(auto pair:{std::pair<int,int>{1,100},{5,200},{3,300}}){auto a=new PetBattleAura;a->ID=pair.first;a->AbilityID=pair.second;b.PetAuras.push_back(a);}b.AddAura(0,0,100,3,1,0,1);check(b.RoundEvents.back().slot==1);b.AddAura(0,0,400,3,1,0,0);check(b.PetAuras.back()->ID==6);break;}
case 1:{for(int t=0;t<2;++t){b.Pets[t*3]->attack=false;b.Teams[t]->ActiveAbilityId=30;b.Teams[t]->activeAbilityTurnMax=4;}b.Update(1000);check(!b.Teams[0]->ActiveAbilityId&&!b.Teams[1]->ActiveAbilityId);b.Teams[0]->finish=true;b.Teams[0]->Ready=b.Teams[1]->Ready=true;auto n=b.Teams[1]->updates;b.Update(1000);check(b.Teams[1]->updates==n&&!b.Teams[0]->Ready&&!b.Teams[1]->Ready);break;}
case 2:{b.SwapPet(0,-2);check(b.Teams[0]->ActivePetID==0);b.SwapPet(0,3);check(b.Teams[0]->ActivePetID==0);b.Pets[1]->States[BATTLEPET_STATE_swapInLock]=1;b.SwapPet(0,1);check(b.Teams[0]->ActivePetID==0);b.Pets[1]->States[BATTLEPET_STATE_swapInLock]=0;b.SwapPet(0,1);check(b.Teams[0]->ActivePetID==1);break;}
case 3:{e.Target=3;b.Pets[0]->States[BATTLEPET_STATE_Stat_CritChance]=100;b.Pets[3]->States[BATTLEPET_STATE_Stat_CritChance]=0;check(e.CalculateDamage(10)==20&&lastRoll==100);break;}
case 4:{e.AddTarget(PETBATTLE_TARGET_TARGET_TEAM);check(e.Targets==std::vector<uint32>({3,4,5}));e.Targets.clear();e.AddTarget(PETBATTLE_TARGET_HEAD);check(e.Targets==std::vector<uint32>({0,3}));for(int n=0;n<3;++n){e.Targets.clear();check(e.AddTarget(PetBattleAbilityImplicitTarget(PETBATTLE_TARGET_CASTER_TEAM_0+n)));check(e.Targets==std::vector<uint32>{uint32(n)});}break;}
case 5:{e.Targets={0,1,2};check(e.Execute());check(PetBattleAbilityEffect::hits==e.Targets);break;}
case 6:{data.BattlePetEffectPropertiesID=MAX_PETBATTLE_EFFECT_TYPES;e.Targets={0};e.SelectTargets();check(e.Targets.empty()&&!e.Execute()&&errorLogs==1);break;}
case 7:{e.AbilityID=726;data.Param[0]=4;b.Pets[0]->Health=50;check(e.HandleHealPercent()&&b.Pets[0]->Health==54);break;}
case 8:{PetBattleAura a;a.Duration=a.MaxDuration=-1;for(int n=0;n<5;++n){a.Process(&b);check(a.Duration==-1&&!a.Expired);}break;}
case 9:{BattlePetAbilityEntry ability;BattlePetAbilityTurnEntry turn;turn.BattlePetAbilityID=900;turn.ID=900;data.BattlePetAbilityTurnID=900;sBattlePetAbilityStore.push_back(&ability);sBattlePetAbilityTurnStore.push_back(&turn);sBattlePetAbilityEffectStore.push_back(&data);b.RoundEvents.emplace_back();b.RoundEvents.back().BuffTurn=99;PetBattleAura a;a.AbilityID=900;a.Turn=3;a.MaxDuration=9;a.Duration=9;a.Process(&b);check(b.RoundEvents.size()==3);auto it=b.RoundEvents.begin();check(it++->BuffTurn==99);check(it->BuffTurn==3);break;}
default:return 2;}
std::cout<<"PASS complete production pet battle scope "<<c<<"; DB2/cast/session endpoints controlled\n";}
