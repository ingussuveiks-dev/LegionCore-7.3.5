#include "NativeHost.inc"
#include "Policy.inc"
#include "Actor.inc"
#include "Instance.inc"
using Instance=instance_sharasdal_InstanceMapScript;
using Lady=npc_lady_zitrin::npc_lady_zitrinAI;
inline CriteriaTreeEntry wallLeaf{47133,29662},bossLeaf{47380,29803};
struct Run{
 Player owner,neighbor;Unit pet,guardian,charmed,unowned;Creature boss,erunak,grash,adel;
 Instance instance{&liveMap};std::unique_ptr<Lady> ai;
 Run(){mapConversations.clear();conversationAttempts=conversationCreates=conversationDeletes=0;scenario=Scenario{};scenario.step=4;scenario.currentTree=47132;manager.active=&scenario;warnings=0;
  db2.rows.clear();db2.rows[47132]={&wallLeaf,&bossLeaf};db2.rows[47134]={};
  activeInstance=&instance;owner.guid={1,1};neighbor.guid={1,2};neighbor.gm=true;
  pet.owner=guardian.owner=owner.guid;charmed.charmer=owner.guid;
  liveMap.players={{&owner},{&neighbor}};ObjectAccessor::players={&owner,&neighbor};
  boss.guid={2,362095};boss.dbGuid=362095;boss.entry=104856;boss.nearest=&neighbor;
  erunak.guid={2,362093};erunak.dbGuid=362093;erunak.entry=102826;
  grash.guid={2,362092};grash.dbGuid=362092;grash.entry=105298;
  adel.guid={2,362094};adel.dbGuid=362094;adel.entry=104853;
  ObjectAccessor::creatures={&boss,&erunak,&grash,&adel};ai=std::make_unique<Lady>(&boss);
 }
 ~Run(){activeInstance=nullptr;ObjectAccessor::players.clear();ObjectAccessor::creatures.clear();liveMap.players.clear();}
 Unit* Killer(int source){Unit* values[]={&owner,&pet,&guardian,&charmed,&erunak,&grash,&adel};return values[source];}
 void Kill(Unit* killer){boss.alive=false;if(killer)killer->NativeKillDispatch(&boss);else instance.CreatureDies(&boss,nullptr);ai->JustDied(killer);}
 void Tick(uint32 diff=0){instance.Update(diff);}
 uint64 Count(){return scenario.achievements.progress[47380].Counter;}
 void Wall(){owner.UpdateAchievementCriteria(92,49819);}
};
int main(){
#ifdef BASELINE
 {Run r;r.Wall();r.Kill(&r.pet);assert(r.Count()==1&&scenario.step==5&&r.neighbor.credits[50397]==1&&!r.owner.credits[50397]&&conversationCreates==1&&conversationCaster==r.pet.guid);}
 {Run r;r.owner.alive=false;r.neighbor.world=false;r.boss.nearest=nullptr;r.Kill(&r.pet);assert(!r.Count()&&!r.owner.credits[50397]&&conversationCaster==r.pet.guid);}
 {Run r;r.boss.dbGuid=362091;r.Kill(&r.owner);assert(r.Count()==1&&r.neighbor.credits[50397]==1);}
 std::cout<<"PASS COUNTERFACTUALd7610f9d1: actual old Lady callback credits nearest GM spectator for pet kill, loses dead-owner victory, credits early illusion as final boss and starts post-pickup1923 from pet. Unguarded null method call is retained in source but deliberately not invoked; no fabricated crash/client claim.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int source=0;source<7;++source)for(bool wallFirst:{false,true}){
  Run r;r.owner.quest.Status=status;if(wallFirst)r.Wall();r.Kill(r.Killer(source));
  assert(r.Count()==1&&r.owner.credits[50397]==1&&!r.neighbor.credits[50397]&&!r.instance.ladyPending&&conversationCreates==0);
  if(!wallFirst){assert(scenario.step==4&&!scenario.achievements.progress[47133].Counter);r.Wall();}
  assert(scenario.step==5&&scenario.currentTree==47134);r.instance.CreatureDies(&r.boss,r.Killer(source));r.Tick(10000);
  assert(r.owner.credits[50397]==1&&r.Count()==1);++cases;
 }
 for(int denied=0;denied<23;++denied){Run r;Unit* killer=&r.pet;
  switch(denied){
   case 0:r.boss.dbGuid=362091;break;case 1:r.boss.entry=102839;break;
   case 2:r.boss.map=&otherMap;break;case 3:r.boss.world=false;break;case 4:r.boss.preDelete=true;break;
   case 5:scenario.id=7;break;case 6:scenario.step=5;break;case 7:r.pet.map=&otherMap;break;
   case 8:r.pet.phase=false;break;case 9:r.owner.world=false;break;case 10:r.owner.contact=false;break;
   case 11:r.owner.gm=true;break;case 12:r.owner.teleport=true;break;case 13:r.owner.map=&otherMap;break;
   case 14:r.owner.classId=5;break;case 15:r.owner.rewarded=true;break;case 16:r.owner.record=false;break;
   case 17:r.owner.phase=false;break;case 18:r.owner.near=false;break;case 19:killer=nullptr;break;
   case 20:killer=&r.unowned;break;case 21:r.pet.owner={1,99};break;case 22:killer=&r.adel;r.adel.dbGuid=362078;break;
  }
  r.Kill(killer);r.Tick(10000);assert(!r.Count()&&!r.owner.credits[50397]&&!r.neighbor.credits[50397]);++cases;
 }
 // Native Kill may already be resolving the last blow while a source dies or
 // despawns. Retain its original core owner/known-ally rules; do not confuse
 // source lifetime with the eligible participant's quest/world admission.
 for(int state=0;state<3;++state){Run r;Unit* killer=&r.pet;
  if(state==0)r.pet.world=false;
  if(state==1){killer=&r.adel;r.adel.preDelete=true;}
  if(state==2){killer=&r.grash;r.grash.world=false;}
  r.Kill(killer);assert(r.Count()==1&&r.owner.credits[50397]==1);++cases;
 }
 {Run r;r.boss.alive=true;r.instance.CreatureDies(&r.boss,&r.owner);r.Tick(10000);assert(!r.Count()&&!r.instance.ladySeen);}
 {Run r;Creature illusion;illusion.entry=104856;illusion.dbGuid=362091;illusion.alive=false;r.pet.NativeKillDispatch(&illusion);
  assert(!r.Count()&&!r.instance.ladySeen);r.Kill(&r.pet);assert(r.Count()==1&&r.owner.credits[50397]==1&&conversationCreates==0);
 }
 for(int lower=0;lower<3;++lower){Run r;Unit* killer=lower==0?&r.erunak:lower==1?&r.grash:&r.adel;
  if(lower==0)r.erunak.dbGuid=362063;if(lower==1)r.grash.dbGuid=362066;if(lower==2)r.adel.dbGuid=362078;
  r.Kill(killer);assert(!r.Count()&&warnings==1&&conversationCreates==0);++cases;
 }
 {Run r;r.owner.alive=false;r.neighbor.world=false;liveMap.players={{&r.owner}};r.boss.nearest=nullptr;r.Kill(&r.guardian);assert(r.Count()==1&&r.owner.credits[50397]==1);}
 {Run r;r.Kill(nullptr);r.instance.CreatureDies(&r.boss,&r.owner);assert(!r.Count()&&r.instance.ladySeen&&warnings==1);}
 {Run r;r.neighbor.gm=false;r.Kill(&r.adel);assert(!r.Count()&&warnings==1);}
 {Run r;r.neighbor.gm=false;r.owner.group=r.neighbor.group=10;r.Kill(&r.adel);assert(r.Count()==1&&r.owner.credits[50397]+r.neighbor.credits[50397]==1);}
 for(uint32 early:{0u,1u,2u,3u}){Run r;scenario.step=early;scenario.currentTree=early==0?47120:early==1?47122:early==2?47126:47130;r.Kill(&r.pet);
  assert(!r.Count()&&r.instance.ladyPending&&!r.owner.credits[50397]);r.Tick(10000);assert(!warnings&&!r.Count());
  scenario.step=4;scenario.currentTree=47132;r.Wall();r.Tick();assert(r.Count()==1&&scenario.step==5&&!r.instance.ladyPending);++cases;
 }
 {Run r;scenario.rejected.insert(50397);r.Kill(&r.guardian);assert(!r.Count()&&r.owner.credits[50397]==1);
  r.Tick(999);assert(r.owner.credits[50397]==1);scenario.rejected.clear();r.Tick(1);assert(r.Count()==1&&r.owner.credits[50397]==2);}
 {Run r;scenario.rejected.insert(50397);r.Kill(&r.pet);for(int i=0;i<40;++i)r.Tick(1000);
  assert(warnings==1&&r.instance.ladyExhausted&&r.owner.credits[50397]==30);
  scenario.rejected.clear();r.Tick(10000);assert(!r.Count());r.instance.OnPlayerEnter(&r.neighbor);r.Tick();assert(!r.Count());
  r.instance.OnPlayerEnter(&r.owner);r.Tick();assert(r.Count()==1&&r.owner.credits[50397]==31);}
 {Run r;scenario.step=1;scenario.currentTree=47122;r.Kill(&r.pet);liveMap.players.clear();r.owner.world=false;
  scenario.step=4;scenario.currentTree=47132;for(int i=0;i<40;++i)r.Tick(1000);assert(warnings==1&&r.instance.ladyExhausted);
  r.owner.world=true;liveMap.players={{&r.owner},{&r.neighbor}};r.instance.OnPlayerEnter(&r.owner);r.Tick();assert(r.Count()==1&&!r.neighbor.credits[50397]);}
 for(int changed=0;changed<4;++changed){Run r;scenario.step=1;scenario.currentTree=47122;r.Kill(&r.pet);auto held=r.owner.quest.ScriptLifetime;
  switch(changed){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.quest.Status=0;break;case 3:r.owner.rewarded=true;break;}
  scenario.step=4;scenario.currentTree=47132;r.Tick();assert(!r.Count()&&!r.instance.ladyPending&&warnings==1);++cases;
 }
 {Run r;scenario.step=1;scenario.currentTree=47122;
  auto corpse=std::make_unique<Creature>();corpse->entry=104856;corpse->dbGuid=362095;corpse->alive=false;
  r.pet.NativeKillDispatch(corpse.get());assert(r.instance.ladyPending);corpse.reset();
  scenario.step=4;scenario.currentTree=47132;r.Tick();assert(r.Count()==1&&r.owner.credits[50397]==1);}
 {Run r;r.owner.onCredit=[&](uint32 asset){assert(asset==50397);r.instance.CreatureDies(&r.boss,&r.owner);r.Tick();};r.Kill(&r.pet);assert(r.Count()==1&&r.owner.credits[50397]==1);}
 {Run r;scenario.step=1;scenario.currentTree=47122;r.Kill(&r.pet);scenario.step=5;scenario.currentTree=47134;r.Tick();assert(!r.Count()&&!r.instance.ladyPending&&warnings==1);}
 {Run r;scenario.rejected.insert(50397);r.Kill(&r.pet);r.ai->Reset();scenario.rejected.clear();r.Tick(1000);assert(r.Count()==1);}
 for(int source=0;source<4;++source){Run r;r.boss.m_lootRecipient=r.owner.guid;
  Unit* killer=source==0?nullptr:source==1?&r.unowned:source==2?&r.pet:&r.neighbor;
  if(source==2)r.owner.alive=false;
  r.Kill(killer);assert(r.Count()==1&&r.owner.credits[50397]==1&&!r.neighbor.credits[50397]);++cases;
 }
 {Run r;r.neighbor.gm=false;r.boss.m_lootRecipient=r.neighbor.guid;r.Kill(&r.pet);
  assert(r.Count()==1&&r.neighbor.credits[50397]==1&&!r.owner.credits[50397]);}
 {Run r;r.boss.m_lootRecipient=r.neighbor.guid;r.Kill(&r.pet);assert(r.Count()==1&&r.owner.credits[50397]==1&&!r.neighbor.credits[50397]);}
 {Run r;r.boss.m_lootRecipient={1,999};r.Kill(&r.unowned);assert(!r.Count()&&warnings==1&&!r.neighbor.credits[50397]);}
 for(int denied=0;denied<4;++denied){Run r;r.boss.m_lootRecipient=r.owner.guid;
  if(denied==0)r.owner.world=false;if(denied==1)r.owner.map=&otherMap;if(denied==2)r.owner.phase=false;if(denied==3)r.owner.near=false;
  r.Kill(&r.unowned);assert(!r.Count()&&!r.owner.credits[50397]&&warnings==1);++cases;
 }
 std::cout<<"PASS "<<cases<<" Lady contexts plus exact native Unit::Kill/owner/quest/Scenario gate, actual full Lady/instance, both final wall/boss orders and no premature1923, player/pet/guardian/charmer/known upper3 allies, dead owner, native loot recipient/environmental and dead-owner credit, denied untagged/null/unowned/ambiguous or proximity-only claim, early real victory, duplicates/reentry/refusal bound/quest replacement/reentrant death and freed corpse. Native criteria acceptance/registry/geometry/groups/damage bridge are controlled where stated; no actual client combat/full-scenario claim.\n";
#endif
}
