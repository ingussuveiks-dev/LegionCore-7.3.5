// Real core objects and dispatch; controlled bare player profile, no character saves.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
#include "EventObjectData.h"
#include "ConditionMgr.h"
#include "VantusRuneLifecycle.h"
#include <cmath>
#include "FourthValorPredicate.inc"
#include "FourthTenScripts.inc"
#include "FourthVantusTables.inc"
struct FourthListTag{using Type=std::vector<AuraScript*> Aura::*;friend Type Access(FourthListTag);};
template struct FixtureAccess<FourthListTag,&Aura::m_loadedScripts>;
struct FourthInstanceTag{using Type=InstanceScript* InstanceMap::*;friend Type Access(FourthInstanceTag);};
template struct FixtureAccess<FourthInstanceTag,&InstanceMap::i_data>;
struct FourthBossTag{using Type=std::vector<BossInfo> InstanceScript::*;friend Type Access(FourthBossTag);};
template struct FixtureAccess<FourthBossTag,&InstanceScript::bosses>;
template<class T>struct FourthBound{
 T script;Aura* aura;std::string name;
 FourthBound(Unit&caster,Unit&owner,uint32 id,char const*n,bool missing=false):aura(caster.AddAura(id,&owner)),name(n){Require(aura,"Selected aura refused");if(missing)caster.GetMap()->RemoveFromMap(caster.ToCreature(),false);script._Init(&name,id);Require(script._Load(aura),"Load refused");script._Register();auto&list=aura->*Access(FourthListTag{});Require(list.empty(),"Unexpected automatic script");list.push_back(&script);}
 ~FourthBound(){(aura->*Access(FourthListTag{})).clear();}
 float Calc(SpellEffIndex index,float amount){bool recalc=false;Require(aura->GetEffect(index),"Native effect absent");aura->CallScriptEffectCalcAmountHandlers(aura->GetEffect(index),amount,recalc);return amount;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Strings absent");sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 auto bindings=WorldDatabase.Query(
#include "FourthTenBindings.inc"
 );Require(bool(bindings),"Bindings absent");unsigned rows=0;std::set<std::string>names;do{auto f=bindings->Fetch();Require(sSpellMgr->GetSpellInfo(f[0].GetUInt32()),"Bound spell missing");names.insert(f[1].GetString());++rows;std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(bindings->NextRow());Require(rows==50&&names.size()==15,"Expected50bindings/15names");
 auto Pairs=[&](auto const&entries){for(auto const&e:entries){auto rune=sSpellMgr->GetSpellInfo(e.Rune);auto helper=sSpellMgr->GetSpellInfo(e.Effect);Require(rune&&helper,"Vantus pair absent");Require(std::string(rune->SpellName)==helper->SpellName,"Vantus native names differ");}};
 Pairs(FVantusEN::cases);Pairs(FVantusNH::cases);Pairs(FVantusTOV::cases);Pairs(FVantusTOS::cases);Pairs(FVantusAntorus::cases);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}},{1520,{}},{1530,{}},{1648,{}},{1676,{}},{1712,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}},{1520,{}},{1530,{}},{1648,{}},{1676,{}},{1712,{}}});
 unsigned contexts=0;MapInstanced parent(1600,300);InstanceMap map(1600,300,9999401,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer p(session,map,995,{50,803,805,0});p.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE,1.f);p.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED,1.f);
 auto Source=[&](auto op){Actor source(&map,299,1001+contexts,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&source)),"Caster admission");op(source);if(source.IsInWorld())map.RemoveFromMap(static_cast<Creature*>(&source),false);p.RemoveAllAuras();++contexts;};
 {FourthBound<FValor::aura_commanders_valor_location>b(p,p,219567,"aura_commanders_valor_location");b.aura->CallScriptAuraUpdateHandlers(1);Require(b.aura->IsRemoved(),"Native Valor outside altar not removed");++contexts;}
 Source([&](Actor&s){using T=FTirathon::spell_tirathon_beam_dmg::spell_tirathon_beam_dmg_AuraScript;FourthBound<T>b(s,p,190833,"spell_tirathon_beam_dmg",true);Require(!b.script.GetCaster(),"Caster should not resolve");b.aura->CallScriptEffectUpdateHandlers(499,b.aura->GetEffect(EFFECT_0));Require(b.script.castTimer==1,"Native beam early deadline");b.aura->CallScriptEffectUpdateHandlers(1,b.aura->GetEffect(EFFECT_0));Require(b.script.castTimer==500,"Native beam missing-caster deadline");});
 Source([&](Actor&s){using T=FSwift::spell_dru_swift_flight_passive::spell_dru_swift_flight_passive_AuraScript;FourthBound<T>b(s,p,40121,"spell_dru_swift_flight_passive",true);p.SetSkill(SKILL_RIDING,0,375,375);Require(b.Calc(EFFECT_1,280)==310,"Native rider375 selection");p.SetSkill(SKILL_RIDING,0,374,375);Require(b.Calc(EFFECT_1,280)==280,"Native rider374 amount");});
 Source([&](Actor&s){using T=FZone::spell_dk_anti_magic_zone::spell_dk_anti_magic_zone_AuraScript;FourthBound<T>b(s,p,50461,"spell_dk_anti_magic_zone",true);Require(!b.script.GetCaster(),"Caster should not resolve");Require(b.Calc(EFFECT_0,0)==136800,"Native missing-caster AMZ fallback");});
 Source([&](Actor&s){FourthBound<FStamina::spell_gen_artificial_stamina::spell_gen_artificial_stamina_AuraScript>b(s,p,211309,"spell_gen_artificial_stamina",true);Require(!b.script.GetCaster()&&b.Calc(EFFECT_0,17)==17,"Native missing-caster stamina");});
 Source([&](Actor&s){FourthBound<FThreat::spell_gen_artificial_threat>b(s,p,226829,"spell_gen_artificial_threat",true);Require(!b.script.GetCaster()&&b.Calc(EFFECT_2,17)==17,"Native missing-caster threat");});
 Source([&](Actor&s){FourthBound<FCenser::spell_censer_of_eternal_agony>b(s,p,148385,"spell_censer_of_eternal_agony",true);p.SetPvP(false);b.aura->CallScriptAfterEffectApplyHandlers(b.aura->GetEffect(EFFECT_0),b.aura->GetApplicationOfTarget(p.GetGUID()),AURA_EFFECT_HANDLE_REAL);Require(p.IsPvP(),"Native Censer owner apply");p.SetPvP(false);b.aura->CallScriptEffectPeriodicHandlers(b.aura->GetEffect(EFFECT_3),b.aura->GetApplicationOfTarget(p.GetGUID()));Require(p.IsPvP(),"Native Censer owner periodic");});
 auto WrongMap=[&](auto*type,auto const&entries,char const*name){using T=typename std::remove_pointer<decltype(type)>::type;for(auto const&e:entries){FourthBound<T>b(p,p,e.Rune,name);Require(p.AddAura(e.Effect,&p),"Vantus helper fixture refused");b.aura->CallScriptEffectUpdateHandlers(1000,b.aura->GetEffect(EFFECT_0));Require(!p.HasAura(e.Effect)&&p.HasAura(e.Rune),"Native wrong-map helper/parent isolation");p.RemoveAura(e.Rune);++contexts;}};
 WrongMap(static_cast<FVantusEN::spell_vantus_rune_the_emerald_nightmare_AuraScript*>(nullptr),FVantusEN::cases,"spell_vantus_rune_the_emerald_nightmare");
 WrongMap(static_cast<FVantusNH::spell_vantus_rune_the_nighthold_AuraScript*>(nullptr),FVantusNH::cases,"spell_vantus_rune_the_nighthold");
 WrongMap(static_cast<FVantusTOV::spell_vantus_rune_trial_of_valor_AuraScript*>(nullptr),FVantusTOV::cases,"spell_vantus_rune_trial_of_valor");
 WrongMap(static_cast<FVantusTOS::spell_vantus_rune_tomb_of_sargeras_AuraScript*>(nullptr),FVantusTOS::cases,"spell_vantus_rune_tomb_of_sargeras");
 WrongMap(static_cast<FVantusAntorus::spell_vantus_rune_antorus_AuraScript*>(nullptr),FVantusAntorus::cases,"spell_vantus_rune_antorus");
 }session->SetPlayer(nullptr);delete session;Require(contexts==47,"Expected47contexts");
 auto Raid=[&](auto*type,auto const&entries,uint32 mapId,char const*name){using T=typename std::remove_pointer<decltype(type)>::type;
 auto pose=WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature WHERE map=%u LIMIT 1",mapId);Require(bool(pose),"Native raid spawn pose absent");auto f=pose->Fetch();Position pos(f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat());MapInstanced raidParent(mapId,300);InstanceMap raid(mapId,300,9999500+mapId,DIFFICULTY_NORMAL_RAID,&raidParent);auto state=new InstanceScript(&raid);state->SetBossNumber(32);Require(!(raid.*Access(FourthInstanceTag{})),"Unexpected instance script");raid.*Access(FourthInstanceTag{})=state;for(auto&boss:state->*Access(FourthBossTag{}))boss.state=NOT_STARTED;
 auto sess=Session();{NativePlayer player(sess,raid,995,pos);player.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE,1.f);player.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED,1.f);
 for(auto const&e:entries){FourthBound<T>b(player,player,e.Rune,name);(state->*Access(FourthBossTag{}))[e.Boss].state=IN_PROGRESS;b.aura->CallScriptEffectUpdateHandlers(1000,b.aura->GetEffect(EFFECT_0));Require(player.HasAura(e.Effect),"Native Vantus in-progress cast refused");(state->*Access(FourthBossTag{}))[e.Boss].state=FAIL;b.aura->CallScriptEffectUpdateHandlers(1000,b.aura->GetEffect(EFFECT_0));Require(!player.HasAura(e.Effect)&&player.HasAura(e.Rune),"Native failed encounter cleanup");(state->*Access(FourthBossTag{}))[e.Boss].state=IN_PROGRESS;b.aura->CallScriptEffectUpdateHandlers(1000,b.aura->GetEffect(EFFECT_0));Require(player.HasAura(e.Effect),"Native Vantus retry refused");player.RemoveAura(e.Rune);Require(!player.HasAura(e.Effect),"Native parent removal helper leaked");(state->*Access(FourthBossTag{}))[e.Boss].state=NOT_STARTED;++contexts;}
 }sess->SetPlayer(nullptr);delete sess;};
 Raid(static_cast<FVantusEN::spell_vantus_rune_the_emerald_nightmare_AuraScript*>(nullptr),FVantusEN::cases,FVantusEN::mapId,"spell_vantus_rune_the_emerald_nightmare");
 Raid(static_cast<FVantusNH::spell_vantus_rune_the_nighthold_AuraScript*>(nullptr),FVantusNH::cases,FVantusNH::mapId,"spell_vantus_rune_the_nighthold");
 Raid(static_cast<FVantusTOV::spell_vantus_rune_trial_of_valor_AuraScript*>(nullptr),FVantusTOV::cases,FVantusTOV::mapId,"spell_vantus_rune_trial_of_valor");
 Raid(static_cast<FVantusTOS::spell_vantus_rune_tomb_of_sargeras_AuraScript*>(nullptr),FVantusTOS::cases,FVantusTOS::mapId,"spell_vantus_rune_tomb_of_sargeras");
 Raid(static_cast<FVantusAntorus::spell_vantus_rune_antorus_AuraScript*>(nullptr),FVantusAntorus::cases,FVantusAntorus::mapId,"spell_vantus_rune_antorus");
 Require(contexts==87,"Expected87contexts");CloseAltheaNative();std::cout<<"PASS native50bindings/15names/40named-pairs;87 real owner/calc/deadline/cleanup contexts including all40 native casts+FAIL/retry/parent cleanup in actual raid maps. Complete15classes compiled against Release core. Positive combat/kill/Void helper/Maw damage are source-ASAN with modeled endpoints, not native/client traversal;bare profile,spawn pose,boss states and missing NPC caster controlled without SaveToDB;no full-core ASAN claimed.\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
