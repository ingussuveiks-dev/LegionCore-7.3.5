// Full production instance and real Conversation/Map/Player/Scenario services.
// Earlier victories are controlled native criteria,not combat/path traversal.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "ScriptedGossip.h"
#include "ConversationData.h"
#include "TuurePolicy.inc"
#include "TuureInstance.inc"
struct OpeningDataTag {using Type=ConversationDataMap ConversationDataStoreMgr::*;friend Type Access(OpeningDataTag);};
template struct FixtureAccess<OpeningDataTag,&ConversationDataStoreMgr::_conversationDataList>;
struct RouteActor : Actor
{
 using Actor::Actor;
 void Bind(uint64 spawn){this->*Access(AltheaGuardDbTag{})=spawn;}
 void AddToWorld() override {Actor::AddToWorld();if(auto map=GetMap()->ToInstanceMap())if(auto script=map->GetInstanceScript())script->OnCreatureCreate(this);}
};
int main(int argc,char** argv){try{
 InitializeAltheaGuardsNative(argc,argv);sWaypointMgr->Load();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_tuure;sScriptMgr->SwapScriptContext(true);
 auto data=sConversationDataStore->GetConversationData(uint32(2039));auto actors=sConversationDataStore->GetConversationActor(2039);Require(data&&data->size()==1&&actors&&actors->size()==1,"Native2039 data differs");
 Require((*actors)[0].actorId==52779&&(*actors)[0].creatureId==106318,"Native2039 virtual actor differs");
 auto& mutableData=sConversationDataStore->*Access(OpeningDataTag{});auto original=mutableData.at(2039);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_BLOODELF)})for(uint32 mode=0;mode<12;++mode){
#ifdef TUURE_ROUTE_OPENING_BASELINE
  if(mode)continue;
#endif
#ifdef TUURE_ROUTE_OPENING_FAULTS
  if(mode<10)continue;
#else
  if(mode>=10)continue;
#endif
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9986100+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual Tuure instance absent");
  auto scenario=sScenarioMgr->AddScenario(&map,1085);auto session=Session();
  {NativePlayer owner(session,map,710,{0,1235,-52,0});owner.SetByteValue(UNIT_FIELD_BYTES_0,0,race);owner.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_PRIEST);owner.setFactionForRace(race);owner.SetPhaseId({6374},false);owner.SetQuestStatus(41993,QUEST_STATUS_INCOMPLETE);owner.SetQuestSlot(0,41993);auto model=sChrRacesStore.LookupEntry(race);Require(model,"Native race missing");owner.SetNativeDisplayId(model->MaleDisplayID);owner.SetDisplayId(model->MaleDisplayID);
   RouteActor boros(&map,106134,8200,{-6.55612f,1235.61f,-52.2829f,0});boros.Bind(361419);boros.SetPhaseId({6374},false);Require(map.AddToMap(static_cast<Creature*>(&boros)),"Boros admission failed");
   scenario->SetCurrentStep(1);owner.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50562);Require(TuureFirstFight::HasVictory(scenario)&&scenario->GetCurrentStep()==2,"Controlled earlier native victory prerequisite absent");
   owner.AddQuestDelayedEvent(41993,0,[](){});script->routeRecipient=owner.GetGUID();script->routeAttempt=owner.getQuestStatus(41993)->ScriptLifetime;script->routeActor=boros.GetGUID();script->firstRoute=decltype(script->firstRoute)::Pending;script->routeDelay=0;
   scenario->UpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE,106221,1,0,&boros,&owner);Require(script->HasRouteProgress(47883),"Controlled native second-kill prerequisite absent");
   RouteActor jace(&map,106137,8201,{-2,1235,-52,0});jace.Bind(361432);jace.SetPhaseId({6374},false);Require(map.AddToMap(static_cast<Creature*>(&jace)),"Jace admission failed");auto before=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();
   Require(script->GetBoros()==&boros,"Controlled native Boros capture absent");Require(!boros.HasUnitState(UNIT_STATE_STUNNED),"Controlled healed Boros still stunned");
   Require(script->routeAttempt.lock()==owner.getQuestStatus(41993)->ScriptLifetime,"Controlled route lease absent");
   mutableData.erase(2039);script->UpdateRoute(0);Require(Stories(map,2039)==0,"Missing data produced a native story");mutableData[2039]=original;boros.UpdateWaypointID(0);
   Require(script->routeOpening&&script->firstRoute==decltype(script->firstRoute)::Moving,"Initial failure delayed native route/Jace handoff");
