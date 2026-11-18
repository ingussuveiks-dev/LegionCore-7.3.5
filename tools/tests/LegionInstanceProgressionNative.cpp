// Real factories, GO publication, native boss/door state machine and saved re-entry.
// Combat transitions, individual boss AIs and client collision remain separate.
#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
// Isolate the header's legacy unscoped enums, while registering its unchanged
// production factory through the ordinary Legion loader. Baseline is pinned.
#undef DataHeader
namespace VioletActual {
#include "VioletInstanceActual.inc"
}
#undef DataHeader
void AddSC_instance_violet_hold_legion(){VioletActual::AddSC_instance_violet_hold_legion();}
struct DoorScope{uint32 map,bosses;DoorData const* data;size_t count;};
#include "LegionDoorArrays.inc"
struct InstanceDoorsTag{using Type=DoorInfoMap InstanceScript::*;friend Type Access(InstanceDoorsTag);};
template struct FixtureAccess<InstanceDoorsTag,&InstanceScript::doors>;
static uint32 nextDoor=150000;
struct PublishedInstanceDoor:GameObject{
 PublishedInstanceDoor(InstanceMap& map,uint32 entry){
  auto row=WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 FROM gameobject WHERE id=%u AND map=%u ORDER BY guid LIMIT 1",entry,map.GetId());
  Require(bool(row),"Door has no native DB placement;investigate instead of inventing a position");auto f=row->Fetch();Position pose{f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
  Require(Create(nextDoor++,entry,&map,1,pose,G3D::Quat(f[4].GetFloat(),f[5].GetFloat(),f[6].GetFloat(),f[7].GetFloat()),255,GO_STATE_READY),"Native placed door create failed");
  map.LoadGrid(pose.GetPositionX(),pose.GetPositionY());Require(map.AddToMap(static_cast<GameObject*>(this)),"Native door publication failed");
 }
 ~PublishedInstanceDoor(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}
};
static void CheckDoors(InstanceScript& script,DoorScope const& scope,std::vector<std::unique_ptr<PublishedInstanceDoor>>const& doors){
 for(auto const& door:doors){bool open=true;for(size_t i=0;i<scope.count;++i){auto const& row=scope.data[i];if(row.entry==door->GetEntry()){auto state=script.GetBossState(row.bossId);open&=row.type==DOOR_TYPE_ROOM?state!=IN_PROGRESS:row.type==DOOR_TYPE_PASSAGE?state==DONE:state==IN_PROGRESS;}}
  Check(door->GetGoState()==(open?GO_STATE_ACTIVE:GO_STATE_READY),"Actual registered native door state mismatch");}
}
int main(int argc,char**argv){try{
 InitializeDungeon(argc,argv,true);sObjectMgr->LoadInstanceEncounters();auto modes=sDB2Manager.GetAllMapsDifficultyes();unsigned pairs=0,doorObjects=0,knownFailures=0;
 for(auto mapId:{1456u,1458u,1466u,1477u,1492u,1493u,1501u,1516u,1544u,1571u,1651u,1677u,1753u})for(auto const& pair:modes[mapId]){
  auto difficulty=Difficulty(pair.first);MapInstanced parent(mapId,300);InstanceMap map(mapId,300,9976000+pairs*4,difficulty,&parent);map.CreateInstanceData(nullptr);auto script=map.GetInstanceScript();Check(script,"Native instance factory missing");
  unsigned bosses=0;while(bosses<64&&script->GetBossState(bosses)!=TO_BE_DECIDED)++bosses;Check(bosses>0&&bosses<64,"Native encounter count missing");
  auto scope=std::find_if(std::begin(doorScopes),std::end(doorScopes),[&](DoorScope const& row){return row.map==mapId;});
  if(scope!=std::end(doorScopes)){
   auto const& registered=script->*Access(InstanceDoorsTag{});Check(registered.size()==scope->count-1,"Actual linked door table includes extra/missing rows");std::set<uint32> entries;for(size_t i=0;i+1<scope->count;++i)entries.insert(scope->data[i].entry);
   std::vector<std::unique_ptr<PublishedInstanceDoor>> doors;for(auto entry:entries){std::cout<<"CREATE map="<<mapId<<" mode="<<unsigned(difficulty)<<" door="<<entry<<std::endl;doors.push_back(std::make_unique<PublishedInstanceDoor>(map,entry));++doorObjects;}CheckDoors(*script,*scope,doors);
   for(unsigned boss=0;boss<bosses;++boss)for(EncounterState state:{IN_PROGRESS,FAIL,NOT_STARTED,DONE}){
    // Exercise the shared state machine without inventing victory in a real
    // player run or triggering untested encounter-specific outro summons.
    Check(script->InstanceScript::SetBossState(boss,state),"Native state transition rejected");Check(!script->InstanceScript::SetBossState(boss,state),"Native repeated state transition was not idempotent");CheckDoors(*script,*scope,doors);
   }
   doors.clear();for(auto const& bossRow:registered)for(auto const& set:bossRow.second.bossInfo->door)Check(set.empty(),"Removed native door left dangling registered pointer");
   // Real late publication must immediately inherit the saved boss state.
   for(auto entry:entries)doors.push_back(std::make_unique<PublishedInstanceDoor>(map,entry));CheckDoors(*script,*scope,doors);
  }
  // Seed checkpoint states through the actual base state API; replay uses the
  // ordinary InstanceSave/CreateInstanceData path and each real Load override.
  for(unsigned boss=0;boss<bosses;++boss)script->InstanceScript::SetBossState(boss,EncounterState(boss%5));
#ifndef EXPECT_VIOLET_SAVE_BASELINE
  if(mapId==1544)Check(script->IsEncounterInProgress(),"Violet ignored actual native active boss combat");
#endif
  auto data=script->GetSaveData();InstanceSave checkpoint(mapId,9976001+pairs*4,difficulty,0,data,0,true);InstanceMap restored(mapId,300,checkpoint.GetInstanceId(),difficulty,&parent);restored.CreateInstanceData(&checkpoint);auto loaded=restored.GetInstanceScript();Check(loaded,"Native saved instance factory missing");
  for(unsigned boss=0;boss<bosses;++boss){auto state=EncounterState(boss%5);if(state==IN_PROGRESS||state==SPECIAL)state=NOT_STARTED;
#ifdef EXPECT_VIOLET_SAVE_BASELINE
   if(mapId==1544){Check(loaded->GetBossState(boss)==TO_BE_DECIDED,"Expected original Violet native state loss changed");continue;}
#endif
   Check(loaded->GetBossState(boss)==state,"Native saved re-entry lost or incorrectly resumed an encounter state");}
#ifndef EXPECT_VIOLET_SAVE_BASELINE
  if(mapId==1544)Check(!loaded->IsEncounterInProgress(),"Violet resumed interrupted native boss combat after entry");
#endif
  if(scope!=std::end(doorScopes)){
   std::set<uint32> entries;for(size_t i=0;i+1<scope->count;++i)entries.insert(scope->data[i].entry);
   std::vector<std::unique_ptr<PublishedInstanceDoor>> doors;for(auto entry:entries){doors.push_back(std::make_unique<PublishedInstanceDoor>(restored,entry));++doorObjects;}CheckDoors(*loaded,*scope,doors);
  }
#ifdef EXPECT_VIOLET_SAVE_BASELINE
  if(mapId==1544){++knownFailures;std::cout<<"COUNTERFACTUAL Violet saved reload leaves8 native boss states TO_BE_DECIDED;not a successful persisted run\n";}
#endif
  std::cout<<"INSTANCE map="<<mapId<<" mode="<<unsigned(difficulty)<<" bosses="<<bosses<<" saved="<<data<<'\n';++pairs;
 }
 Check(pairs==46,"Native mode scope changed");
#ifndef EXPECT_VIOLET_SAVE_BASELINE
 // Actual legacy saves retain the selected boss IDs, including valid ID zero.
 for(auto difficulty:{DIFFICULTY_NORMAL,DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON})
 for(unsigned variant=0;variant<5;++variant){
  MapInstanced parent(1544,300);std::string data=variant==0?"V H 0 0 0 0 0":variant==1?"V H 3 0 0 0 4":variant==2?"V H 3 3 0 2 5":variant==3?"V H 3 3 3 1 6":"V H 1 4 0 0 4";
  InstanceSave checkpoint(1544,9979000+variant,difficulty,0,data,0,true);InstanceMap map(1544,300,checkpoint.GetInstanceId(),difficulty,&parent);map.CreateInstanceData(&checkpoint);auto script=map.GetInstanceScript();
  for(unsigned boss=0;boss<8;++boss){bool done=variant==1?boss==0:variant==2?(boss==2||boss==5):variant==3?(boss==1||boss==6||boss==7):false;Check(script->GetBossState(boss)==(done?DONE:NOT_STARTED),"Legacy Violet save invented or lost a selected victory");}
  Check(script->GetData(VioletActual::DATA_MAIN_EVENT_PHASE)==(variant==3?DONE:NOT_STARTED),"Legacy Violet completion/retry phase lost");
  if(variant==3){PublishedInstanceDoor door(map,VioletActual::GO_MAIN_DOOR);Check(door.GetGoState()==GO_STATE_ACTIVE,"Completed Violet checkpoint kept late main door closed");}
  auto saved=script->GetSaveData();InstanceSave again(1544,9980000+variant,difficulty,0,saved,0,true);InstanceMap roundtrip(1544,300,again.GetInstanceId(),difficulty,&parent);roundtrip.CreateInstanceData(&again);
  for(unsigned boss=0;boss<8;++boss)Check(roundtrip.GetInstanceScript()->GetBossState(boss)==script->GetBossState(boss),"Upgraded legacy Violet save lost a boss state on second entry");
 }
#endif
#ifdef EXPECT_VIOLET_SAVE_BASELINE
 Check(knownFailures==3,"Expected original Violet failure in all3 native modes not reproduced");
#endif
 std::cout<<"PASS "<<checked<<" native instance/door checks,pairs="<<pairs<<",doorObjects="<<doorObjects<<";qualified native state API,actual placed/published GOs,all-owned-gate predicates,idempotency,remove/recreate and saved-reentry. Combat/full key/client remain separate;knownCounterfactuals="<<knownFailures<<'\n';
 CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
