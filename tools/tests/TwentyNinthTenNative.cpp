#include "ActorQueueLifetimeNative.h"
#include "TwentyNinthNativeScripts.inc"
#include "HasabelWaveNativeScripts.inc"
using namespace TwentyNinthNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999788,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,10991,{52,803,805,0}),owner(&map,299,10992,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor))&&map.AddToMap(static_cast<Creature*>(&owner)),"actual actor insertion");actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());Require(actor.GetAnyOwner()==&owner,"actual owner lookup");
RunActorQueue<KaelthasIntro::Frame>(actor,0,3000,false,1,0);
RunActorQueue<KaelthasIntro::Frame>(actor,1,10000,false,1,0);
RunActorQueue<KaelthasIntro::Frame>(actor,2,18000,false,1,UINT32_MAX);
RunActorQueue<DomatraxIntro::Frame>(actor,0,1000,false,1,0);
RunActorQueue<DomatraxIntro::Frame>(actor,1,3000,false,2,UINT32_MAX);
RunActorQueue<DrokDialogue::Frame>(actor,0,2500,false,1,0);
RunActorQueue<DrokDialogue::Frame>(actor,1,6000,false,1,0);
RunActorQueue<InquisitionDialogue::Frame>(actor,0,6500,false,1,0);
RunActorQueue<InquisitionDialogue::Frame>(actor,1,5000,false,0,UINT32_MAX);
RunActorQueue<MaidenEnrage::Frame>(actor,0,480000,false,1,0);
RunActorQueue<MaidenEnrage::Frame>(actor,1,8000,false,1,0);
RunActorQueue<MaidenEnrage::Frame>(actor,2,3000,false,1,UINT32_MAX);
RunActorQueue<SaprishTrap::Frame>(actor,0,1500,false,1,0);
RunActorQueue<NezharTentacle::Frame>(actor,0,500,false,1,0);
RunActorQueue<EvalaVisibility::Frame>(actor,0,8000,false,1,UINT32_MAX);
RunActorQueue<EvalaVisibility::Frame>(actor,1,500,true,1,0);
RunActorQueue<EvalaVisibility::Frame>(actor,2,500,true,1,UINT32_MAX,&owner);
RunActorQueue<EvalaVisibility::Frame>(actor,3,10,false,1,UINT32_MAX);
RunActorQueue<DaglopRide::Frame>(actor,0,1000,false,2,1);
RunActorQueue<DaglopRide::Frame>(actor,1,2000,false,1,UINT32_MAX);
RunActorQueue<DaglopRide::Frame>(actor,2,1500,false,1,0);
RunActorQueue<ChengChallenge::Frame>(actor,0,2000,false,3,2);
RunActorQueue<ChengChallenge::Frame>(actor,1,1000,false,1,0);
RunActorQueue<ChengChallenge::Frame>(actor,2,8000,false,1,UINT32_MAX);
RunActorQueue<ChengChallenge::Frame>(actor,3,10000,false,1,UINT32_MAX);
RunActorQueue<HasabelWaveNative::HasabelWave::Frame>(actor,0,123,true,1);
Observe::Reset();actorQueueSelectedGuid=owner.GetGUID();auto burst=new InquisitionDialogue::Frame(&actor);Require(actor.AIM_Initialize(burst),"burst owner");burst->Queue(1);Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"burst AI retired");PumpQueue(actor,5000);Require(!Observe::calls,"outer burst schedules only");PumpQueue(actor,2000);Require(Observe::calls>0&&Observe::actor==&actor,"actual nested unit queues use actor after AI deletion");auto burstCount=Observe::calls;PumpQueue(actor,60000);Require(Observe::calls==burstCount,"burst finite");actorQueueSelectedGuid=ObjectGuid::Empty;
Observe::Reset();auto lost=new EvalaVisibility::Frame(&actor);Require(actor.AIM_Initialize(lost),"missing target producer");lost->Queue(2);Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"retired target producer");map.RemoveFromMap(static_cast<Creature*>(&owner),false);PumpQueue(actor,500,true);Require(!Observe::calls,"removed target GUID cannot spawn tomb");owner.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&owner)),"target reentry");auto dead=new EvalaVisibility::Frame(&actor);Require(actor.AIM_Initialize(dead),"dead target producer");dead->Queue(2);Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"retired dead producer");owner.Unit::setDeathState(CORPSE);PumpQueue(actor,500,true);Require(!Observe::calls,"dead native target cannot spawn tomb");owner.Unit::setDeathState(ALIVE);owner.SetHealth(owner.GetMaxHealth());
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:25 primary plus1 supplemental event/combat Unit-owned queues after Creature::AIM_Initialize deletes original callback AI;authored delays,one-shot and real Unit cancellation. Native owner lookup,movement/unit-state changes;cast/aura/despawn endpoints observed,not full registered content/client traversal or full-core ASAN;no character writes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
