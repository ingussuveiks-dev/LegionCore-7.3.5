#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "DevourerInstance.inc"
#include "Entrance.inc"
struct EntrySceneTag{using Type=uint32 Player::*;friend Type Access(EntrySceneTag);};
template struct FixtureAccess<EntrySceneTag,&Player::sceneInstanceID>;
static void PlaceEntry(NativePlayer& player,Position const& pos){player.RemoveFromGrid();player.Relocate(pos);player.GetMap()->LoadGrid(pos.GetPositionX(),pos.GetPositionY());player.GetMap()->AddToGrid(static_cast<Player*>(&player),Cell(Trinity::ComputeCellCoord(pos.GetPositionX(),pos.GetPositionY())));}
static uint64 EntryLeaf(Scenario* scenario,uint32 id){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto at=rows->find(id);return at==rows->end()?0:at->second.Counter;}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});
 sSpellMgr->LoadSpellScene();sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(29790,29801)");Require(rules&&!rules->Fetch()[0].GetUInt32(),"Entrance criteria need new loader rules");uint32 cases=0;
#ifdef DEVOURER_ENTRANCE_BASELINE
 uint32 modes=4;
#else
#ifdef DEVOURER_ENTRANCE_FAULTS
 uint32 modes=4;
#else
 uint32 modes=24;
#endif
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9925000+cases,DIFFICULTY_N_SCENARIO,&parent);
#ifdef DEVOURER_ENTRANCE_FAULTS
  map.CreateInstanceData(nullptr);
#else
  if(mode!=14)map.CreateInstanceData(nullptr);
#endif
  auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(4);auto first=Session(),second=Session();
  {Position pose{1854.15f,1275.45f,91.7998f,1.54631f};auto roguePose=pose;roguePose.m_positionX+=50;NativePlayer rogue(first,map,101,roguePose),visitor(second,map,102,pose);
   for(auto p:{&rogue,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);}
#ifndef DEVOURER_ENTRANCE_FAULTS
   if(mode==0)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}
   if(mode==4){auto below=pose;below.m_positionZ=90;PlaceEntry(visitor,below);auto arrivalPose=pose;arrivalPose.m_positionX+=10;PlaceEntry(rogue,arrivalPose);}
   if(mode==5){visitor.SetGameMaster(true);auto arrivalPose=pose;arrivalPose.m_positionX+=10;PlaceEntry(rogue,arrivalPose);}
   if(mode==6){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);rogue.SetQuestStatus(41924,QUEST_STATUS_NONE);}if(mode==7)scenario->SetCurrentStep(3);
   if(mode==9)visitor.Unit::setDeathState(JUST_DIED);if(mode==10)visitor.SetPhaseMask(2,false);if(mode==11){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);const_cast<RewardedQuestSet&>(visitor.getRewardedQuests()).insert(41924);}if(mode==12)visitor.SetQuestStatus(41924,QUEST_STATUS_COMPLETE);if(mode==13)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);
   if(mode==16||mode==20||mode==21){auto away=pose;away.m_positionX+=50;PlaceEntry(visitor,away);}if(mode==17)visitor.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50404);if(mode==18)visitor.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50299);
#endif
   Actor source(&map,mode==15?105542:105660,32001,pose);Require(map.AddToMap(static_cast<Creature*>(&source)),"Actual final Akaari absent");auto ai=new npc_akaari_last_probe::npc_akaari_shadowgore_lastAI(&source);Require(source.AIM_Initialize(ai),"Actual full entrance AI unavailable");
   std::unique_ptr<CreditParty> party;std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;
#ifndef DEVOURER_ENTRANCE_BASELINE
#ifndef DEVOURER_ENTRANCE_FAULTS
   if(mode==8)source.Unit::setDeathState(JUST_DIED);
   if(mode==20){pet=std::make_unique<CreditPet>(rogue,map);pet->Relocate(pose);}if(mode==21){guardian=std::make_unique<CreditGuardian>(rogue,map);guardian->Relocate(pose);}
   if(mode==22){PlaceEntry(rogue,pose);party=std::make_unique<CreditParty>(&rogue,&visitor);}
#endif
#endif
#ifdef DEVOURER_ENTRANCE_FAULTS
   auto criterion=mode<2?const_cast<CriteriaEntry*>(sCriteriaStore.LookupEntry(mode==0?29790:29801)):nullptr;uint32 asset=criterion?criterion->Asset:0;if(criterion)criterion->Asset=UINT32_MAX;
   if(mode>=2)visitor.ApplySpellImmune(0,IMMUNITY_ID,209626,true);
