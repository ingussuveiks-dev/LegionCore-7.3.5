// Actual DB load -> native Creature Add/RemoveFromWorld, keeping the companion.
// Earlier escort/guard receipts are controlled; no character persistence.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct RecreationPlayerLink { Player& player; RecreationPlayerLink(Player& p, Map& m):player(p){p.GetMapRef().link(&m,&p);} ~RecreationPlayerLink(){player.GetMapRef().unlink();} };
struct LoadedMagnar final : Creature {
    MagnarAI& Brain(){auto ai=dynamic_cast<MagnarAI*>(AI());Require(ai,"Actual DB script factory did not select Magnar");return *ai;}
    ~LoadedMagnar(){CombatStop(true);CastStop();RemoveAllAuras();m_Events.KillAllEvents(true);if(IsInWorld())GetMap()->RemoveFromMap(static_cast<Creature*>(this),false);ResetMap();}
};
static void CompleteWaves(LoadedMagnar& boss,Player& player,Unit* killer,Scenario* scenario){
   boss.Brain().stop_combat=true;boss.Brain().LaunchEventWave(boss.Brain().wave++);unsigned deaths=0;while(boss.Brain().first_phase&&deaths<10){auto active=boss.Brain().activeWave;Require(!active.empty(),"Actual recreated wave stuck");for(auto id:active){auto member=boss.GetMap()->GetCreature(id);Require(member&&member->IsAlive(),"Actual recreated opponent absent");member->SetLootRecipient(&player);member->LowerPlayerDamageReq(uint32(member->GetMaxHealth()));killer->Kill(member,false);killer->Kill(member,false);++deaths;}}
   Require(deaths==10&&WaveCredit(scenario)==1&&scenario->GetCurrentStep()==4,"Actual recreated10 deaths did not advance46427 exactly once");
}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 Shadow("creature","guid=361400");Shadow("smart_scripts","source_type=0 AND entryorguid=94813");Shadow("creature_template_addon","entry IN(96034,96468,97123,94707,94813)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sObjectMgr->LoadCreatures();
 // Keep the actual loaded DB row while preventing a second automatic grid copy
 // when an active native reserve summons loads this same grid.
 sObjectMgr->RemoveCreatureFromGrid(361400,sObjectMgr->GetCreatureData(361400));
 sObjectMgr->LoadCreatureTemplateAddons();sSmartScriptMgr->LoadSmartAIFromDB();
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;new npc_magnar_icebreaker;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto originalLoot=lootName;lootName+=" WHERE Entry IN(108218,94707,94813) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=originalLoot;Require(loaded,"Actual reserve loot load failed");
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)})for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<12;++mode){
#ifdef MAGNAR_RECREATION_BASELINE
  if(kind||mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9999300+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4715,346,-37.7f,0});RecreationPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);
   HruthnirActor companion(&map,96468,16302,{4720.80f,347.91f,-37.71f,0});companion.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&companion)),"Companion admission failed");companion.SetPhaseId({6999},false);script->OnCreatureCreate(&companion);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;
   auto load=[&]{auto boss=std::make_unique<LoadedMagnar>();Require(boss->LoadCreatureFromDB(361400,&map,false),"Actual original DB boss load failed");Require(map.AddToMap(static_cast<Creature*>(boss.get())),"Actual original DB boss admission failed");Require(boss->GetDBTableGUIDLow()==361400&&boss->GetPhases().count(6999),"Original DB boss identity/phase changed");boss->Brain();return boss;};
   auto boss=load();auto& brain=companion.Brain();brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);brain.magnarStartInstance=map.GetInstanceId();brain.magnarStartOwner=player.GetGUID();brain.magnarStartAttempt=player.GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;auto token=brain.magnarStartToken;
   brain.MovementInform(POINT_MOTION_TYPE,7);Require(brain.magnarStartPresented&&StartStories(map)==1,"Real497 start scene absent");companion.Tick(5999);Require(!*token,"Actual cue ran early");companion.Tick(1);std::cout<<"CONTROL start token="<<*token<<" encounter="<<boss->Brain().encounterStarted<<" receipt="<<boss->Brain().GetData(MagnarStart::Accepted)<<" first="<<boss->Brain().first_phase<<" roster="<<bool(boss->Brain().reserveGUIDs[11])<<" phase="<<boss->GetPhases().count(6999)<<" world="<<boss->IsInWorld()<<" alive="<<boss->IsAlive()<<" step="<<scenario->GetCurrentStep()<<std::endl;Require(*token&&boss->Brain().GetData(MagnarStart::Accepted)&&boss->Brain().reserveGUIDs[11],"Actual first start was not acknowledged");
