// Actual DB134170 spawn, GameObject publication, model33309 and shared handoff.
// Earlier real scene/guard receipt and physical companion pose are controlled.
#define HRUTHNIR_START_SUPPORT_ONLY
#include "HruthnirStartNative.cpp"
#include "GameObjectModel.h"
struct BarrierObject:GameObject {~BarrierObject(){if(IsInWorld())GetMap()->RemoveFromMap(static_cast<GameObject*>(this),false);ResetMap();}};
struct BarrierPlayerLink {Player& p;BarrierPlayerLink(Player& a,Map& m):p(a){p.GetMapRef().link(&m,&p);}~BarrierPlayerLink(){p.GetMapRef().unlink();}};
struct BarrierStepTag {using Type=uint8 Scenario::*;friend Type Access(BarrierStepTag);};
template struct FixtureAccess<BarrierStepTag,&Scenario::currentStep>;
static bool BarrierMeshHit(GameObject& door){
 auto const& box=door.m_model->getBounds();auto lo=box.low(),hi=box.high();
 for(float z:{0.25f,0.5f,0.75f})for(float cross:{0.25f,0.5f,0.75f})for(bool x:{false,true}){
  G3D::Vector3 from,to,result;float height=lo.z+(hi.z-lo.z)*z;
  if(x){float y=lo.y+(hi.y-lo.y)*cross;from={lo.x-2,y,height};to={hi.x+2,y,height};}
  else{float a=lo.x+(hi.x-lo.x)*cross;from={a,lo.y-2,height};to={a,hi.y+2,height};}
  if(door.m_model->getObjectHitPos(door.GetPhases(),false,from,to,result,0))return true;
 }return false;
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");Shadow("gameobject","guid=134170");Shadow("creature_text","CreatureID=96468");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE barrier_clone LIKE gameobject");WorldDatabase.DirectExecute("INSERT INTO barrier_clone SELECT * FROM gameobject");WorldDatabase.DirectExecute("UPDATE barrier_clone SET guid=134171,position_x=position_x+10");WorldDatabase.DirectExecute("INSERT INTO gameobject SELECT * FROM barrier_clone");WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
 sObjectMgr->LoadPageTexts();sObjectMgr->LoadGameObjectTemplate();sObjectMgr->LoadGameobjects();sCreatureTextMgr->LoadCreatureTexts();LoadGameObjectModelList(sWorld->GetDataPath());HashMapHolder<GameObject>::SetSize(20000);
 auto source=sObjectMgr->GetGOData(134170);auto info=sObjectMgr->GetGameObjectTemplate(243897);Require(source&&source->id==243897&&source->mapid==1495&&source->phaseMask==3&&info&&info->type==GAMEOBJECT_TYPE_DOOR&&info->displayId==33309,"Actual native barrier catalog changed");
 sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());new ShieldsFixture::instance_shields_rest;sScriptMgr->SwapScriptContext(true);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1495,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1495,{}}});
 uint32 cases=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<13;++mode){
#ifdef HRUTHNIR_BARRIER_BASELINE
  if(mode)continue;
#endif
  MapInstanced parent(1495,300);InstanceMap map(1495,300,9995200+cases++,DIFFICULTY_N_SCENARIO,&parent);map.CreateInstanceData(nullptr);auto script=dynamic_cast<ShieldsFixture::instance_shields_rest::instance_shields_rest_InstanceMapScript*>(map.GetInstanceScript());Require(script,"Actual shared instance missing");auto scenario=sScenarioMgr->AddScenario(&map,909);scenario->SetCurrentStep(3);auto session=Session();
  {
   NativePlayer player(session,map,901,{4765,340,-37.19f,0});BarrierPlayerLink link(player,map);player.SetByteValue(UNIT_FIELD_BYTES_0,0,race);player.setFactionForRace(race);player.SetQuestStatus(39191,QUEST_STATUS_INCOMPLETE);player.SetQuestSlot(0,39191);player.SetPhaseId({6999},true);
   HruthnirActor companion(&map,96468,12702,{4763.73f,340.87f,-37.19f,0});companion.*Access(AltheaGuardDbTag{})=361306;Require(map.AddToMap(static_cast<Creature*>(&companion)),"Native companion missing");companion.SetPhaseId({6999},true);script->OnCreatureCreate(&companion);
   std::unique_ptr<BarrierObject> door;
   auto add=[&](bool modified=false){door=std::make_unique<BarrierObject>();Require(door->LoadFromDB(modified&&mode==8?134171:134170,&map),"Native DB barrier load failed");Require(door->GetPhaseMask()==3&&door->GetGoState()==GO_STATE_READY,"Native DB barrier initial state differs");if(modified&&mode==6)door->SetPhaseMask(8,false);if(modified&&mode==7)door->SetPhaseId({6985},false);if(modified&&mode==12)door->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);Require(map.AddToMap(static_cast<GameObject*>(door.get())),"Native barrier publication failed");Require(door->m_model&&door->GetDisplayId()==33309,"Actual33309 model not loaded");};add();
   Require(door->GetPhaseMask()==3&&BarrierMeshHit(*door)&&player.InSamePhase(door.get()),"Native closed barrier control missing");auto oldGuid=door->GetGUID();
#ifndef HRUTHNIR_BARRIER_BASELINE
   if(mode){
    // The initial scene/guard victory and instance dispatch are earlier seeded
    // endpoints; publish the real replacement at each admitted/refused state.
    script->magnarGuardCreditSent=script->magnarGuardsDelivered=true;
    if(mode==1)scenario->SetCurrentStep(2);if(mode==2)script->magnarGuardCreditSent=false;if(mode==3)script->magnarGuardsDelivered=false;
    if(mode==4||mode==5){sScenarioMgr->RemoveScenario(map.GetInstanceId());scenario=sScenarioMgr->AddScenario(&map,mode==4?1068:1082);scenario->SetCurrentStep(3);}
    if(mode==9)scenario->SetCurrentStep(4);if(mode==10)scenario->SetCurrentStep(5);if(mode==11){Require(scenario->GetStepCount(true)==6,"Native terminal endpoint changed");scenario->*Access(BarrierStepTag{})=6;}
    door.reset();add(true);bool release=mode>=9;
    if(release)Require(door->GetPhaseMask()==4&&door->GetGoState()==(mode==12?GO_STATE_ACTIVE_ALTERNATIVE:GO_STATE_ACTIVE)&&!BarrierMeshHit(*door),"Admitted replacement lost native open state/phase");
    else Require(door->GetPhaseMask()==(mode==6?8:3)&&door->GetGoState()==GO_STATE_READY&&BarrierMeshHit(*door),"Refused replacement was altered");
    auto mask=door->GetPhaseMask();auto state=door->GetGoState();script->RestoreMagnarBarrier(door.get());Require(door->GetPhaseMask()==mask&&door->GetGoState()==state&&!WaveCredit(scenario),"Repeat/refusal changed barrier or progression");
    door->Update(5000);Require(BarrierMeshHit(*door)==!release,"Native GO loot/update cycle changed collision contract");
    std::cout<<"PASS barrier scope race="<<race<<" mode="<<mode<<": actual DB/new GUID/pre-publication/real33309 mesh/native GO update,guard/delivery/stage/shared class/custom phase or clone admission,terminal/open alternative/repeat state preserved,no progress\n";
   }else
#endif
   {
   script->magnarGuardCreditSent=true;script->TryAdvanceMagnarGuards();Require(script->magnarGuardsDelivered&&door->GetPhaseMask()==4&&scenario->GetCurrentStep()==3&&!WaveCredit(scenario),"Actual chamber handoff did not release phase or preserved progression");
#ifdef HRUTHNIR_BARRIER_BASELINE
   Require(door->GetGoState()==GO_STATE_READY&&BarrierMeshHit(*door)&&!player.InSamePhase(door.get()),"Pinned hidden-but-colliding barrier did not reproduce");std::cout<<"COUNTERFACTUAL barrier race="<<race<<": actual handoff phase4 hides original33309 door while ready-state mesh still blocks native phase-ID rays\n";
#else
   Require(door->GetGoState()==GO_STATE_ACTIVE&&!door->m_model->isCollisionEnabled()&&!BarrierMeshHit(*door)&&!player.InSamePhase(door.get()),"Released native barrier still blocks model");
#endif
   door.reset();add();Require(door->GetGUID()!=oldGuid&&script->magnarDoorGUID==door->GetGUID(),"Real new barrier GUID not rebound");script->TryAdvanceMagnarGuards();
#ifdef HRUTHNIR_BARRIER_BASELINE
   Require(door->GetPhaseMask()==3&&door->GetGoState()==GO_STATE_READY&&BarrierMeshHit(*door)&&player.InSamePhase(door.get()),"Pinned recreated closed visible barrier did not reproduce");std::cout<<"COUNTERFACTUAL barrier reload race="<<race<<": real DB reload/new GUID republishes mask3/closed33309 after delivered handoff; instance returns early\n";
#else
   Require(door->GetPhaseMask()==4&&door->GetGoState()==GO_STATE_ACTIVE&&!BarrierMeshHit(*door)&&!player.InSamePhase(door.get()),"Recreated native barrier did not retain release");std::cout<<"PASS native barrier race="<<race<<": actual DB load/publication/original handoff/open collision,real remove+new GUID/republication/native33309 mesh and phase release,no progression; earlier scene/guard endpoints controlled\n";
#endif
   }
   companion.Brain().Reset();companion.StopMoving();CleanStories(map);door.reset();
  }session->SetPlayer(nullptr);delete session;sScenarioMgr->RemoveScenario(map.GetInstanceId());
 }
 std::cout<<"PASS native barrier cases="<<cases<<std::endl;CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
