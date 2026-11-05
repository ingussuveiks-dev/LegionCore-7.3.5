// Whole original/repaired AI; real native Player/Pet/Map/Conversation.
// Earlier quest/stage/pose and direct LOS invocation are controlled.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "DevourerInstance.inc"
#include "Zirus.inc"
static uint32 Stories1884(Map& map){uint32 count=0;for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1884)++count;return count;}
static void PlaceIntro(NativePlayer& player,Position const& pos){player.RemoveFromGrid();player.Relocate(pos);player.GetMap()->LoadGrid(pos.GetPositionX(),pos.GetPositionY());player.GetMap()->AddToGrid(static_cast<Player*>(&player),Cell(Trinity::ComputeCellCoord(pos.GetPositionX(),pos.GetPositionY())));}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1620,{}},{1607,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1620,{}},{1607,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_devourer;sScriptMgr->SwapScriptContext(true);uint32 cases=0;
#ifdef ZIRUS_INTRO_BASELINE
 uint32 modes=6;
#else
#ifdef ZIRUS_INTRO_FAULTS
 uint32 modes=2;
#else
 uint32 modes=18;
#endif
#endif
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<modes;++mode){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9931000+cases,DIFFICULTY_N_SCENARIO,&parent);if(mode!=15)map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(3);auto first=Session(),second=Session(),third=Session();
  {Position pose{1854.8f,1327.93f,66.22f,0};auto distantPose=pose;distantPose.m_positionX+=100;NativePlayer rogue(first,map,101,distantPose),visitor(second,map,102,pose),outsider(third,map,103,distantPose);
   for(auto p:{&rogue,&visitor}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);p->setFactionForRace(race);p->SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);p->SetQuestSlot(0,41924);}
   std::unique_ptr<CreditParty> party;if(mode==17)party=std::make_unique<CreditParty>(&visitor,&rogue);
   auto lines=const_cast<std::vector<ConversationData>*>(sConversationDataStore->GetConversationData(uint32(1884)));Require(lines&&lines->size()==1,"Original1884 missing");auto original=*lines;
#ifdef ZIRUS_INTRO_FAULTS
   lines->clear();
#else
   if(mode==0)visitor.SetGameMaster(true);if(mode==1)visitor.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);if(mode==2){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);}if(mode==3)scenario->SetCurrentStep(2);
#ifdef ZIRUS_INTRO_BASELINE
   if(mode==4)lines->clear();
#else
   if(mode==4)visitor.SetGameMaster(true);
#endif
   if(mode==6)visitor.Unit::setDeathState(CORPSE);if(mode==7)visitor.SetPhaseMask(2,false);if(mode==8){auto upper=pose;upper.m_positionZ+=40;PlaceIntro(visitor,upper);}if(mode==9){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestSlot(0,0);const_cast<RewardedQuestSet&>(visitor.getRewardedQuests()).insert(41924);}if(mode==10)scenario->SetCurrentStep(4);if(mode==11)visitor.SetQuestStatus(41924,QUEST_STATUS_COMPLETE);if(mode==12){auto outside=pose;outside.m_positionX+=26;PlaceIntro(visitor,outside);}
#endif
   Actor source(&map,mode==14?105660:105542,37001,pose);Require(map.AddToMap(static_cast<Creature*>(&source)),"Native source missing");auto ai=new npc_zirus_probe::npc_devourer_zirusAI(&source);Require(source.AIM_Initialize(ai),"Whole Zirus intro missing");
#ifndef ZIRUS_INTRO_BASELINE
#ifndef ZIRUS_INTRO_FAULTS
   if(mode==13)source.Unit::setDeathState(CORPSE);if(mode==16){CreditPet pet(rogue,map);ai->MoveInLineOfSight(&pet);ai->MoveInLineOfSight(&source);ai->MoveInLineOfSight(nullptr);Require(!ai->introDone&&!Stories1884(map),"Nonplayer consumed intro");}
#endif
#endif
   ai->MoveInLineOfSight(&visitor);
#ifdef ZIRUS_INTRO_BASELINE
   Require(ai->introDone&&Stories1884(map)==uint32(mode!=4),"Pinned one-shot counterfactual changed");CleanStories(map);*lines=original;scenario->SetCurrentStep(3);PlaceIntro(visitor,distantPose);PlaceIntro(rogue,pose);ai->MoveInLineOfSight(&rogue);ai->UpdateAI(1000);Require(!Stories1884(map),"Pinned consumed intro replayed");
#else
#ifdef ZIRUS_INTRO_FAULTS
   Require(!ai->introDone&&!Stories1884(map),"Failed native factory consumed intro");*lines=original;if(mode==1){visitor.SetQuestStatus(41924,QUEST_STATUS_NONE);visitor.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);}ai->UpdateAI(999);Require(!Stories1884(map),"Native retry before1s");ai->UpdateAI(1);Require(ai->introDone&&Stories1884(map)==1,"Stationary qualified retry failed");
#else
   if(mode<5){Require(!ai->introDone&&!Stories1884(map),"Ineligible first invoker consumed intro");*lines=original;scenario->SetCurrentStep(3);if(mode!=4)PlaceIntro(visitor,distantPose);auto qualifiedPose=pose;if(mode==4)qualifiedPose.m_positionX+=1;PlaceIntro(rogue,qualifiedPose);ai->UpdateAI(1000);}
   bool accepted=mode<6||mode==11||mode>=16;Require(ai->introDone==accepted&&Stories1884(map)==uint32(accepted),"Native intro admission differs");
   if(accepted){ai->Reset();ai->MoveInLineOfSight(&rogue);ai->UpdateAI(1000);Require(Stories1884(map)==1,"Accepted intro replayed after Reset/LOS");}
#endif
   for(auto story:*Access(StoriesTag{}))if(story&&story->GetMap()==&map&&story->GetEntry()==1884){Require(story->GetDuration()==7500&&story->GetActors().size()==1&&story->GetActors()[0]->actorId==52523&&story->GetActors()[0]->creatureId==105542&&story->GetActors()[0]->displayId==69190&&story->GetActors()[0]->unk1==0&&story->GetActors()[0]->unk2==1&&story->GetActors()[0]->unk3==0&&story->GetLines().size()==1&&story->GetLines()[0]->id==3989&&story->GetLines()[0]->textId==0,"Native1884 original virtual role/payload/lifetime changed");}
#endif
   *lines=original;Require(scenario->GetCurrentStep()==((mode==10)?4:3)&&rogue.GetQuestStatus(41924)==QUEST_STATUS_INCOMPLETE&&!rogue.HasItemCount(128476,1),"Intro granted progression/item");CleanStories(map);
  }for(auto s:{first,second,third}){s->SetPlayer(nullptr);delete s;}sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
#ifdef ZIRUS_INTRO_BASELINE
 std::cout<<"COUNTERFACTUAL "<<cases<<" native GM/class/quest/stage/refused1884 one-shot contexts; later legitimate stationary arrival cannot restore consumed intro. Earlier state/LOS controlled, client separate\n";
#else
 std::cout<<"PASS "<<cases<<" whole-class native Zirus1884 admission/retry/once/payload contexts; original3989/7500 and virtual actor, no progress grant. Earlier state/direct LOS/clock endpoints controlled, client separate\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
