#include "ActorQueueLifetimeNative.h"
#include "ThirtyFirstNativeScripts.inc"
#include "CoreDelayTalkNative.h"
using namespace ThirtyFirstNative;
int main(int argc,char**argv){try{InitializeAltheaGuardsNative(argc,argv);dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});MapInstanced parent(1600,300);InstanceMap map(1600,300,9999788,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,10991,{52,803,805,0}),owner(&map,299,10992,{53,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor))&&map.AddToMap(static_cast<Creature*>(&owner)),"actual actor insertion");actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());Require(actor.GetAnyOwner()==&owner,"actual owner lookup");
RunActorQueue<GGEngineering::Frame>(actor,0,700,false,1,0);
RunActorQueue<GGEngineering::Frame>(actor,1,700,false,3,UINT32_MAX);
RunActorQueue<TheramoreGuard::Frame>(actor,0,3000,false,1,0);
RunActorQueue<TheramoreGuard::Frame>(actor,1,4000,false,2,1);
RunActorQueue<TheramoreGuard::Frame>(actor,2,11500,false,1,0);
RunActorQueue<TheramoreGuard::Frame>(actor,3,14500,false,3,1);
RunActorQueue<NezharVoidtender::Frame>(actor,0,1500,false,1,0);
RunActorQueue<SaprishCompanions::Frame>(actor,0,500,true,2,UINT32_MAX);
RunActorQueue<SaprishCompanions::Frame>(actor,1,500,true,2,UINT32_MAX);
RunActorQueue<SaprishCompanions::Frame>(actor,2,1500,false,1,0);
RunActorQueue<BankerRift::Frame>(actor,0,2000,false,1,UINT32_MAX);
RunActorQueue<NightholdTrash::Frame>(actor,0,1500,false,1,UINT32_MAX);
RunActorQueue<NightholdTrash::Frame>(actor,1,1500,false,1,UINT32_MAX);
RunActorQueue<NightholdTrash::Frame>(actor,2,1500,false,1,UINT32_MAX);
RunActorQueue<LuraPortal::Frame>(actor,0,1000,false,1,UINT32_MAX);
RunActorQueue<LuraPortal::Frame>(actor,1,7000,false,1,UINT32_MAX);
RunActorQueue<NezharPortal::Frame>(actor,0,500,false,2,UINT32_MAX);
RunActorQueue<NezharPortal::Frame>(actor,1,2000,false,1,UINT32_MAX);
RunActorQueue<SottWalkerAfterNezhar::Frame>(actor,0,2000,false,1,UINT32_MAX);
RunActorQueue<SottWalkerAfterNezhar::Frame>(actor,1,12000,false,1,0);
RunActorQueue<SottWalkerAfterNezhar::Frame>(actor,2,60000,false,1,UINT32_MAX);
RunActorQueue<CoreDelayTalk::Frame>(actor,0,1000,false,1,UINT32_MAX);
Observe::queryPresent=false;RunActorQueue<NezharVoidtender::Frame>(actor,0,1500,false,0);RunActorQueue<LuraPortal::Frame>(actor,1,7000,false,0);RunActorQueue<SottWalkerAfterNezhar::Frame>(actor,1,12000,false,0);Observe::queryPresent=true;
for(uint32 index:{0u,1u}){actorQueueSelectedGuid=owner.GetGUID();owner.SetMaxHealth(5000000123ULL);owner.SetHealth(owner.GetMaxHealth());RunActorQueue<SaprishCompanions::Frame>(actor,index,500,true,2);Require(actor.GetMaxHealth()==owner.GetMaxHealth()&&actor.GetHealth()==owner.GetMaxHealth(),"actual uint64 health synchronized");Observe::Reset();auto f=new SaprishCompanions::Frame(&actor);Require(actor.AIM_Initialize(f),"removed boss producer");f->Queue(index);Require(actor.AIM_Initialize(new NullCreatureAI(&actor)),"retired removed boss producer");map.RemoveFromMap(static_cast<Creature*>(&owner),false);PumpQueue(actor,500,true);Require(!Observe::calls,"removed boss GUID rejects health sync");owner.SetMap(&map);Require(map.AddToMap(static_cast<Creature*>(&owner)),"boss return");owner.Unit::setDeathState(CORPSE);RunActorQueue<SaprishCompanions::Frame>(actor,index,500,true,0);owner.Unit::setDeathState(ALIVE);owner.SetHealth(owner.GetMaxHealth());actorQueueSelectedGuid=ObjectGuid::Empty;}
actor.SetGuidValue(UNIT_FIELD_CREATED_BY,ObjectGuid::Empty);RunActorQueue<GGEngineering::Frame>(actor,0,700,false,0);actor.SetGuidValue(UNIT_FIELD_CREATED_BY,owner.GetGUID());
VerifyActualDelayTalk(actor,owner.GetGUID());
 CloseAltheaNative();std::cout<<"PASS native ten selected cases:22 root/compiled actual event/combat Unit-owned queues after Creature::AIM_Initialize deletes original callback AI;authored delays,one-shot and real Unit cancellation. Native owner lookup,movement/unit-state changes;cast/aura/despawn endpoints observed,not full registered content/client traversal or full-core ASAN;no character writes\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<std::endl;return 1;}}