#ifdef TUURE_ROUTE_OPENING_BASELINE
   Require(script->routeOpening,"Pinned unchecked request no longer consumes its event");script->UpdateRoute(1000);Require(Stories(map,2039)==0,"Pinned lost2039 unexpectedly retried");std::cout<<"COUNTERFACTUAL race="<<race<<" failed real CreateConversation consumes2039 permanently;expected broken\n";
#else
   Require(!script->routeTauntDone,"Failed native creation consumed opening dialogue");
   if(mode==1)owner.SetQuestStatus(41993,QUEST_STATUS_NONE);
   if(mode==2)owner.getQuestStatus(41993)->ScriptLifetime=std::make_shared<uint8>();
   if(mode==3)map.PlayerRelocation(&owner,300,1235,-52,0);
   if(mode==4)owner.SetGameMaster(true);
   if(mode==5)boros.AddUnitState(UNIT_STATE_STUNNED);
   if(mode==6)scenario->SetCurrentStep(3);
   if(mode==7)map.RemoveFromMap(static_cast<Creature*>(&boros),false);
   if(mode==8)owner.SetPhaseId({6375},false);
   std::unique_ptr<RouteActor> foreign;
   if(mode==9){foreign=std::make_unique<RouteActor>(&map,106134,8202,boros.GetPosition());foreign->Bind(999999);foreign->SetPhaseId({6374},false);Require(map.AddToMap(static_cast<Creature*>(foreign.get())),"Foreign Boros control absent");}
   if(mode>=10){
    mutableData.erase(2039);
    for(uint32 i=0;i<39;++i)script->UpdateRoute(1000);mutableData[2039]=original;
    Require(script->routeTauntExhausted&&script->routeTauntTries==30&&!Stories(map,2039),"Native failure retry is unbounded or fabricated a story");
    if(mode==10){script->OnPlayerEnter(&owner);Require(!script->routeTauntExhausted,"Original player reentry did not rearm pending dialogue");}
    else {owner.SetPhaseId({6375},false);script->UpdateRoute(1000);owner.SetPhaseId({6374},false);} // Readiness edge,no create/login.
   }
   if(mode<10){script->UpdateRoute(999);Require(Stories(map,2039)==0,"Retry skipped native interval or original owner/actor context");}script->UpdateRoute(1);
   if(mode==1||mode==2||mode==6){Require(Stories(map,2039)==0,"Ended attempt/phase published late dialogue");}
   else{
    if(mode==3)map.PlayerRelocation(&owner,0,1235,-52,0);
    if(mode==4)owner.SetGameMaster(false);
    if(mode==5)boros.ClearUnitState(UNIT_STATE_STUNNED);
    if(mode==7){boros.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&boros)),"Actual Boros reentry failed");boros.GetMotionMaster()->MovePath(9859002,false);boros.UpdateWaypointID(0);}
    if(mode==8)owner.SetPhaseId({6374},false);
    script->UpdateRoute(1000);Require(script->routeTauntDone&&Stories(map,2039)==1,"Valid original context did not recover actual2039");script->UpdateRoute(1000);Require(Stories(map,2039)==1,"Repeated callback replayed successful story");
    Conversation* story=nullptr;for(auto item:*Access(StoriesTag{}))if(item&&item->GetMap()==&map&&item->GetEntry()==2039)story=item;
    Require(story&&story->GetCasterGUID()==boros.GetGUID()&&story->GetDuration()==9700&&story->GetLines().size()==1,"Native caster/duration/lines changed");
    auto actor=story->GetActor(0);Require(actor&&actor->actorId==52779&&actor->creatureId==106318,"Virtual Lady actor was replaced by a creature lookup");
   }
   Require(scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==before,"Dialogue retry changed progression receipts");
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" actual2039 retry/original quest/owner/actor/phase/once PASS\n";
#endif
   ++cases;
  }
  session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());map.UnloadAll();
 }
#ifdef TUURE_ROUTE_OPENING_BASELINE
 std::cout<<"PASS2 expected-broken native conversation controls;not repaired gameplay\n";
#else
#ifdef TUURE_ROUTE_OPENING_FAULTS
 Require(cases==4,"Missing recovered fault cases");std::cout<<"PASS4 recovered native exhausted-dialogue faults;expected warnings retained\n";
#else
 Require(cases==20,"Missing native route-opening cases");std::cout<<"PASS20 current native route-opening contexts;earlier victory/native receipts controlled,no full-client claim\n";
#endif
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
