#include "ActorQueueLifetimeNative.h"
#include "ThirtySecondNativeScripts.inc"
using namespace ThirtySecondNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999788,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,10991,{52,803,805,0}),owner(&map,299,10992,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor))&&map.AddToMap(static_cast<Creature*>(&owner)),"actual actor insertion");actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());Require(actor.GetAnyOwner()==&owner,"actual owner lookup");
RunActorQueue<KingarothDialogue::Frame>(actor,0,3000,false,1,UINT32_MAX);
RunActorQueue<KingarothDialogue::Frame>(actor,1,35000,true,1,0);
RunActorQueue<MaievCathedral::Frame>(actor,0,3000,false,2,UINT32_MAX);
RunActorQueue<MaievCathedral::Frame>(actor,1,1000,false,1,0);
RunActorQueue<MaievCathedral::Frame>(actor,2,2000,false,1,0);
RunActorQueue<MaievCathedral::Frame>(actor,3,3000,false,1,UINT32_MAX);
RunActorQueue<MaievCathedral::Frame>(actor,4,25000,false,3,1);
RunActorQueue<MaievCathedral::Frame>(actor,5,500,false,2,UINT32_MAX);
RunActorQueue<MephistrothIntro::Frame>(actor,0,5000,false,2,UINT32_MAX);
RunActorQueue<MephistrothIntro::Frame>(actor,1,18000,false,1,0);
RunActorQueue<GalindreRecovery::Frame>(actor,0,100,false,4,0);
RunActorQueue<GalindreRecovery::Frame>(actor,1,5000,false,1,UINT32_MAX);
RunActorQueue<KarazhanKhadgar::Frame>(actor,0,5000,false,2,1);
actor.SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_IN_COMBAT);
RunActorQueue<ElisandeEvade::Frame>(actor,0,60000,true,1,0);
RunActorQueue<GiantLasher::Frame>(actor,0,13000,false,1,1);
RunActorQueue<KiljaedenPresenters::Frame>(actor,0,500,false,1,UINT32_MAX);
RunActorQueue<KiljaedenPresenters::Frame>(actor,1,500,false,1,UINT32_MAX);
RunActorQueue<KiljaedenPresenters::Frame>(actor,2,500,false,2,1);
RunActorQueue<KiljaedenPresenters::Frame>(actor,3,500,false,1,UINT32_MAX);
RunActorQueue<ApocronVolley::Frame>(actor,0,2100,false,5,0);
RunActorQueue<ApocronVolley::Frame>(actor,1,3100,false,5,0);
RunActorQueue<ApocronVolley::Frame>(actor,2,4100,false,8,0);
RunActorQueue<EpicusIntro::Frame>(actor,0,1000,false,3,1);
actorQueueRandomMaximum=true;for(uint32 i=0;i<3;++i)RunActorQueue<ApocronVolley::Frame>(actor,i,ApocronVolley::delays[i],false,11);actorQueueRandomMaximum=false;
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:23 root/compiled actual event/combat Unit-owned queues after Creature::AIM_Initialize deletes original callback AI;authored delays,one-shot and real Unit cancellation. Native owner lookup,movement/unit-state changes;cast/aura/despawn endpoints observed,not full registered content/client traversal or full-core ASAN;no character writes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
