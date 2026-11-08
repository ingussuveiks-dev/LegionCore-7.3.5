#define main HolyGraveyardUnusedMain
#include "TuureGraveyardNative.cpp"
#undef main
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto entry=sSpellMgr->GetSpellTargetPosition(201678);auto middle=sWorldSafeLocsStore.LookupEntry(5440);auto late=sWorldSafeLocsStore.LookupEntry(5441);auto spell=sSpellMgr->GetSpellInfo(201678);auto dungeon=sLFGMgr->GetLFGDungeon(1059);
 Require(entry&&entry->target_mapId==1489&&middle&&late&&middle->MapID==1489&&late->MapID==1489&&std::string(middle->Name->Str[0])=="7.0 Artifacts - Niskara - DK- BLood Niskara GY 1 - LWB"&&std::string(late->Name->Str[0])=="7.0 Artifacts - Niskara - DK- BLood Niskara GY 2 - LWB","Native Blood recovery sources absent");
 Require(spell&&spell->GetEffect(EFFECT_0)->Effect==227&&dungeon&&dungeon->dbc&&dungeon->dbc->ScenarioID==940&&dungeon->map==1489&&dungeon->difficulty==DIFFICULTY_N_SCENARIO&&std::fabs(dungeon->x-entry->target_X)<.001f&&std::fabs(dungeon->y-entry->target_Y)<.001f&&std::fabs(dungeon->z-entry->target_Z)<.001f,"Native201678/1059 route disagrees");
 auto steps=sScenarioMgr->GetScenarioSteps(940);Require(steps&&steps->size()==11,"Native Blood eleven steps absent");
 for(auto pair:{std::make_pair(2u,29470u),std::make_pair(6u,29497u)}){auto criterion=sCriteriaStore.LookupEntry(pair.second);unsigned asset=pair.first==2?48612:48687,root=pair.first==2?46708:46759;Require(criterion&&criterion->Type==92&&criterion->Asset==asset&&(*steps)[pair.first]->Criteriatreeid==root,"Native barrier root/asset changed");bool contained=false;for(auto leaf:sCriteriaTreeStore)if(leaf->CriteriaID==pair.second){auto current=leaf;for(unsigned depth=0;current&&depth<64;++depth){if(current->ID==root)contained=true;current=sCriteriaTreeStore.LookupEntry(current->Parent);}}Require(contained,"Barrier criterion does not belong to its real native root");}
 auto rows=WorldDatabase.Query("SELECT entryorguid,source_type,id,action_param2,target_type,target_param1 FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2 IN(48612,48687) ORDER BY action_param2");Require(rows&&rows->GetRowCount()==2,"Native barrier dispatcher graph changed");for(unsigned index=0;index<2;++index){auto f=rows->Fetch();Require(f[0].GetUInt32()==(index?10153900:101539)&&f[1].GetUInt32()==(index?9:0)&&f[2].GetUInt32()==(index?17:14)&&f[3].GetUInt32()==(index?48687:48612)&&f[4].GetUInt32()==18&&f[5].GetUInt32()==50,"Native barrier dispatch/eligible-player radius changed");if(!index)rows->NextRow();}
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new TuureGraveyardFixture::instance_tuure;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9997900+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<TuureGraveyardFixture::instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered whole1489 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,940);
  auto expected=[&](unsigned step){return step<3?Position{entry->target_X,entry->target_Y,entry->target_Z,entry->target_Orientation}:step<7?Position{middle->Loc.X,middle->Loc.Y,middle->Loc.Z,middle->Loc.O*float(M_PI)/180}:Position{late->Loc.X,late->Loc.Y,late->Loc.Z,late->Loc.O*float(M_PI)/180};};
  for(unsigned step:{0u,3u,7u}){auto pos=expected(step);auto owner=std::make_unique<NativePlayer>(session,map,921,pos);Require(std::fabs(map.GetHeight(owner->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5)-pos.GetPositionZ())<(step?.5f:.1f),"Real Blood recovery floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool completed,unsigned place){auto owner=std::make_unique<NativePlayer>(session,map,922,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_DEATH_KNIGHT);p.setFactionForRace(race);p.SetQuestStatus(40740,place%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40740);if(completed){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,48857);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==11,"Native Blood completed control failed");}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(40740);auto fight=script->firstFight;auto route=script->firstRoute;auto target=expected(step);auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1489,"Native cemetery dispatch absent");
#ifdef BLOOD_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(pos)<.001f,"Pinned Blood corpse echo changed");
#else
   Require(gy->GetExactDist(target)<.001f&&std::fabs(gy->GetOrientation()-target.GetOrientation())<.001f,"Native Blood checkpoint/facing mismatch");Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(target)<.001f,"Repeated Blood checkpoint changed");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost state absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native same-map release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/delayed revival/ghost cleanup failed");
#ifdef BLOOD_GRAVEYARD_BASELINE
   Require(p.GetExactDist(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+2)<.001f,"Pinned unsafe release destination changed");
#else
   Require(p.GetExactDist(target.GetPositionX(),target.GetPositionY(),target.GetPositionZ()+2)<.001f,"Native ACK lost selected Blood checkpoint");
#endif
   session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&p.GetHealth()==p.GetMaxHealth()/2&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(40740)==quest&&!p.HasItemCount(128402,1,true)&&script->firstFight==fight&&script->firstRoute==route,"Blood release changed actor/scenario/quest/item state");++cases;++releases;
  };
  for(unsigned step=0;step<11;++step)for(unsigned place=0;place<4;++place){scenario->SetCurrentStep(step);auto pos=place==0?expected(0):place==1?expected(7):place==2?Position{-828.85f,4372,738.64f,0}:Position{late->Loc.X,late->Loc.Y,-1000,0};exercise(pos,step,false,place);}
  scenario->SetCurrentStep(10);exercise(expected(7),11,true,0);
  for(unsigned step:{2u,6u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,940);scenario->SetCurrentStep(step-1);auto owner=std::make_unique<NativePlayer>(session,map,923,expected(7));auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_DEATH_KNIGHT);p.SetQuestStatus(40740,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40740);unsigned asset=step==2?48612:48687;p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step-1,"Premature barrier event advanced native phase");scenario->SetCurrentStep(step);auto before=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifdef BLOOD_GRAVEYARD_BASELINE
   Require(before&&before->GetExactDist(late->Loc.X,late->Loc.Y,late->Loc.Z)<.001f,"Pinned early boundary echo changed");
