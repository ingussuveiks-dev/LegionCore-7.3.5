// Complete actual Freya AI and native Creature maxhealth APIs.
// Elder roster/count and health inputs controlled; no client/raid traversal.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ObjectVisitors.hpp"
#include "ScriptSystem.h"
#include <boost/multiprecision/cpp_int.hpp>
#include "../../src/server/scripts/Northrend/Ulduar/Ulduar/ulduar.h"
namespace FreyaUnderTest {
#include "FreyaElderHealth.inc"
}
using namespace FreyaUnderTest;
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sScriptSystemMgr->LoadScriptTexts();
 auto binding=WorldDatabase.Query("SELECT ScriptName FROM creature_template WHERE entry=32906");
 Require(binding&&binding->Fetch()[0].GetString()=="boss_freya","Actual Freya ScriptName binding missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{603,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{603,{}}});
 MapInstanced parent(603,300);InstanceMap map(603,300,9999762,DIFFICULTY_10_N,&parent);
 Actor boss(&map,32906,19502,{2345.0f,-52.0f,425.0f,0.0f});
 Require(map.AddToMap(static_cast<Creature*>(&boss)),"Actual Freya insertion");
 auto ai=new boss_freya::boss_freyaAI(&boss);Require(boss.AIM_Initialize(ai),"Complete Freya AI installation");
 uint32 cases=0,failures=0;uint64 const maximum=std::numeric_limits<uint64>::max();
 for(uint8 elders:{uint8(0),uint8(1),uint8(2),uint8(3)}){
  std::vector<uint64> inputs={1,4,5,6,99,100,101,4294967295ull,4294967296ull,1099511627793ull,maximum/60,maximum/60+1,maximum/2,maximum-1,maximum};
  uint64 boundary=uint64(boost::multiprecision::uint128_t(maximum)*5/(5+elders));
  inputs.push_back(boundary);if(boundary<maximum)inputs.push_back(boundary+1);
  for(uint64 health:inputs){
   boss.SetMaxHealth(health);boss.SetFullHealth();boss.ResetPlayerDamageReq();ai->EldersCount=elders;
   ai->EnterCombat(nullptr);
   boost::multiprecision::uint128_t exact=boost::multiprecision::uint128_t(health)+boost::multiprecision::uint128_t(health)*elders*20/100;
   uint64 expected=exact>maximum?maximum:uint64(exact);
#ifdef FREYA_ELDER_HEALTH_BASELINE
   if(boss.GetMaxHealth()!=expected)++failures;
#else
   Require(boss.GetMaxHealth()==expected&&boss.GetHealth()==expected,"Freya must preserve exact native64 authored20percent or saturate representable range");
   Require(boss.IsDamageEnoughForLootingAndReward()==(elders!=0),"Native elder branch loot eligibility changed");
#endif
   ++cases;boss.CastStop();boss.RemoveAllAuras();boss.m_Events.KillAllEvents(true);ai->Reset();
  }
 }
 map.RemoveFromMap(static_cast<Creature*>(&boss),false);CloseAltheaNative();
#ifdef FREYA_ELDER_HEALTH_BASELINE
 Require(failures>0,"Pinned Freya multiply/add overflow did not reproduce");std::cout<<"PASS expected-broken Freya actual AI/native Creature: "<<failures<<" of "<<cases<<" authored health calculations overflow\n";
#else
 std::cout<<"PASS native complete Freya AI: "<<cases<<" controlled health/elder inputs,0..3elders,independent128bit reference,exact integer rounding,overflow saturation,fullhealth and existing shared loot eligibility;raid combat/client separate\n";
#endif
 return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
