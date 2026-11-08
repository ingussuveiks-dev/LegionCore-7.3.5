#define main HolyGraveyardUnusedMain
#include "TuureGraveyardNative.cpp"
#undef main
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto early=sWorldSafeLocsStore.LookupEntry(5654);auto late=sWorldSafeLocsStore.LookupEntry(5655);auto spell=sSpellMgr->GetSpellInfo(217812);auto entry=sSpellMgr->GetSpellTargetPosition(217812);auto dungeon=sLFGMgr->GetLFGDungeon(1310);
 Require(early&&late&&early->MapID==1489&&late->MapID==1489&&std::string(early->Name->Str[0])=="7.0 Order Hall - Warrior - Niskara - GY 1"&&std::string(late->Name->Str[0])=="7.0 Order Hall - Warrior - Niskara - GY 2","Native Hodir recovery sources absent");
 Require(spell&&spell->GetEffect(EFFECT_0)->Effect==227&&entry&&entry->target_mapId==1489&&dungeon&&dungeon->dbc&&dungeon->dbc->ScenarioID==1146&&dungeon->map==1489&&dungeon->difficulty==DIFFICULTY_N_SCENARIO&&(dungeon->dbc->Flags&LFG_FLAG_USER_TELEPORT_NOT_ALLOWED)&&dungeon->x==0&&dungeon->y==0&&dungeon->z==0,"Native217812/1310 route disagrees");
 auto steps=sScenarioMgr->GetScenarioSteps(1146);auto criterion=sCriteriaStore.LookupEntry(30856);auto final=sCriteriaStore.LookupEntry(30885);Require(steps&&steps->size()==6&&(*steps)[2]->Criteriatreeid==49314&&criterion&&criterion->Type==92&&criterion->Asset==52537&&final&&final->Type==0&&final->Asset==110620,"Native Hodir landing/completion graph changed");
 bool contained=false;for(auto leaf:sCriteriaTreeStore)if(leaf->CriteriaID==30856){auto current=leaf;for(unsigned depth=0;current&&depth<64;++depth){if(current->ID==49314)contained=true;current=sCriteriaTreeStore.LookupEntry(current->Parent);}}Require(contained,"Landing criterion does not belong to actual step2");
 auto graph=WorldDatabase.Query("SELECT COUNT(*) FROM smart_scripts root JOIN smart_scripts cleanup ON cleanup.entryorguid=root.entryorguid AND cleanup.source_type=root.source_type AND cleanup.id=root.link JOIN conditions c ON c.SourceTypeOrReferenceId=22 AND c.SourceGroup=root.id+1 AND c.SourceEntry=root.entryorguid AND c.SourceId=root.source_type WHERE root.entryorguid=157 AND root.source_type=13 AND root.id=0 AND root.event_type=89 AND root.action_type=205 AND root.action_param1=92 AND root.action_param2=52537 AND root.target_type=7 AND cleanup.event_type=61 AND cleanup.action_type=28 AND cleanup.action_param1=217815 AND cleanup.target_type=7 AND c.ConditionTypeOrReference=44 AND c.ConditionValue1=1146 AND c.ConditionValue2=2");Require(graph&&graph->Fetch()[0].GetUInt32()==1,"Actual repaired EO157 landing dispatch missing");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new TuureGraveyardFixture::instance_tuure;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9998100+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<TuureGraveyardFixture::instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered whole1489 instance absent");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,1146);
  auto expected=[&](unsigned step){auto safe=step<3?early:late;return Position{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,safe->Loc.O*float(M_PI)/180};};
  for(unsigned step:{0u,3u}){auto pos=expected(step);auto owner=std::make_unique<NativePlayer>(session,map,921,pos);owner->SetPhaseId({5896},false);Require(std::fabs(map.GetHeight(owner->GetPhases(),pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+5)-pos.GetPositionZ())<.5f,"Real phase5896 Hodir recovery floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool completed,unsigned place){auto owner=std::make_unique<NativePlayer>(session,map,922,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);p.setFactionForRace(race);p.SetPhaseId({5896},false);p.SetQuestStatus(42974,place%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,42974);if(completed){scenario->UpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE,110620,1,0,nullptr,&p);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==6,"Native Hodir completed control failed");}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(42974);auto fight=script->firstFight;auto route=script->firstRoute;auto target=expected(step);auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1489,"Native cemetery dispatch absent");
#ifdef HODIR_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(pos)<.001f,"Pinned Hodir corpse echo changed");
#else
   Require(gy->GetExactDist(target)<.001f&&std::fabs(gy->GetOrientation()-target.GetOrientation())<.001f,"Native Hodir checkpoint/facing mismatch");Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(target)<.001f,"Repeated Hodir checkpoint changed");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost state absent");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native same-map release absent");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/delayed revival/ghost cleanup failed");
