#include "ActorQueueLifetimeNative.h"
#include "ThirtiethNativeScripts.inc"
using namespace ThirtiethNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999788,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,10991,{52,803,805,0}),owner(&map,299,10992,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor))&&map.AddToMap(static_cast<Creature*>(&owner)),"actual actor insertion");actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());Require(actor.GetAnyOwner()==&owner,"actual owner lookup");
RunActorQueue<AggramarActors::Frame>(actor,0,3000,false,4,UINT32_MAX,nullptr,5);
RunActorQueue<AggramarActors::Frame>(actor,1,500,false,1,UINT32_MAX);
RunActorQueue<VarimathrasShadow::Frame>(actor,0,100,false,2,UINT32_MAX);
RunActorQueue<VarimathrasShadow::Frame>(actor,1,100,false,1,UINT32_MAX);
RunActorQueue<VarimathrasShadow::Frame>(actor,2,600,false,1,UINT32_MAX);
RunActorQueue<VarimathrasShadow::Frame>(actor,3,800,false,1,UINT32_MAX);
RunActorQueue<TrilliaxOutro::Frame>(actor,0,250,false,1,UINT32_MAX);
RunActorQueue<TrilliaxOutro::Frame>(actor,1,16500,false,1,UINT32_MAX);
RunActorQueue<MaylaDialogue::Frame>(actor,0,3000,false,1,UINT32_MAX);
RunActorQueue<AzsunaFighter::Frame>(actor,0,3000,false,3,UINT32_MAX);
RunActorQueue<EventTeleporter::Frame>(actor,0,100,false,1,0);
RunActorQueue<WorldbreakerTriggers::Frame>(actor,0,200,true,1,UINT32_MAX);
RunActorQueue<WorldbreakerTriggers::Frame>(actor,1,4000,true,2,UINT32_MAX);
RunActorQueue<VizaduumDoor::Frame>(actor,0,15000,false,1,UINT32_MAX);
RunActorQueue<ZuraalWalker::Frame>(actor,0,7500,false,2,UINT32_MAX);
RunActorQueue<ZuraalWalker::Frame>(actor,1,10500,false,1,UINT32_MAX);
RunActorQueue<ZuraalWalker::Frame>(actor,2,17000,false,1,UINT32_MAX);
RunActorQueue<LuraAlleria::Frame>(actor,0,5000,false,2,UINT32_MAX);
RunActorQueue<LuraAlleria::Frame>(actor,1,5000,false,1,UINT32_MAX);
RunActorQueue<LuraAlleria::Frame>(actor,2,5500,false,1,0);
RunActorQueue<LuraAlleria::Frame>(actor,3,5000,false,1,UINT32_MAX);
RunActorQueue<LuraAlleria::Frame>(actor,4,5000,false,1,UINT32_MAX);
RunActorQueue<LuraAlleria::Frame>(actor,5,5500,false,1,0);
Observe::queryPresent=false;RunActorQueue<VizaduumDoor::Frame>(actor,0,15000,false,0);RunActorQueue<LuraAlleria::Frame>(actor,2,5500,false,0);RunActorQueue<LuraAlleria::Frame>(actor,5,5500,false,0);Observe::queryPresent=true;
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:23 root/24 compiled actual event/combat Unit-owned queues after Creature::AIM_Initialize deletes original callback AI;authored delays,one-shot and real Unit cancellation. Native owner lookup,movement/unit-state changes;cast/aura/despawn endpoints observed,not full registered content/client traversal or full-core ASAN;no character writes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
