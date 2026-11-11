// Actual aura absorption,triggered105992and native health;no character writes.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
enum { SPELL_TIMELOOP_HEAL=105992 };
#include "TimeLoopScript.inc"
struct LoopScriptsTag{using Type=std::vector<AuraScript*> Aura::*;friend Type Access(LoopScriptsTag);};
template struct FixtureAccess<LoopScriptsTag,&Aura::m_loadedScripts>;
static void BindLoop(Player&player){
 static std::string name="spell_ultraxion_time_loop";
 Aura*aura=player.AddAura(105984,&player);Require(aura&&aura->GetEffect(EFFECT_1),"Actual TimeLoop aura absent");
 auto*script=new spell_ultraxion_time_loop::spell_ultraxion_time_loop_AuraScript;
 script->_Init(&name,105984);Require(script->_Load(aura),"TimeLoop rejected Player");script->_Register();
 auto&loaded=aura->*Access(LoopScriptsTag{});Require(loaded.empty(),"Unexpected automatic TimeLoop binding");loaded.push_back(script);
 float amount=100;bool recalc=false;aura->CallScriptEffectCalcAmountHandlers(aura->GetEffect(EFFECT_1),amount,recalc);Require(amount==-1,"Actual infinite absorb hook not dispatched");aura->GetEffect(EFFECT_1)->SetAmount(amount);
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999771,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer player(session,map,997,{50,803,805,0});Actor attacker(&map,299,24001,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&attacker)),"Attacker admission failed");
 for(auto sample:{std::array<uint64,3>{1000,500,499},std::array<uint64,3>{1000,500,500},std::array<uint64,3>{4000000000ull,4000000000ull,1},std::array<uint64,3>{4000000000ull,500,500},std::array<uint64,3>{1099511627793ull,500,500}}){
  player.RemoveAllAuras();player.SetMaxHealth(sample[0]);player.SetHealth(sample[1]);BindLoop(player);
  uint32 absorb=0,resist=0;attacker.CalcAbsorbResist(&player,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,uint32(sample[2]),&absorb,&resist);
  player.m_Events.Update(10);
  std::cout<<"TIMELOOP maximum="<<sample[0]<<" health="<<sample[1]<<" incoming="<<sample[2]<<" absorb="<<absorb<<" resist="<<resist<<" after="<<player.GetHealth()<<" aura="<<player.HasAura(105984)<<std::endl;
  bool lethal=sample[2]>=sample[1];
#ifdef TIME_LOOP_BASELINE
  if(sample[0]==4000000000ull&&sample[2]==1)Require(absorb==1&&!player.HasAura(105984),"Pinned false consumption absent");
#else
  Require(resist==0&&absorb==(lethal?sample[2]:0)&&player.HasAura(105984)==!lethal,"Actual TimeLoop consume/absorb decision differs from lethal boundary");
  Require(player.GetHealth()==(lethal?sample[0]:sample[1]),"Actual105992full heal changed");
#endif
 }
#ifndef TIME_LOOP_BASELINE
 uint32 boundaries=0;
 for(uint64 maximum:{1000ull,2147483647ull,2147483648ull,4000000000ull,4294967295ull,1099511627793ull,9223372036854775808ull,18446744073709551615ull})for(uint32 hit:{0u,1u,499u}){
  player.RemoveAllAuras();player.SetMaxHealth(maximum);player.SetFullHealth();BindLoop(player);uint32 absorb=0,resist=0;
  attacker.CalcAbsorbResist(&player,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,hit,&absorb,&resist);
  Require(!absorb&&!resist&&player.GetHealth()==maximum&&player.HasAura(105984),"Nonlethal wide health consumed TimeLoop");
  attacker.CalcAbsorbResist(&player,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,hit,&absorb,&resist);
  Require(!absorb&&player.HasAura(105984),"Repeated nonlethal event consumed TimeLoop");++boundaries;
 }
 std::cout<<"PASS "<<boundaries<<" native wide nonlethal/repeat boundaries throughUINT64_MAX;ordinary lethal105992heal retained\n";
#endif
 player.RemoveAllAuras();player.CombatStop(true);attacker.CombatStop(true);
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();
#ifdef TIME_LOOP_BASELINE
 std::cout<<"PASS expected-broken pinned TimeLoop false-consumption counterfactual;not repaired full-heal proof\n";
#else
 std::cout<<"PASS native TimeLoop unsigned decision,actual full105992heal and one-use aura;full modifier/absorb matrix is Test-TimeLoopHealing;client remains separate\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
