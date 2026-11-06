#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "PointMovementGenerator.h"
#include "RecoveryNative.inc"
#include "LegacyArrivalNative.inc"
struct LoadingTag{using Type=ObjectGuid WorldSession::*;friend Type Access(LoadingTag);};
template struct FixtureAccess<LoadingTag,&WorldSession::m_playerLoading>;
struct EntryClockTag{using Type=FunctionProcessor Unit::*;friend Type Access(EntryClockTag);};
template struct FixtureAccess<EntryClockTag,&Unit::m_Functions>;
struct ArrivalProbe:InstanceScript{
 using InstanceScript::InstanceScript;bool myscenario=false,jump=true;uint32 outrotimer=5000,ready=0;std::vector<ObjectGuid> helpers;
 void SetData(uint32 type,uint32 value)override{Require(type==WarswordsArrivalRecoveryNative::Ready&&value==1,"Native arrival bootstrap differs");++ready;}
 void OnCreatureCreate(Creature*actor)override{if(actor->GetEntry()==95708)helpers.push_back(actor->GetGUID());}
#include "LegacyEntryNative.inc"
};
struct ArrivalProbeFactory:InstanceMapScript{
 ArrivalProbeFactory():InstanceMapScript("instance_warswords",1511){}
 InstanceScript* GetInstanceScript(InstanceMap*map)const override{return new ArrivalProbe(map);}
};
struct ImpactReceipt:PlayerScript{
 uint32 impacts=0;ImpactReceipt():PlayerScript("native_warswords_impact_receipt"){}
 void OnSpellCast(Player*,Spell* spell,bool)override{if(spell->GetSpellInfo()->Id==191477)++impacts;}
};
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
 Shadow("spell_target_position","id=205744");
 auto existing=WorldDatabase.Query("SELECT COUNT(*) FROM spell_target_position");Require(existing&&existing->Fetch()[0].GetUInt32()<=1,"Native landing row cardinality differs");
 if(!existing->Fetch()[0].GetUInt32()) WorldDatabase.DirectExecute("INSERT INTO spell_target_position(id,target_map,target_position_x,target_position_y,target_position_z,target_orientation) VALUES(205744,1511,3386.46,1406.94,68.94,0)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellTargetPositions();
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1511,{}}});
 MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1511,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ArrivalProbeFactory;auto recovery=new player_warswords_arrival_recovery_native;auto impact=new ImpactReceipt;sScriptMgr->SwapScriptContext(true);
 MapInstanced parent(1511,300);InstanceMap map(1511,300,9987000,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);
 auto data=dynamic_cast<ArrivalProbe*>(map.GetInstanceScript());Require(data,"Actual native instance pointer missing");
 auto scenario=sScenarioMgr->AddScenario(&map,944);scenario->SetCurrentStep(0);auto session=Session();
 {
  NativePlayer player(session,map,901,{3349.74f,1355.64f,182.79f,0.715585f});player.SetQuestStatus(40043,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,40043);
  player.GetMotionMaster()->Initialize(); // Normal Player creation installs its idle slot before controlled movement.
  session->*Access(LoadingTag{})=player.GetGUID();Require(!player.CanContact(),"Real loading session is unexpectedly contactable");
  data->OnPlayerEnter(&player);(player.*Access(EntryClockTag{})).Update(3000);Require(!data->myscenario&&data->jump&&player.movespline->Finalized(),"Pinned actual3s callback did not drop loading entry");
  std::cout<<"COUNTERFACTUAL native real WorldSession loading/Player CanContact rejects pinned3s closure; no queued native descent remains. Not an ordinary passing run\n";
  player.Unit::RemoveFromWorld();WarswordsArrivalRecoveryNative::Enter(&player);
  Require(player.IsMapObjectArrivalProtected()&&player.HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY|MOVEMENTFLAG_ROOT),"Pre-create native bounded root/gravity protection missing");
  player.Unit::AddToWorld();auto& observer=*recovery;
  observer.OnMapChanged(&player);Require(WarswordsArrivalRecoveryNative::entries.size()==1,"Destination map-changed notification erased its new entry");
  observer.OnUpdate(&player,15000);Require(player.movespline->Finalized()&&WarswordsArrivalRecoveryNative::entries.size()==1,"Loading entry lost retry or moved before native contact");
  (session->*Access(LoadingTag{})).Clear();player.NotifyMapObjectArrivalLoaded(1511);observer.OnUpdate(&player,1);Require(player.movespline->Finalized(),"Descent bypassed native loading grace");
  player.UpdateMapObjectArrival(8000);Require(!player.IsMapObjectArrivalProtected(),"Native arrival ACK/grace did not release");
  observer.OnUpdate(&player,1);auto first=player.movespline->GetId();
  Require(!player.movespline->Finalized()&&player.HasAura(189695)&&player.HasAura(205534)&&data->ready,"Actual native189695/205534/spline start/instance bootstrap failed");
  auto landing=player.movespline->FinalDestination();Require(std::fabs(landing.x-3386.46f)<0.1f&&std::fabs(landing.z-68.94f)<0.1f,"Native descent destination differs");
  player.movespline->updateState(4999);auto point=player.movespline->ComputePosition();player.Relocate(point.x,point.y,point.z,point.orientation);
  observer.OnUpdate(&player,4999);Require(player.movespline->GetId()==first,"Native finish preceded5000ms");
