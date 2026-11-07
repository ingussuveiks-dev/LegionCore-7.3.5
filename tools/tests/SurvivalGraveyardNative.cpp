// Real Player release/ACK, global fallback and registered whole instance.
// Earlier quest/death/step are controlled; no character DB writes or client.
#define main FuryGraveyardNativeUnusedMain
#include "WarswordsGraveyardNative.cpp"
#undef main
#include "PlayerDefines.h"
static WorldSafeLocsEntry const* HunterSafe(unsigned step){return sWorldSafeLocsStore.LookupEntry(step>=7?5296:5295);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
#ifdef SURVIVAL_GRAVEYARD_BASELINE
 OriginalSharedGraveyardEntranceData();
#endif
 sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();sObjectMgr->LoadGraveyardZones();
 Require(sQuestDataStore->GetQuestTemplate(40385),"Real native Hunter40385 template missing");
 auto gates=WorldDatabase.Query("SELECT COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29205");Require(gates&&!gates->Fetch()[0].GetUInt32(),"Native harpoon criterion requires extra live data gates before boundary proof");
 unsigned steps=0,harpoonRoot=0;for(auto s:sScenarioStepStore)if(s->ScenarioID==973){++steps;if(s->OrderIndex==6)harpoonRoot=s->Criteriatreeid;}Require(steps==13&&harpoonRoot,"Native Hunter step count/harpoon root changed");auto criterion=sCriteriaStore.LookupEntry(29205);Require(criterion&&criterion->Type==92&&criterion->Asset==47608,"Native harpoon credit identity changed");
 bool harpoonInRoot=false;for(auto tree:sCriteriaTreeStore)if(tree->CriteriaID==29205){auto current=tree;for(unsigned depth=0;current&&depth<64;++depth){if(current->ID==harpoonRoot){harpoonInRoot=true;break;}current=sCriteriaTreeStore.LookupEntry(current->Parent);}}Require(harpoonInRoot,"Actual stage6 root does not contain native harpoon criterion");std::cout<<"CONTROL native harpoon root="<<harpoonRoot<<" criterion29205/47608 descendant"<<std::endl;
 auto teleport=WorldDatabase.Query("SELECT action_type,action_param1,target_type,target_x,target_y,target_z FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=4 AND event_type=61");Require(teleport&&teleport->GetRowCount()==1,"Native harpoon teleport row missing/ambiguous");auto tf=teleport->Fetch();Require(tf[0].GetUInt32()==62&&tf[1].GetUInt32()==1511&&tf[2].GetUInt32()==23&&std::fabs(tf[3].GetFloat()-3601.95f)<0.01f&&std::fabs(tf[4].GetFloat()-1782.54f)<0.01f&&std::fabs(tf[5].GetFloat()-1.86404f)<0.01f,"Native real harpoon destination/owner changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new instance_warswords;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0,releases=0,wrongClassFallback=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  MapInstanced parent(1511,300);InstanceMap map(1511,300,9999000+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<instance_warswords::instance_warswords_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Registered shared1511 instance missing");auto scenario=sScenarioMgr->AddScenario(&map,973);auto session=Session();
  for(unsigned id:{5295u,5296u}){auto safe=sWorldSafeLocsStore.LookupEntry(id);Require(safe&&safe->MapID==1511&&std::string(safe->Name->Str[0]).find("Surv Hunter Acquisition GY ")==0,"Matching Hunter cemetery changed");NativePlayer p(session,map,601,{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,0});p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);Require(std::fabs(map.GetHeight(p.GetPhases(),safe->Loc.X,safe->Loc.Y,safe->Loc.Z+5)-safe->Loc.Z)<0.5f,"Real Hunter cemetery floor unsupported");}
  for(unsigned step=0;step<13;++step)for(unsigned place=0;place<4;++place){
   auto original=sWorldSafeLocsStore.LookupEntry(place==0?5295:place==1?5296:5228);Position pos{original->Loc.X,original->Loc.Y,original->Loc.Z,0};if(place==2)pos.Relocate(3601.95f,1782.54f,1.86404f,0);
   scenario->SetCurrentStep(step);auto safe=HunterSafe(step);Require(safe,"Expected native Hunter source missing");auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto wave=script->waves.wave;auto active=script->waves.active;auto started=script->myscenario;
   NativePlayer p(session,map,602,pos);p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.setFactionForRace(race);p.SetQuestStatus(40385,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40385);
   auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());
#ifdef SURVIVAL_GRAVEYARD_BASELINE
   Require(!gy,"Pinned missing Hunter instance cemetery did not reproduce");auto fallback=sObjectMgr->GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ(),1511,p.GetTeam());Require(fallback,"Real original outdoor fallback missing");if(place==3){Require(fallback->ID==5228,"Wrong-class outdoor control changed");++wrongClassFallback;}
