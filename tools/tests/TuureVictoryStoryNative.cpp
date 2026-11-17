// Whole instance, native Unit::Kill/50562/Conversation and original quest leases.
// Quest admission, actor poses and clocks are controlled; no client traversal.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ScriptedGossip.h"
#include "ConversationData.h"
#include "TuurePolicy.inc"
#include "TuureInstance.inc"
struct VictoryDataTag{using Type=ConversationDataMap ConversationDataStoreMgr::*;friend Type Access(VictoryDataTag);};
template struct FixtureAccess<VictoryDataTag,&ConversationDataStoreMgr::_conversationDataList>;
struct VictoryActor:Actor{
 using Actor::Actor;
 void Bind(uint64 spawn){this->*Access(AltheaGuardDbTag{})=spawn;}
 void AddToWorld() override{Actor::AddToWorld();if(auto map=GetMap()->ToInstanceMap())if(auto script=map->GetInstanceScript())script->OnCreatureCreate(this);}
};
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_tuure;sScriptMgr->SwapScriptContext(true);
 auto lines=sConversationDataStore->GetConversationData(uint32(2009));auto roles=sConversationDataStore->GetConversationCreature(2009);
 Require(lines&&lines->size()==4&&roles&&roles->size()==3,"Native2009 data differs");
 Require((*roles)[0].creatureId==106137&&(*roles)[1].creatureId==106134&&!(*roles)[2].creatureId,"Native2009 roles differ");
 auto& data=sConversationDataStore->*Access(VictoryDataTag{});auto original=data.at(2009);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<18;++mode){
#ifdef TUURE_VICTORY_STORY_BASELINE
  if(mode)continue;
#endif
#ifdef TUURE_VICTORY_STORY_FAULTS
  if(mode<16)continue;
#else
  if(mode>=16)continue;
#endif
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9986300+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual instance missing");
  auto scenario=sScenarioMgr->AddScenario(&map,1085);auto session=Session();
  {NativePlayer owner(session,map,710,{0,1235,-52,0});owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.setFactionForRace(race);owner.SetPhaseId({6374},false);owner.SetQuestStatus(41993,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,41993);auto model=sChrRacesStore.LookupEntry(race);Require(model,"Race data missing");owner.SetNativeDisplayId(model->MaleDisplayID);owner.SetDisplayId(model->MaleDisplayID);
   VictoryActor boros(&map,106134,8300,{-6,1235,-52,0}),jace(&map,106137,8301,{-2,1235,-52,0}),opponent(&map,106201,8302,{2,1235,-52,0}),target(&map,106221,8303,{4,1235,-52,0});boros.Bind(361419);jace.Bind(361432);opponent.Bind(361428);target.Bind(361429);
   for(auto actor:{&boros,&jace,&opponent,&target}){actor->SetPhaseId({6374},false);Require(map.AddToMap(static_cast<Creature*>(actor)),"Actual role admission failed");}
   uint32 blocked=UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC|UNIT_FLAG_NOT_ATTACKABLE_1;target.SetUInt32Value(UNIT_FIELD_FLAGS,blocked|0x8000);
   scenario->SetCurrentStep(1);Require(!TuureFirstFight::HasVictory(scenario),"Fresh scenario has victory");
   std::unique_ptr<CreditPet> pet;std::unique_ptr<CreditGuardian> guardian;Unit* killer=&owner;
   if(mode==10){pet=std::make_unique<CreditPet>(owner,map);pet->SetPhaseId({6374},false);killer=pet.get();}
   if(mode==11){guardian=std::make_unique<CreditGuardian>(owner,map);guardian->SetPhaseId({6374},false);killer=guardian.get();}
   opponent.SetLootRecipient(&owner);opponent.LowerPlayerDamageReq(uint32(opponent.GetMaxHealth()));opponent.SetHealth(1);killer->Kill(&opponent,false);killer->Kill(&opponent,false);
   Require(!opponent.IsAlive()&&script->firstFight==decltype(script->firstFight)::Pending&&script->recipientGuid==owner.GetGUID(),"Actual repeated Kill lost original player/pet/guardian receipt");
   data.erase(2009);script->Update(1000);data[2009]=original;
   Require(TuureFirstFight::HasVictory(scenario)&&scenario->GetCurrentStep()==2&&script->firstFight==decltype(script->firstFight)::Unlocking&&script->unlockDelay==16000&&!Stories(map,2009),"Real50562 or independent16000ms unlock missing");
   auto before=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();
#ifdef TUURE_VICTORY_STORY_BASELINE
   script->Update(1000);Require(!Stories(map,2009)&&script->unlockDelay==15000,"Pinned lost2009 unexpectedly recovered or timer changed");std::cout<<"COUNTERFACTUAL race="<<race<<" actual Kill/50562, failed2009 permanently lost;earned unlock continues\n";
#else
   Require(script->victoryStoryPending&&!script->victoryStoryDone,"Failed2009 consumed request");
   if(mode==1)owner.SetQuestStatus(41993,QUEST_STATUS_NONE);
   if(mode==2)owner.getQuestStatus(41993)->ScriptLifetime=std::make_shared<uint8>();
   if(mode==3)map.PlayerRelocation(&owner,300,1235,-52,0);
   if(mode==4)owner.SetGameMaster(true);
   if(mode==5)boros.AddUnitState(UNIT_STATE_STUNNED);
   if(mode==6)scenario->SetCurrentStep(3);
   if(mode==7)map.RemoveFromMap(static_cast<Creature*>(&jace),false);
   if(mode==8)owner.SetPhaseId({6375},false);
   std::unique_ptr<VictoryActor> foreign;
   if(mode==9){foreign=std::make_unique<VictoryActor>(&map,106137,8304,boros.GetPosition());foreign->Bind(999999);foreign->SetPhaseId({6374},false);Require(map.AddToMap(static_cast<Creature*>(foreign.get())),"Foreign role control missing");}
   if(mode==12){owner.Unit::setDeathState(CORPSE);owner.SetHealth(0);}
   if(mode==13)map.RemoveFromMap(static_cast<Creature*>(&boros),false);
   if(mode==14){data.erase(2009);script->Update(15999);Require(target.HasFlag(UNIT_FIELD_FLAGS,blocked)&&script->unlockDelay==1&&!Stories(map,2009),"Unlock occurred before16000ms");script->Update(1);Require(script->firstFight==decltype(script->firstFight)::Done&&!(target.GetUInt32Value(UNIT_FIELD_FLAGS)&blocked)&&target.HasFlag(UNIT_FIELD_FLAGS,0x8000)&&!Stories(map,2009),"Failed dialogue delayed earned unlock");data[2009]=original;}
   if(mode==15){data.erase(2009);script->Update(16000);data[2009]=original;Require(!(target.GetUInt32Value(UNIT_FIELD_FLAGS)&blocked),"Next death control bypasses earned unlock");target.SetLootRecipient(&owner);target.LowerPlayerDamageReq(uint32(target.GetMaxHealth()));owner.Kill(&target,false);Require(script->firstRoute!=decltype(script->firstRoute)::Waiting,"Actual next death did not end first-story window");before=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();script->CompleteVictoryStory(1);}
   if(mode>=16){data.erase(2009);for(uint32 i=0;i<39;++i)script->Update(1000);data[2009]=original;Require(script->victoryStoryExhausted&&script->victoryStoryTries==30&&!Stories(map,2009)&&!(target.GetUInt32Value(UNIT_FIELD_FLAGS)&blocked),"Failure bound fabricated story or delayed unlock");if(mode==16)script->OnPlayerEnter(&owner);else{jace.SetPhaseId({6375},false);script->Update(1000);jace.SetPhaseId({6374},false);}}
   if(mode<14){script->Update(999);Require(!Stories(map,2009),"Retry interval or original context skipped");script->Update(1);}
   if(mode==1||mode==2||mode==6||mode==15){Require(!Stories(map,2009)&&!script->victoryStoryPending,"Ended original story published late");}
   else{
    if(mode==3)map.PlayerRelocation(&owner,0,1235,-52,0);
    if(mode==4)owner.SetGameMaster(false);
    if(mode==5)boros.ClearUnitState(UNIT_STATE_STUNNED);
    if(mode==7){jace.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&jace)),"Actual Jace reentry failed");}
    if(mode==8)owner.SetPhaseId({6374},false);
    if(mode==13){boros.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&boros)),"Actual Boros reentry failed");}
    script->Update(1000);Require(script->victoryStoryDone&&Stories(map,2009)==1,"Original context did not recover actual2009");script->Update(1000);Require(Stories(map,2009)==1,"Successful2009 replayed");
    Conversation* story=nullptr;for(auto item:*Access(StoriesTag{}))if(item&&item->GetMap()==&map&&item->GetEntry()==2009)story=item;
    Require(story&&story->GetCasterGUID()==boros.GetGUID()&&story->GetDuration()==28700&&story->GetLines().size()==4,"Native caster/duration/lines changed");
    auto a=story->GetCreature(0),b=story->GetCreature(1),c=story->GetCreature(2);auto placeholder=ObjectGuid::Create<HighGuid::Player>(0xFFFFFFFFFF);
    Require(a&&b&&c&&a->GuidLow==jace.GetGUID().GetLowPart()&&a->GuidHight==jace.GetGUID().GetHighPart()&&b->GuidLow==boros.GetGUID().GetLowPart()&&b->GuidHight==boros.GetGUID().GetHighPart()&&c->GuidLow==placeholder.GetLowPart()&&c->GuidHight==placeholder.GetHighPart(),"Native roles/placeholder changed or selected clone");
   }
   Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==before,"Optional dialogue changed progression receipts");
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" actual Kill/50562/2009 original lease and independent unlock PASS\n";
#endif
   ++cases;
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());map.UnloadAll();
 }
#ifdef TUURE_VICTORY_STORY_BASELINE
 Require(cases==2,"Baseline case count differs");std::cout<<"PASS2 expected-broken native controls;not repaired gameplay\n";
#elif defined(TUURE_VICTORY_STORY_FAULTS)
 Require(cases==4,"Fault case count differs");std::cout<<"PASS4 recovered bounded native dialogue faults;warnings retained,unlock independent\n";
#else
 Require(cases==32,"Current case count differs");std::cout<<"PASS32 current native first-victory story contexts;real death/credit,controlled poses/clocks,no full-client claim\n";
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
