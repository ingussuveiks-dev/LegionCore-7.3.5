#define HATI_OWNER_TEST
#include "HatiReactionTest.cpp"
#include "Load.inc"
#include "PrimaryBaseline.inc"

int main(){
 auto retained=RunReactionCases();uint32 cases=0,broken=0;
 for(uint32 entry:{100324u,106548u,106549u,106550u,106551u}){
  for(uint32 extras=0;extras<4;++extras)for(uint32 primary=0;primary<3;++primary){
   // Same actual GetPet body and controlled-list lookup endpoints in both callbacks.
   // primary=0: dismissed;1: living;2: corpse (existing policy retained).
   auto setup=[&](auto& r){r.hati.entry=entry;r.other.entry=777;r.pet.alive=primary!=2;
    if(!primary){r.player.pet=nullptr;r.player.m_Controlled.erase(1);ObjectAccessor::actors.erase(1);}
    if(extras){r.player.m_Controlled.insert(3);ObjectAccessor::actors[3]=&r.other;}
    if(extras==2)ObjectAccessor::actors.erase(3); // stale guardian GUID
    if(extras==3)r.other.alive=false;
   };
   {Run<BondPrimaryBaseline> r;setup(r);r.Tick(2000);bool mismatch=!primary&&(extras==1||extras==3);assert((r.hati.despawns==0)==(primary||mismatch));broken+=mismatch;}
   {Run<BondFixed> r;setup(r);r.Tick(1999);assert(!r.hati.despawns);r.Tick(1);
    assert(r.hati.despawns==(!primary?1u:0u));assert(!r.other.despawns&&r.player.casts.empty());
    if(!primary)assert(r.hati.despawnDelay==1000);++cases;}
  }
  // No Hati: a guardian never replaces the primary pet as summon prerequisite.
  for(bool primary:{false,true}){Run<BondFixed> r;r.player.m_Controlled.erase(2);ObjectAccessor::actors.erase(2);r.other.entry=777;r.player.m_Controlled.insert(3);ObjectAccessor::actors[3]=&r.other;
   if(!primary){r.player.pet=nullptr;r.player.m_Controlled.erase(1);ObjectAccessor::actors.erase(1);}
   r.Tick(2000);assert(r.player.casts.size()==(primary?1u:0u));assert(!r.other.despawns);++cases;}
  // Dismiss while guardian persists; complete actual removal, then recall/retry.
  {Run<BondFixed> r;r.hati.entry=entry;r.other.entry=777;r.player.m_Controlled.insert(3);ObjectAccessor::actors[3]=&r.other;r.Tick(2000);assert(!r.hati.despawns);
   r.player.pet=nullptr;r.player.m_Controlled.erase(1);ObjectAccessor::actors.erase(1);r.Tick(2000);assert(r.hati.despawns==1);
   r.player.m_Controlled.erase(2);ObjectAccessor::actors.erase(2);for(uint32 i=0;i<10;++i)r.Tick(2000);assert(r.player.casts.empty());
   r.player.pet=&r.pet;r.player.m_Controlled.insert(1);ObjectAccessor::actors[1]=&r.pet;r.Tick(2000);assert(r.player.casts==std::vector<uint32>{211147});
   r.player.m_Controlled.insert(2);ObjectAccessor::actors[2]=&r.hati;r.pet.m_reactState=REACT_PASSIVE;r.Tick(2000);assert(r.hati.GetReactState()==REACT_PASSIVE&&r.player.casts.size()==1&&!r.other.despawns);++cases;}
 }
 assert(broken==10);
 // Pointer identity: a different controlled Pet object is not the selected primary.
 {Run<BondFixed> r;Pet otherPet;otherPet.entry=777;ObjectAccessor::actors[1]=&otherPet;r.Tick(2000);assert(r.hati.despawns==1&&!otherPet.despawns);++cases;}
 std::cout<<"PASS "<<cases<<" primary-pet/guardian lifecycle contexts plus "<<retained<<" retained reaction cases; pinned063ac15fa has10 incorrect guardian-retained Hati controls. Full callback/native dispatcher/real Player::GetPet with controlled services; existing corpse policy retained. Actual client dismissal/recall/revive remains separate.\n";
}
