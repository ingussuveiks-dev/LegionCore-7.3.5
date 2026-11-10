#include "ActorQueueLifetimeNative.h"
#include "TwentyEighthNativeScripts.inc"
using namespace TwentyEighthNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999788,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,10991,{52,803,805,0}),owner(&map,299,10992,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor))&&map.AddToMap(static_cast<Creature*>(&owner)),"actual actor insertion");actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());Require(actor.GetAnyOwner()==&owner,"actual owner lookup");
RunActorQueue<HasabelOutro::Frame>(actor,0,1000,false,1);
RunActorQueue<ImonarShips::Frame>(actor,0,12000,true,1);
RunActorQueue<ImonarShips::Frame>(actor,1,8000,true,1);
RunActorQueue<TichondriusSpire::Frame>(actor,0,3000,false,1);
RunActorQueue<ElisandeOrb::Frame>(actor,0,1000,false,1);
RunActorQueue<SasszineJellyfish::Frame>(actor,0,1000,false,1);
RunActorQueue<CordanaLight::Frame>(actor,0,1000,true,1);
RunActorQueue<ArmyMinions::Frame>(actor,0,3000,false,1);
RunActorQueue<EvalaMinion::Frame>(actor,0,14000,false,1);
RunActorQueue<DeepbeardRecovery::Frame>(actor,0,1000,false,2);
RunActorQueue<SmashspiteBat::Frame>(actor,0,200,false,1);
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:11 actual event/combat Unit-owned queues after Creature::AIM_Initialize deletes original callback AI;authored delays,one-shot and real Unit cancellation. Native owner lookup,movement/unit-state changes;cast/aura/despawn endpoints observed,not full registered content/client traversal or full-core ASAN;no character writes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
