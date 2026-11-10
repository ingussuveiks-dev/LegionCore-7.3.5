#include <map>
#include <unordered_map>
#include "ObjectGuid.h"
#include "MovementPackets.h"
#ifdef LOOT_COUNTER_BASELINE
#include "PinnedLootCreature.h"
#else
#include "Creature.h"
#endif
#include <Windows.h>
#include <iostream>
#include <limits>
#include <type_traits>
static void Require(bool value,char const* text){if(!value){std::cerr<<text<<'\n';std::exit(1);}}
int main(){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 static_assert(std::is_same<decltype(Creature::m_PlayerDamageReq),uint32>::value,"Native counter layout must stay unchanged");
 Creature actor;Require(!actor.IsDamageEnoughForLootingAndReward(),"Fresh real Creature eligibility must be false");
 std::cout<<"Native counter offset="<<(reinterpret_cast<char*>(&actor.m_PlayerDamageReq)-reinterpret_cast<char*>(&actor))<<'\n';
#ifdef LOOT_COUNTER_BASELINE
 static_assert(std::is_same<decltype(&Creature::LowerPlayerDamageReq),void(Creature::*)(uint32)>::value,"Pinned original API required");
 actor.LowerPlayerDamageReq(uint32(uint64(1)<<32));Require(!actor.IsDamageEnoughForLootingAndReward(),"Original Freya-sized argument no longer truncates");
 actor.ResetPlayerDamageReq();actor.LowerPlayerDamageReq(UINT32_MAX);actor.LowerPlayerDamageReq(1);Require(!actor.IsDamageEnoughForLootingAndReward(),"Original accumulated damage no longer wraps to zero");
 std::cout<<"PASS expected-broken real Creature:64bit positive argument truncates and accumulated32bit damage wraps eligibility to false;size="<<sizeof(Creature)<<'\n';
#else
 static_assert(std::is_same<decltype(&Creature::LowerPlayerDamageReq),void(Creature::*)(uint64)>::value,"Full-width native damage API required");
 actor.LowerPlayerDamageReq(0);Require(!actor.IsDamageEnoughForLootingAndReward(),"Zero damage granted eligibility");
 actor.LowerPlayerDamageReq(73);actor.LowerPlayerDamageReq(9);Require(actor.m_PlayerDamageReq==82&&actor.IsDamageEnoughForLootingAndReward(),"Ordinary native accumulation changed");
 actor.ResetPlayerDamageReq();Require(!actor.IsDamageEnoughForLootingAndReward(),"Native reset did not retire eligibility");
 for(uint64 value:{1ull,2147483648ull,4294967295ull,4294967296ull,4294967297ull,1099511627793ull,18446744073709551615ull}){
  actor.ResetPlayerDamageReq();actor.LowerPlayerDamageReq(value);Require(actor.IsDamageEnoughForLootingAndReward(),"Positive full-width damage lost eligibility");
  Require(actor.m_PlayerDamageReq==std::min<uint64>(value,UINT32_MAX),"Initial damage did not preserve exact/saturated sum");
  auto previous=actor.m_PlayerDamageReq;for(uint64 more:{1ull,4294967296ull,18446744073709551615ull}){
   auto expected=uint32(uint64(previous)+std::min<uint64>(more,uint64(UINT32_MAX)-previous));
   actor.LowerPlayerDamageReq(more);Require(actor.IsDamageEnoughForLootingAndReward()&&actor.m_PlayerDamageReq==expected,"Later damage changed native eligibility or exact/saturated sum");previous=actor.m_PlayerDamageReq;
  }
 }
 actor.ResetPlayerDamageReq();Require(!actor.IsDamageEnoughForLootingAndReward(),"Reuse/reset retains prior credit");
 std::cout<<"PASS actual Creature constructor/native eligibility API:zero,ordinary exact sum,seven wide boundaries,21later positive updates and reset;no DB,loot grants,players or synthetic counter;size="<<sizeof(Creature)<<'\n';
#endif
}