#else
   Require(before&&before->GetExactDist(expected(step))<.001f,"Unacknowledged barrier unlocked later checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step+1,"Native barrier acknowledgement did not advance");auto after=map.GetClosestGraveYard(entry->target_X,entry->target_Y,entry->target_Z);
#ifdef BLOOD_GRAVEYARD_BASELINE
   Require(after&&after->GetExactDist(expected(0))<.001f,"Pinned later boundary echo changed");
#else
   Require(after&&after->GetExactDist(expected(step+1))<.001f,"Accepted barrier failed to unlock its native checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step+1&&!p.HasItemCount(128402,1,true),"Duplicate barrier event skipped next phase or granted Maw");++cases;
  }
  for(unsigned id:{1085u,972u,1145u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);auto safe=sWorldSafeLocsStore.LookupEntry(id==1085?5480:5433);Require(gy&&(id==1145?gy->GetExactDist(12,34,-56)<.001f:gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f),"Retained Holy/Hunter/other cemetery changed");++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==102&&releases==90,"Blood native count changed");
#ifdef BLOOD_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinned9d0d0d1bb Blood native102/release90:whole old instance echoes unsafe corpse poses and ignores both real barrier checkpoints;current native support,not historical binary or safe recovery"<<std::endl;
#else
 std::cout<<"PASS Blood native102/release90:Human+Orc/all11stages+completed/both active statuses,four death contexts,actual201678/5440/5441 floors/facing,real ghost release/ACK/half-health/repeat cleanup,both real48612 at2->3 and48687 at6->7 with early/repeated denials,retained Holy/Hunter/other/no-scenario;no callback quest/item/progression grants. Earlier death/state/final criterion endpoints controlled;actual client/barrier visuals/pet/group/corpse traversal separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
