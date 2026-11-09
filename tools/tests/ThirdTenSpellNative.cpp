#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
#include "PlayerDefines.h"
#include <cmath>
namespace ThirdScripts {
#include "ThirdTenConstants.inc"
#include "ThirdTenSpellScripts.inc"
}
struct ThirdListTag{using Type=std::vector<AuraScript*> Aura::*;friend Type Access(ThirdListTag);};
template struct FixtureAccess<ThirdListTag,&Aura::m_loadedScripts>;
struct ThirdMountTag{using Type=MountCapabilityEntry const* Unit::*;friend Type Access(ThirdMountTag);};
template struct FixtureAccess<ThirdMountTag,&Unit::_mount>;
template<class T>struct ThirdBound{
 T script;Aura* aura;std::string name;
 ThirdBound(Unit& caster,Player& owner,uint32 id,char const*n,bool removeBeforeLoad=false):aura(caster.AddAura(id,&owner)),name(n){Require(aura,"Selected aura refused");if(removeBeforeLoad){Require(caster.IsCreature(),"Controlled missing caster must be NPC");caster.GetMap()->RemoveFromMap(caster.ToCreature(),false);}script._Init(&name,id);Require(script._Load(aura),"Selected Load refused");script._Register();auto&list=aura->*Access(ThirdListTag{});Require(list.empty(),"Unexpected auto script");list.push_back(&script);}
 ~ThirdBound(){(aura->*Access(ThirdListTag{})).clear();}
 void Apply(){aura->CallScriptAfterEffectApplyHandlers(aura->GetEffect(EFFECT_0),aura->GetApplicationOfTarget(aura->GetUnitOwner()->GetGUID()),AURA_EFFECT_HANDLE_REAL);}
 float Calc(float amount){bool recalc=false;aura->CallScriptEffectCalcAmountHandlers(aura->GetEffect(EFFECT_0),amount,recalc);return amount;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Strings absent");sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 auto bindings=WorldDatabase.Query(
#include "ThirdTenSpellBindings.inc"
 );Require(bool(bindings),"Selected bindings absent");std::set<std::string> names;unsigned rows=0;do{auto f=bindings->Fetch();names.insert(f[1].GetString());++rows;std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(bindings->NextRow());Require(names.size()==11&&rows==12,"11 names/12 bindings required");
 for(uint32 id:{49357u,52845u,43900u,43899u,49379u,49378u,113095u,74589u,70764u,71084u,242459u,207472u,232698u,194913u,195975u,115181u,195558u,16739u})Require(sSpellMgr->GetSpellInfo(id),"Selected spell absent");
 MountCapabilityEntry const* fast=nullptr;for(auto row:sMountCapabilityStore){auto info=sSpellMgr->GetSpellInfo(row->ModSpellAuraID);if(info&&info->Effects[0]->ApplyAuraName==SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED&&info->Effects[0]->BasePoints==100){fast=row;break;}}Require(fast,"Native fast ground mount capability absent");std::cout<<"CONTROL capability="<<fast->ID<<" spell="<<fast->ModSpellAuraID<<'\n';
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 unsigned cases=0;for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){MapInstanced parent(1600,300);InstanceMap map(1600,300,9999300+race,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer p(session,map,995,{50,803,805,0});p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.SetNativeDisplayId(49);p.SetDisplayId(49);p.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE,1.f);p.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED,1.f);
 auto SeedMount=[&](){Require(p.AddAura(43900,&p),"Native mount aura refused");Require(p.AddAura(fast->ModSpellAuraID,&p),"Native speed aura refused");p.*Access(ThirdMountTag{})=fast;Require(p.GetSpeedRate(MOVE_RUN)>=2.f,"Native mounted speed setup failed");};
 SeedMount();auto mountedSpeed=p.GetSpeedRate(MOVE_RUN);p.RemoveAurasByType(SPELL_AURA_MOUNTED);auto unmountedSpeed=p.GetSpeedRate(MOVE_RUN);std::cout<<"CONTROL native dismount "<<mountedSpeed<<" -> "<<unmountedSpeed<<'\n';Require(mountedSpeed>=2.f&&unmountedSpeed<2.f,"Native speed loss not established");p.RemoveAllAuras();
 auto admission=p.CastSpell(&p,43899,TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_DONT_REPORT_CAST_ERROR));std::cout<<"CONTROL mount cast result="<<uint32(admission)<<" expected refusal="<<uint32(SPELL_FAILED_NOT_HERE)<<'\n';Require(admission==SPELL_FAILED_NOT_HERE,"Bare profile mount-capability refusal differs from CheckLocation");++cases;
 auto Source=[&](auto op){Actor actor(&map,299,1001+cases,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Caster admission");op(actor);if(actor.IsInWorld())map.RemoveFromMap(static_cast<Creature*>(&actor),false);p.RemoveAllAuras();p.SetDisplayId(49);++cases;};
 Source([&](Actor& source){ThirdBound<ThirdScripts::spell_item_faded_wizard_hat>b(source,p,74589,"spell_item_faded_wizard_hat");auto before=source.GetDisplayId();map.RemoveFromMap(static_cast<Creature*>(&source),false);b.aura->CallScriptEffectApplyHandlers(b.aura->GetEffect(EFFECT_0),b.aura->GetApplicationOfTarget(p.GetGUID()),AURA_EFFECT_HANDLE_REAL);Require(p.GetDisplayId()!=49&&source.GetDisplayId()==before,"Native Hat recipient failed");});
 Source([&](Actor& source){ThirdBound<ThirdScripts::spell_gen_citizen_costume_alliance_darkmoon>b(source,p,70764,"spell_gen_citizen_costume_alliance_darkmoon");map.RemoveFromMap(static_cast<Creature*>(&source),false);b.Apply();Require(p.GetDisplayId()!=49,"Native Alliance costume recipient failed");});
 Source([&](Actor& source){ThirdBound<ThirdScripts::spell_gen_citizen_costume_horde_darkmoon>b(source,p,71084,"spell_gen_citizen_costume_horde_darkmoon");map.RemoveFromMap(static_cast<Creature*>(&source),false);b.Apply();Require(p.GetDisplayId()!=49,"Native Horde costume recipient failed");});
 Source([&](Actor& source){using T=ThirdScripts::spell_item_ocean_embrace::spell_item_ocean_embrace_AuraScript;ThirdBound<T>b(source,p,242459,"spell_item_ocean_embrace");for(uint32 id=242460;id<=242465;++id)Require(p.AddAura(id,&p),"Ocean helper refused");map.RemoveFromMap(static_cast<Creature*>(&source),false);p.RemoveAura(242459);for(uint32 id=242460;id<=242465;++id)Require(!p.HasAura(id),"Native Ocean recipient helper leaked");});
 Source([&](Actor& source){ThirdBound<ThirdScripts::spell_pri_shadowform>b(source,p,232698,"spell_pri_shadowform");map.RemoveFromMap(static_cast<Creature*>(&source),false);b.Apply();Require(p.HasAura(185908),"Native Shadow owner visual absent");for(uint32 id:{185909u,185910u,185911u})Require(p.AddAura(id,&p),"Shadow helper refused");p.RemoveAura(232698);for(uint32 id:{185908u,185909u,185910u,185911u})Require(!p.HasAura(id),"Native Shadow owner cleanup leaked");});
 Source([&](Actor& source){using T=ThirdScripts::spell_item_prydaz_abs::spell_item_prydaz_abs_AuraScript;ThirdBound<T>b(source,p,207472,"spell_item_prydaz_abs");map.RemoveFromMap(static_cast<Creature*>(&source),false);float expected=p.CountPctFromMaxHealth(25)*(p.isInTankSpec()?0.6f:1.f);Require(std::abs(b.Calc(1)-expected)<1,"Native owner Prydaz calculation failed");});
 Source([&](Actor& source){using T=ThirdScripts::spell_gen_hammer_of_supreme_justice::spell_gen_hammer_of_supreme_justice_AuraScript;ThirdBound<T>b(source,p,195558,"spell_gen_hammer_of_supreme_justice",true);Require(!b.script.GetCaster(),"Native missing caster at Load absent");b.aura->CallScriptEffectUpdateHandlers(3000,b.aura->GetEffect(EFFECT_0));Require(b.script.m_checkTimer==0,"Native missing-caster one-shot deadline not consumed");});
 {ThirdBound<ThirdScripts::spell_item_demon_hunters_aspect>b(p,p,113095,"spell_item_demon_hunters_aspect");b.Apply();Require(p.AddAura(16739,&p),"Native other transform refused");auto other=p.GetDisplayId();Require(other!=49,"Native orb model missing");p.RemoveAura(113095);Require(p.HasAura(16739)&&p.GetDisplayId()==other,"Native other transform overwritten by Aspect cleanup");++cases;}p.RemoveAllAuras();
 }session->SetPlayer(nullptr);delete session;}
 Require(cases==18,"Native18contexts changed");CloseAltheaNative();std::cout<<"PASS native12bindings/11classes;two actual2-to1 mount-speed controls and explicit native bare-profile mount-capability refusal,16 real owner/model/cleanup/calc/missing-caster-at-Load contexts. Earlier profile/mount capability controlled;Brewfest selection/faction/reverse,models/item-spec/queued effects separately source-ASAN;actual client traversal and full-core ASAN not claimed.\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
