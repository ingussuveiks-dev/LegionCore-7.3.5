#include "NativeHost.inc"
#include "Policy.inc"
#include "Actor.inc"
#include "Instance.inc"
using Lady=npc_lady_zitrin::npc_lady_zitrinAI;
using Instance=instance_sharasdal_InstanceMapScript;
inline CriteriaTreeEntry ascentLeaf{47131,29795},arrivalLeaf{47378,29661},wallLeaf{47133,29662},bossLeaf{47380,29803};
struct Run {
 Player owner,neighbor;Unit pet,guardian;Creature illusion,grash,wave;CreatureAI guide,temporary;
 Instance instance{&liveMap};std::unique_ptr<Lady> ai;
 Run(){scenario=Scenario{};scenario.step=3;scenario.currentTree=47130;manager.active=&scenario;warnings=0;
  db2.rows.clear();db2.rows[47130]={&ascentLeaf,&arrivalLeaf};db2.rows[47132]={&wallLeaf,&bossLeaf};
  scenario.achievements.progress[47131].Counter=1;
  owner.guid={1,1};neighbor.guid={1,2};pet.owner=guardian.owner=owner.guid;
  activeInstance=&instance;liveMap.players={{&owner},{&neighbor}};ObjectAccessor::players={&owner,&neighbor};
  illusion.guid={2,362091};illusion.dbGuid=362091;illusion.entry=104856;illusion.wave=&wave;
  grash.guid={2,362092};grash.dbGuid=362092;grash.entry=105298;grash.primary=&guide;
  ObjectAccessor::creatures={&illusion,&grash};ai=std::make_unique<Lady>(&illusion);
  instance.OnCreatureCreate(&illusion);instance.OnCreatureCreate(&grash);
 }
 ~Run(){activeInstance=nullptr;ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();liveMap.players.clear();}
 void Reach(Unit* unit=nullptr){ai->MoveInLineOfSight_Safe(unit?unit:&owner);}
 void Tick(uint32 diff=0){illusion.Tick(diff);}
 uint64 Count(){return scenario.achievements.progress[47378].Counter;}
 void HideGrash(){ObjectAccessor::creatures={&illusion};}
 void ShowGrash(){ObjectAccessor::creatures={&illusion,&grash};instance.OnCreatureCreate(&grash);}
};
int main(){
#ifdef BASELINE
 {Run r;r.neighbor.gm=true;r.neighbor.record=false;r.Reach(&r.neighbor);assert(r.Count()==1&&scenario.step==4&&r.neighbor.credits[50314]==1&&!r.owner.credits[50314]&&r.illusion.despawns==1);}
 {Run r;scenario.step=2;scenario.currentTree=47126;r.Reach();assert(!r.Count()&&r.owner.credits[50314]==1&&r.illusion.despawns==1&&r.ai->_introDone);}
 {Run r;r.HideGrash();r.Reach();assert(r.Count()==1&&r.illusion.despawns==1&&r.guide.actions.empty());}
 {Run r;r.ai.reset();r.illusion.dbGuid=362095;r.ai=std::make_unique<Lady>(&r.illusion);r.Reach();assert(r.Count()==1&&r.illusion.despawns==1);}
 std::cout<<"PASS COUNTERFACTUAL ef0bbcbbf: native SafeLOS consumes illusion for a GM without40341; wrong stage rejected criterion still despawns actor; missing Grash loses continuation; real boss moved across X cutoff is also consumed. Expected broken behavior, not an ordinary successful scenario.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int mode=0;mode<4;++mode){Run r;r.owner.quest.Status=status;
  if(mode==1){r.grash.primary=&r.temporary;r.grash.i_disabledAI=&r.guide;}
  if(mode==2)r.grash.i_disabledAI=&r.guide;
  if(mode==3){r.grash.primary=nullptr;r.grash.i_disabledAI=&r.guide;r.grash.IsAIEnabled=false;}
  r.Reach();assert(r.Count()==1&&scenario.step==4&&r.owner.credits[50314]==1&&!r.neighbor.credits[50314]);
  assert(r.guide.actions==std::vector<int32>{3}&&r.temporary.actions.empty()&&r.illusion.motion.points==std::vector<uint32>{0}&&r.illusion.despawns==1&&r.illusion.despawnDelay==1000);
  assert(r.ai->_introDone&&!r.ai->intro&&!r.ai->_gauntletPending);r.Reach();r.Reach(&r.neighbor);r.Tick(30000);
  assert(r.owner.credits[50314]==1&&r.guide.actions.size()==1&&r.illusion.despawns==1);++cases;
 }
 for(int denied=0;denied<27;++denied){Run r;
  switch(denied){
   case 0:r.owner.world=false;break;case 1:r.owner.contact=false;break;case 2:r.owner.alive=false;break;case 3:r.owner.gm=true;break;
   case 4:r.owner.teleport=true;break;case 5:r.owner.classId=5;break;case 6:r.owner.rewarded=true;break;case 7:r.owner.record=false;break;
   case 8:r.owner.quest.Status=0;break;case 9:r.owner.map=&otherMap;break;case 10:r.owner.phase=false;break;case 11:r.owner.detect=false;break;
   case 12:r.owner.los=false;break;case 13:r.owner.near=false;break;case 14:r.illusion.world=false;break;case 15:r.illusion.preDelete=true;break;
   case 16:r.illusion.alive=false;break;case 17:r.illusion.mapId=1;break;case 18:r.illusion.entry=999;break;case 19:r.illusion.dbGuid=362095;break;
   case 20:scenario.id=99;break;case 21:scenario.step=2;break;case 22:scenario.step=4;break;case 23:manager.active=nullptr;break;
   case 24:scenario.achievements.progress[47131].Counter=0;break;case 25:r.illusion.map=&otherMap;break;case 26:r.owner.quest.Status=6;break;
  }
  r.Reach();r.Tick(10000);assert(!r.Count()&&!r.owner.credits[50314]&&!r.neighbor.credits[50314]&&!r.illusion.despawns&&r.guide.actions.empty());manager.active=&scenario;++cases;
 }
 {Run r;r.ai->MoveInLineOfSight_Safe(nullptr);r.Reach(&r.pet);r.Reach(&r.guardian);r.Reach(&r.grash);assert(!r.Count()&&!r.illusion.despawns);++cases;}
 // The boss cannot become the gauntlet illusion by moving across X=-15.
 {Run r;r.ai.reset();r.illusion.dbGuid=362095;r.ai=std::make_unique<Lady>(&r.illusion);r.Reach();r.ai->UpdateAI(6000);assert(!r.ai->intro&&!r.Count()&&!r.illusion.summons&&!r.illusion.despawns);++cases;}
 // A genuine end arrival may wait for the native criterion. Do not stop its
 // waves, despawn, change route, choose another recipient or spam per frame.
 for(int mode=0;mode<4;++mode){Run r;scenario.rejected.insert(50314);r.Reach();assert(!r.Count()&&r.ai->_gauntletPending&&r.ai->intro&&!r.ai->_introDone&&!r.illusion.despawns);
  if(mode==1){r.illusion.primary=&r.temporary;r.illusion.i_disabledAI=r.ai.get();}
  if(mode==2)r.illusion.i_disabledAI=r.ai.get();
  if(mode==3){r.illusion.primary=nullptr;r.illusion.i_disabledAI=r.ai.get();r.illusion.IsAIEnabled=false;}
  r.Tick(999);assert(r.owner.credits[50314]==1);r.Reach(&r.neighbor);scenario.rejected.clear();r.Tick(1);
  assert(r.Count()==1&&r.owner.credits[50314]==2&&!r.neighbor.credits[50314]&&r.guide.actions.size()==1&&r.illusion.despawns==1);++cases;
 }
 for(int missing=0;missing<7;++missing){Run r;switch(missing){case 0:r.HideGrash();break;case 1:r.grash.dbGuid=362066;break;case 2:r.grash.phase=false;break;case 3:r.grash.alive=false;break;case 4:r.grash.preDelete=true;break;case 5:r.grash.primary=nullptr;break;case 6:r.grash.map=&otherMap;break;}
  r.Reach();assert(r.Count()==1&&r.ai->_gauntletEarned&&r.ai->_gauntletPending&&!r.ai->intro&&!r.illusion.despawns&&r.guide.actions.empty());
  r.grash.dbGuid=362092;r.grash.phase=true;r.grash.alive=true;r.grash.preDelete=false;r.grash.primary=&r.guide;r.grash.map=&liveMap;r.ShowGrash();r.Tick(0);
  assert(r.Count()==1&&r.owner.credits[50314]==1&&r.guide.actions==std::vector<int32>{3}&&r.illusion.despawns==1);++cases;
 }
 {Run r;r.HideGrash();r.Reach();for(int i=0;i<40;++i)r.Tick(1000);assert(warnings==1&&r.ai->_gauntletExhausted&&!r.illusion.despawns&&r.owner.credits[50314]==1);
  r.instance.OnPlayerEnter(&r.neighbor);assert(r.ai->_gauntletExhausted);r.ai->SetGUID(r.owner.guid,0);assert(r.ai->_gauntletExhausted);
  r.owner.world=false;r.owner.record=false;r.owner.quest.ScriptLifetime.reset();r.ShowGrash();r.Tick(0);
  assert(r.Count()==1&&r.owner.credits[50314]==1&&r.guide.actions.size()==1&&r.illusion.despawns==1);++cases;
 }
 for(int mode=0;mode<4;++mode){Run r;scenario.rejected.insert(50314);r.Reach();for(int i=0;i<40;++i)r.Tick(1000);assert(warnings==1&&r.ai->_gauntletExhausted&&r.owner.credits[50314]==30);
  if(mode==1){r.illusion.primary=&r.temporary;r.illusion.i_disabledAI=r.ai.get();}
  if(mode==2)r.illusion.i_disabledAI=r.ai.get();
  if(mode==3){r.illusion.primary=nullptr;r.illusion.i_disabledAI=r.ai.get();r.illusion.IsAIEnabled=false;}
  r.instance.RearmGauntlet(r.grash.guid,105298);assert(r.ai->_gauntletExhausted);r.instance.OnPlayerEnter(&r.neighbor);assert(r.ai->_gauntletExhausted);
  scenario.rejected.clear();r.instance.OnPlayerEnter(&r.owner);r.Tick(0);assert(r.Count()==1&&r.owner.credits[50314]==31&&r.guide.actions.size()==1);++cases;
 }
 for(int changed=0;changed<6;++changed){Run r;scenario.rejected.insert(50314);r.Reach();auto held=r.owner.quest.ScriptLifetime;
  switch(changed){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.rewarded=true;break;case 3:r.owner.quest.Status=0;break;case 4:r.owner.quest.ScriptLifetime.reset();break;case 5:scenario.step=2;break;}
  scenario.rejected.clear();r.Tick(1000);assert(!r.Count()&&!r.ai->_gauntletPending&&!r.illusion.despawns&&r.guide.actions.empty());++cases;
 }
 {Run r;scenario.rejected.insert(50314);r.Reach();r.owner.near=false;r.Tick(1000);assert(r.owner.credits[50314]==1&&!r.Count());r.owner.near=true;scenario.rejected.clear();r.Tick(1000);assert(r.Count()==1);++cases;}
 {Run r;scenario.rejected.insert(50314);r.Reach();for(int i=0;i<40;++i)r.Tick(1000);r.owner.near=false;r.Tick(0);r.owner.near=true;scenario.rejected.clear();r.Tick(0);assert(r.Count()==1&&r.owner.credits[50314]==31);++cases;}
 {Run r;r.owner.onCredit=[&](uint32 asset){assert(asset==50314);r.ai->MoveInLineOfSight(&r.owner);r.Reach();r.Tick(0);};r.guide.onAction=[&](int32 action){assert(action==3);r.Reach();r.Tick(0);};r.Reach();assert(r.Count()==1&&r.owner.credits[50314]==1&&r.guide.actions.size()==1&&r.illusion.despawns==1);++cases;}
 {Run r;scenario.rejected.insert(50314);r.Reach();r.ai->Reset();scenario.rejected.clear();r.Tick(1000);assert(r.Count()==1&&r.guide.actions.size()==1);++cases;}
 {Run r;r.HideGrash();r.Reach();r.ai->Reset();r.owner.world=false;r.owner.quest.ScriptLifetime.reset();r.ShowGrash();r.Tick(0);assert(r.Count()==1&&r.guide.actions.size()==1&&r.owner.credits[50314]==1);++cases;}
 // An externally accepted real native receipt can transition between retries;
 // the already bound arrival then finishes only NPC presentation, not credit.
 {Run r;scenario.rejected.insert(50314);r.Reach();scenario.achievements.progress[47378].Counter=1;scenario.step=4;scenario.currentTree=47132;r.Tick(1000);assert(r.guide.actions.size()==1&&r.owner.credits[50314]==1);++cases;}
 {Run r;r.HideGrash();r.Reach();scenario.step=5;r.Tick(1000);assert(!r.ai->_gauntletPending&&r.illusion.despawns==1&&r.guide.actions.empty()&&r.owner.credits[50314]==1);++cases;}
 {Run r;scenario.rejected.insert(50314);r.Reach();r.ai.reset();r.illusion.primary=nullptr;r.illusion.IsAIEnabled=false;r.Tick(30000);assert(!r.Count()&&r.guide.actions.empty());++cases;}
 // Retain the native2s initial/6s recurring wave payload, but gate it to the
 // actual illusion and actual1066 step3. No wave spawn supplies progression.
 {Run r;r.ai->UpdateAI(1999);assert(!r.illusion.summons);r.ai->UpdateAI(1);assert(r.illusion.summons==1&&r.wave.motion.points==std::vector<uint32>{1}&&r.wave.casts==std::vector<uint32>{209132}&&!r.Count());r.ai->UpdateAI(5999);assert(r.illusion.summons==1);r.ai->UpdateAI(1);assert(r.illusion.summons==2);r.Reach();r.ai->UpdateAI(6000);assert(r.illusion.summons==2);++cases;}
 for(int denied=0;denied<5;++denied){Run r;switch(denied){case 0:scenario.step=2;break;case 1:scenario.step=4;break;case 2:scenario.id=7;break;case 3:r.illusion.world=false;break;case 4:r.illusion.dbGuid=362095;break;}r.ai->UpdateAI(20000);assert(!r.illusion.summons&&!r.Count());++cases;}
 std::cout<<"PASS "<<cases<<" actual complete Lady/illusion and instance/SafeLOS/quest/lifecycle/Scenario gate cases under ASAN: native ascent+arrival receipt, original caller, finite refusal/missing-guide recovery, original suspended Grash once, stage handoff and unchanged wave payload. Registry/range/criteria acceptance/movement/network services controlled; no client/full1066 claim.\n";
#endif
}