#else
   Require(gy&&gy->GetMapId()==1511&&gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<0.001f&&std::fabs(gy->GetOrientation()-safe->Loc.O*float(M_PI)/180)<0.001f,"Native Hunter cemetery crossed wrong side of wards or used a Fury pose");Require(map.GetClosestGraveYard(0,0,0)==gy,"Native repeated cemetery pointer changed");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Actual native Ghost aura did not establish release state");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear()&&!p.IsAlive(),"Native dead release did not start same-map transfer");
   WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);
#ifdef SURVIVAL_GRAVEYARD_BASELINE
   Require(!p.IsAlive()&&p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&!p.IsBeingTeleported(),"Original outdoor transfer unexpectedly revived Hunter");
#else
   bool revived=p.IsAlive()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&!p.IsBeingTeleported()&&p.GetHealth()==p.GetMaxHealth()/2&&p.GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z+2)<0.001f;
   if(!revived)std::cerr<<"CONTROL native Hunter recovery failed race="<<race<<" step="<<step<<" place="<<place<<" alive="<<p.IsAlive()<<" ghost="<<p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)<<" aura8326="<<p.HasAura(8326)<<" health="<<p.GetHealth()<<" xyz="<<p.GetPositionX()<<','<<p.GetPositionY()<<','<<p.GetPositionZ()<<std::endl;
   Require(revived,"Actual Hunter native ACK/delayed half-health revival failed");session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&p.GetHealth()==p.GetMaxHealth()/2,"Duplicate ACK changed native Hunter recovery");
#endif
   Require(scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&script->waves.wave==wave&&script->waves.active==active&&script->myscenario==started&&p.GetQuestStatus(40385)==QUEST_STATUS_INCOMPLETE&&!p.HasItemCount(128808,1,true),"Cemetery release granted scenario/wave/quest/item progression");++cases;++releases;
  }
  // Supply only the real harpoon criterion endpoint, not its cast/flight. The
  // actual Scenario engine must acknowledge6->7 before the later GY unlocks.
  scenario->SetCurrentStep(6);{auto early=HunterSafe(6);auto late=HunterSafe(7);NativePlayer p(session,map,603,{late->Loc.X,late->Loc.Y,late->Loc.Z,0});p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.SetQuestStatus(40385,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40385);
   auto before=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifndef SURVIVAL_GRAVEYARD_BASELINE
   Require(before&&before->GetExactDist(early->Loc.X,early->Loc.Y,early->Loc.Z)<0.001f,"Unacknowledged harpoon unlocked later cemetery");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,47608);Require(scenario->GetCurrentStep()==7,"Native harpoon criterion did not acknowledge stage6->7");auto after=map.GetClosestGraveYard(early->Loc.X,early->Loc.Y,early->Loc.Z);
#ifdef SURVIVAL_GRAVEYARD_BASELINE
   Require(!before&&!after,"Pinned missing-boundary cemetery changed");
#else
   // The callback owns one reusable location; the old selection was inspected
   // before the second call rather than treated as an immutable snapshot.
   Require(after&&after->GetExactDist(late->Loc.X,late->Loc.Y,late->Loc.Z)<0.001f,"Acknowledged real harpoon credit did not unlock later cemetery");
#endif
   ++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,944);auto fury=sWorldSafeLocsStore.LookupEntry(5228);auto f=map.GetClosestGraveYard(0,0,0);Require(f&&f->GetExactDist(fury->Loc.X,fury->Loc.Y,fury->Loc.Z)<0.001f,"Survival recovery changed shared Fury cemetery");++cases;sScenarioMgr->RemoveScenario(map.GetInstanceId());CheckNoScenarioGraveyard(map,script,0,0,0);++cases;
 }
 Require(cases==110&&releases==104,"Native Hunter regression count changed");
#ifdef SURVIVAL_GRAVEYARD_BASELINE
 Require(wrongClassFallback==26,"Original wrong-class outdoor counterfactual count changed");std::cout<<"COUNTERFACTUAL pinned5d3d9312b Hunter native110/release104:outdoor transfer leaves104 ghosts,26 chooseFury5228;native harpoon6->7 but no cemetery;Fury retained. Expected broken,not ordinary success"<<std::endl;
#else
 Require(!wrongClassFallback,"Repaired Hunter used old outdoor path");std::cout<<"PASS native Survival graveyard110/release104:Human+Orc,all13 steps,four death-site contexts,grounded5295 before harpoon and5296 after acknowledged6->7,real release/ACK/half-health revival and ghost cleanup,repeated ACK,retained Fury/no-scenario;no scenario/wave/quest/item grant. Prior death/quest/stage and harpoon credit endpoint controlled;client/corpse/pet/full flight separate"<<std::endl;
#endif
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
