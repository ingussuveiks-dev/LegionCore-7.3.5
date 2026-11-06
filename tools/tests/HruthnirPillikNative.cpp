// Actual shared-instance death record, AI-owned cue and native scenario criterion.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
struct PillikActor:Actor {
 using Actor::Actor;
 void AddToWorld()override{Actor::AddToWorld();Require(AIM_Initialize(new SmartAI(this)),"Native Pillik SmartAI missing");}
};
struct PillikMapLink {
 Player& player;PillikMapLink(Player& p,Map& map):player(p){player.GetMapRef().link(&map,&player);}
 ~PillikMapLink(){player.GetMapRef().unlink();}
};
static uint64 PillikCredit(Scenario* scenario,uint32 event=46289){uint64 count=0;for(auto const& row:*scenario->GetAchievementMgr().GetCriteriaProgressMap()){auto tree=sCriteriaTreeStore.LookupEntry(row.first);auto c=tree?sCriteriaStore.LookupEntry(tree->CriteriaID):nullptr;if(c&&c->Type==92&&c->Asset==event)count+=row.second.Counter;}return count;}
static void PillikRegister(InstanceScript* script,Creature& actor){script->OnCreatureCreate(&actor);script->OnCreatureCreateForScript(&actor);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 auto first=sCriteriaStore.LookupEntry(28952),second=sCriteriaStore.LookupEntry(28953);
 Require(first&&second&&first->Type==92&&first->Asset==46288&&second->Type==92&&second->Asset==46289,"Matching native Protection entrance criteria differ");
 auto route=WorldDatabase.Query("SELECT COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id=9859004");Require(route&&route->Fetch()[0].GetUInt32()==7&&route->Fetch()[1].GetUInt32()==1&&route->Fetch()[2].GetUInt32()==7,"Original7-node Hruthnir route missing");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 Shadow("smart_scripts","source_type=0 AND entryorguid=94714");Shadow("creature_text","CreatureID IN(96468,94714)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSmartScriptMgr->LoadSmartAIFromDB();sCreatureTextMgr->LoadCreatureTexts();
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto originalLootName=lootName;lootName+=" WHERE Entry IN(108218,108216,108217,96571) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=originalLootName;Require(loaded&&LootTemplates_Creature.HaveLootFor(96571),"Native Rattling Dead cleanup loot inputs missing");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 kind=0;kind<3;++kind)for(uint32 mode=0;mode<19;++mode){
#ifdef HRUTHNIR_ENTRY_BASELINE
  if(mode!=13)continue;
#endif
#ifdef HRUTHNIR_PILLIK_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9989200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
  auto script=map.GetInstanceScript();Require(dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(script),"Exported actual shared instance missing");
  auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(mode==7||mode>=13?0:1);auto session=Session();auto peerSession=Session();
  {
   NativePlayer owner(session,map,901,{4820,140,-9.1f,0});PillikMapLink ownerLink(owner,map);
   auto prepare=[&](Player& p){p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.setFactionForRace(race);p.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,39191);p.SetPhaseId({6999},true);};prepare(owner);
   HruthnirActor companion(&map,96468,12202,{4874.49f,102.114f,13.1651f,0});companion.*Access(AltheaGuardDbTag{})=361306;
   Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native Hruthnir admission failed");companion.SetPhaseId({6999},true);PillikRegister(script,companion);
   PillikActor boss(&map,94714,12201,{4807.08f,142.043f,-9.1058f,0});boss.*Access(AltheaGuardDbTag{})=361361;
   Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native Pillik admission failed");boss.SetPhaseId({6999},true);PillikRegister(script,boss);
   Actor scenery(&map,96571,12204,{4808.08f,142.043f,-9.1058f,0}),foreign(&map,96571,12205,{4809.08f,142.043f,-9.1058f,0});
   Require(map.AddToMap(static_cast<Creature*>(&scenery))&&map.AddToMap(static_cast<Creature*>(&foreign)),"Native scenery admission failed");scenery.SetPhaseId({6999},true);foreign.SetPhaseId({6985},true);
   std::unique_ptr<NativePlayer> peer;std::unique_ptr<PillikMapLink> peerLink;std::unique_ptr<CreditParty> party;
   if(mode==9||mode==14||mode==18){peer=std::make_unique<NativePlayer>(peerSession,map,902,companion.GetPosition());prepare(*peer);peerLink=std::make_unique<PillikMapLink>(*peer,map);if(mode==9||mode==18)party=std::make_unique<CreditParty>(&owner,peer.get());else peer->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_MAGE);}
   auto& brain=companion.Brain();
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit*killer=&owner;
   if(kind==1){pet=std::make_unique<CreditPet>(owner,map);pet->SetPhaseId({6999},true);killer=pet.get();}
   if(kind==2){guardian=std::make_unique<CreditGuardian>(owner,map);guardian->SetPhaseId({6999},true);killer=guardian.get();}
   auto kill=[&]{boss.SetLootRecipient(&owner);boss.LowerPlayerDamageReq(uint32(boss.GetMaxHealth()));
    if(mode==6){boss.SetMaxHealth(1000);boss.SetHealth(1000);killer->DealDamage(&boss,UINT32_MAX-4096u,nullptr,DIRECT_DAMAGE,SPELL_SCHOOL_MASK_NORMAL,nullptr,false);}else killer->Kill(&boss,false);
    killer->Kill(&boss,false);Require(!boss.IsAlive(),"Native Pillik killing blow failed");};
   if(mode==7){kill();Require(scenario->GetCurrentStep()==0&&!PillikCredit(scenario),"Early death fabricated prerequisite46288");}
   if(mode==13)owner.Relocate(5200,140,-9.1f,0);
   if(mode==15)owner.Unit::setDeathState(CORPSE);
   if(mode==16)owner.SetQuestStatus(39191,QUEST_STATUS_NONE);
   if(mode==17){brain.UpdateAI(500);Require(scenario->GetCurrentStep()==0&&!PillikCredit(scenario,46288),"Entry poll fabricated a missing waypoint");}
   brain.MovementInform(WAYPOINT_MOTION_TYPE,6);
   Require(scenario->GetCurrentStep()==(mode==13||mode==15||mode==16?0:1)&&!brain.secondfighting&&!companion.isInCombat(),"Native approach/prerequisite endpoint differs");
   if(mode==1){brain.UpdateLifecycle(4999);brain.UpdateAI(0);Require(!PillikCredit(scenario),"Native five-second cue ran early");}
   if(mode==2){brain.UpdateLifecycle(5000);brain.UpdateAI(0);Require(brain.secondfighting&&companion.isInCombat()&&!PillikCredit(scenario),"Native ordinary combat request failed");}
   if(mode==3){map.RemoveFromMap(static_cast<Creature*>(&boss),false);brain.UpdateLifecycle(5000);brain.UpdateAI(500);Require(scenario->GetCurrentStep()==1&&!PillikCredit(scenario),"Missing living actor was treated as victory");if(!boss.FindMap())boss.SetMap(&map);Require(boss.FindMap()==&map,"Native fixture lost map before readmission");Require(map.AddToMap(static_cast<Creature*>(&boss)),"Native missing actor readmission failed");PillikRegister(script,boss);}
   if(mode==10){brain.UpdateLifecycle(1000);script->Initialize();PillikRegister(script,companion);PillikRegister(script,boss);brain.MovementInform(WAYPOINT_MOTION_TYPE,6);}
   if(mode==9){Actor clone(&map,94714,12203,boss.GetPosition());Require(map.AddToMap(static_cast<Creature*>(&clone)),"Native unrelated clone admission failed");clone.SetPhaseId({6999},true);PillikRegister(script,clone);killer->Kill(&clone,false);Require(!PillikCredit(scenario)&&scenario->GetCurrentStep()==1&&!script->GetData(PillikHandoff::Death),"Foreign94714 death counted for the original spawn");}
   if(mode==11)boss.IsAIEnabled=false;
   if(mode==12)owner.Unit::setDeathState(CORPSE);
   if(mode!=7)kill();
   if(mode==11)boss.IsAIEnabled=true;
   if(mode==4)map.RemoveFromMap(static_cast<Creature*>(&boss),false);
   if(mode==8)owner.SetQuestStatus(39191,QUEST_STATUS_NONE);
   std::unique_ptr<PausedAI> pause;if(mode==5)pause=std::make_unique<PausedAI>(companion,false);
   if(mode==10){brain.UpdateLifecycle(4000);brain.UpdateAI(500);Require(!PillikCredit(scenario),"Old epoch's five-second callback consumed a fresh cue");brain.UpdateLifecycle(1000);}
   else if(mode!=2&&mode!=3){brain.UpdateLifecycle(mode==1?1:5000);}
   brain.UpdateAI(500);script->Update(500);
   if(mode==13||mode==15||mode==16){Require(scenario->GetCurrentStep()==0&&!PillikCredit(scenario),"Unavailable owner fabricated prerequisite");if(mode==13)owner.Relocate(companion.GetPosition());if(mode==15)owner.Unit::setDeathState(ALIVE);if(mode==16)owner.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);brain.UpdateAI(499);Require(scenario->GetCurrentStep()==0,"Native entry retry ran early");brain.UpdateAI(1);}
   if(mode==5||mode==8){Require(scenario->GetCurrentStep()==1&&!PillikCredit(scenario),"Unavailable native AI/eligible player consumed handoff");if(pause)pause->Restore();if(mode==8)owner.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);brain.UpdateAI(499);Require(!PillikCredit(scenario),"Native500ms retry ran early");brain.UpdateAI(1);}
