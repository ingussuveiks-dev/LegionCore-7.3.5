#include "../../src/server/game/PrecompiledHeaders/gamePCH.h"
#include "BattlePetPackets.h"
#include "WorldPacket.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <list>
#include <vector>
namespace Regression {
constexpr uint8 PETBATTLE_NULL_ID=255,MAX_PETBATTLE_TEAM=2,MAX_PETBATTLE_ABILITIES=3,PETBATTLE_TYPE_PVE=0;
#include "NativeStates.inc"
using PetBattleEventUpdateList=std::list<struct PetBattleEventUpdate>;
#include "NativeTypes.inc"
#include "NativeEvents.inc"
constexpr uint32 PETBATTLE_ROUND_RESULT_CATCH_OR_KILL=2,PETBATTLE_EVENT_SET_STATE=6;
struct Team{uint32 ActivePetID=3,ActiveAbilityId=99,activeAbilityTurn=1,activeAbilityTurnMax=2;uint8 GetTeamInputFlags(){return 3;}uint8 GetTeamTrapStatus(){return 4;}};
struct Pet{int32 Cooldowns[3]={-1,-1,-1},Lockdowns[3]={},Abilities[3]={},States[NUM_BATTLEPET_STATES]={};uint8 ID=0,TeamID=1;};
struct PetBattle;
struct Aura{uint32 TargetPetID=3,expired=0;void Expire(PetBattle*){++expired;}};
struct StateInfo{uint32 Flags=1;};struct StateStore{StateInfo entry;StateInfo*LookupEntry(uint32){return &entry;}}sBattlePetStateStore;
struct PetBattle{uint32 BattleType=0,Turn=7,RoundResult=2,RoundTurn=6;Team team;Team*Teams[2]={&team,&team};Pet*Pets[6]={};std::list<Aura*>PetAuras;std::vector<uint8>PetXDied={3};std::list<PetBattleEvent>RoundEvents;
 void SetPetState(uint32,uint32,uint32,uint32,int32,bool=false,uint32=0);void Kill(int8,int8,uint32,bool=false,uint32=0);
};
#include "NativeState.inc"
#include "NativeKill.inc"
struct WorldSession{
 ByteBuffer bytes;uint32 sends=0;
 void SendPacket(WorldPacket const* packet){bytes=static_cast<ByteBuffer const&>(*packet);++sends;}
 void SendPetBattleFirstRound(PetBattle*);void SendPetBattleRoundResult(PetBattle*);void SendPetBattleReplacementMade(PetBattle*);void Baseline(PetBattle*);
};
#include "Converter.inc"
#include "First.inc"
#include "Round.inc"
#include "Replacement.inc"
#define SendPetBattleRoundResult Baseline
#include "Baseline.inc"
#undef SendPetBattleRoundResult
struct Parsed{uint32 ability;uint16 flags,turn;uint8 type;std::vector<int32>values;};
std::vector<Parsed> Parse(ByteBuffer bytes,uint8 time,uint16 expectedAura=9){
 uint32 round,count,abilities;uint8 state;bytes>>round>>state>>count;assert(round==7&&state==2);
 for(int i=0;i<2;++i){uint8 input,trap;uint16 timer;bytes>>input>>trap>>timer;assert(input==3&&trap==4&&timer==time);}
 bytes>>abilities;assert(!abilities);auto deaths=bytes.ReadBits(3);assert(deaths==1);
 std::vector<Parsed> result;
 for(uint32 i=0;i<count;++i){Parsed p;uint16 aura,turn;uint8 effect,depth;int8 caster;uint32 targets;
  bytes>>p.ability>>p.flags>>aura>>turn>>effect>>caster>>depth>>targets;p.turn=turn;assert(aura==expectedAura&&caster==0&&depth==1);
  for(uint32 t=0;t<targets;++t){p.type=uint8(bytes.ReadBits(3));int8 pet;bytes>>pet;assert(pet==3);
   uint32 n=p.type==PET_BATTLE_EFFECT_TARGET_EX_AURA?4:p.type==PET_BATTLE_EFFECT_TARGET_EX_STATE?2:p.type==PET_BATTLE_EFFECT_TARGET_EX_ABILITY_CHANGE?3:p.type==PET_BATTLE_EFFECT_TARGET_EX_FRONT_PET?0:1;
   for(uint32 j=0;j<n;++j){int32 value;bytes>>value;p.values.push_back(value);}}
  result.push_back(p);
 }
 uint8 dead;bytes>>dead;assert(dead==3&&bytes.rpos()==bytes.size());return result;
}
PetBattleEvent Event(uint32 type,uint32 turn=5){return PetBattleEvent(type,0,0x1000,12345,turn,9,1);}
int main(){
 PetBattle battle;auto health=Event(0);health.UpdateHealth(3,0);auto dead=Event(6,6);dead.UpdateState(3,1,1);battle.RoundEvents={health,dead};
 WorldSession session;session.Baseline(&battle);auto baseline=Parse(session.bytes,0);assert(baseline[1].values==std::vector<int32>({1,0})&&baseline[1].turn==0&&baseline[1].flags==0);
 std::cout<<"COUNTERFACTUAL native baseline packet writes IsDead=0 instead of1 and discards event turn/flags.\n";
 {PetBattle lethal;Pet victim;lethal.Pets[3]=&victim;Aura aura;lethal.PetAuras={&aura};lethal.Kill(0,3,12345,false,0x1000);assert(victim.States[1]==1&&lethal.team.ActiveAbilityId==0&&aura.expired==1&&lethal.RoundEvents.size()==1);lethal.Kill(0,3,12345);assert(lethal.RoundEvents.size()==1);lethal.Pets[3]=nullptr;session.SendPetBattleRoundResult(&lethal);auto wire=Parse(session.bytes,0,0);assert(wire.size()==1&&wire[0].values==std::vector<int32>({1,1}));}
 auto aura=Event(1,7);aura.UpdateBuff(3,41,242,-1,3);
 auto revive=Event(6,8);revive.UpdateState(3,1,0);
 auto speed=Event(7,9);speed.UpdateSpeed(3,273);
 auto trigger=Event(4,10);trigger.Trigger(3,723);
 auto front=Event(4,11);front.UpdateFrontPet(3);
 auto onehp=Event(0,12);onehp.UpdateHealth(3,1);
 auto state=Event(6,13);state.UpdateState(3,42,-7);
 auto emote=Event(8,14);PetBattleEventUpdate e{};e.UpdateType=PET_BATTLE_EFFECT_TARGET_EX_NPC_EMOTE;e.TargetPetID=3;e.NpcEmote.BroadcastTextID=987;emote.Updates.push_back(e);
 auto change=Event(9,15);e.UpdateType=PET_BATTLE_EFFECT_TARGET_EX_ABILITY_CHANGE;change.Updates.push_back(e);
 battle.RoundEvents={health,dead,aura,revive,speed,trigger,front,onehp,state,emote,change};
 uint32 cases=0;
 for(uint32 pvp:{0u,1u,2u})for(auto send:{&WorldSession::SendPetBattleFirstRound,&WorldSession::SendPetBattleRoundResult,&WorldSession::SendPetBattleReplacementMade}){
  battle.BattleType=pvp;(session.*send)(&battle);auto out=Parse(session.bytes,pvp?30:0);assert(out.size()==11);
  assert(out[1].values==std::vector<int32>({1,1})&&out[1].turn==6&&out[1].flags==0x1000);
  assert(out[2].values==std::vector<int32>({41,242,-1,3}));assert(out[3].values==std::vector<int32>({1,0})&&out[3].turn==8);
  assert(out[4].values==std::vector<int32>({273}));assert(out[5].values==std::vector<int32>({723}));assert(out[6].values.empty());
  assert(out[7].values==std::vector<int32>({1})&&out[7].flags==0x1000&&out[7].turn==12);
  assert(out[8].values==std::vector<int32>({42,-7}));assert(out[9].values==std::vector<int32>({987}));assert(out[10].values==std::vector<int32>({0,0,0}));
  ++cases;
 }
 auto mixed=Event(6,16);mixed.UpdateState(3,1,1).UpdateHealth(3,1);auto mapped=BuildPetBattleRoundEffect(mixed);assert(mapped.Flags==0x1000&&mapped.TurnInstanceID==16&&mapped.EffectTargetData.front().Params.State.StateValue==1);
 std::cout<<"PASS "<<cases<<" actual first/round/replacement senders and actual7.3.5 BattleRound::Write/ByteBuffer wire decode: death, revive, aura, signed state, 1HP, trigger, speed, emote, empty/ability-change payloads; native metadata/order/PetXDied/PvP clocks preserved. Client animation retest separate.\n";
 return 0;
}
} // Regression
int main(){return Regression::main();}
