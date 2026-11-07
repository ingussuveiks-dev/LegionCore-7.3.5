// Actual native login handlers and DB2/LFG route; no character DB writes.
#define MAGNAR_RECREATION_SUPPORT_ONLY
#include "MagnarRecreationNative.cpp"
#include "LFGMgr.h"
#include "AreaTriggerData.h"
struct MagnarResumeTag { using Type=ScenarioResume Player::*; friend Type Access(MagnarResumeTag); };
template struct FixtureAccess<MagnarResumeTag,&Player::m_scenarioResume>;
static void Admit(Player& player,unsigned race){player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},false);}
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 Shadow("creature","guid=361400");Shadow("smart_scripts","source_type=0 AND entryorguid=94813");Shadow("creature_template_addon","entry IN(96034,96468,97123,94707,94813)");
#ifndef MAGNAR_LOGIN_LIVE
 Shadow("lfg_entrances","1");
#ifdef MAGNAR_LOGIN_BASELINE
 WorldDatabase.DirectExecute("DELETE FROM lfg_entrances WHERE dungeonId=1028");
#else
 std::string migration=
#include "MagnarLoginMigration.inc"
 ;std::string statement;bool quoted=false;for(char ch:migration){if(ch=='\'')quoted=!quoted;if(ch==';'&&!quoted){WorldDatabase.DirectExecute(statement.c_str());statement.clear();}else statement+=ch;}
#endif
#endif
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sObjectMgr->LoadCreatures();sObjectMgr->RemoveCreatureFromGrid(361400,sObjectMgr->GetCreatureData(361400));sObjectMgr->LoadCreatureTemplateAddons();sSmartScriptMgr->LoadSmartAIFromDB();
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();
 auto native=sLfgDungeonsStore.LookupEntry(1028);auto dungeon=sLFGMgr->GetLFGDungeon(uint16(909),uint16(1495));auto spell=sSpellMgr->GetSpellInfo(187803);
 Require(native&&native->ScenarioID==909&&native->MapID==1495&&native->DifficultyID==12&&(native->Flags&LFG_FLAG_USER_TELEPORT_NOT_ALLOWED)&&dungeon&&dungeon->id==1028,"Matching native Protection route changed");
 Require(spell&&std::string(spell->SpellName)=="Jump to Prot Scenario"&&spell->GetEffect(EFFECT_1)->Effect==227,"Matching native Protection entry spell changed");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;new npc_magnar_icebreaker;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 auto& lootName=LootTemplates_Creature.*Access(LootNameTag{});auto original=lootName;lootName+=" WHERE Entry IN(108218,94707,94813) ORDER BY Entry,Item";auto loaded=(LootTemplates_Creature.*Access(LootLoadTag{}))();lootName=original;Require(loaded,"Actual reserve loot absent");
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  ScenarioResume saved;auto session=Session();unsigned instanceId=9999700+race;Position chamber{4715,346,-37.7f,0};
  {
   MapInstanced parent(1495,300);InstanceMap map(1495,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
   auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual old shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);
   auto boss=std::make_unique<LoadedMagnar>();
   {
    NativePlayer player(session,map,901,chamber);RecreationPlayerLink link(player,map);Admit(player,race);
    HruthnirActor companion(&map,96468,16502,{4720.80f,347.91f,-37.71f,0});companion.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&companion)),"Companion admission failed");companion.SetPhaseId({6999},false);script->OnCreatureCreate(&companion);script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;
    Require(boss->LoadCreatureFromDB(361400,&map,false)&&map.AddToMap(static_cast<Creature*>(boss.get())),"Original DB boss load failed");auto& brain=companion.Brain();brain.magnarChamberStarted=true;brain.magnarIntroInstanceGeneration=script->GetData(1060);brain.magnarStartInstance=instanceId;brain.magnarStartOwner=player.GetGUID();brain.magnarStartAttempt=player.GetQuestCancellationReceipt(39191);brain.magnarStartToken=std::make_shared<bool>(false);brain.magnarArrivalPending=true;brain.MovementInform(POINT_MOTION_TYPE,7);companion.Tick(6000);Require(boss->Brain().GetData(MagnarStart::Accepted),"Native first start not acknowledged");
    CompleteWaves(*boss,player,&player,scenario);Require(script->magnarCombat.earned,"Real owned10/46427 did not earn4");
    player.RecordScenarioEntrance();saved=player.*Access(MagnarResumeTag{});
