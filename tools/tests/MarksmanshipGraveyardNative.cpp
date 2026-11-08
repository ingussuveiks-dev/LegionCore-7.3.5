#define main HolyGraveyardUnusedMain
#include "TuureGraveyardNative.cpp"
#undef main
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);sAreaTriggerDataStore->LoadAreaTriggerTeleports();sLFGMgr->LoadLFGDungeons();sSpellMgr->LoadSpellTargetPositions();
 auto early=sWorldSafeLocsStore.LookupEntry(5433);auto late=sWorldSafeLocsStore.LookupEntry(5434);Require(early&&late&&early->MapID==1489&&late->MapID==1489&&std::string(early->Name->Str[0])=="7.0 Artifacts - Niskara - MM Niskara GY 1 - CRT"&&std::string(late->Name->Str[0])=="7.0 Artifacts - Niskara - MM Niskara GY 2 - CRT","Actual Hunter cemetery identities absent");
 auto steps=sScenarioMgr->GetScenarioSteps(972);Require(steps&&steps->size()==8,"Native972 eight-step sequence changed");auto criterion=sCriteriaStore.LookupEntry(29305);Require(criterion&&criterion->Type==92&&criterion->Asset==47927,"Actual Herald acknowledgement changed");bool contained=false;for(auto leaf:sCriteriaTreeStore)if(leaf->CriteriaID==29305){auto current=leaf;for(unsigned depth=0;current&&depth<64;++depth){if(current->ID==46385)contained=true;current=sCriteriaTreeStore.LookupEntry(current->Parent);}}Require(contained&&(*steps)[4]->Criteriatreeid==46385,"Native Herald4->5 root changed");
 auto row=WorldDatabase.Query("SELECT link,event_type,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=100836 AND source_type=0 AND id=3");Require(row&&row->GetRowCount()==1,"Native Herald death dispatch missing");auto f=row->Fetch();Require(f[0].GetUInt32()==4&&f[1].GetUInt32()==6&&f[2].GetUInt32()==205&&f[3].GetUInt32()==92&&f[4].GetUInt32()==47927&&f[5].GetUInt32()==7,"Native Herald owner/group credit bridge changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1489,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1489,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new TuureGraveyardFixture::instance_tuure;sScriptMgr->SwapScriptContext(true);unsigned cases=0,releases=0;
 for(unsigned race:{unsigned(RACE_HUMAN),unsigned(RACE_ORC)}){
  MapInstanced parent(1489,300);InstanceMap map(1489,300,9998000+race,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<TuureGraveyardFixture::instance_tuure::instance_tuure_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Real shared1489 registration failed");auto session=Session();auto scenario=sScenarioMgr->AddScenario(&map,972);
  for(auto safe:{early,late}){auto owner=std::make_unique<NativePlayer>(session,map,911,Position{safe->Loc.X,safe->Loc.Y,safe->Loc.Z,0});Require(std::fabs(map.GetHeight(owner->GetPhases(),safe->Loc.X,safe->Loc.Y,safe->Loc.Z+5)-safe->Loc.Z)<.5f,"Native Hunter cemetery floor unsupported");}
  auto exercise=[&](Position pos,unsigned step,bool completed,unsigned place){auto owner=std::make_unique<NativePlayer>(session,map,912,pos);auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,0,race);p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.setFactionForRace(race);p.SetQuestStatus(40419,place%2?QUEST_STATUS_COMPLETE:QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40419);
   if(completed){p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,48210);Require(scenario->IsCompleted(false)&&scenario->GetCurrentStep()==8,"Native completed bow control failed");}
   auto count=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();auto quest=p.GetQuestStatus(40419);auto fight=script->firstFight;auto route=script->firstRoute;auto safe=step<5?early:late;
   auto gy=map.GetClosestGraveYard(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ());Require(gy&&gy->GetMapId()==1489,"Instance cemetery dispatch absent");
#ifdef MARKSMAN_GRAVEYARD_BASELINE
   Require(gy->GetExactDist(pos)<.001f,"Original Hunter corpse echo not reproduced");
#else
   Require(gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f&&std::fabs(gy->GetOrientation()-safe->Loc.O*float(M_PI)/180)<.001f,"Native Hunter checkpoint/facing failed");Require(map.GetClosestGraveYard(0,0,-1000)==gy&&gy->GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z)<.001f,"Repeated selection changed checkpoint");
#endif
   p.SetHealth(0);p.setDeathState(CORPSE);p.CastSpell(&p,8326,true);p.SetHealth(1);Require(p.HasAura(8326)&&p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST),"Native ghost state missing");p.RepopAtGraveyard();Require(p.IsBeingTeleportedNear(),"Native release did not start transfer");WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};ack.MoverGUID=p.GetGUID();session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&!p.IsBeingTeleported()&&!p.HasAura(8326)&&!p.HasFlag(PLAYER_FIELD_PLAYER_FLAGS,PLAYER_FLAGS_GHOST)&&p.GetHealth()==p.GetMaxHealth()/2,"Native ACK/half-health/ghost cleanup failed");
#ifdef MARKSMAN_GRAVEYARD_BASELINE
   Require(p.GetExactDist(pos.GetPositionX(),pos.GetPositionY(),pos.GetPositionZ()+2)<.001f,"Pinned unsafe destination changed");
