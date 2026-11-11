// Real aura -> custom cast -> native effect/modifiers/crit/absorb/proc/health.
// Only poses, health, crit stat and absorb capacities are controlled; no saves.
#define main RetainedTimeLoopDecisionMain
#include "TimeLoopNative.cpp"
#undef main
#include "HealingAmount.h"
#include "BattlegroundScore.h"
static uint32 hits=0, procObservations=0;
static uint64 hitGain=0, procAmount=0, procOverheal=0;
static bool procCrit=false;
struct NativeHealScore final:BattlegroundScore {
 NativeHealScore(ObjectGuid guid):BattlegroundScore(guid,TEAM_ALLIANCE){}
 using BattlegroundScore::UpdateScore;
 void BuildObjectivesBlock(std::vector<int32>&)override{}
};
class time_loop_heal_observer final: public SpellScriptLoader {
public: time_loop_heal_observer():SpellScriptLoader("test_time_loop_heal_observer"){}
 class Script final: public SpellScript {
  PrepareSpellScript(Script);
  void Observe(){++hits;hitGain=GetHitHeal64();}
  void Register()override{AfterHit+=SpellHitFn(Script::Observe);}
 };
 SpellScript* GetSpellScript()const override{return new Script;}
};
class LoopProcObserver final:public AuraScript {
 PrepareAuraScript(LoopProcObserver);
 bool Observe(ProcEventInfo& event){
  if(event.GetDamageInfo()&&event.GetDamageInfo()->GetSpellInfo()&&event.GetDamageInfo()->GetSpellInfo()->Id==105992){
   auto damage=event.GetDamageInfo();auto heal=event.GetHealInfo();
   Require(damage&&heal&&damage->GetDamage()==heal->GetHeal(),"Native proc records disagree");
   ++procObservations;procAmount=damage->GetDamage();procOverheal=damage->GetOverHeal();
   procCrit=bool(event.GetHitMask()&PROC_EX_CRITICAL_HIT);
  }
  // This observes the actual pre-admission hook; it never grants a proc.
  return false;
 }
 void Register()override{DoCheckProc+=AuraCheckProcFn(LoopProcObserver::Observe);}
};
static void BindProcObserver(Player& player){
 static std::string name="test_time_loop_proc_observer";
 auto aura=player.AddAura(59915,&player);Require(aura,"Native proc aura missing");
 Require(aura->GetSpellInfo()->GetAuraOptions(player.GetSpawnMode())->IsProcAura,"Observer must use a native proc aura");
 auto script=new LoopProcObserver;script->_Init(&name,59915);Require(script->_Load(aura),"Proc observer load");script->_Register();
 auto& loaded=aura->*Access(LoopScriptsTag{});Require(loaded.empty(),"Unexpected automatic proc binding");loaded.push_back(script);
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");Shadow("spell_script_names","spell_id=105992");
 WorldDatabase.DirectExecute("DELETE FROM spell_script_names WHERE spell_id=105992");WorldDatabase.DirectExecute("INSERT INTO spell_script_names VALUES(105992,'test_time_loop_heal_observer')");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 auto& names=const_cast<ObjectMgr::ScriptNameContainer&>(sObjectMgr->GetAllScriptNames());names.emplace_back("test_time_loop_heal_observer");std::sort(names.begin(),names.end());
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new time_loop_heal_observer;sScriptMgr->SwapScriptContext(true);sScriptDataStore->LoadSpellScriptNames();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999773,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer player(session,map,996,{50,803,805,0});Actor attacker(&map,299,24002,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&attacker)),"Attacker admission failed");
 auto spell=sSpellMgr->GetSpellInfo(105992);Require(spell&&spell->GetEffect(EFFECT_0,map.GetDifficultyID())->Effect==SPELL_EFFECT_HEAL&&spell->HasAttribute(SPELL_ATTR3_NO_DONE_BONUS),"Native fixed-heal profile changed");
 uint32 shieldId=0;for(uint32 id=1;id<sSpellMgr->GetSpellInfoStoreSize()&&!shieldId;++id)if(auto info=sSpellMgr->GetSpellInfo(id))if(info->GetEffect(EFFECT_0,map.GetDifficultyID())->ApplyAuraName==SPELL_AURA_SCHOOL_HEAL_ABSORB)shieldId=id;
 Require(shieldId,"Native healing absorption missing");uint32 cases=0;
 for(uint64 maximum:{1000ull,2147483647ull,2147483648ull,4000000000ull,4294967295ull,1099511627793ull,9223372036854775808ull,18446744073709551615ull})
 for(float healingPct:{-50.f,0.f,1.f,25.f})for(bool crit:{false,true})for(float shield:{0.f,100.f,2147483648.f,18446744073709551616.f}){
  player.RemoveAllAuras();player.SetMaxHealth(maximum);player.SetHealth(500);player.SetFloatValue(PLAYER_FIELD_SPELL_CRIT_PERCENTAGE,crit?100.f:0.f);BindLoop(player);BindProcObserver(player);
  auto effect=player.GetAuraEffect(105984,EFFECT_0);Require(effect&&effect->GetAuraType()==SPELL_AURA_MOD_HEALING_PCT,"Parent healing-taken modifier missing");effect->SetAmount(healingPct);
  if(shield){auto aura=player.AddAura(shieldId,&player);Require(aura&&aura->GetEffect(EFFECT_0),"Healing shield missing");aura->GetEffect(EFFECT_0)->SetAmount(shield);}
  uint64 amount=player.SpellHealingBonusTaken64(&player,spell,maximum,HEAL,EFFECT_0);
  if(crit)amount=player.SpellCriticalHealingBonus64(spell,amount);
  uint64 absorbed=std::min(amount,HealingAmount::FromFloat(shield));uint64 gain=std::min(amount-absorbed,maximum-500);
  hits=procObservations=0;hitGain=procAmount=procOverheal=0;procCrit=false;uint32 absorb=0,resist=0;
  attacker.CalcAbsorbResist(&player,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,500,&absorb,&resist);player.m_Events.Update(10);
#ifdef TIME_LOOP_HEAL_BASELINE
  if(maximum==4000000000ull&&healingPct==0.f&&!crit&&!shield){Require(player.GetHealth()==500,"Pinned negative float payload no longer reproduces");std::cout<<"COUNTERFACTUAL full-heal maximum="<<maximum<<" after="<<player.GetHealth()<<std::endl;}
#else
  if(player.GetHealth()!=500+gain)std::cout<<"FAILED_HEAL max="<<maximum<<" pct="<<healingPct<<" crit="<<crit<<" shield="<<shield<<" expected="<<500+gain<<" actual="<<player.GetHealth()<<std::endl;
  Require(absorb==500&&!resist&&!player.HasAura(105984)&&player.GetHealth()==500+gain,"Full native TimeLoop amount/consumption mismatch");
  Require(hits==1&&hitGain==gain,"Native AfterHit full-width gain or exactly-once mismatch");
  if(amount){Require(procObservations&&procAmount==amount&&procOverheal==amount-gain&&procCrit==crit,"Actual healing proc payload lost full width or crit policy");}
#endif
  auto before=player.GetHealth();auto priorHits=hits;absorb=resist=0;attacker.CalcAbsorbResist(&player,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,500,&absorb,&resist);player.m_Events.Update(10);
  Require(!absorb&&!resist&&hits==priorHits&&player.GetHealth()==before,"Consumed TimeLoop recast or absorbed a second hit");++cases;
 }
 // Direct exact payload must retain low bits without the parent 1% modifier.