#endif
   if(mode==23){visitor.SendSpellScene(1254,sSpellMgr->GetSpellInfo(209626),true,&pose);visitor.SceneCompleted(visitor.*Access(EntrySceneTag{}));}
   ai->UpdateAI(1000);
#ifdef DEVOURER_ENTRANCE_BASELINE
   Require(ai->introDone,"Pinned entrance one-shot not consumed");Require(EntryLeaf(scenario,53045)==(mode==0?0:1)&&EntryLeaf(scenario,47367)==(mode==0?0:1),"Pinned native entrance criteria differ");
   if(mode<3){rogue.Relocate(pose);visitor.Relocate(pose.GetPositionX()+100,pose.GetPositionY(),pose.GetPositionZ(),0);ai->UpdateAI(1000);Require(EntryLeaf(scenario,53045)==(mode==0?0:1)&&EntryLeaf(scenario,47367)==(mode==0?0:1),"Pinned one-shot changed after legitimate arrival");std::cout<<"COUNTERFACTUAL race="<<race<<" mode="<<mode<<" ineligible GM/class/quest visitor consumes entrance; native counters="<<EntryLeaf(scenario,53045)<<','<<EntryLeaf(scenario,47367)<<" legitimate later arrival cannot repair consumed progression\n";}
#else
#ifdef DEVOURER_ENTRANCE_FAULTS
   ai->UpdateAI(1000);
   if(criterion){Require(!ai->introDone&&ai->entryVisual,"Missing native receipt consumed entrance or lost scene");criterion->Asset=asset;ai->UpdateAI(999);Require(!ai->introDone,"Credit retry before1s");ai->UpdateAI(1);Require(ai->introDone&&ai->entryVisual,"Native restored receipt did not recover");}
   else{Require(ai->introDone&&!ai->entryVisual,"Failed native scene blocked progress or claimed launch");visitor.ApplySpellImmune(0,IMMUNITY_ID,209626,false);auto before=visitor.*Access(EntrySceneTag{});if(mode==3){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);}ai->UpdateAI(999);Require((visitor.*Access(EntrySceneTag{}))==before,"Presentation retried before1s");ai->UpdateAI(1);Require(ai->entryVisual==(mode==2),"Presentation recovery/abandoned attempt admission differs");if(mode==3)Require(ai->entryPlayer.IsEmpty()&&!ai->entryReceipt,"Cancelled presentation attempt retained recipient");}
   Require(EntryLeaf(scenario,53045)==1&&EntryLeaf(scenario,47367)==1&&scenario->GetCurrentStep()==5,"Explicit recovered progress differs");
#else
   if(mode<3||mode==16||mode==20||mode==21){Require(!ai->introDone&&!EntryLeaf(scenario,53045)&&!EntryLeaf(scenario,47367),"Ineligible first arrival consumed native entrance");PlaceEntry(rogue,pose);auto away=pose;away.m_positionX+=100;PlaceEntry(visitor,away);ai->UpdateAI(1000);}
   bool accepted=mode<=5||mode==12||mode>=16;
   Require(ai->introDone==accepted&&ai->entryVisual==accepted,"Native entrance admission/scene receipt differs");
   Require(EntryLeaf(scenario,53045)==uint64(accepted)&&EntryLeaf(scenario,47367)==uint64(accepted),"Native entrance leaf receipts differ");
   if(accepted){Require(scenario->GetCurrentStep()==5,"Native entrance did not reach duel step");auto before=visitor.*Access(EntrySceneTag{});if(mode==19)ai->Reset();ai->UpdateAI(1000);Require((visitor.*Access(EntrySceneTag{}))==before&&EntryLeaf(scenario,47367)==1&&EntryLeaf(scenario,53045)==1,"Native acknowledged entrance replayed");}
   else Require(scenario->GetCurrentStep()==(mode==7?3:4),"Refused entrance advanced native step");
#endif
#endif
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" stage="<<scenario->GetCurrentStep()<<" scene="<<visitor.HasSceneStatus(1254,SCENE_LAUNCH)<<'\n';
   party.reset();pet.reset();guardian.reset();for(auto p:{&rogue,&visitor}){p->CastStop();p->RemoveAllAuras();p->m_Events.KillAllEvents(true);}CleanStories(map);
  }for(auto s:{first,second}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" whole-source native Devourer entrance contexts; earlier quest/stage/poses seeded, no client traversal claim\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
