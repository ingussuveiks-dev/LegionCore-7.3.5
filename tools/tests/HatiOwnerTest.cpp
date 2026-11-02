#define HATI_OWNER_TEST
#include "HatiReactionTest.cpp"
#include "Load.inc"
#include "OwnerBaseline.inc"
#include <string>
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="counterfactual"){
  // Actual complete pre-fix callback/native update dispatch enters actual
  // Player::GetPet with null this. GUID storage/lookup are controlled services.
  Run<BondOwnerBaseline> r;Creature npc;r.other.entry=777;r.aura.caster=&npc;npc.m_Controlled={3};ObjectAccessor::actors[3]=&r.other;
  assert(r.script._Load(&r.aura));r.Tick(2000);return 9;
 }
 auto retained=RunReactionCases();uint32 cases=0;
 for(uint32 dead=0;dead<2;++dead)for(uint32 makeup=0;makeup<5;++makeup){
  Run<BondFixed> r;Creature npc;r.other.entry=777;npc.alive=!dead;r.aura.caster=&npc;
  if(makeup==1)npc.m_Controlled={99};
  if(makeup==2){npc.m_Controlled={3};ObjectAccessor::actors[3]=&r.other;}
  if(makeup==3){npc.m_Controlled={2};}
  if(makeup==4){npc.m_Controlled={2,3};ObjectAccessor::actors[3]=&r.other;}
  assert(r.script._Load(&r.aura));r.Tick(1999);r.Tick(1);
  assert(npc.casts.empty()&&r.hati.GetReactState()==REACT_HELPER&&!r.hati.despawns&&!r.other.despawns&&r.script.update==0);
  for(uint32 i=0;i<100;++i)r.Tick(2000);assert(npc.casts.empty()&&!r.hati.despawns);++cases;
 }
 {Run<BondFixed> r;r.aura.caster=nullptr;assert(r.script._Load(&r.aura));r.Tick(2000);assert(r.player.casts.empty());++cases;}
 {Run<BondFixed> r;Creature npc;r.other.entry=777;npc.m_Controlled={3};ObjectAccessor::actors[3]=&r.other;r.aura.caster=&npc;r.Tick(2000);r.aura.caster=&r.player;r.pet.m_reactState=REACT_PASSIVE;r.Tick(2000);assert(r.hati.GetReactState()==REACT_PASSIVE);++cases;}
 std::cout<<"PASS "<<cases<<" non-Player/dead/missing caster admission contexts plus retained "<<retained<<" Hati reaction cases through native AuraScript::_Load/native update/real Player::GetPet. Script/GM-applied non-Player aura can load safely but cannot enter player-only companion lifecycle. No live character mutation/client claim.\n";
}
