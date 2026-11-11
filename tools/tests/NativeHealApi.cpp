// Full native32 amount range against unsigned64health;no character saves.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
#include "NativeNiuzao.inc"
struct HealAuraScriptsTag{using Type=std::vector<AuraScript*> Aura::*;friend Type Access(HealAuraScriptsTag);};
template struct FixtureAccess<HealAuraScriptsTag,&Aura::m_loadedScripts>;
struct HealObserver:NullCreatureAI{
 using NullCreatureAI::NullCreatureAI;uint32 calls=0;uint64 amount=0,gain=0;
 void OnHealApplied(Unit*,uint64 finalAmount,uint64 actualGain)override{++calls;amount=finalAmount;gain=actualGain;}
};
struct HealInstanceTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(HealInstanceTag);};
template struct FixtureAccess<HealInstanceTag,&InstanceMap::i_data>;
struct HealBeforeActor:Actor{
 using Actor::Actor;
#include "NativeHealApiBefore.inc"
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 Require(sObjectMgr->LoadTrinityStrings(),"Native startup strings missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();
 sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999772,DIFFICULTY_N_SCENARIO,&parent);
 HealBeforeActor healer(&map,299,25001,{50,803,805,0});Actor target(&map,299,25002,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&healer))&&map.AddToMap(static_cast<Creature*>(&target)),"Heal actors not admitted");
 Require(target.AIM_Initialize(new HealObserver(&target)),"Native heal observer absent");auto observer=static_cast<HealObserver*>(target.AI());uint32 cases=0;
 for(uint64 maximum:{1000ull,4294967295ull,1099511627793ull,18446744073709551615ull})for(uint64 start:{1ull,500ull,maximum-1,maximum})for(uint64 request:{0ull,1ull,100ull,2147483647ull,2147483648ull,4294967295ull,1099511627793ull,9223372036854775808ull,18446744073709551615ull}){
  target.SetMaxHealth(maximum);target.SetHealth(start);observer->calls=0;observer->amount=0;observer->gain=0;
  auto expected=std::min<uint64>(request,maximum-start);
#ifdef NATIVE_HEAL_BASELINE
  auto gain=healer.BeforeDealHeal(&target,request,nullptr);
#else
  auto gain=healer.DealHeal(&target,request,nullptr);
#endif
#ifdef NATIVE_HEAL_BASELINE
  if(maximum==1099511627793ull&&start==500&&request==4294967295u){Require(gain<0&&target.GetHealth()==499&&!observer->calls,"Original negative heal counterfactual disappeared");std::cout<<"COUNTERFACTUAL native request="<<request<<" gain="<<gain<<" health="<<target.GetHealth()<<" callbacks="<<observer->calls<<std::endl;}
#else
  Require(gain==expected&&target.GetHealth()==start+expected&&target.IsAlive(),"Native heal changed sign,lost health or returned narrowed gain");
  Require(observer->calls==(request?1:0)&&(!request||(observer->amount==request&&observer->gain==expected)),"Final native heal callback lost full-width gain");
#endif
  ++cases;
 }
