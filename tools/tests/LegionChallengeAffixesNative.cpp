// Real registered Might aura, native amount calculation, owner updates and proc
// veto dispatch. The proc receipt is controlled; no full melee/client claim.
#define CHALLENGE_LIFECYCLE_SUPPORT_ONLY
#include "LegionChallengeLifecycleNative.cpp"
#include "GameTables.h"
#ifdef CHALLENGE_AFFIX_BASELINE
#include "ChallengeAffixesActual.inc"
#else
void AddSC_challenge_scripts();
#endif
struct AffixBindingsTag{using Type=SpellScriptsContainer ScriptDataStoreMgr::*;friend Type Access(AffixBindingsTag);};
template struct FixtureAccess<AffixBindingsTag,&ScriptDataStoreMgr::_spellScriptsStore>;
struct AffixActor:Creature{
 uint64 unscaledHealth=0;
 AffixActor(InstanceMap& map,uint32 entry,uint32 id){Require(Create(id,&map,1,entry,0,ALLIANCE,2934.49f,715.757f,554.286f,1.55255f),"Native affix actor create failed");unscaledHealth=GetMaxHealth();SetHomePosition(*this);Require(map.AddToMap(static_cast<Creature*>(this)),"Native affix actor publication failed");}
 void AddToWorld()override{sObjectAccessor->AddObject(static_cast<Creature*>(this));Unit::AddToWorld();Require(AIM_Initialize(new NullCreatureAI(this)),"Controlled affix actor AI failed");GetInstanceScript()->OnCreatureCreate(this);GetInstanceScript()->OnCreatureCreateForScript(this);}
 ~AffixActor(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
struct AffixScenarioScope{
 InstanceMap& map;Scenario* scenario;
 AffixScenarioScope(InstanceMap& m,LifecyclePlayer& owner):map(m),scenario(sScenarioMgr->AddScenario(&m,1175)){Require(scenario,"Native affix scenario publication failed");scenario->CreateChallenge(&owner);Require(scenario->GetChallenge()&&scenario->GetChallenge()->_canRun,"Native affix challenge missing");}
 ~AffixScenarioScope(){map.m_scenarios.erase(scenario);sScenarioMgr->RemoveScenario(map.GetInstanceId());}
};
static bool ProcBlocked(Aura& aura,Creature& source,Player& victim){auto effect=aura.GetEffect(EFFECT_8);auto application=aura.GetApplicationOfTarget(source.GetGUID());Require(effect&&application,"Real Necrotic effect/application absent");ProcEventInfo event(&source,&victim,&victim,PROC_FLAG_DONE_MELEE_AUTO_ATTACK,PROC_SPELL_TYPE_DAMAGE,PROC_SPELL_PHASE_HIT,PROC_HIT_NORMAL,nullptr,nullptr,nullptr);return aura.CallScriptEffectProcHandlers(effect,application,event);}
static Aura* Renew(AffixActor& actor){actor.RemoveAurasDueToSpell(ChallengersMight);auto aura=actor.AddAura(ChallengersMight,&actor);Require(aura&&aura->m_loadedScripts.size()==1,"Actual unique Might aura script not loaded");return aura;}
static void LoadNativeMightProc(){
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE affix_proc_seed AS SELECT * FROM spell_proc_event WHERE entry=206150");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_proc_event LIKE affix_proc_seed");WorldDatabase.DirectExecute("INSERT INTO spell_proc_event SELECT * FROM affix_proc_seed");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE affix_check_seed AS SELECT * FROM spell_proc_check WHERE entry IN(206150,209858)");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_proc_check LIKE affix_check_seed");WorldDatabase.DirectExecute("INSERT INTO spell_proc_check SELECT * FROM affix_check_seed");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE affix_bonus_seed AS SELECT * FROM spell_bonus_data WHERE entry IN(206150,209858)");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_bonus_data LIKE affix_bonus_seed");WorldDatabase.DirectExecute("INSERT INTO spell_bonus_data SELECT * FROM affix_bonus_seed");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sSpellMgr->LoadSpellPrcoCheck();sSpellMgr->LoadSpellBonusess();
 auto info=sSpellMgr->GetSpellInfo(206150);auto rows=sSpellMgr->GetSpellProcEvent(206150);std::cout<<"PROC_META nativeChance="<<unsigned(info->GetAuraOptions(DIFFICULTY_MYTHIC_KEYSTONE)->ProcChance)<<" nativeCategoryMs="<<info->GetAuraOptions(DIFFICULTY_MYTHIC_KEYSTONE)->ProcCategoryRecovery<<" worldRows="<<(rows?rows->size():0)<<'\n';if(rows)for(auto const&row:*rows)std::cout<<"PROC_ROW flags="<<row.procFlags<<" extra="<<row.procEx<<" cooldown="<<row.cooldown<<" chance="<<row.customChance<<" effects="<<row.effectMask<<'\n';
}
static unsigned MeleeProc(AffixActor& actor,LifecyclePlayer& victim){DamageInfo damage(&actor,&victim,1,nullptr,SPELL_SCHOOL_MASK_NORMAL,DIRECT_DAMAGE,0);actor.ProcDamageAndSpell(&victim,PROC_FLAG_DONE_MELEE_AUTO_ATTACK,0,PROC_EX_NORMAL_HIT,&damage);auto aura=victim.GetAura(ChallengerNecrotic,actor.GetGUID());return aura?aura->GetStackAmount():0;}
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);sObjectMgr->LoadItemTemplates();sObjectMgr->LoadItemTemplateAddon();sObjectMgr->LoadScenarioData();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());AddSC_challenge_scripts();sScriptMgr->SwapScriptContext(true);
 auto binding=WorldDatabase.Query("SELECT spell_id,ScriptName FROM spell_script_names WHERE spell_id=206150");Require(binding&&binding->GetRowCount()==1,"Review changed native Might binding scope");auto f=binding->Fetch();Require(f[1].GetString()=="spell_challengers_might","Native Might binding changed");auto scriptId=sObjectMgr->GetScriptId(f[1].GetString().c_str());Require(sScriptMgr->GetSpellScriptLoader(scriptId),"Actual Might registration missing");(sScriptDataStore->*Access(AffixBindingsTag{})).emplace(206150,std::make_pair(scriptId,true));
 LoadNativeMightProc();
 unsigned contexts=0,procCases=0;
 for(uint32 level:{2u,4u,7u,10u,15u})for(unsigned profile=0;profile<4;++profile){
  MapInstanced parent(1492,300);InstanceMap map(1492,300,9936000+contexts,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);LifecyclePlayer owner(map,501,Position{2934.49f,711.757f,554.286f,1.55255f});auto route=sMapChallengeModeStore.LookupEntry(208);Require(route,"Native Maw route missing");auto key=owner.Key(*route);key->SetModifier(ITEM_MODIFIER_CHALLENGE_KEYSTONE_LEVEL,level);owner.m_challengeKeyInfo.Level=level;owner.m_challengeKeyInfo.Affix=profile?uint32(Affixes::Necrotic):0;owner.m_challengeKeyInfo.Affix1=0;owner.m_challengeKeyInfo.Affix2=profile==2?uint32(Affixes::Tyrannical):profile==3?uint32(Affixes::Fortified):0;
  AffixScenarioScope progress(map,owner);auto challenge=progress.scenario->GetChallenge();AffixActor boss(map,96754,171001),trash(map,97043,171002);Check(boss.IsDungeonBoss()&&!trash.IsDungeonBoss(),"Native boss/trash classification changed");
  auto health=sChallengeModeHealthTable.GetRow(challenge->GetChallengeLevel());auto damage=sChallengeModeDamageTable.GetRow(challenge->GetChallengeLevel());Require(health&&damage,"Native challenge scaling row missing");
  for(auto actor:{&boss,&trash}){auto aura=actor->GetAura(ChallengersMight);Require(aura&&aura->m_loadedScripts.size()==1,"Native create hook did not apply scripted Might");float hp=health->Scalar,dmg=damage->Scalar;if(actor->IsDungeonBoss()&&challenge->HasAffix(Affixes::Tyrannical)){hp*=1.4f;dmg*=1.15f;}else if(!actor->IsDungeonBoss()&&challenge->HasAffix(Affixes::Fortified)){hp*=1.2f;dmg*=1.3f;}Check(std::fabs(aura->GetEffect(EFFECT_0)->GetAmount()-(hp-1)*100)<.02f,"Native Might health amount differs from matching GT/profile");Check(std::fabs(aura->GetEffect(EFFECT_1)->GetAmount()-(dmg-1)*100)<.02f,"Native Might damage amount differs from matching GT/profile");Check(aura->GetEffect(EFFECT_8)->GetAmount()==float(challenge->HasAffix(Affixes::Necrotic)&&!actor->IsAffixDisabled(Affixes::Necrotic)),"Native Necrotic eligibility flag differs");double expected=double(actor->unscaledHealth)*hp;Check(expected>0&&std::fabs(double(actor->GetMaxHealth())-expected)/expected<.00001,"Native health modifier did not apply matching scaled health");}
  {DungeonPet pet(owner,map);DungeonGuardian guardian(owner,map);for(auto actor:{static_cast<Creature*>(&pet),static_cast<Creature*>(&guardian)}){auto script=map.GetInstanceScript();script->OnCreatureCreateForScript(actor);script->OnCreatureUpdateDifficulty(actor);script->EnterCombatForScript(actor,&trash);Check(!actor->HasAura(ChallengersMight),"Native player pet/guardian received enemy Might scaling");}}
  if(level==10&&profile==1){
   for(uint32 elapsed:{0u,1u,249u,499u,500u,750u,999u,1000u,1500u})for(bool split:{false,true}){
    auto aura=Renew(trash);Check(!ProcBlocked(*aura,trash,owner),"First native Necrotic proc blocked");Check(ProcBlocked(*aura,trash,owner),"Immediate repeated native proc not blocked");for(uint32 left=elapsed;left;){uint32 tick=split?std::min(left,100u):left;aura->UpdateOwner(tick,&trash);left-=tick;}
#ifdef CHALLENGE_AFFIX_BASELINE
    Check(ProcBlocked(*aura,trash,owner)==(elapsed<500),"Expected original double-effect countdown changed");
#else
    Check(ProcBlocked(*aura,trash,owner)==(elapsed<1000),"Native Necrotic accepted a proc before one elapsed second");
#endif
    ++procCases;
   }
   auto first=Renew(trash);auto other=Renew(boss);Check(!ProcBlocked(*first,trash,owner)&&!ProcBlocked(*other,boss,owner),"Separate actors shared proc cooldown");first->UpdateOwner(250,&trash);Check(ProcBlocked(*other,boss,owner),"Updating one actor released another actor's cooldown");Check(!ProcBlocked(*Renew(trash),trash,owner),"Native remove/reapply retained stale proc cooldown");
   auto aura=Renew(trash);owner.RemoveAurasDueToSpell(ChallengerNecrotic);auto stacks=MeleeProc(trash,owner);Check(stacks==1,"Native melee receipt did not apply the first real Necrotic stack");std::cout<<"MELEE_STACK time=0 count="<<stacks<<'\n';unsigned time=0;for(auto tick:{499u,1u,499u,1u}){aura->UpdateOwner(tick,&trash);time+=tick;stacks=MeleeProc(trash,owner);
#ifdef CHALLENGE_AFFIX_BASELINE
    Check(stacks==(time<500?1u:time<1000?2u:3u),"Expected original fast real Necrotic stack cadence changed");
#else
    Check(stacks==(time<1000?1u:2u),"Native melee receipt added a real Necrotic stack before1000ms");
#endif
    std::cout<<"MELEE_STACK time="<<time<<" count="<<stacks<<'\n';}owner.RemoveAurasDueToSpell(ChallengerNecrotic);
  }
  std::cout<<"AFFIX level="<<level<<" profile="<<profile<<" nativeBossHp="<<boss.GetMaxHealth()<<" nativeTrashHp="<<trash.GetMaxHealth()<<'\n';++contexts;
 }
 Check(contexts==20&&procCases==18,"Affix test scope changed");
#ifdef CHALLENGE_AFFIX_BASELINE
 std::cout<<"COUNTERFACTUAL "<<checked<<" native checks:real Might OnEffectUpdate11+12 admits Necrotic after500ms despite its1000ms reset;zero-amount Quaking/Explosive effects still update. Not ordinary successful gameplay\n";
#else
 std::cout<<"PASS "<<checked<<" native affix checks:20 key/profile contexts,actual scripted206150/GT health+damage,18 proc timing cases,split updates,independent actors and remove/reapply;real Aura::UpdateOwner and proc-veto dispatch. Accepted proc receipt/actor AI controlled;full melee/client/all affixes remain separate\n";
#endif
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
