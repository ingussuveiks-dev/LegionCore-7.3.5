// Real Scenario/AchievementMgr progress; deaths and prior quest/stage controlled.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "WavesArrival.inc"
#include "WavesNative.inc"
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 auto rules=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(29100,29055)");
 Require(rules&&!rules->Fetch()[0].GetUInt32(),"Fury criteria require additional native data loader");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 MapInstanced parent(1511,300);InstanceMap map(1511,300,9986900,DIFFICULTY_N_SCENARIO,&parent);
 auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(1);auto session=Session();
 {
  NativePlayer player(session,map,901,{3427.55f,1486.10f,57.40f,0});
  player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
  WarswordsWavesNative::State state(&map);std::vector<ObjectGuid> spawned;
  auto actual=[&](uint32 tree){auto rows=scenario->GetAchievementMgr().GetCriteriaProgressMap();auto it=rows->find(tree);return it==rows->end()?uint64(0):it->second.Counter;};
  state.Start();Require(state.active&&!state.wave,"Native Fury wave admission failed");
  for(uint32 wave=0;wave<3;++wave){
   auto actors=state.actors;
   for(uint32 slot=0;slot<3;++slot){
    auto actor=map.GetCreature(actors[slot]);Require(actor&&actor->IsAlive(),"Native tracked Fury summon missing");spawned.push_back(actor->GetGUID());
    actor->CombatStop(true);actor->setDeathState(JUST_DIED);state.Death(actor);state.Death(actor);
    if(slot<2)Require(actual(45974)==wave,"Partial wave granted native credit");
   }
#ifdef PROGRESS_BASELINE
   Require(actual(45974)>=1&&state.wave==0,"Pinned wrong-key counterfactual no longer reproduces");
   std::cout<<"COUNTERFACTUAL real native CriteriaTree45974 stores wave credit; pinned Criteria29100 lookup stays zero and repeats the same death receipt; not a successful ordinary run\n";
   break;
#else
   Require(actual(45974)==wave+1&&state.wave==wave+1,"Real native tree progress did not acknowledge exactly one wave");
   state.Update(1000);Require(actual(45974)==wave+1,"Repeated Update duplicated native wave credit");
   std::cout<<"CASE native Fury wave="<<state.wave<<" tree45974="<<actual(45974)<<" criteria29100-key-present="<<scenario->GetAchievementMgr().GetCriteriaProgressMap()->count(29100)<<'\n';
#endif
  }
#ifndef PROGRESS_BASELINE
  Require(scenario->GetCurrentStep()==1&&!state.leader.IsEmpty(),"Native leader appeared at wrong stage");
  auto leader=map.GetCreature(state.leader);Require(leader&&leader->GetEntry()==97649&&leader->IsAlive(),"Real Fury leader missing");spawned.push_back(leader->GetGUID());
  leader->CombatStop(true);leader->setDeathState(JUST_DIED);state.Death(leader);state.Death(leader);state.Update(1000);
  Require(actual(45974)==3&&actual(45861)==1&&scenario->GetCurrentStep()==2&&!state.active,"Native wave/leader tree failed actual1->2 transition");
  std::cout<<"PASS real native three waves/9 summons plus97649: CriteriaTree45974=3/45861=1, actual stage1->2, repeated death/update no extra credit. Quest/stage and dead notifications controlled; not real combat/client traversal\n";
#endif
  for(auto guid:spawned)if(auto actor=map.GetCreature(guid)){actor->CombatStop(true);actor->DespawnOrUnsummon();}
  map.RemoveAllObjectsInRemoveList();CleanStories(map);
 }
 session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
