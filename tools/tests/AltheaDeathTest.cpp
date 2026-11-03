#define ALTHEA_REGRESSION
#define main retained_merlis_main
#include "KingslayersDeathCreditTest.cpp"
#undef main
struct Althea:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "Althea.inc"
};
struct Before:ScriptedAI{using ScriptedAI::ScriptedAI;
#include "AltheaBefore.inc"
};
int main()
{
 Map map,otherMap;Player owner,closer;owner.map=closer.map=&map;owner.objectGuid={1,1};closer.objectGuid={1,2};
 HashMapHolder<Player>::_objectVector[1]=&owner;HashMapHolder<Player>::_objectVector[2]=&closer;
 Unit pet;pet.map=&map;pet.owner=owner.objectGuid;Creature corpse,garona;corpse.map=garona.map=&map;corpse.nearest=&closer;corpse.garona=&garona;
 {Before old(&corpse);old.JustDied(&pet);assert(Conversation::casters==std::vector<Unit*>{&pet});ResetConversations();garona.delayed.clear();}
 std::cout<<"COUNTERFACTUAL complete pinned21ace Althea assigns2431 to pet; null reference is source-traced only, not a reproduced native crash. Original nearest-player51613 behavior intentionally retained.\n";
 uint32 cases=0;
 for(uint32 kind=0;kind<9;++kind)for(bool fail:{false,true})for(bool companion:{false,true})
 {
  ResetConversations();owner.criteria.clear();closer.criteria.clear();garona.delayed.clear();garona.elapsed=0;garona.despawns=0;
  corpse.garona=companion?&garona:nullptr;corpse.nearest=&closer;owner.preDelete=false;owner.inWorld=true;pet.owner=owner.objectGuid;pet.charmer={};pet.map=&map;Unit unowned;unowned.map=&map;
  Unit* killer=&pet;Unit* expected=&owner;
  switch(kind){case 0:killer=&owner;break;case 1:break;case 2:pet.charmer=closer.objectGuid;expected=&closer;break;case 3:killer=nullptr;expected=nullptr;break;case 4:killer=&unowned;expected=&unowned;break;case 5:pet.owner={1,20};expected=&pet;break;case 6:owner.preDelete=true;expected=&pet;break;case 7:pet.map=&otherMap;expected=&pet;break;case 8:corpse.nearest=nullptr;break;}
  Conversation::fail=fail;Althea ai(&corpse);ai.JustDied(killer);
  assert(Conversation::casters==(expected?std::vector<Unit*>{expected}:std::vector<Unit*>{}));
  assert(closer.criteria.size()==(kind==8?0u:1u)&&owner.criteria.empty());
  assert(garona.delayed.size()==(companion?1u:0u));
  if(companion){garona.Advance(4999);assert(!garona.despawns);garona.Advance(1);assert(garona.despawns==1&&garona.despawnDelay==5000);assert(Conversation::casters.back()==&garona);}
  if(fail)assert(!Conversation::live);++cases;
 }
 ResetConversations();
 std::cout<<"PASS "<<cases<<" actual Althea ASAN contexts: null/direct/pet/guardian-owner/charm/missing-owner/map/predelete/NPC, failed creation and absent Garona; original nearest-player credit and actual5s2432/5s despawn retained. Native owner/accessor extracted; factory/timer endpoints controlled, no native crash or full scenario/client certification.\n";
}
