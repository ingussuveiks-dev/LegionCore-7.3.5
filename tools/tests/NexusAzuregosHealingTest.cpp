// Reuse the exact production dispatcher/condition/timer/action host. Additional
// production52/44/HEALTH_PCT branches retain real kill and healing prerequisites.
#define NEXUS_HEALING_TEST
#define main NexusMageReceiverNotRun
#include "NexusAzuregosReceiverTest.cpp"
#undef main
struct HealingRun {
 Unit source,mobs[3],player,pet,guardian;SmartScript receiver,senders[3];std::vector<Condition>rules;
 HealingRun(SmartAIEventList const&rows,std::vector<std::vector<uint32>>const&cs){
  grid.clear();receipts.clear();clockTime=scriptErrors=0;nativeConditions={};hooks={};healingScenario={};healingScenarioManager={};acceptCriteria=true;failCast=false;world={};otherWorld.id=2;scriptMgr.rows=rows;
  source.entry=104832;source.health=1;source.ai.script=&receiver;receiver.me=&source;receiver.mEvents=scriptMgr.GetScript(104832,0);
  for(uint32 i=0;i<3;++i){mobs[i].entry=105799+i;mobs[i].ai.script=&senders[i];senders[i].me=&mobs[i];senders[i].mEvents=scriptMgr.GetScript(mobs[i].entry,0);grid.push_back(&mobs[i]);}grid.push_back(&source);player.player=true;pet.owner=guardian.owner=&player;
  for(auto const&r:cs){assert(r.size()==8);Condition c;c.SourceEntry=104832;c.SourceGroup=r[0];c.ElseGroup=r[1];c.ConditionType=r[2];c.ConditionTarget=r[3];c.ConditionValue1=r[4];c.ConditionValue2=r[5];c.ConditionValue3=r[6];c.NegativeCondition=r[7]!=0;rules.push_back(c);}
  for(auto&c:rules)nativeConditions.SmartEventConditionStore[{104832,0}][c.SourceGroup].push_back(&c);
 }
 void Die(uint32 i,Unit*k){if(!mobs[i].alive)return;mobs[i].alive=false;senders[i].ProcessEventsFor(SMART_EVENT_DEATH,k);}
 void Tick(uint32 diff=100){clockTime+=diff;for(auto&e:receiver.mEvents)receiver.UpdateTimer(e,diff);receiver.Tick(diff);}
 void Finish(){for(uint32 i=0;i<600;++i)Tick();}
 bool Started(){return !receiver.mTimedActionList.empty()||source.auraRemovals;}
};
int main(int argc,char**argv){
 assert(argc==6);auto old=Rows(argv[1]),fixed=Rows(argv[2]);auto originalConditions=Read(argv[3]),fixedConditions=Read(argv[4]);assert(originalConditions.size()==3&&fixedConditions.size()==5);auto initialHealth=Read(argv[5]);assert(initialHealth==std::vector<std::vector<uint32>>({{104832,1,0}}));
 // Original null-invoker precheck loses Scion credit; original independent
 // full-health event can start the guide before those enemies are defeated.
 {HealingRun r(old,originalConditions);for(uint32 i=0;i<3;++i)r.Die(i,&r.player);assert(receipts.empty());}
 {HealingRun r(old,originalConditions);r.source.health=100;r.Tick();assert(!healingScenario.achievement.scions&&healingScenario.healed&&r.Started());}
 // Merely fixing the three targets exposes the malformed call to HEALTH_PCT.
 {HealingRun r(fixed,fixedConditions);for(auto&e:r.receiver.mEvents)if(e.event_id==0)e.link=3;for(uint32 i=0;i<3;++i)r.Die(i,&r.player);assert(scriptErrors==1&&!healingScenario.healed&&r.source.health==1);}
 // Converting that health root to LINK61 would falsely grant healing at1HP.
 {HealingRun r(fixed,fixedConditions);for(auto&e:r.receiver.mEvents){if(e.event_id==0)e.link=3;if(e.event_id==3)e.event.type=SMART_EVENT_LINK;}for(uint32 i=0;i<3;++i)r.Die(i,&r.player);assert(!scriptErrors&&healingScenario.healed&&r.source.health==1&&r.Started());}
 uint32 runs=0;std::vector<uint32>order{0,1,2};do{for(uint32 kind=0;kind<4;++kind)for(bool earlyHealing:{false,true}){
  HealingRun r(fixed,fixedConditions);Unit*k=kind==0?&r.player:kind==1?&r.pet:kind==2?&r.guardian:nullptr;
  if(earlyHealing)r.source.health=100;r.Tick();assert(!r.Started()&&!healingScenario.healed);
  for(uint32 i=0;i<3;++i){r.Die(order[i],k);r.Tick();if(i<2)assert(!r.Started()&&!healingScenario.achievement.scions);else assert(healingScenario.achievement.scions);}
  if(!earlyHealing){assert(!r.Started()&&!healingScenario.healed);r.source.health=98;r.Tick();assert(!r.Started());r.source.health=99;r.Tick();}
  assert(healingScenario.healed&&r.source.auraRemovals==1&&!scriptErrors);r.source.ai.SetData(1,1);r.Finish();
  std::vector<std::pair<uint32,uint32>>actual;for(auto const&x:receipts)actual.push_back({x.action,x.payload});
  assert((actual==std::vector<std::pair<uint32,uint32>>{{205,49864},{205,50369},{206,1681},{206,1682},{206,1683},{206,1684},{11,207752},{12,104902},{205,50369},{41,0}}));
  ++runs;
 }}while(std::next_permutation(order.begin(),order.end()));
 // Accepted death dispatch is not equivalent to real criteria storage.
 {HealingRun r(fixed,fixedConditions);acceptCriteria=false;r.source.health=100;for(uint32 i=0;i<3;++i)r.Die(i,&r.pet);r.Tick();assert(!r.Started()&&!healingScenario.healed);healingScenario.achievement.scions=true;acceptCriteria=true;r.Tick();assert(r.Started()&&healingScenario.healed);}
 for(uint32 mode=0;mode<7;++mode){HealingRun r(fixed,fixedConditions);healingScenario.achievement.scions=true;r.source.health=100;if(mode==0)healingScenario.id=1101;if(mode==1)healingScenario.step=1;if(mode==2)healingScenarioManager.present=false;if(mode==3)r.source.map=&otherWorld;if(mode==4)r.source.map=nullptr;if(mode==5)hooks.allow=false;if(mode==6)r.source.maxHealth=0;r.Tick();assert(!r.Started()&&!healingScenario.healed);}
 // Recreated/eligible source may retry from existing real Scion progress, but
 // a completed step cannot replay its healing finale on fresh AI state.
 {HealingRun r(fixed,fixedConditions);healingScenario.achievement.scions=true;r.Tick();assert(!r.Started());r.source.health=100;r.Tick();assert(r.Started());}
 std::cout<<"PASS native Priest1065 health/data/criteria-tree/step admission: "<<runs<<" kill-order/killer/heal-order sequences, original null-invoker and early-heal counterfactuals, malformed LINK and unsafe LINK61 counterfactuals, delayed real criteria acknowledgment, actual99..100 threshold, missing/wrong/completed context and once-only original8-entry guide list. Damage/heal, allocation, criteria/conversation/guide transport endpoints controlled; no full client claim.\n";
}