#ifndef MAGNAR_RECREATION_BASELINE
   if(mode==9){std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit* killer=&player;if(kind==1){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(kind==2){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}CompleteWaves(*boss,player,killer,scenario);}
#endif
   auto old=boss->GetGUID();auto roster=boss->Brain().reserveGUIDs;
#ifndef MAGNAR_RECREATION_BASELINE
   if(mode==11){map.GetCreature(roster[0])->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();}if(mode==2)boss->IsAIEnabled=false;
#endif
   std::unique_ptr<LoadedMagnar> stillLive;
   if(mode==10)stillLive=std::move(boss);else boss.reset();map.RemoveAllObjectsInRemoveList();unsigned orphaned=0;for(auto id:roster)if(map.GetCreature(id))++orphaned;
#ifndef MAGNAR_RECREATION_BASELINE
   Require(mode==10||!orphaned,"Retired first-phase boss orphaned native summons");
#endif
   boss=load();
#ifndef MAGNAR_RECREATION_BASELINE
   if(mode==1){boss.reset();map.RemoveAllObjectsInRemoveList();boss=load();Require(script->magnarRemovedBosses.size()==2,"Intermediate native recreation chain lost a removal");}
   if(mode==3)boss->IsAIEnabled=false;if(mode==4)companion.IsAIEnabled=false;if(mode==5)script->magnarGuardsDelivered=false;if(mode==6)script->Initialize();if(mode==7)(*boss).*Access(AltheaGuardDbTag{})=361401;if(mode==8)brain.magnarStartToken=std::make_shared<bool>(false);
   if(mode>=3&&mode<=7){brain.UpdateMagnarReoffer(1000);Require(brain.magnarStartToken==token&&!brain.magnarOfferToken&&!boss->Brain().encounterStarted,"Unavailable recreation context consumed start");if(mode==3)boss->IsAIEnabled=true;if(mode==4)companion.IsAIEnabled=true;if(mode==5)script->magnarGuardsDelivered=true;if(mode==7)(*boss).*Access(AltheaGuardDbTag{})=361400;}
#endif
   Require(boss->GetGUID()!=old,"Actual instance DB loader reused the old full GUID");
   for(unsigned elapsed=0;elapsed<10000;elapsed+=1000)brain.UpdateMagnarReoffer(1000);
#ifdef MAGNAR_RECREATION_BASELINE
   Require(orphaned==12&&brain.magnarStartToken==token&&*token&&brain.magnarStartedBoss==old&&!brain.magnarOfferToken&&!boss->Brain().GetData(MagnarStart::Accepted)&&!boss->Brain().reserveGUIDs[0]&&!WaveCredit(scenario),"Pinned full DB recreation dead end did not reproduce");
   std::cout<<"COUNTERFACTUAL actual DB recreation race="<<race<<": native361400 remove/reload produces a new fullGUID while surviving companion retains its consumed old-boss start;10s no offer/reserve/46427;orphaned old reserves="<<orphaned<<". Expected broken,not successful gameplay\n";
#else
   bool denied=mode==6||mode==8||mode==9||mode==10;
   if(denied){if(mode==9)Require(script->magnarRemovedBosses.empty()&&scenario->GetCurrentStep()==4,"Earned combat removal manufactured a first-phase recreation receipt");Require(brain.magnarStartToken&&!brain.magnarOfferToken&&!boss->Brain().GetData(MagnarStart::Accepted)&&WaveCredit(scenario)==(mode==9?1u:0u),"Unobserved/epoch/pending/step4 recreation manufactured offer or progress");}
   else{
   std::cout<<"CONTROL reoffer token="<<bool(brain.magnarStartToken)<<" combat="<<companion.isInCombat()<<" offer="<<bool(brain.magnarOfferToken)<<" removed="<<script->magnarRemovedBosses.size()<<std::endl;
   Require(!brain.magnarStartToken&&!boss->Brain().GetData(MagnarStart::Accepted)&&!WaveCredit(scenario),"Actual original boss recreation did not release only its consumed start for ordinary reoffer");
   for(unsigned elapsed=0;elapsed<30000&&!brain.magnarOfferToken;elapsed+=100){companion.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&companion);brain.UpdateAI(100);}
   Require(brain.magnarOfferToken&&!brain.GetData(1061),"Actual native point6 arrival did not arm the original offer");companion.Tick(12999);Require(!brain.GetData(1061),"Recreation skipped native13s offer");companion.Tick(1);Require(brain.GetData(1061)&&!StartStories(map)&&!WaveCredit(scenario)&&!boss->Brain().GetData(MagnarStart::Accepted),"Native recreation offer must await real selection,497 and6s start");
   auto& menu=player.PlayerTalkClass->GetGossipMenu();menu.SetMenuId(18743);menu.AddMenuItem(0,GossipOptionNpc::None,"",0,0,"",0);brain.sGossipSelect(&player,18743,0);
   for(unsigned elapsed=0;elapsed<30000&&!brain.magnarStartPresented;elapsed+=100){companion.Unit::Update(100);map.MoveAllCreaturesInMoveList();player.Relocate(&companion);brain.UpdateAI(100);}
   Require(brain.magnarStartPresented&&StartStories(map)==1&&!boss->Brain().encounterStarted,"Actual recreation selection/point7/497 missing");companion.Tick(5999);Require(!boss->Brain().encounterStarted,"Recreation skipped native6s start");companion.Tick(1);Require(boss->Brain().GetData(MagnarStart::Accepted)&&boss->Brain().reserveGUIDs[11],"Actual new boss acknowledgement/reserve missing");
   auto fresh=boss->Brain().reserveGUIDs;std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit* killer=&player;if(kind==1){pet=std::make_unique<CreditPet>(player,map);killer=pet.get();}if(kind==2){guardian=std::make_unique<CreditGuardian>(player,map);killer=guardian.get();}
   CompleteWaves(*boss,player,killer,scenario);killer->DealDamage(boss.get(),UINT32_MAX-4096,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);Require(boss->Brain().finaleStarted,"Actual recreated lethal hit lost504");auto delay=boss->Brain().GetFinaleHandoffDelay();boss->Brain().UpdateFinale(delay-1);Require(scenario->GetCurrentStep()==4,"Recreated504 advanced early");boss->Brain().UpdateFinale(1);Require(scenario->GetCurrentStep()==5&&WaveCredit(scenario)==1,"Actual recreated45784 handoff missing");
   for(auto id:fresh)if(auto member=map.GetCreature(id))member->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();
   }
   std::cout<<"PASS actual DB recreation race="<<race<<" killer="<<kind<<" mode="<<mode<<": native remove/reload/owned cleanup and ordinary point6/13s/menu/point7/497/6s/native waves/504 or scoped refusal\n";
#endif
   boss.reset();stillLive.reset();for(auto id:roster)if(auto member=map.GetCreature(id))member->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();CleanStories(map);
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Magnar recreation cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
