// Genuine owned10/46427 -> native whole Creature DB removal/reload in step4.
#define MAGNAR_RECREATION_SUPPORT_ONLY
#include "MagnarRecreationNative.cpp"
struct DuelDisabledTag{using Type=UnitAI* Unit::*;friend Type Access(DuelDisabledTag);};
template struct FixtureAccess<DuelDisabledTag,&Unit::i_disabledAI>;
static unsigned DuelStories(Map& map){unsigned n=0;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==504)++n;return n;}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("creature","guid=361400");Shadow("smart_scripts","source_type=0 AND entryorguid=94813");Shadow("creature_template_addon","entry IN(96034,96468,97123,94707,94813)");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sObjectMgr->LoadCreatures();sObjectMgr->RemoveCreatureFromGrid(361400,sObjectMgr->GetCreatureData(361400));sObjectMgr->LoadCreatureTemplateAddons();sSmartScriptMgr->LoadSmartAIFromDB();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;new npc_magnar_icebreaker;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto original=lootName;lootName+=" WHERE Entry IN(108218,94707,94813) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=original;Require(loaded,"Actual reserve loot absent");
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<22;++mode){
#ifdef MAGNAR_DUEL_DEFERRED_BASELINE
  if(mode!=21)continue;
#elif defined(MAGNAR_DUEL_RECREATION_BASELINE)
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9999500+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared script absent");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4715,346,-37.7f,0});RecreationPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor companion(&map,96468,16502,{4720.80f,347.91f,-37.71f,0});companion.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&companion)),"Companion admission failed");companion.SetPhaseId({6999},false);script->OnCreatureCreate(&companion);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;
   auto load=[&]{auto actor=std::make_unique<LoadedMagnar>();Require(actor->LoadCreatureFromDB(361400,&map,false)&&map.AddToMap(static_cast<Creature*>(actor.get())),"Actual original DB boss load failed");actor->Brain();return actor;};auto boss=load();
   auto& brain=companion.Brain();brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=player.GetGUID();brain.magnarStartAttempt=player.GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;brain.MovementInform(POINT_MOTION_TYPE,7);companion.Tick(6000);Require(boss->Brain().GetData(MagnarStart::Accepted),"Native497/6s first start not acknowledged");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit* killer=&player;if(kind==1){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(kind==2){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}
   if(mode!=9)CompleteWaves(*boss,player,killer,scenario);else scenario->SetCurrentStep(4);Require(mode==9||script->magnarCombat.earned,"Actual ten deaths did not prepare earned metadata");if(mode==17)boss->SetHealth(1);
#ifndef MAGNAR_DUEL_RECREATION_BASELINE
   if(mode==3||mode==11||mode==19){killer->DealDamage(boss.get(),UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(boss->Brain().finaleStarted&&DuelStories(map)==1,"Old pending504 absent");if(mode==11)boss->Brain().UpdateFinale(boss->Brain().GetFinaleHandoffDelay());}
   if(mode==4)boss->IsAIEnabled=false;
#endif
   auto oldHealth=boss->GetHealth();auto oldMax=boss->GetMaxHealth();auto old=boss->GetGUID();auto roster=boss->Brain().reserveGUIDs;auto pose=boss->GetPosition();std::unique_ptr<LoadedMagnar> stillLive;if(mode==10)stillLive=std::move(boss);
#ifndef MAGNAR_DUEL_RECREATION_BASELINE
   else if(mode==19){auto saved=boss->GetAI();boss->SetDisabledCurrentAI();auto temporary=new NullCreatureAI(boss.get());boss->SetAI(temporary);boss->CombatStop(true);boss->CastStop();boss->RemoveAllAuras();boss->GetMap()->RemoveFromMap(static_cast<Creature*>(boss.get()),false);Require(static_cast<MagnarAI*>(saved)->finaleConversation.IsEmpty()&&!static_cast<MagnarAI*>(saved)->finalePending,"Actual native suspended presentation not retired");boss->SetAI(saved);(*boss).*Access(DuelDisabledTag{})=nullptr;delete temporary;boss.reset();}
#endif
   else boss.reset();map.RemoveAllObjectsInRemoveList();for(auto id:roster)Require(mode==10||!map.GetCreature(id),"Actual duel removal orphaned old owned actors");boss=load();Require(boss->GetGUID()!=old&&scenario->GetCurrentStep()==(mode==11?5u:4u)&&WaveCredit(scenario)==(mode==9?0u:1u),"DB recreation changed identity/earned stage unexpectedly");
#ifndef MAGNAR_DUEL_RECREATION_BASELINE
   if(mode!=10)Require(!DuelStories(map),"Native whole removal orphaned old504");
   if(mode==1){boss.reset();map.RemoveAllObjectsInRemoveList();boss=load();}
   if(mode==5)boss->IsAIEnabled=false;if(mode==8)script->Initialize();if(mode==12)boss->AddUnitState(UNIT_STATE_ROOT);if(mode==13)player.SetQuestStatus(39191,QUEST_STATUS_NONE);if(mode==14)player.Relocate(5000,346,-37.7f,0);if(mode==15)boss->SetPhaseId({6998},false);if(mode==16)boss->SetMaxHealth(boss->GetMaxHealth()*2);
   if(mode==6)Require(boss->AIM_Initialize(),"Pre-transfer native factory replacement failed");
   if(mode==5||mode==15){boss->Brain().UpdateAI(0);Require(boss->Brain().first_phase&&!boss->Brain().finaleStarted&&scenario->GetCurrentStep()==4,"Transient context consumed duel ticket");boss->IsAIEnabled=true;boss->SetPhaseId({6999},false);}
#endif
#ifndef MAGNAR_DUEL_RECREATION_BASELINE
   if(mode==21){std::uint64_t hp=0;Require(MagnarCombat::Rebind(boss.get(),boss->AI(),hp),"Controlled health-callback endpoint did not bind real earned ticket");boss->SetHealth(hp);boss->IsAIEnabled=false;boss->Brain().UpdateAI(0);Require(boss->Brain().first_phase&&script->magnarCombat.earned&&!boss->Brain().finaleStarted,"Disabled health-callback endpoint lost earned binding");boss->IsAIEnabled=true;}
   if(mode==20){boss->Brain().AttackStart(&player);Require(boss->isInCombat()&&boss->getVictim()==&player&&boss->Brain().events.HasEvent(EVENT_4),"Native pre-recovery engagement did not arm original first-phase timer");}
#endif
   boss->Brain().UpdateAI(0);
#if !defined(MAGNAR_DUEL_RECREATION_BASELINE) && !defined(MAGNAR_DUEL_DEFERRED_BASELINE)
   bool denied=mode==8||mode==9||mode==10||mode==11;
   if(!denied){Require(!boss->Brain().first_phase&&boss->Brain().wave==7&&boss->Brain().summons.empty()&&script->magnarCombat.earned&&script->magnarRetiredCombat.actor.IsEmpty(),"Observed earned duel did not bind once without old actors");Require(fabs(boss->GetHealthPct()-100.0*oldHealth/oldMax)<0.01,"Duel checkpoint health fraction changed");Require(boss->GetExactDist(pose)<0.01f,"Duel recovery forced a new position");if(mode==20)Require(boss->getVictim()==&player&&boss->Brain().events.HasEvent(EVENT_1)&&boss->Brain().events.HasEvent(EVENT_2)&&boss->Brain().events.HasEvent(EVENT_3)&&!boss->Brain().events.HasEvent(EVENT_4),"Already-engaged recovery lost victim/combat timers or retained wave timer");if(mode==12)Require(boss->HasUnitState(UNIT_STATE_ROOT),"Duel recovery bypassed root");if(mode==2){boss.reset();map.RemoveAllObjectsInRemoveList();boss=load();boss->Brain().UpdateAI(0);Require(!boss->Brain().first_phase,"Second earned whole-body transfer lost ticket");}if(mode==7){auto health=boss->GetHealth();Require(boss->AIM_Initialize(),"Post-transfer AI replacement failed");Require(!boss->Brain().first_phase&&boss->GetHealth()==health,"Post-transfer AI replacement lost duel/health");}}
   if(mode==13||mode==14){killer->DealDamage(boss.get(),UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(!boss->Brain().finaleStarted&&!DuelStories(map)&&scenario->GetCurrentStep()==4,"Ineligible owner completed recreated duel");player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.Relocate(4715,346,-37.7f,0);}
#endif
#if !defined(MAGNAR_DUEL_RECREATION_BASELINE) && !defined(MAGNAR_DUEL_DEFERRED_BASELINE)
   if(mode==18){auto before=boss->GetDistance(&player);Require(boss->GetHealth(killer)>1,"Native duel body has no nonlethal health");killer->DealDamage(boss.get(),1,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(!boss->Brain().finaleStarted,"Nonlethal hit surrendered recreated duel");boss->Brain().AttackStart(&player);Require(boss->getVictim()==&player,"Ordinary native engagement lost victim");for(unsigned elapsed=0;elapsed<20000&&!boss->IsWithinMeleeRange(&player);elapsed+=100){boss->Unit::Update(100);map.MoveAllCreaturesInMoveList();}std::cout<<"CONTROL native chase before="<<before<<" after="<<boss->GetDistance(&player)<<std::endl;Require(boss->GetDistance(&player)<before&&boss->IsWithinMeleeRange(&player),"Recreated duel cannot reach ordinary melee through native movement");Require(boss->Brain().events.HasEvent(EVENT_1)&&boss->Brain().events.HasEvent(EVENT_2)&&boss->Brain().events.HasEvent(EVENT_3)&&!boss->Brain().events.HasEvent(EVENT_4),"Recreated engagement lost spell timers or restarted waves");}
#endif
   auto health=boss->GetHealth(killer);killer->DealDamage(boss.get(),UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);
#if defined(MAGNAR_DUEL_RECREATION_BASELINE) || defined(MAGNAR_DUEL_DEFERRED_BASELINE)
   Require(boss->Brain().first_phase&&boss->IsAlive()&&boss->GetHealth(killer)==health&&!boss->Brain().finaleStarted&&!DuelStories(map)&&scenario->GetCurrentStep()==4&&WaveCredit(scenario)==1,"Pinned genuine4 recreation immortal body did not reproduce");
#ifdef MAGNAR_DUEL_DEFERRED_BASELINE
   Require(script->magnarCombat.earned&&script->magnarRetiredCombat.actor.IsEmpty(),"Deferred control did not keep its legitimate consumed binding");std::cout<<"COUNTERFACTUAL deferred health resume race="<<race<<" killer="<<kind<<": strip only actual NeedsReset fallback;real4/ticket consumed,health endpoint disabled/re-enabled,same AI remains first_phase and discards lethal/no504. Modeled missing-fallback control,not prior Git source or gameplay success\n";
#else
   std::cout<<"COUNTERFACTUAL earned4 DB recreation race="<<race<<" killer="<<kind<<": real10 deaths/46427/native removal/newGUID;fresh first_phase discards actual lethal hit,no504/45784. Old/new native pose z="<<pose.GetPositionZ()<<'/'<<boss->GetPositionZ()<<". Expected broken,not gameplay success\n";
#endif
#else
   if(denied){Require(boss->Brain().first_phase&&!boss->Brain().finaleStarted&&!DuelStories(map)&&boss->GetHealth(killer)==health&&WaveCredit(scenario)==(mode==9?0u:1u),"Unobserved/epoch/unearned/completed context granted duel victory");}
   else{Require(!boss->Brain().first_phase&&boss->Brain().finaleStarted&&DuelStories(map)==1,"Recreated genuine earned duel cannot surrender through504");auto delay=boss->Brain().GetFinaleHandoffDelay();boss->Brain().UpdateFinale(delay-1);Require(scenario->GetCurrentStep()==4,"Recreated duel handoff early");boss->Brain().UpdateFinale(1);Require(scenario->GetCurrentStep()==5&&WaveCredit(scenario)==1,"Recreated duel lost real45784 handoff");
   }
   std::cout<<"PASS earned4 DB recreation race="<<race<<" killer="<<kind<<" mode="<<mode<<": native ownership/health/presentation/once-only46427/504/45784 or strict refusal\n";
#endif
   boss.reset();stillLive.reset();map.RemoveAllObjectsInRemoveList();
#ifndef MAGNAR_DUEL_RECREATION_BASELINE
   Require(!DuelStories(map),"Final native boss removal orphaned504");
#endif
   CleanStories(map);
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native earned duel recreation cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
