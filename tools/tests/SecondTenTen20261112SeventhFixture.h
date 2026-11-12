#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int8=int8_t;using int16=int16_t;using int32=int32_t;using int64=int64_t;
#include "PetEnums.inc"
static void Require(bool ok,char const*message){if(!ok)throw std::runtime_error(message);}
template<class T,class P>static T CalculatePct(T amount,P percent){return T(amount*percent/100);}template<class T,class P>static void AddPct(T&amount,P percent){amount+=CalculatePct(amount,percent);}static bool roll_chance_i(int32 chance){return chance>=100;}
#define TC_LOG_ERROR(...) do{throw std::runtime_error("Unexpected production diagnostic in controlled pet callback");}while(false)
struct BattlePetAbilityEntry{uint32 ID=0,Flags=0,Cooldown=0;int8 PetTypeEnum=0;};
struct BattlePetAbilityTurnEntry{uint32 ID=0,BattlePetAbilityID=0;uint8 OrderIndex=1,EventTypeEnum=255,TurnTypeEnum=0;};
struct BattlePetAbilityEffectEntry{uint32 ID=0,BattlePetAbilityTurnID=0,AuraBattlePetAbilityID=0;uint16 BattlePetEffectPropertiesID=0,Param[6]{};uint8 OrderIndex=1;};
struct BattlePetAbilityStateEntry{uint32 ID=0,BattlePetAbilityID=0,BattlePetStateID=0;int32 Value=0;};
struct BattlePetSpeciesEntry{uint32 ID=0;uint8 PetTypeEnum=0;};struct BattlePetStateEntry{uint32 ID=0,Flags=1;};
template<class T>struct Store{std::list<T>values;std::vector<T*>rows;T*Add(T v){values.push_back(v);auto p=&values.back();rows.push_back(p);return p;}T*LookupEntry(uint32 id){for(auto p:rows)if(p->ID==id)return p;return nullptr;}auto begin(){return rows.begin();}auto end(){return rows.end();}};
static Store<BattlePetAbilityEntry>sBattlePetAbilityStore;static Store<BattlePetAbilityTurnEntry>sBattlePetAbilityTurnStore;static Store<BattlePetAbilityEffectEntry>sBattlePetAbilityEffectStore;static Store<BattlePetAbilityStateEntry>sBattlePetAbilityStateStore;static Store<BattlePetSpeciesEntry>sBattlePetSpeciesStore;static Store<BattlePetStateEntry>sBattlePetStateStore;
struct DamageMods{float Humanoid=1,Dragonkin=1,Flying=1,Undead=1,Critter=1,Magic=1,Elemental=1,Beast=1,Aquatic=1,Mechanical=1;};struct DamageTable{DamageMods row;DamageMods*GetRow(uint32){return &row;}};static DamageTable sBattlePetTypeDamageModTable;
struct PetBattle;struct PetBattleAura;
struct BattlePetInstance{uint32 ID=0,TeamID=0,Species=0,Level=1;int32 Health=100,InfoMaxHealth=100;int32 States[NUM_BATTLEPET_STATES]{},Cooldowns[3]{-1,-1,-1},Lockdowns[3]{};uint32 Abilities[3]{};bool IsAlive(){return !States[BATTLEPET_STATE_Is_Dead];}int32 GetMaxHealth();int32 GetSpeed(){return 1;}};
struct PetBattleTeam{uint32 TeamPetCount=3,ActivePetID=0,ActiveAbilityId=0,activeAbilityTurn=0,activeAbilityTurnMax=0;std::shared_ptr<BattlePetInstance>TeamPets[3];};
struct PetBattleEventUpdate{uint32 TargetPetID=0;int32 Health=0,MaxHealth=0;struct{uint32 ID=0,AbilityID=0;int32 Duration=0;uint32 Turn=0;}Buff;};
struct PetBattleEvent{uint32 EventType=0,Flags=0,AbilityEffectID=0,RoundTurn=0,BuffTurn=0;int32 SourcePetID=0;std::vector<PetBattleEventUpdate>Updates;PetBattleEvent(uint32 type=0,int32 source=0,uint32 flags=0,uint32 effect=0,uint32 round=0,uint32 buff=0,uint32=0):EventType(type),Flags(flags),AbilityEffectID(effect),RoundTurn(round),BuffTurn(buff),SourcePetID(source){}void UpdateBuff(uint32 target,uint32 id,uint32 ability,int32 duration,uint32 turn){PetBattleEventUpdate u;u.TargetPetID=target;u.Buff={id,ability,duration,turn};Updates.push_back(u);}void UpdateHealth(uint32 p,int32 n){PetBattleEventUpdate u;u.TargetPetID=p;u.Health=n;Updates.push_back(u);}void UpdateMaxHealth(uint32 p,int32 n){PetBattleEventUpdate u;u.TargetPetID=p;u.MaxHealth=n;Updates.push_back(u);}void UpdateState(uint32,uint32,int32){}void Trigger(uint32,uint32){}};
struct PetBattle{std::array<std::shared_ptr<BattlePetInstance>,6>Pets;std::array<PetBattleTeam*,2>Teams;std::list<PetBattleAura*>PetAuras;std::list<PetBattleEvent>RoundEvents;std::vector<std::pair<uint32,uint32>>RoundPetSpeedUpdate;std::vector<uint32>PetXDied;uint32 RoundTurn=0,RoundResult=PETBATTLE_ROUND_RESULT_NONE,RoundFirstTeamCasting=0,WeatherAbilityId=0;PetBattle(){for(uint32 t=0;t<2;++t){Teams[t]=new PetBattleTeam;Teams[t]->ActivePetID=t*3;for(uint32 j=0;j<3;++j){auto p=std::make_shared<BattlePetInstance>();p->ID=t*3+j;p->TeamID=t;Pets[p->ID]=p;Teams[t]->TeamPets[j]=p;}}}~PetBattle();PetBattleCastResult Cast(uint32,uint32,uint32,uint32,uint32,int32=PETBATTLE_NULL_SLOT);bool AddAura(uint32,uint32,uint32,int32,int32,uint32,uint32);void SetPetState(uint32,uint32,uint32,uint32,int32,bool=false,uint32=0);void Kill(int8,int8,uint32,bool=false,uint32=0);};
struct PetBattleAura{uint32 ID=0,AbilityID=0,TriggerId=0,CasterPetID=0,TargetPetID=0,Turn=1;int32 Duration=1,MaxDuration=1;bool Expired=false;std::vector<std::pair<uint32,int32>>AppliedStates;void Apply(PetBattle*);void Remove(PetBattle*);void Process(PetBattle*);void Expire(PetBattle*);};
PetBattle::~PetBattle(){for(auto t:Teams)delete t;for(auto a:PetAuras)delete a;}
#include "PetEffectClass.inc"
#define MAX_PETBATTLE_EFFECT_TYPES 235
static const uint32 FailFlags=PETBATTLE_EVENT_FLAG_MISS|PETBATTLE_EVENT_FLAG_BLOCKED|PETBATTLE_EVENT_FLAG_DODGE|PETBATTLE_EVENT_FLAG_REFLECT|PETBATTLE_EVENT_FLAG_IMMUNE;
#include "PetHandlers.inc"
// Controlled ordinary terminal endpoint; actual resurrection/credit is native-tested separately.
void PetBattleAbilityEffect::Kill(uint32 target){PetBattleInstance->Kill(Caster,target,EffectInfo->ID,false,Flags);StopChain=true;}
void PetBattleAbilityEffect::Trigger(uint32,uint32){}