#else
   Require(p.GetExactDist(safe->Loc.X,safe->Loc.Y,safe->Loc.Z+2)<.001f,"Native ACK did not reach Hunter cemetery");
#endif
   session->HandleMoveTeleportAck(ack);Require(p.IsAlive()&&p.GetHealth()==p.GetMaxHealth()/2&&scenario->GetCurrentStep()==step&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==count&&p.GetQuestStatus(40419)==quest&&!p.HasItemCount(128826,1,true)&&script->firstFight==fight&&script->firstRoute==route,"Release changed scenario/controller/quest/item state");++cases;++releases;
  };
  for(unsigned step=0;step<8;++step)for(unsigned place=0;place<4;++place){scenario->SetCurrentStep(step);Position pos=place==0?Position{early->Loc.X,early->Loc.Y,early->Loc.Z,0}:place==1?Position{late->Loc.X,late->Loc.Y,late->Loc.Z,0}:place==2?Position{-828.85f,4372,738.64f,0}:Position{late->Loc.X,late->Loc.Y,-1000,0};exercise(pos,step,false,place);}
  scenario->SetCurrentStep(7);exercise({late->Loc.X,late->Loc.Y,late->Loc.Z,0},8,true,0);
  // Fresh real criteria graph: only the native accepted47927 at4 unlocks5.
  sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,972);scenario->SetCurrentStep(3);{auto owner=std::make_unique<NativePlayer>(session,map,913,Position{late->Loc.X,late->Loc.Y,late->Loc.Z,0});auto& p=*owner;p.SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_HUNTER);p.SetQuestStatus(40419,QUEST_STATUS_INCOMPLETE);p.SetQuestSlot(0,40419);p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,47927);Require(scenario->GetCurrentStep()==3,"Unrelated early event advanced native stage");scenario->SetCurrentStep(4);auto before=map.GetClosestGraveYard(late->Loc.X,late->Loc.Y,late->Loc.Z);
#ifdef MARKSMAN_GRAVEYARD_BASELINE
   Require(before&&before->GetExactDist(late->Loc.X,late->Loc.Y,late->Loc.Z)<.001f,"Pinned boundary echo changed");
#else
   Require(before&&before->GetExactDist(early->Loc.X,early->Loc.Y,early->Loc.Z)<.001f,"Unacknowledged Herald unlocked later cemetery");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,47927);Require(scenario->GetCurrentStep()==5,"Native Herald event did not acknowledge4->5");auto after=map.GetClosestGraveYard(early->Loc.X,early->Loc.Y,early->Loc.Z);
#ifdef MARKSMAN_GRAVEYARD_BASELINE
   Require(after&&after->GetExactDist(early->Loc.X,early->Loc.Y,early->Loc.Z)<.001f,"Pinned later echo changed");
#else
   Require(after&&after->GetExactDist(late->Loc.X,late->Loc.Y,late->Loc.Z)<.001f,"Acknowledged Herald did not unlock native5434");
#endif
   p.UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2,47927);Require(scenario->GetCurrentStep()==5&&!p.HasItemCount(128826,1,true),"Duplicate native event skipped Inquisitor or granted bow");++cases;}
  for(unsigned id:{1085u,940u}){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,id);auto gy=map.GetClosestGraveYard(12,34,-56);auto holy=sWorldSafeLocsStore.LookupEntry(5480);if(id==1085)Require(gy&&gy->GetExactDist(holy->Loc.X,holy->Loc.Y,holy->Loc.Z)<.001f,"Retained Holy policy changed");else{
#ifdef MARKSMAN_GRAVEYARD_BASELINE
Require(gy&&gy->GetExactDist(12,34,-56)<.001f,"Pinned Blood policy changed");
#else
auto blood=sSpellMgr->GetSpellTargetPosition(201678);Require(gy&&blood&&gy->GetExactDist(blood->target_X,blood->target_Y,blood->target_Z)<.001f,"Native Blood stage0 recovery changed");
#endif
}++cases;}
  sScenarioMgr->RemoveScenario(map.GetInstanceId());Require(map.GetClosestGraveYard(12,34,-56)->GetExactDist(12,34,-56)<.001f,"No-scenario policy changed");++cases;session->SetPlayer(nullptr);delete session;
 }
 Require(cases==74&&releases==66,"MM native count changed");
#ifdef MARKSMAN_GRAVEYARD_BASELINE
 std::cout<<"COUNTERFACTUAL pinnedf3c39b87a MM native74/release66:whole old instance echoes unsafe corpse positions,actual Herald4->5 still has no cemetery boundary;current support libraries,not historical binary or safe recovery"<<std::endl;
#else
 std::cout<<"PASS MM native74/release66:Human+Orc/all8stages+completed/incomplete+complete,four death contexts,real5433/5434 floors/facing,actual ghost release/ACK/half-health/repeat cleanup,real47927/29305/root46385 boundary4->5 plus early/duplicate denials,retained Holy/Blood/no-scenario;no callback progression/item/quest grants. Earlier death/state/final credit endpoints controlled;full client/corpse/pet/group/Herald combat separate"<<std::endl;
#endif
 CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
