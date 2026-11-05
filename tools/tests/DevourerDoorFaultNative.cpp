// Separate deliberate native actor/criteria refusal controls, not clean runs.
#define DEVOURER_DOOR_SUPPORT_ONLY
#include "DevourerDoorNative.cpp"
int main(int argc,char** argv){try{
 InitializeDoorNative(argc,argv);uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 fault=0;fault<2;++fault){
  MapInstanced parent(1607,300);InstanceMap map(1607,300,9920000+cases,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto scenario=sScenarioMgr->AddScenario(&map,1078);scenario->SetCurrentStep(2);auto session=Session();
  {Position pose{1854.51f,1296.27f,62.1306f,0};NativePlayer player(session,map,101,pose);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_ROGUE);player.setFactionForRace(race);player.SetQuestStatus(41924,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,41924);DoorKey key(player,cases);player.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,50908);Require(player.CastSpell(&player,211687,true)==SPELL_CAST_OK,"Native reclaim prerequisite refused");
   DoorObject go;Require(go.Create(7100+cases*2,249390,&map,1,pose,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(&go)),"Native control absent");go.SetLootState(GO_READY);std::unique_ptr<DoorObject> barrier;
   auto add=[&]{barrier=std::make_unique<DoorObject>();Position b{1855.38f,1292.18f,62.5271f,0};Require(barrier->Create(7101+cases*2,249391,&map,1,b,G3D::Quat(0,0,0,1),255,GO_STATE_READY)&&map.AddToMap(static_cast<GameObject*>(barrier.get())),"Native restored actor absent");barrier->SetLootState(GO_READY);};
   auto criterion=const_cast<CriteriaEntry*>(sCriteriaStore.LookupEntry(29820));Require(criterion&&criterion->Type==92&&criterion->Asset==50536,"Actual door criterion differs");int32 asset=criterion->Asset;if(fault){add();criterion->Asset=INT32_MAX;}
   go.Use(&player);go.Use(&player);Require(go.GetPhaseMask()==1&&player.HasItemCount(136802,1)&&DoorCredit(scenario)==0&&scenario->GetCurrentStep()==2,"Refusal consumed key/barrier/credit");if(fault)criterion->Asset=asset;else add();go.Use(&player);Require(go.GetPhaseMask()==4&&barrier->GetPhaseMask()==4&&!player.HasItemCount(136802,1)&&DoorCredit(scenario)==1&&scenario->GetCurrentStep()==3,"Native restored actor/criterion retry failed");go.Use(&player);Require(DoorCredit(scenario)==1,"Restored door replayed");
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());++cases;
 }
 std::cout<<"PASS "<<cases<<" explicit native missing-barrier/rejected50536 controls: original key and both actors retained, exact restored native retry once. Earlier quest/key/cage/cache endpoints controlled; client separate\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