#ifdef HODIR_GRAVEYARD_BASELINE
   Require(p.GetExactDist(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+2)<.001f,"Pinned unsafe release destination changed");
#else
   Require(p.GetExactDist(target.GetPositionX(),target.GetPositionY(),target.GetPositionZ()+2)<.001f,"Native ACK lost selected Hodir checkpoint");
#endif
   session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&p.GetHealth()==p.GetMaxHealth()/2&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(42974)==quest&&!p.HasItemCount(128910,1,true)&&script->firstFight==fight&&script->firstRoute==route,"Hodir release changed actor/scenario/quest/item state");++cases;++releases;
  };
  for(unsigned step=0;step<6;++step)for(unsigned place=0;place<4;++place){scenario->SetCurrentStep(step);auto pos=place==0?expected(0):place==1?expected(3):place==2?Position{-828.85f,4372,738.64f,0}:Position{late->Loc.X,late->Loc.Y,-1000,0};exercise(pos,step,false,place);}
  scenario->SetCurrentStep(5);exercise(expected(3),6,true,0);
  for(unsigned step:{2u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,1146);scenario->SetCurrentStep(step-1);auto owner=std::make_unique<NativePlayer>(session,map,923,expected(3));auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_WARRIOR);p.SetQuestStatus(42974,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,42974);unsigned asset=52537;p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step-1,"Premature landing event advanced native phase");scenario->SetCurrentStep(step);auto before=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifdef HODIR_GRAVEYARD_BASELINE
   Require(before&&before->GetExactDist(late->Loc.X,late->Loc.Y,late->Loc.Z)<.001f,"Pinned early boundary echo changed");
#else
   Require(before&&before->GetExactDist(expected(step))<.001f,"Unacknowledged landing unlocked later checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step+1,"Native landing acknowledgement did not advance");auto after=map.GetClosestGraveYard(early->Loc.X,early->Loc.Y,early->Loc.Z);
#ifdef HODIR_GRAVEYARD_BASELINE
   Require(after&&after->GetExactDist(expected(0))<.001f,"Pinned later boundary echo changed");
#else
   Require(after&&after->GetExactDist(expected(step+1))<.001f,"Accepted landing failed to unlock its native checkpoint");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,asset);Require(scenario->GetCurrentStep()==step+1&&!p.HasItemCount(128910,1,true),"Duplicate landing event skipped next phase or granted an artifact");++cases;
  }
  for(unsigned id:{1085u,972u,940u,1145u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);if(id==1085||id==972){auto safe=sWorldSafeLocsStore.LookupEntry(id==1085?5480:5433);Require(gy&&gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f,"Retained Holy/MM recovery changed");}else if(id==940){auto blood=sSpellMgr->GetSpellTargetPosition(201678);Require(gy&&gy->GetExactDist(blood->target_X,blood->target_Y,blood->target_Z)<.001f,"Retained Blood entry changed");}else Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"Other1145 fallback changed");++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());auto gy=map.GetClosestGraveYard(12,34,-56);Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==62&&releases==50,"Hodir native count changed");
#ifdef HODIR_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinnedd2f2840b2 Hodir native62/release50:whole prior instance echoes unsafe corpse coordinates and ignores acknowledged landing;current native support,not historical binary or safe recovery"<<std::endl;
#else
 std::cout<<"PASS Hodir native62/release50:Human+Orc/all6 stages+native completed sentinel6/both active statuses,four death contexts,actual5654/5655 phase5896 floors/facing,native ghost release/ACK/half-health/repeat cleanup,real52537 boundary2->3 with early/repeated denials,retained Holy/MM/Blood/other/no-scenario;no recovery progression/item/aura grants. Earlier death/state/final criterion endpoints controlled;actual client/dive/pet/group/corpse traversal separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