#ifndef TIME_LOOP_HEAL_BASELINE
 for(uint64 amount:{0ull,1ull,2147483648ull,4294967295ull,1099511627793ull,9223372036854775808ull,18446744073709551615ull}){
  player.RemoveAllAuras();player.SetMaxHealth(UINT64_MAX);player.SetHealth(1);player.SetFloatValue(PLAYER_FIELD_SPELL_CRIT_PERCENTAGE,0.f);hits=0;
  CustomSpellValues values;values.AddHealingMod(EFFECT_0,amount);player.CastCustomSpell(105992,values,&player,true);player.m_Events.Update(10);
  Require(player.GetHealth()==1+std::min<uint64>(amount,UINT64_MAX-1)&&hits==1,"Native identity payload rounded/lost or cast duplicated");
 }
 // Real proc/absorption records and score accumulation retain unsigned64.
 DamageInfo record(&player,&player,UINT64_MAX,spell,SPELL_SCHOOL_MASK_HOLY,SPELL_DIRECT_DAMAGE,0,true);record.AbsorbHealing(1099511627793ull);Require(record.GetDamage()==UINT64_MAX-1099511627793ull&&record.GetAbsorb()==1099511627793ull,"Wide native absorption record narrowed");
 HealInfo heal(&player,&player,UINT64_MAX,spell,SPELL_SCHOOL_MASK_HOLY);heal.AbsorbHeal(1099511627793ull);Require(heal.GetHeal()==UINT64_MAX-1099511627793ull,"Wide HealInfo narrowed");
 NativeHealScore score(player.GetGUID());score.UpdateScore(SCORE_HEALING_DONE,1099511627793ull);Require(score.GetScore(SCORE_HEALING_DONE)==1099511627793ull,"Native BG healing score narrowed");score.UpdateScore(SCORE_HEALING_DONE,UINT64_MAX);Require(score.GetScore(SCORE_HEALING_DONE)==UINT64_MAX,"Native BG healing score wrapped");
#endif
 player.RemoveAllAuras();player.CombatStop(true);attacker.CombatStop(true);
#ifdef TIME_LOOP_HEAL_BASELINE
 std::cout<<"PASS expected-broken pinned4d61 TimeLoop payload:4billion maximum remains500HP;other contexts inspected,not certified successful healing\n";
#else
 std::cout<<"PASS "<<cases<<" actual TimeLoop heal contexts:unsigned64 health,healing-taken modifiers,forced native crit/noncrit,partial/full absorb,AfterHit gain,pre-admission proc records and once-only consumption;7 exact identity casts and wide BG counters\n";
#endif
 }session->SetPlayer(nullptr);delete session;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
