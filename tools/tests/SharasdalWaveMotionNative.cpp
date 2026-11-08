// Real authored wave spline/map movement, attached contact and seat lifecycle.
#define SLAGHAMMER_COMPANION_SUPPORT_ONLY
#include "SlaghammerCompanionNative.cpp"
#include "AreaTriggerData.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "MotionMaster.h"
#include "PathGenerator.h"
#include <memory>
namespace SharasdalMotionFixture {
#include "SharasdalWave.inc"
#include "SharasdalWaveLanes.inc"
#include "SharasdalWaveEnd.inc"
}
// Exercise the same private native movement dispatcher without changing core access.
struct WaveMotionTickTag {friend auto WaveMotionTick(WaveMotionTickTag);};
template<auto Method>struct WaveMotionTickAccess {friend auto WaveMotionTick(WaveMotionTickTag){return Method;}};
template struct WaveMotionTickAccess<&Unit::UpdateSplineMovement>;
static void TickMotion(Unit* unit,uint32 diff){(unit->*WaveMotionTick(WaveMotionTickTag{}))(diff);}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);Require(sObjectMgr->LoadTrinityStrings(),"Native strings absent");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("spell_proc_event","1");
 for(auto name:{"areatrigger_template","areatrigger_data","areatrigger_actions"})Shadow(name,"entry=6547 OR customEntry=11160");
 for(auto name:{"areatrigger_template_circle","areatrigger_template_sequence","areatrigger_polygon","areatrigger_scripts"})Shadow(name,"entry IN(6547,11160)");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");sSpellMgr->LoadSpellProcEvents();sObjectAccessor->SetGuidSize(HighGuid::AreaTrigger,100000);sAreaTriggerDataStore->LoadAreaTriggerScripts();sAreaTriggerDataStore->LoadAreaTriggerActionsAndData();
 auto data=sAreaTriggerDataStore->GetAreaTriggerInfo(6547);Require(data&&data->HasAttached&&!data->isMoving&&data->spellId==209132,"Actual attached wave metadata changed");
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new SharasdalMotionFixture::npc_sharasdal_waterwall;sScriptMgr->SwapScriptContext(true);
 unsigned cases=0;
 for(unsigned race:{unsigned(RACE_DRAENEI),unsigned(RACE_ORC)})for(unsigned lane=0;lane<3;++lane)for(unsigned mode=0;mode<4;++mode){
  auto parent=std::make_unique<MapInstanced>(1600,300);auto mapOwner=std::make_unique<InstanceMap>(1600,300,9999400+cases,DIFFICULTY_N_SCENARIO,parent.get());auto& map=*mapOwner;auto scenario=sScenarioMgr->AddScenario(&map,1066);scenario->SetCurrentStep(3);auto session=Session();
  {
   Position start=SharasdalMotionFixture::sumpos[lane];Position end=SharasdalMotionFixture::waveEnd;
   Position pose{(start.GetPositionX()+end.GetPositionX())/2,(start.GetPositionY()+end.GetPositionY())/2,(start.GetPositionZ()+end.GetPositionZ())/2,0};if(mode==1)pose.m_positionY+=20;
   auto player=std::make_unique<NativePlayer>(session,map,993,pose);player->SetByteValue(UNIT_FIELD_BYTES_0,0,race);player->SetByteValue(UNIT_FIELD_BYTES_0,1,CLASS_SHAMAN);player->setFactionForRace(race);player->SetPhaseId({6120},false);player->SetQuestStatus(40341,QUEST_STATUS_INCOMPLETE);player->SetQuestSlot(0,40341);auto info=sChrRacesStore.LookupEntry(race);Require(info&&sCreatureDisplayInfoStore.LookupEntry(info->MaleDisplayID),"Actual race model missing");player->SetNativeDisplayId(info->MaleDisplayID);player->SetDisplayId(info->MaleDisplayID);
   auto wave=map.SummonCreature(105251,start,nullptr,0);Require(wave&&wave->GetVehicleKit(),"Native wave missing");wave->SetPhaseId({6120},false);auto guid=wave->GetGUID();
   PathGenerator path(wave);Require(path.CalculatePath(end.GetPositionX(),end.GetPositionY(),end.GetPositionZ()),"Actual lane path could not calculate");
   auto pathType=path.GetPathType();std::cout<<"LANE "<<lane<<" navmesh="<<(MMAP::MMapFactory::createOrGetMMapManager()->GetNavMeshQuery(1600,map.GetInstanceId())!=nullptr)<<" path="<<unsigned(pathType)<<" points="<<path.GetPath().size()<<std::endl;
   Require((pathType&PATHFIND_NORMAL)&&!(pathType&(PATHFIND_NOT_USING_PATH|PATHFIND_NOPATH|PATHFIND_INCOMPLETE)),"Authored lane did not use a complete actual navigation path");
   // Match the real controller order: launch the authored point, then ordinary FX.
   wave->GetMotionMaster()->MovePoint(1,end.GetPositionX(),end.GetPositionY(),end.GetPositionZ());Require(!wave->movespline->Finalized(),"Authored native point did not launch");
   Require(wave->CastSpell(wave,209132)==SPELL_CAST_OK,"Authored moving209132 cast rejected");auto at=wave->GetAreaObject(209132);Require(at&&at->GetTargetGuid()==guid,"Real native attached wave absent");
   bool contact=false,boarded=false,aborted=false,interrupted=false;unsigned updates=0;auto previous=start;auto criteria=scenario->GetAchievementMgr().GetCriteriaProgressMap()->size();
   while(!wave->IsDespawn()&&updates++<2400){
    TickMotion(wave,50);map.MoveAllCreaturesInMoveList();wave->GetMotionMaster()->UpdateMotion(50);map.MoveAllCreaturesInMoveList();
    at->Update(50);map.MoveAllAreaTriggersInMoveList();
    Require(at->GetExactDist(wave)<0.02f,"Attached AT lagged behind actual wave pose");
    if(at->IsUnitAffected(player->GetGUID()))contact=true;
    if(mode==2&&contact&&!aborted){Require(!player->GetVehicleBase(),"Join executed before asynchronous endpoint");wave->RemoveVehicleKit();player->m_Events.Update(1);Require(wave->CreateVehicleKit(4717,105251),"Native aborted-join kit restoration failed");aborted=true;}
    player->m_Events.Update(50);TickMotion(player.get(),50);
    if(player->GetVehicleBase()==wave){boarded=true;Require(wave->GetVehicleKit()->GetPassenger(0)==player.get()&&player->GetTransGUID()==guid&&player->GetExactDist(wave)<12,"Moving native seat/base/transport pose disagree");}
    if(mode==3&&!interrupted&&boarded){wave->AddUnitState(UNIT_STATE_ROOT);wave->GetMotionMaster()->UpdateMotion(50);auto stopped=wave->GetPosition();TickMotion(wave,250);wave->GetMotionMaster()->UpdateMotion(250);Require(wave->GetExactDist(&stopped)<0.02f&&!wave->IsDespawn(),"Rooted native route moved/finished wave");wave->ClearUnitState(UNIT_STATE_ROOT);wave->GetMotionMaster()->UpdateMotion(1);Require(!wave->movespline->Finalized(),"Native point did not resume after root");interrupted=true;}
    if(updates>1&&!wave->IsDespawn())Require(wave->GetPositionX()<=previous.GetPositionX()+0.05f,"Authored wave moved away from destination");previous=wave->GetPosition();
   }
   std::cout<<"MOTION race="<<race<<" lane="<<lane<<" mode="<<mode<<" updates="<<updates<<" contact="<<contact<<" boarded="<<boarded<<" end="<<wave->GetPositionX()<<","<<wave->GetPositionY()<<","<<wave->GetPositionZ()<<std::endl;
   Require(wave->IsDespawn()&&wave->GetExactDist(&end)<0.1f,"Natural Point finalize did not reach authored endpoint/cleanup");
   Require(mode==1?(!contact&&!boarded):(contact&&boarded),"Moving contact/ride outcome differs");Require(mode!=2||aborted,"Aborted moving-join branch absent");Require(mode!=3||interrupted,"Interrupted route control absent");Require(!player->GetVehicleBase(),"Natural Point cleanup retained player");
   wave->m_Events.Update(99);map.RemoveAllObjectsInRemoveList();Require(map.GetCreature(guid)==wave,"Original100ms cleanup grace shortened");wave->m_Events.Update(1);map.RemoveAllObjectsInRemoveList();Require(!map.GetCreature(guid),"Native completed moving wave retained");
   player->m_Events.Update(1000);Require(!player->GetVehicleBase()&&player->GetQuestStatus(40341)==QUEST_STATUS_INCOMPLETE&&!player->HasItemCount(128911,1,true)&&scenario->GetCurrentStep()==3&&scenario->GetAchievementMgr().GetCriteriaProgressMap()->size()==criteria,"Moving wave changed player progress/item/vehicle");++cases;
  }
  sScenarioMgr->RemoveScenario(map.GetInstanceId());session->SetPlayer(nullptr);delete session;
 }
 Require(cases==24,"Moving-wave coverage changed");std::cout<<"PASS native moving waves24:authored3 lanes/both factions,actual MovePoint/spline/map relocation/attached6547 geometric contact/queued46598 seat/moving aborted-join recovery/native root pause/resumption/natural POINT ejection99+1ms removal;outside control/no quest-item-progress mutation. Manual native dispatcher clock/earlier stage controlled;client/camera/pet portrait/full1066 separate"<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