#ifndef NATIVE_HEAL_BASELINE
 {
  DamageInfo physical(&healer,&target,1,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);
  physical.AbsorbDamage(1);physical.ModifyDamage(UINT32_MAX);physical.AbsorbDamage(-1);
#ifdef HEALING_PHYSICAL_UNDO_BASELINE
  Require(physical.GetDamage()==4294967296ull,"Expected unsigned64migration physical undo counterfactual absent");
  std::cout<<"COUNTERFACTUAL physical undo crosses native32domain after capped positive modification\n";
#else
  Require(physical.GetDamage()==UINT32_MAX&&physical.GetAbsorb()==1,"Physical undo crossed native32domain");
  DamageInfo bounded(&healer,&target,UINT64_MAX,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);
  Require(bounded.GetDamage()==UINT32_MAX,"Default physical constructor crossed native32domain");
  DamageInfo healing(&healer,&target,UINT64_MAX,nullptr,SPELL_SCHOOL_MASK_HOLY,SPELL_DIRECT_DAMAGE,0,true);
  Require(healing.GetDamage()==UINT64_MAX,"Healing record lost full64domain");
#endif
 }
 uint32 absorbs=0;
 for(uint32 incoming:{0u,1u,2147483647u,2147483648u,4294967295u})for(int64 shield:{INT64_MIN,int64(-1),int64(0),int64(1),int64(2147483647),int64(2147483648),int64(4294967295),INT64_MAX}){
  DamageInfo info(&healer,&target,incoming,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);info.AbsorbDamage(shield);
  uint32 expected=uint32(std::min<uint64>(incoming,shield>0?uint64(shield):0));Require(info.GetAbsorb()==expected&&info.GetDamage()==incoming-expected,"Native damage absorption changed sign/truncated");++absorbs;
 }
 for(uint32 incoming:{1u,2147483647u,2147483648u,4294967295u})for(int64 undo:{int64(-1),int64(-2147483648ll),int64(-4294967295ll),INT64_MIN}){
  DamageInfo info(&healer,&target,incoming,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);info.AbsorbDamage(incoming);info.AbsorbDamage(undo);
  uint32 restored=uint32(std::min<uint64>(incoming,uint64(-(undo+1))+1));Require(info.GetDamage()==restored&&info.GetAbsorb()==incoming-restored,"Native rejected absorption undo lost full32range");++absorbs;
 }
 auto binding=WorldDatabase.Query("SELECT spell_id FROM spell_script_names WHERE ScriptName='spell_endurance_of_niuzao'");Require(bool(binding),"Native Niuzao binding absent");auto niuzao=binding->Fetch()[0].GetUInt32();auto session=Session();{
  NativePlayer player(session,map,998,{53,803,805,0});player.SetMaxHealth(1000);player.SetFullHealth();static std::string name="spell_endurance_of_niuzao";
  for(uint32 incoming:{1u,2147483648u,4294967295u}){
   player.RemoveAllAuras();auto aura=player.AddAura(niuzao,&player);Require(aura&&aura->GetEffect(EFFECT_0),"Native Niuzao aura absent");auto script=new spell_endurance_of_niuzao::spell_endurance_of_niuzao_AuraScript;
   script->_Init(&name,niuzao);Require(script->_Load(aura),"Native Niuzao load refused");script->_Register();auto&loaded=aura->*Access(HealAuraScriptsTag{});Require(loaded.empty(),"Unexpected automatic Niuzao script");loaded.push_back(script);
   DamageInfo info(&healer,&player,incoming,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);info.AbsorbDamage(incoming);float amount=float(incoming);
   aura->CallScriptEffectAfterAbsorbHandlers(aura->GetEffect(EFFECT_0),aura->GetApplicationOfTarget(player.GetGUID()),info,amount);
   Require(info.GetDamage()==incoming&&!info.GetAbsorb(),"Actual Niuzao rejection did not restore full32damage");
  }
  player.RemoveAllAuras();
 }session->SetPlayer(nullptr);delete session;std::cout<<"PASS actual Niuzao after-absorb rejection callback:3native boundaries throughUINT32_MAX\n";
 uint32 shieldId=0;
 for(uint32 id=1;id<sSpellMgr->GetSpellInfoStoreSize()&&!shieldId;++id)if(auto info=sSpellMgr->GetSpellInfo(id))
  if(info->GetEffect(EFFECT_0,map.GetDifficultyID())->ApplyAuraName==SPELL_AURA_SCHOOL_HEAL_ABSORB)shieldId=id;
 Require(shieldId,"Matching heal-absorb aura absent");std::cout<<"NATIVE_HEAL_ABSORB id="<<shieldId<<std::endl;
 auto spell=sSpellMgr->GetSpellInfo(105992);Require(spell,"Actual healing spell absent");uint32 healAbsorbs=0;
 for(uint32 amount:{1u,100u,2147483647u,2147483648u,4294967295u})for(float capacity:{0.f,1.f,100.f,2147483648.f,4294967296.f}){
  target.RemoveAllAuras();auto aura=healer.AddAura(shieldId,&target);Require(aura&&aura->GetEffect(EFFECT_0),"Actual heal-absorb application failed");aura->GetEffect(EFFECT_0)->SetAmount(capacity);
  uint64 available=amount,absorbed=0;healer.CalcHealAbsorb(&target,spell,available,absorbed);
  uint32 expected=uint32(std::min<double>(amount,capacity));Require(available==amount-expected&&absorbed==expected,"Native heal absorption lost signed/float remaining amount");++healAbsorbs;
 }
 target.RemoveAllAuras();target.SetMaxHealth(1099511627793ull);target.SetHealth(500);observer->calls=0;
 Require(healer.HealBySpell(&target,spell,4294967295u)==4294967295ll&&target.GetHealth()==4294967795ull&&observer->gain==4294967295ll,"Actual HealBySpell narrowed positive gain");
 auto state=new InstanceScript(&map);map.*Access(HealInstanceTag{})=state;
 Actor peer(&map,299,25003,{52,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&peer)),"Shared peer admission failed");state->AddToDamageManager(&target);state->AddToDamageManager(&peer);
 uint32 shared=0;
 for(uint64 maximum:{1000ull,1099511627793ull,18446744073709551615ull})for(uint64 request:{1ull,100ull,2147483648ull,4294967295ull,1099511627793ull,18446744073709551615ull}){
  target.SetMaxHealth(maximum);target.SetHealth(500);peer.SetMaxHealth(maximum);peer.SetHealth(maximum-1);auto expected=std::min<uint64>(request,maximum-500);
  Require(healer.DealHeal(&target,request)==expected&&target.GetHealth()==500+expected&&peer.GetHealth()==maximum,"Actual shared heal changed sign/overflowed or lost caller skip");++shared;
 }
 peer.CombatStop(true);std::cout<<"PASS native absorption="<<absorbs<<" heal-absorb="<<healAbsorbs<<" shared-heal="<<shared<<" and actualUINT32_MAXHealBySpell exact gain\n";
#endif
 healer.CombatStop(true);target.CombatStop(true);CloseAltheaNative();std::cout<<"PASS "<<cases<<
#ifdef NATIVE_HEAL_BASELINE
 " expected-broken native heal inspections,explicitUINT32_MAXbecomesnegative control;not successful repaired healing\n";
#else
 " native DealHeal cases:fulluint64amounts,unsigned64health,exact positive gain,zero/overheal and real final AI callback;not full spell pipeline/client proof\n";
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
