#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "SpellScript.h"
#include "PlayerDefines.h"
#include "ScriptsData.h"
#include <cmath>
namespace NextScripts {
enum {SPELL_FLASK_OF_THE_NORTH_SP=67016,SPELL_FLASK_OF_THE_NORTH_AP=67017,SPELL_FLASK_OF_THE_NORTH_STR=67018,SPELL_WARRIOR_SHIELD_BLOCK_AURA=132404,SPELL_MAGE_COMET_STORM_VISUAL=228601,SPELL_MAGE_BRAIN_FREEZE_AURA=190446,SPELL_MAGE_FLURRY_VISUAL=228596,SPELL_MAGE_FLURRY_CHILL_PROC=228358,SPELL_MAGE_BRAIN_FREEZE_IMPROVED=231584,SPELL_MAGE_BONE_CHILLING=205027,SPELL_MAGE_BONE_CHILLING_BUFF=205766};
#include "NextTenSpellScripts.inc"
}
struct NextScriptsTag{using Type=std::vector<AuraScript*> Aura::*;friend Type Access(NextScriptsTag);};
template struct FixtureAccess<NextScriptsTag,&Aura::m_loadedScripts>;
template<class T>struct NextBound{
 T script;Aura* aura;std::string name;
 NextBound(Unit& caster,Player& owner,uint32 id,char const*n):aura(caster.AddAura(id,&owner)),name(n){Require(aura,"Selected aura refused");script._Init(&name,id);Require(script._Load(aura),"Selected Load refused");script._Register();auto&list=aura->*Access(NextScriptsTag{});Require(list.empty(),"Unexpected auto script");list.push_back(&script);}
 ~NextBound(){(aura->*Access(NextScriptsTag{})).clear();}
 float Calc(unsigned index,float amount){bool recalc=false;aura->CallScriptEffectCalcAmountHandlers(aura->GetEffect(SpellEffIndex(index)),amount,recalc);Require(recalc,"Native calculation hook not dispatched");return amount;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Strings absent");
 sSpellMgr->LoadSpellBonusess();sSpellMgr->LoadSpellThreats();
 auto bindings=WorldDatabase.Query(
#include "NextTenSpellBindings.inc"
 );Require(bool(bindings),"Selected bindings absent");std::set<std::string> names;do{auto f=bindings->Fetch();names.insert(f[1].GetString());std::cout<<"BIND "<<f[0].GetUInt32()<<" "<<f[1].GetString()<<'\n';}while(bindings->NextRow());Require(names.size()==10,"Ten distinct live names required");
 for(uint32 id:{253826u,253825u,188639u,132404u,67019u,67016u,67017u,67018u,153595u,228601u,44614u,232661u,250863u,242609u,233490u,233496u,233497u,233498u,233499u,196364u,87649u}){auto info=sSpellMgr->GetSpellInfo(id);Require(info,"Selected spell absent");auto row=sSpellStore.LookupEntry(id);Require(row,"Spell row absent");std::cout<<"CATALOG "<<id<<" "<<row->Description->Str[0]<<'\n';}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_script_names","spell_id=67019");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new NextScripts::spell_item_flask_of_the_north();sScriptMgr->SwapScriptContext(true);sScriptDataStore->LoadSpellScriptNames();
 unsigned cases=0;for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999500+race,DIFFICULTY_N_SCENARIO,&parent);auto session=Session();{
 NativePlayer p(session,map,995,{50,803,805,0});p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);
 for(uint8 cls=1;cls<=12;++cls){p.SetByteValue(UNIT_FIELD_BYTES_0,1,cls);Require(p.CastSpell(&p,67019,true)==SPELL_CAST_OK,"Registered native Flask refused");bool stat=p.HasAura(67016)||p.HasAura(67017)||p.HasAura(67018);Require(stat,"Flask did not select native stat");if(cls==CLASS_DEMON_HUNTER)Require(p.HasAura(67017),"DH did not get Agility");if(cls==CLASS_MONK)Require(p.HasAura(67016)||p.HasAura(67017),"Monk stat outside authored choices");p.RemoveAllAuras();}p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
 {NextBound<NextScripts::spell_gen_umbral_shift>b(p,p,253826,"spell_gen_umbral_shift");Require(b.Calc(1,321)==0,"Umbral absent enabler");Require(p.AddAura(253825,&p),"Umbral enabler refused");Require(b.Calc(1,321)==321,"Umbral enabled");p.RemoveAura(253825);Require(b.Calc(1,321)==0,"Umbral enabler removal");++cases;}p.RemoveAllAuras();
 {NextBound<NextScripts::spell_warr_shatter_the_bones>b(p,p,188639,"spell_warr_shatter_the_bones");Require(b.Calc(0,17)==0,"Shatter without Shield");Require(p.AddAura(132404,&p),"Shield refused");Require(b.Calc(0,17)==17,"Shatter with Shield");p.RemoveAura(132404);Require(b.Calc(0,17)==0,"Shatter Shield removal");++cases;}p.RemoveAllAuras();
 {Actor source(&map,299,1001,{51,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&source)),"Caster admission");NextBound<NextScripts::spell_item_chocolate_cookie::spell_item_chocolate_cookie_AuraScript>b(source,p,87649,"spell_item_chocolate_cookie");map.RemoveFromMap(static_cast<Creature*>(&source),false);Require(!b.script.GetCaster()&&b.script.GetUnitOwner()==&p,"Actual caster removal/owner");for(uint8 n:{90,91,92}){b.aura->SetStackAmount(n);b.script.OnStackChange(nullptr,AURA_EFFECT_HANDLE_REAL);}++cases;}p.RemoveAllAuras();
 {NextBound<NextScripts::spell_item_walling_souls>b(p,p,242609,"spell_item_walling_souls");p.AddSpellCooldown(242609,0,getPreciseTime()+100);DamageInfo damage(&p,&p,100,sSpellMgr->GetSpellInfo(242609),SPELL_SCHOOL_MASK_SHADOW,SPELL_DIRECT_DAMAGE,100);auto before=p.GetSpellCooldownDelay(242609);for(float amount:{0.f,0.1f,0.99f})b.script.Trigger(nullptr,damage,amount);auto after=p.GetSpellCooldownDelay(242609);Require(std::abs(before-after)<0.1,"No absorption changed native cooldown");float amount=1;b.script.Trigger(nullptr,damage,amount);Require(std::abs(p.GetSpellCooldownDelay(242609)-(after-30))<0.1,"Positive capped formula changed");++cases;}p.RemoveAllAuras();
 }session->SetPlayer(nullptr);delete session;}
 Require(cases==8,"Eight native contexts required");CloseAltheaNative();std::cout<<"PASS ten bindings/matching data/all complete classes compile;24 registered native Flask casts/all12 classes/two races;8 Human/Orc actual Aura calculation/Shield/enabler changes,removed caster cookie90/91/92,zero/fractional/positive native cooldown. Achievement counters/casts/summon paths source-modeled or client pending;not full-core ASAN.\n";return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
