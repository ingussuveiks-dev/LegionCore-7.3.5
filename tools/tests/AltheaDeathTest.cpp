#define ALTHEA_REGRESSION
#define main retained_merlis_main
#include "KingslayersDeathCreditTest.cpp"
#undef main
struct Althea:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AltheaVictory.inc"
#include "Althea.inc"
};
struct Before:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AltheaBefore.inc"
};
void Dispatch(Creature* victim,Unit* killer,Althea& ai){
#include "AltheaKillGuard.inc"
 victim->m_IsInKillingProcess=true;ai.JustDied(killer);victim->dead=true;victim->m_IsInKillingProcess=false;
}
int main()
{
 Map map,otherMap;Scenario scenario;scenario.currentTree=48584;CriteriaTreeEntry victory{48585,30473};db2Manager.trees[48584]={&victory};scenarioMgr.scenarios[1]=&scenario;
 Player owner,closer;owner.map=closer.map=&map;owner.objectGuid={1,1};closer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&closer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Creature corpse,garona;corpse.objectGuid={9,42};corpse.map=garona.map=&map;corpse.nearest=&closer;corpse.garona=&garona;
 {Before old(&corpse);old.JustDied(&pet);assert(Conversation::casters==std::vector<Unit*>{&pet});ResetConversations();garona.delayed.clear();}
 std::cout<<"COUNTERFACTUAL complete pinned21ace Althea assigns2431 to pet and51613 to the unqualified nearby player; null reference source-traced only, not a reproduced native crash.\n";
 uint32 cases=0;
 for(uint32 kind=0;kind<9;++kind)for(bool fail:{false,true})for(bool companion:{false,true})
 {
  ResetConversations();AltheaDeathConversation::sources.clear();owner.criteria.clear();closer.criteria.clear();garona.delayed.clear();garona.elapsed=0;garona.despawns=0;
  corpse.garona=companion?&garona:nullptr;corpse.nearest=&closer;corpse.players={&closer,&owner};closer.gm=true;owner.preDelete=false;owner.inWorld=true;pet.owner=owner.objectGuid;pet.charmer={};pet.map=&map;Unit unowned;unowned.map=&map;
  Unit* killer=&pet;Unit* expected=&owner;
  switch(kind){case 0:killer=&owner;break;case 1:break;case 2:pet.charmer=closer.objectGuid;expected=&closer;break;case 3:killer=nullptr;expected=nullptr;break;case 4:killer=&unowned;expected=&unowned;break;case 5:pet.owner={1,20};expected=&pet;break;case 6:owner.preDelete=true;expected=&pet;break;case 7:pet.map=&otherMap;expected=&pet;break;case 8:corpse.nearest=nullptr;break;}
  Conversation::fail=fail;Althea ai(&corpse);ai.JustDied(killer);
  assert(Conversation::casters==(expected?std::vector<Unit*>{expected}:std::vector<Unit*>{}));
  assert(AltheaDeathConversation::sources.size()==(expected?1u:0u));
  if(expected){auto actual=AltheaDeathConversation::sources[0];assert(actual.high==corpse.GetGUID().high&&actual.low==corpse.GetGUID().low);}
  assert(closer.criteria.empty()&&owner.criteria.size()==(kind==6?0u:1u));
  assert(garona.delayed.size()==(companion?1u:0u));
  if(companion){garona.Advance(4999);assert(!garona.despawns);garona.Advance(1);assert(garona.despawns==1&&garona.despawnDelay==5000);assert(Conversation::casters.back()==&garona);}
  if(fail)assert(!Conversation::live);++cases;
 }
 ResetConversations();
 std::cout<<"PASS "<<cases<<" actual Althea ASAN contexts: null/direct/pet/guardian-owner/charm/missing-owner/map/predelete/NPC, failed creation and absent Garona; qualified owner/fallback51613 and actual5s2432/5s despawn. Native owner/accessor extracted; factory/timer endpoints controlled.\n";
 uint32 gates=0;
 for(uint32 quest:{42504u,42627u})for(uint32 variant=0;variant<32;++variant){
  owner=Player{};owner.map=&map;owner.quests={{quest,QUEST_STATUS_INCOMPLETE}};closer=Player{};closer.map=&map;closer.gm=true;
  corpse=Creature{};corpse.map=&map;corpse.players={&closer,&owner};scenario.id=1123;scenario.currentTree=48584;scenario._criteriaTreeForUpdate=0;scenario.achievements.progress.clear();scenarioMgr.scenarios[1]=&scenario;map.id=1620;
  bool accept=false;Player* killer=&owner;
  switch(variant){
  case 0:accept=true;break;case 1:owner.inWorld=false;break;case 2:owner.preDelete=true;break;
  case 3:owner.contact=false;break;case 4:owner.gm=true;break;case 5:owner.klass=1;break;
  case 6:owner.teleport=true;break;case 7:owner.map=&otherMap;break;case 8:owner.phase=false;break;
  case 9:owner.near=false;break;case 10:owner.expired=true;break;case 11:corpse.scope=false;break;
  case 12:owner.scope=false;break;case 13:owner.quests.clear();break;case 14:owner.rewarded.insert(quest);break;
  case 15:owner.quests[quest]=QUEST_STATUS_COMPLETE;accept=true;break;case 16:owner.quests[quest]=5;break;
  case 17:corpse.inWorld=false;break;case 18:corpse.preDelete=true;break;case 19:map.id=1500;break;
  case 20:corpse.expired=true;break;case 21:scenarioMgr.scenarios.clear();break;case 22:scenario.id=943;break;
  case 23:scenario.currentTree=48586;break;case 24:scenario.currentTree=48576;break;
  case 25:scenario.currentTree=48586;scenario._criteriaTreeForUpdate=48584;accept=true;break;
  case 26:killer=nullptr;accept=true;break;case 27:closer.gm=false;accept=true;break;
  case 28:corpse.players={&closer};accept=true;break;case 29:corpse.players={nullptr,&closer,&owner};killer=nullptr;accept=true;break;
  case 30:scenario.achievements.progress[48585].Counter=1;break;case 31:scenario.achievements.progress[48585].Counter=0;accept=true;break;
  }
  Althea ai(&corpse);assert(ai.GetVictoryPlayer(killer)==(accept?&owner:nullptr));++gates;
 }
 // The production Kill guard precedes JustDied. It protects both recursive and
 // repeated callbacks without introducing a second local progression counter.
 owner=Player{};owner.map=&map;corpse=Creature{};corpse.map=&map;map.id=1620;scenario.currentTree=48584;scenario._criteriaTreeForUpdate=0;scenario.achievements.progress.clear();scenario.id=1123;scenarioMgr.scenarios[1]=&scenario;
 Althea once(&corpse);corpse.m_IsInKillingProcess=true;Dispatch(&corpse,&owner,once);assert(owner.criteria.empty());
 corpse.m_IsInKillingProcess=false;Dispatch(&corpse,&owner,once);Dispatch(&corpse,&owner,once);assert(owner.criteria==std::vector<uint32>{51613});ResetConversations();
 std::cout<<"PASS "<<gates<<" actual recipient eligibility/owner priority/fallback/native scenario-tree contexts over both quest variants, plus actual native Kill repeated/reentrant admission guard. No per-member group increment or client/full-chain claim.\n";
}
