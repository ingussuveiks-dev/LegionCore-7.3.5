#include "NativeHost.inc"
#include "Policy.inc"
#include "Actor.inc"
#include "Instance.inc"
struct Guide:ScriptedAI {
 bool firstbattle=true;FunctionProcessor _functions;explicit Guide(Creature* actor):ScriptedAI(actor){}
#include "Evade.inc"
};
using Script=instance_sharasdal_InstanceMapScript;
struct Run {
 Player owner,neighbor;Unit pet,guardian;Creature grash,erunak,first,second;Guide guide{&erunak};Script script{&liveMap};npc_grash creatureScript;std::unique_ptr<npc_grash::npc_grashAI> ai;
 Run(){
  scenario=Scenario{};manager.active=&scenario;paths.missing.clear();warnings=0;mapConversations.clear();conversationAttempts=conversationCreates=conversationDeletes=0;conversationFail=false;conversationDuration=17600;conversationCaster={};
  static CriteriaTreeEntry heal{47121,29658},intro{47180,29688},brutes{47161,29677},recruit{47303,29763},guard{47197,29695},rescue{47123,29659};
  db2.rows={{47120,{&heal,&intro,&brutes,&recruit}},{47122,{&guard,&rescue}}};
  owner.guid={1,1};neighbor.guid={1,2};neighbor.classId=5;ObjectAccessor::players={&owner,&neighbor};liveMap.players={{&owner},{&neighbor}};activeInstance=&script;
  pet.owner=guardian.owner=owner.guid;pet.guid={3,1};guardian.guid={3,2};
  grash.guid={2,66};grash.dbGuid=362066;grash.entry=105298;grash.health=100;grash.flags=0x40;grash.erunak=&erunak;grash.nearest=&neighbor;
  erunak.guid={2,63};erunak.dbGuid=362063;erunak.entry=102826;erunak.health=1000;
  first.guid={2,64};first.dbGuid=362064;first.entry=102792;first.nearest=&neighbor;
  second.guid={2,67};second.dbGuid=362067;second.entry=102792;second.nearest=&neighbor;
  ObjectAccessor::creatures={&grash,&erunak,&first,&second};ai=std::make_unique<npc_grash::npc_grashAI>(&grash);
  for(auto actor:ObjectAccessor::creatures)script.OnCreatureCreate(actor);
 }
 ~Run(){mapConversations.clear();ObjectAccessor::creatures.clear();ObjectAccessor::players.clear();liveMap.players.clear();activeInstance=nullptr;}
 uint64 Count(uint32 tree=47161){return scenario.achievements.progress[tree].Counter;}
 void Kill(Creature& actor,Unit* killer){actor.alive=false;script.OnUnitDeath(&actor);if(killer)killer->NativeKillDispatch(&actor);else script.CreatureDies(&actor,nullptr);}
 void Tick(uint32 diff){owner.functions.Update(diff);neighbor.functions.Update(diff);erunak.functions.Update(diff);grash.Tick(diff);script.Update(diff);}
 void Heal(){assert(owner.DealHeal(&grash,899)==899);grash.Tick(0);assert(Count(47121)&&Count(47180));}
 void Hello(){creatureScript.OnGossipHello(&owner,&grash);}void Select(){ai->sGossipSelect(&owner,19509,0);creatureScript.OnGossipSelect(&owner,&grash,1,1001);}
#ifndef BASELINE
 void FinishPresentation(){Tick(60000);assert(conversationCreates==1&&!(grash.flags&1)&&script.bruteRoleWait==17600);Tick(17599);assert(!(grash.flags&1));Tick(1);assert(grash.flags==0x41);}
#endif
};
int main(){
#ifdef BASELINE
 {Run r;r.Heal();r.Kill(r.first,&r.pet);assert(r.Count()==1&&r.neighbor.credits[49861]==1&&!r.owner.credits[49861]&&conversationCreates==1);r.neighbor.functions.Update(5000);assert(r.grash.flags==1);r.Hello();assert(r.owner.sent==1);r.Select();assert(r.Count(47303)==1&&scenario.step==0);r.ai->UpdateAI(1000);assert(r.grash.motion.paths==std::vector<uint32>{10993808});}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);assert(conversationCreates==2);r.guide.EnterEvadeMode();r.guide._functions.Update(1000);assert(conversationCreates==3);}
 std::cout<<"PASS COUNTERFACTUAL514ac7bd0: real Brute pet kill credited nearest unrelated player, first death played1693/queued flag overwrite; Grash accepted recruitment and launched native route with only1/2 Brutes. Two deaths plus evade played3 conversations. No raw captured owner/AI deliberately freed.\n";
