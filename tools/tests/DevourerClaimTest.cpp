// Real callback/storage fragments, actual native lookup and quest receipts.
// Inventory/equip/spell completion services are controlled, not client traversal.
#define DEVOURER_PICKUP_SUPPORT_ONLY
#include "DevourerPickupTest.cpp"
int main(){
#ifdef CLAIM_BASELINE
 for(uint32 mode=0;mode<4;++mode){Fixture f;f.p.owned=true;ObjectGuid request=f.p.guid;if(mode==0)request.high=1;
  assert(ObjectAccessor::GetPlayer(f.go,request)==&f.p);f.ai->SetGUID(request,0);assert(f.p.casts==1);
  if(mode==1)f.p.guid.high=1;if(mode==2){f.p.Abandon();f.p.status=3;}if(mode==3)f.p.scope=std::make_shared<QuestObjectScope>();
  f.Finish();assert(f.p.events==1&&manager.scenario.step==7);
 }
 std::cout<<"COUNTERFACTUAL4 pinned d8973a49b claims: actual native lower32 lookup accepts wrong fullGUID; pending identity/cancel-reaccept/receipt replacement still sends50301. Expected broken, controlled equip and earlier stage.\n";
#else
 uint32 cases=0;
 for(uint32 mode=0;mode<3;++mode){Fixture f;f.p.owned=true;auto request=f.p.guid;if(mode==0)request.high=1;if(mode==1)request.value|=uint64_t(1)<<32;if(mode==2){request.high=2;request.value|=uint64_t(2)<<32;}assert(ObjectAccessor::GetPlayer(f.go,request)==&f.p);f.ai->SetGUID(request,0);f.Finish();assert(!f.p.casts&&!f.p.events&&f.ai->GetGUID(0).IsEmpty());++cases;}
 for(uint32 mode=0;mode<9;++mode){Fixture f;f.LootWeapon();auto receipt=f.p.GetQuestCancellationReceipt(41924);Player replacement;replacement.guid={1,3};replacement.owned=replacement.paired=true;
  if(mode==0)f.p.guid.high=1;if(mode==1)f.p.guid.value|=uint64_t(1)<<32;if(mode==2){f.p.Abandon();f.p.status=3;assert(receipt->load());}if(mode==3)f.p.scope=std::make_shared<QuestObjectScope>();if(mode==4)players[1]=&replacement;if(mode==5)f.p.preDelete=true;if(mode==6)f.p.world=false;if(mode==7)f.p.map=&otherMap;if(mode==8)players.clear();
  f.Finish();assert(!f.p.events&&!replacement.events&&f.ai->GetGUID(0).IsEmpty()&&f.go.getLootState()==GO_READY&&f.p.owned);++cases;
 }
 for(uint32 mode=0;mode<9;++mode){Fixture f;f.p.owned=true;f.p.castHook=[&]{if(mode==0){f.p.Abandon();f.p.status=3;}if(mode==1)f.p.guid.high=1;if(mode==2)f.p.scope=std::make_shared<QuestObjectScope>();if(mode==3)players.clear();if(mode==4)f.p.preDelete=true;if(mode==5)f.p.world=false;if(mode==6)f.p.map=&otherMap;if(mode==7)f.p.owned=false;if(mode==8)f.p.distance=6;};f.ai->SetGUID(f.p.guid,0);assert(f.p.casts==1&&!f.p.events&&f.ai->GetGUID(0).IsEmpty());++cases;}
 for(bool failed:{false,true}){Fixture f;f.p.owned=f.p.paired=true;f.p.castResult=failed?1:0;f.p.castHook=[&]{f.ai->UpdateAI(15000);f.ai->SetGUID(f.p.guid,0);assert(!f.p.events);};f.ai->SetGUID(f.p.guid,0);assert(f.p.casts==1&&!f.p.events);f.p.castHook={};f.p.castResult=0;if(failed)f.ai->SetGUID(f.p.guid,0);f.Finish();assert(f.p.events==1&&!f.p.grants&&manager.scenario.step==7);f.Finish();assert(f.p.events==1);++cases;}
 {Fixture f;f.LootWeapon();f.p.Abandon();f.p.status=3;f.Finish();assert(!f.p.events&&f.ai->GetGUID(0).IsEmpty());assert(f.script.OnGossipHello(&f.p,&f.go));f.Finish();assert(f.p.events==1&&f.p.casts==2&&!f.p.grants);++cases;}
 {Fixture f;f.p.status=1;f.LootWeapon();f.Finish();assert(f.p.events==1&&!f.p.grants);++cases;}
 std::cout<<"PASS "<<cases<<" claim identity/attempt/reentry contexts using actual native HashMapHolder/ObjectAccessor and QuestCancellationReceipt: full128 identity before cast and delayed50301, cancel/reaccept/current receipt, post-cast refresh, blocked synchronous completion/reentry, explicit fresh retry, active COMPLETE, exactly once/no grant. Inventory/cast/equip controlled; native and client separate.\n";
#endif
}