#if defined(HRUTHNIR_ENTRY_BASELINE)
   Require(scenario->GetCurrentStep()==0&&!PillikCredit(scenario),"Pinned missing entry no longer reproduces");
   std::cout<<"COUNTERFACTUAL native entry race="<<race<<" killer="<<kind<<": original waypoint occurs outside100yd, real early Kill and5s cue then returned eligible player remain909 step0/no46288; earlier path endpoint controlled\n";
#elif defined(HRUTHNIR_PILLIK_BASELINE)
   Require(!brain.secondfighting&&scenario->GetCurrentStep()==1&&!PillikCredit(scenario),"Pinned missing Pillik handoff no longer reproduces");
   std::cout<<"COUNTERFACTUAL native Pillik race="<<race<<" killer="<<kind<<": actual early Unit::Kill/duplicate plus actual AI-owned five-second cue leaves909 step1 with no46289; earlier route endpoint controlled, not passing client traversal\n";
#else
   Require(scenario->GetCurrentStep()==2&&PillikCredit(scenario)==1&&brain.pillikProgress->complete&&brain.pillikProgress->moveRequested,"Native Pillik handoff did not advance909 step1->2 exactly once");
   if(mode==7||mode>=13)Require(PillikCredit(scenario,46288)==1,"Actual waypoint prerequisite not exactly once");
   Require(!scenery.IsAlive()&&foreign.IsAlive(),"Early victory lost authored scenery cleanup or affected a foreign phase");
   auto state=brain.pillikProgress;brain.UpdateLifecycle(30000);brain.UpdateAI(500);brain.TryPillikHandoff();
   Require(PillikCredit(scenario)==1&&brain.pillikProgress==state,"Repeated native callback duplicated victory");
   std::cout<<"PASS native Pillik race="<<race<<" killer="<<kind<<" mode="<<mode<<": actual death/AI-owned5s/46289/909 stage1->2 once; native route/prerequisite/loot-owner and real motion request retained, earlier waypoint endpoint controlled\n";
#endif
   pause.reset();companion.CombatStop(true);boss.CombatStop(true);party.reset();peerLink.reset();peer.reset();CleanStories(map);
  }
  session->SetPlayer(nullptr);peerSession->SetPlayer(nullptr);delete session;delete peerSession;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native Pillik contexts="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