#ifdef MAGNAR_LOGIN_BASELINE
    Require(!saved.scenario&&!dungeon->x&&!dungeon->y&&!dungeon->z,"Missing native LFG entrance did not reproduce");std::cout<<"COUNTERFACTUAL native missing1028 race="<<race<<": actual10 kills/46427/4,legacy absent checkpoint fails RecordScenarioEntrance;LFG user teleport remains disabled. Expected broken,not successful gameplay"<<std::endl;++cases;
#else
    Require(saved.map==1495&&saved.scenario==909&&fabs(saved.x-4916.3f)<0.001f&&fabs(saved.y+94)<0.001f&&fabs(saved.z-66.3f)<0.001f,"Actual native Protection entrance checkpoint missing/wrong");
    map.LoadGrid(saved.x,saved.y);float floor=map.GetHeight(player.GetPhases(),saved.x,saved.y,saved.z+5);std::cout<<"CONTROL native Protection entrance floor="<<floor<<" savedZ="<<saved.z<<std::endl;Require(fabs(floor-saved.z)<2,"Native Protection entry is not near a supported floor");
#endif
   }
#ifndef MAGNAR_LOGIN_BASELINE
   for(unsigned mode=0;mode<3;++mode){
    NativePlayer player(session,map,901,chamber);RecreationPlayerLink link(player,map);Admit(player,race);Require((player.*Access(MagnarResumeTag{})).Read(saved.Write()),"Real checkpoint serialization failed");if(mode==1)(player.*Access(MagnarResumeTag{})).boot=saved.boot+1;
    player.PrepareScenarioLogin(1495,instanceId+(mode==2));if(!mode)Require(player.GetScenarioLoginInstance()==instanceId&&player.GetExactDist(chamber)<0.001f,"Exact live instance did not retain real position");else{Require(!player.GetScenarioLoginInstance()&&player.GetScenarioId()==909&&player.GetExactDist(saved.x,saved.y,saved.z)<0.001f,"Fresh boot/numeric mismatch did not use native entrance");}
    Require(scenario->GetCurrentStep()==4&&WaveCredit(scenario)==1&&script->magnarCombat.earned&&!boss->Brain().first_phase&&!boss->Brain().finaleStarted,"Login altered actual earned old duel or granted victory");player.Relocate(chamber);std::cout<<"PASS native Protection login race="<<race<<" mode="<<mode<<std::endl;++cases;
   }
#endif
   boss.reset();map.RemoveAllObjectsInRemoveList();sScenarioMgr->RemoveScenario(instanceId);
  }
#ifndef MAGNAR_LOGIN_BASELINE
  // Both actual old Map/InstanceScript objects are now gone. Recycle exactly
  // the numeric ID, then construct through the real native LFG constructor.
  {
   MapInstanced parent(1495,300);InstanceMap fresh(1495,300,instanceId,DIFFICULTY_N_SCENARIO,&parent);fresh.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(fresh.GetInstanceScript());Require(script,"Actual fresh shared instance missing");
   {
    NativePlayer player(session,fresh,901,chamber);RecreationPlayerLink link(player,fresh);Admit(player,race);Require((player.*Access(MagnarResumeTag{})).Read(saved.Write()),"Fresh checkpoint read failed");player.PrepareScenarioLogin(1495,instanceId);Require(!player.GetScenarioLoginInstance()&&player.GetScenarioId()==909&&player.GetExactDist(saved.x,saved.y,saved.z)<0.001f,"Destroyed map numeric reuse retained stale authority/pose");
    auto progress=sScenarioMgr->AddScenario(&fresh,dungeon,&player,false);Require(progress&&progress->GetScenarioId()==909&&progress->GetCurrentStep()==0&&WaveCredit(progress)==0&&!player.GetScenarioId(),"Actual fresh LFG constructor restored old4/credit or failed selector consumption");
    Require(!script->magnarCombat.earned&&script->magnarRetiredCombat.actor.IsEmpty()&&script->magnarCombat.actor.IsEmpty()&&player.CanResumeScenarioLogin(909)&&!player.HasItemCount(128289,1,true),"Fresh instance inherited old combat/items or lost login admission");player.RecordScenarioEntrance();Require((player.*Access(MagnarResumeTag{})).instance==instanceId,"Fresh actual entrance was not retained");player.Relocate(chamber);++cases;std::cout<<"PASS actual full Map destruction/recycledID/fresh909-0 race="<<race<<": no inherited46427/45784/item;client traversal separate"<<std::endl;
   }sScenarioMgr->RemoveScenario(instanceId);
  }
#endif
  session->SetPlayer(nullptr);delete session;
 }
 std::cout<<"PASS native Magnar login cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