#else
 unsigned cases=0;
 for(uint32 status:{1u,3u})for(int source=0;source<6;++source)for(bool reverse:{false,true}){
  Run r;r.owner.quest.Status=status;r.Heal();r.Hello();assert(!r.owner.sent);Unit* killers[]={&r.owner,&r.pet,&r.guardian,&r.pet,&r.erunak,&r.grash};if(source==3){r.pet.owner={};r.pet.charmer=r.owner.guid;}
  Creature* one=reverse?&r.second:&r.first;Creature* two=reverse?&r.first:&r.second;
  r.guide.EnterEvadeMode();r.guide._functions.Update(60000);assert(!conversationAttempts);
  r.Kill(*one,killers[source]);assert(r.Count()==1&&!conversationAttempts&&!(r.grash.flags&1));r.Tick(60000);r.grash.flags|=1;r.Hello();assert(!r.owner.sent);r.grash.flags&=~1u;
  r.Kill(*one,killers[source]);assert(r.Count()==1);r.Kill(*two,killers[source]);assert(r.Count()==2&&r.owner.credits[49861]==2&&!r.neighbor.credits[49861]&&!conversationAttempts&&scenario.step==0);
  r.FinishPresentation();assert(conversationCaster==r.owner.guid);r.Hello();assert(r.owner.sent==1);r.Select();assert(r.Count(47303)==1&&scenario.step==1&&r.grash.flags==0x40);r.ai->UpdateAI(1000);assert(r.grash.motion.paths==std::vector<uint32>{10993808}&&r.erunak.motion.paths==std::vector<uint32>{10993809});
  r.Tick(60000);r.guide.EnterEvadeMode();r.guide._functions.Update(60000);assert(conversationCreates==1&&r.Count()==2&&r.grash.flags==0x40);++cases;
 }
 for(int deny=0;deny<22;++deny){Run r;Unit* killer=&r.owner;r.first.alive=false;switch(deny){case 0:r.first.alive=true;break;case 1:r.first.entry=999;break;case 2:r.first.dbGuid=999;break;case 3:r.first.map=&otherMap;break;case 4:r.first.world=false;break;case 5:r.first.preDelete=true;break;case 6:scenario.id=99;break;case 7:scenario.step=1;break;case 8:manager.active=nullptr;break;case 9:r.owner.world=false;break;case 10:r.owner.contact=false;break;case 11:r.owner.gm=true;break;case 12:r.owner.teleport=true;break;case 13:r.owner.map=&otherMap;break;case 14:r.owner.phase=false;break;case 15:r.owner.near=false;break;case 16:r.owner.classId=5;break;case 17:r.owner.rewarded=true;break;case 18:r.owner.record=false;break;case 19:r.owner.quest.Status=0;break;case 20:killer=nullptr;break;case 21:r.pet.owner={1,99};killer=&r.pet;break;}r.script.CreatureDies(&r.first,killer);r.Tick(60000);assert(!r.Count()&&!conversationAttempts&&!(r.grash.flags&1));manager.active=&scenario;++cases;}
 {Run r;r.owner.alive=false;r.Kill(r.first,&r.pet);r.Kill(r.second,&r.guardian);assert(r.Count()==2);}
 {Run r;r.neighbor.classId=7;r.owner.group=r.neighbor.group=4;r.Kill(r.first,&r.erunak);r.Kill(r.second,&r.grash);assert(r.Count()==2&&r.owner.credits[49861]+r.neighbor.credits[49861]==2);}
 {Run r;r.neighbor.classId=7;r.Kill(r.first,&r.erunak);r.Kill(r.second,&r.grash);assert(!r.Count()&&!conversationAttempts);}
 {Run r;r.erunak.dbGuid=362093;r.Kill(r.first,&r.erunak);assert(!r.Count());}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.pet);assert(r.Count()==2);r.Tick(0);assert(conversationCreates==1);r.Tick(17600);r.Hello();assert(!r.owner.sent);r.Heal();r.Hello();assert(r.owner.sent==1);r.Select();assert(scenario.step==1);}
 {Run r;scenario.rejected.insert(49861);r.Kill(r.first,&r.owner);assert(!r.Count()&&r.script.bruteCredits[0].pending&&r.owner.credits[49861]==1);r.Tick(999);assert(r.owner.credits[49861]==1);r.Tick(1);assert(r.owner.credits[49861]==2);for(int i=0;i<28;++i)r.Tick(1000);assert(r.script.bruteCredits[0].exhausted&&r.owner.credits[49861]==30&&warnings==1);scenario.rejected.clear();r.script.OnPlayerEnter(&r.neighbor);r.Tick(60000);assert(!r.Count());r.script.OnPlayerEnter(&r.owner);r.first.world=false;r.Tick(0);assert(r.Count()==1&&r.owner.credits[49861]==31);}
 for(int change=0;change<4;++change){Run r;scenario.rejected.insert(49861);r.Kill(r.first,&r.owner);auto held=r.owner.quest.ScriptLifetime;switch(change){case 0:r.owner.quest.ScriptLifetime=std::make_shared<uint8>();break;case 1:r.owner.record=false;break;case 2:r.owner.rewarded=true;break;case 3:r.owner.quest.Status=0;break;}scenario.rejected.clear();r.Tick(1000);assert(!r.Count()&&!r.script.bruteCredits[0].pending&&warnings==1);}
 {Run r;scenario.rejected.insert(49861);r.Kill(r.first,&r.owner);auto held=r.owner.quest.ScriptLifetime;liveMap.players={{&r.neighbor}};for(int i=0;i<30;++i)r.Tick(1000);assert(r.script.bruteCredits[0].exhausted);scenario.rejected.clear();liveMap.players.push_back({&r.owner});r.script.OnPlayerEnter(&r.owner);r.Tick(0);assert(r.Count()==1);}
 {Run r;r.owner.onCredit=[&](uint32 asset){if(asset==49861){r.Kill(r.second,&r.pet);r.script.Update(60000);}};r.Kill(r.first,&r.owner);assert(r.Count()==2&&r.owner.credits[49861]==2);r.owner.onCredit={};r.FinishPresentation();}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);conversationFail=true;r.Tick(0);assert(conversationAttempts==1&&conversationDeletes==1&&!conversationCreates);r.Tick(4999);assert(!(r.grash.flags&1));r.Tick(1);assert(r.grash.flags==0x41&&r.Count()==2);auto attempts=conversationAttempts;conversationFail=false;r.Tick(60000);assert(conversationAttempts==attempts&&!conversationCreates);}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);r.erunak.world=false;r.Tick(5000);assert(!conversationAttempts&&r.grash.flags==0x41);r.erunak.world=true;r.Tick(60000);assert(!conversationAttempts);}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);r.grash.world=false;for(int i=0;i<35;++i)r.Tick(1000);assert(r.script.bruteVoiceExhausted&&r.script.bruteRoleExhausted&&warnings==2&&!conversationAttempts);r.grash.world=true;r.script.OnCreatureCreate(&r.grash);r.Tick(0);assert(conversationCreates==1&&!r.script.bruteRoleExhausted);r.Tick(17600);assert(r.grash.flags==0x41);}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);r.Tick(0);r.grash.AddUnitState(UNIT_STATE_STUNNED);r.Tick(60000);assert(!(r.grash.flags&1)&&!r.script.bruteRoleExhausted);r.grash.ClearUnitState(UNIT_STATE_STUNNED);r.Tick(0);assert(r.grash.flags==0x41);}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);r.owner.quest.ScriptLifetime.reset();r.Tick(5000);assert(!conversationAttempts&&r.grash.flags==0x41&&r.Count()==2);}
 {Run r;scenario.achievements.progress[47161].Counter=2;r.Tick(60000);assert(!(r.grash.flags&1)&&!conversationAttempts);r.Tick(5000);assert(r.grash.flags==0x41&&!r.owner.credits[49861]);}
 {Run r;r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);scenario.step=2;r.Tick(60000);assert(!conversationAttempts&&r.grash.flags==0x40);}
 {Run r;r.script.OnUnitDeath(nullptr);r.script.CreatureDies(nullptr,&r.owner);assert(!r.Count());}
 {Run r;r.Heal();r.Kill(r.first,&r.owner);r.grash.flags|=1;r.owner.PlayerTalkClass->GetGossipMenu().SetMenuId(19509);r.owner.ADD_GOSSIP_ITEM_DB(19509,0,1,1001);r.Select();assert(!r.Count(47303)&&!r.ai->GetData(19509)&&r.grash.motion.paths.empty()&&scenario.step==0);}
 {Run r;r.Heal();r.Kill(r.first,&r.owner);r.Kill(r.second,&r.owner);r.FinishPresentation();r.Hello();r.Select();CreatureAI temporary;r.grash.primary=&temporary;r.grash.i_disabledAI=r.ai.get();r.Tick(60000);assert(r.grash.flags==0x40&&conversationCreates==1);}
 {Run r;r.Heal();Creature tidecaller;tidecaller.entry=102831;tidecaller.dbGuid=362076;tidecaller.guid={2,76};r.Kill(tidecaller,&r.guardian);assert(!r.Count(47197)&&r.script.guardPending);tidecaller.world=false;r.Kill(r.first,&r.pet);r.Kill(r.second,&r.owner);r.FinishPresentation();r.Hello();r.Select();r.Tick(0);assert(scenario.step==1&&r.Count(47197)==1&&!r.script.guardPending&&r.Count()==2&&!r.Count(47123));}
 std::cout<<"PASS "<<cases<<" coupled two-Brute/Grash matrices plus native Kill/owner/quest/criteria and applied heal/AI/function dispatch: both kill orders and six killers, real2/2 before offered or forged recruitment, native stage1 before original routes, exactly-once1693/native duration, independent failed dialogue/cleanup, preserved role bits, finite failures/re-entry, stale attempt/removed corpse, groups/dead owners and reentrant deaths. Registry/geometry/conversation service/group identity/criterion service remain controlled; no client combat/voice/path traversal claimed.\n";
#endif
}
