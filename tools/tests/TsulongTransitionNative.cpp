// Full production boss class and native Creature health/phase APIs.
// Entrance phase/poses and incoming hit/heal endpoints controlled; no client.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ObjectVisitors.hpp"
#include "../../src/server/scripts/Pandaria/TerraceEndlessSpring/terrace_of_endless_spring.h"
namespace TsulongUnderTest {
#include "TsulongTransition.inc"
}
using namespace TsulongUnderTest;
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sCreatureTextMgr->LoadCreatureTexts();
 auto binding=WorldDatabase.Query((std::string("SELECT ScriptName FROM creature_template WHERE entry=")+std::to_string(NPC_TSULONG_BOSS)).c_str());
 Require(binding&&binding->Fetch()[0].GetString()=="boss_tsulong","Actual Tsulong ScriptName binding missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{996,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{996,{}}});
 MapInstanced parent(996,300);InstanceMap map(996,300,9999761,DIFFICULTY_10_N,&parent);
 Actor boss(&map,NPC_TSULONG_BOSS,19501,{-1017.841f,-3049.621f,12.823f,4.72f});
 Require(map.AddToMap(static_cast<Creature*>(&boss)),"Actual Tsulong insertion");
 auto ai=new boss_tsulong::boss_tsulongAI(&boss);Require(boss.AIM_Initialize(ai),"Full Tsulong AI installation");
 uint32 cases=0;
 for(uint64 maximum:{1000ull,4294967296ull,10000000000ull,1099511627793ull}){
  for(uint64 health:{1ull,maximum/4,maximum/2,maximum-1,maximum}){
   boss.SetMaxHealth(maximum);boss.SetHealth(health);ai->phase=PHASE_NIGHT;
   uint32 damage=1;ai->DamageTaken(nullptr,damage,DIRECT_DAMAGE);
   // Native damage is applied after DamageTaken. Deliberately make the last
   // hit occur after the old cache snapshot, before the phase action.
   uint64 actual=health>1?health-1:health;boss.SetHealth(actual);
   ai->DoAction(PHASE_DAY_ACTIVATION);
#ifdef TSULONG_TRANSITION_BASELINE
   if(maximum==1000&&health==maximum/2){Require(boss.GetHealth()!=maximum-actual,"Pinned stale last-hit cache stopped reproducing");++cases;}
   if(maximum==10000000000ull&&health==1){Require(boss.GetHealth()!=maximum-actual,"Pinned signed32 complement stopped reproducing");++cases;}
#else
   Require(ai->phase==PHASE_DAY&&boss.GetHealth()==maximum-actual,"Night/day must use exact current native64 health");
   auto first=boss.GetHealth();ai->DoAction(PHASE_DAY_ACTIVATION);Require(boss.GetHealth()==first,"Duplicate day action inverted twice");
   // A heal after the prior day tick must survive in the night complement.
   uint64 healed=maximum/2+1;boss.SetHealth(healed);ai->DoAction(PHASE_NIGHT_ACTIVATION);
   Require(ai->phase==PHASE_NIGHT&&boss.GetHealth()==maximum-healed,"Day/night omitted latest heal or narrowed health");
   first=boss.GetHealth();ai->DoAction(PHASE_NIGHT_ACTIVATION);Require(boss.GetHealth()==first,"Duplicate night action inverted twice");
   Require(boss.getFaction()==16&&!boss.HasUnitMovementFlag(MOVEMENTFLAG_ROOT),"Original night faction/movement cleanup changed");
   ++cases;
#endif
   boss.CastStop();boss.RemoveAllAuras();boss.m_Events.KillAllEvents(true);
  }
 }
#ifndef TSULONG_TRANSITION_BASELINE
 ai->Reset();boss.SetMaxHealth(1000);boss.SetHealth(1000);
 ai->DoAction(PHASE_DAY_ACTIVATION);ai->DoAction(PHASE_NIGHT_ACTIVATION);
 Require(ai->phase==PHASE_NONE&&boss.GetHealth()==1000,"Invalid/reset phase consumed health inversion");
 ai->phase=PHASE_NIGHT;ai->DoAction(PHASE_DAY_ACTIVATION);
 Require(ai->phase==PHASE_DAY&&boss.GetHealth()==0&&boss.IsAlive(),"No-hit first transition must preserve authored zero complement without death");
 boss.SetHealth(400);ai->DoAction(PHASE_NIGHT_ACTIVATION);Require(boss.GetHealth()==600,"No-hit first cycle cannot recover");
 boss.CastStop();boss.RemoveAllAuras();boss.m_Events.KillAllEvents(true);
#endif
 map.RemoveFromMap(static_cast<Creature*>(&boss),false);CloseAltheaNative();
#ifdef TSULONG_TRANSITION_BASELINE
 Require(cases==2,"Expected both historical counterfactuals");std::cout<<"PASS expected-broken Tsulong: actual class/native Creature reproduces stale final hit and signed32 complement\n";
#else
 std::cout<<"PASS native complete Tsulong AI:20 health ranges/two phases,current final hit/heal,64bit complements,duplicate actions,original faction/root cleanup;no loot/credit/player writes. Full combat/client separate\n";
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