#ifdef COMPLETED_SPLINE
  player.movespline->updateState(player.movespline->Duration());point=player.movespline->ComputePosition();player.Relocate(point.x,point.y,point.z,point.orientation);
#endif
  observer.OnUpdate(&player,1);player.m_Events.Update(1000);(player.*Access(EntryClockTag{})).Update(10);observer.OnUpdate(&player,1);
  std::cout<<"TRACE native finish spline="<<first<<"->"<<player.movespline->GetId()<<" aura="<<player.HasAura(189695)<<" pending="<<WarswordsArrivalRecoveryNative::entries.size()<<std::endl;
  Require(!player.HasAura(189695)&&WarswordsArrivalRecoveryNative::entries.empty(),"Real205744 native finish/spline/aura acknowledgement failed");
#ifdef COMPLETED_SPLINE
  Require(player.movespline->GetId()==first&&player.movespline->Finalized(),"Already-arrived native finish invented a new movement");uint32 expectedImpact=0;
#else
  Require(player.movespline->GetId()!=first,"Native finish did not start its actual landing");uint32 expectedImpact=1;
#endif
  Require(Stories(map,838)==1&&!impact->impacts,"Native838 missing or impact fired before actual arrival");
  player.movespline->updateState(player.movespline->Duration());point=player.movespline->ComputePosition();player.Relocate(point.x,point.y,point.z,point.orientation);
  player.GetMotionMaster()->UpdateMotion(1);player.GetMotionMaster()->UpdateMotion(1);
  Require(impact->impacts==expectedImpact,"Actual arrived native191477 count differs; zero-distance cannot invent movement/impact");
  player.GetMotionMaster()->MoveJump(3400,1410,70,0,3,3);
  EffectMovementGenerator interrupted(205744,191477,player.GetGUID(),1511);Require(interrupted.Update(player,1),"Interrupted movement was already finalized");interrupted.Finalize(player);Require(impact->impacts==expectedImpact,"Interrupted native landing emitted impact");
  player.movespline->updateState(player.movespline->Duration());
  EffectMovementGenerator foreign(205744,191477,player.GetGUID(),1495);foreign.Update(player,1);foreign.Finalize(player);Require(impact->impacts==expectedImpact,"Native landing ignored original map");
  player.Unit::setDeathState(CORPSE);EffectMovementGenerator dead(205744,191477,player.GetGUID(),1511);dead.Update(player,1);dead.Finalize(player);Require(impact->impacts==expectedImpact,"Dead native landing emitted impact");player.Unit::setDeathState(ALIVE);player.SetHealth(player.GetMaxHealth());
  std::cout<<"PASS real native loading >3s, pre-self-create gravity/root hold, actual8s loading ACK grace, CanContact-ready189695/205534 and original spline, real205744 finish/movement/aura acknowledgement; only838 after accepted finish. Earlier quest/stage and clock/ACK controlled, not rendered client flight\n";
  player.GetMotionMaster()->Clear();observer.OnBeforeLogout(&player);
  for(auto guid:data->helpers)if(auto actor=map.GetCreature(guid))actor->DespawnOrUnsummon();map.RemoveAllObjectsInRemoveList();CleanStories(map);
 }
 session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
