#include <cassert>
#include <cstdint>
#include <iostream>
using uint32=uint32_t;
constexpr uint32 QUEST_STATUS_INCOMPLETE=3,QUEST_STATUS_COMPLETE=1;
struct Player;
struct Creature {
 uint32 db=369307;bool los=true;
 uint32 GetDBTableGUIDLow(){return db;}bool IsWithinLOSInMap(Player*){return los;}
};
struct Player {
 bool world=true,alive=true,teleport=false,phase=true,rewarded=false,aura=false,searchSeesSpirit=true;
 uint32 map=1533,quest=QUEST_STATUS_INCOMPLETE,casts=0,removals=0,timer=0,ticks=0;float distance=1;Creature* spirit=nullptr;
 bool IsInWorld(){return world;}bool IsAlive(){return alive;}bool IsBeingTeleported(){return teleport;}
 uint32 GetMapId(){return map;}bool GetQuestRewardStatus(uint32 id){assert(id==40623);return rewarded;}
 uint32 GetQuestStatus(uint32 id){assert(id==40623);return quest;}
 Creature* FindNearestCreature(uint32 entry,float radius){assert(entry==100845&&radius==3);return searchSeesSpirit?spirit:nullptr;}
 bool InSamePhase(Creature*){return phase;}float GetExactDist(Creature*){return distance;}
 bool HasAura(uint32 id){assert(id==198654);return aura;}
 void CastSpell(Player* target,uint32 id,bool triggered){assert(target==this&&id==198654&&triggered);aura=true;timer=1000;++casts;}
 void RemoveAurasDueToSpell(uint32 id){assert(id==198654);if(aura){aura=false;++removals;}}
 void Tick(uint32 elapsed){if(aura){if(timer<=elapsed){++ticks;timer+=1000;}timer-=elapsed;}}
};
#include "DarkRidersSpirits.inc"
int main(){
 Player p;Creature spirit;p.spirit=&spirit;
 // Stationary contact must retain the same aura, not reset its one-second
 // periodic timer on every quarter-second instance update.
 for(uint32 i=0;i<20;++i){DarkRidersSpirits::UpdateContact(&p,988,1);p.Tick(250);}
 assert(p.casts==1&&p.ticks==5&&p.aura);
 p.distance=3.01f;DarkRidersSpirits::UpdateContact(&p,988,1);assert(!p.aura&&p.removals==1);
 p.distance=3;DarkRidersSpirits::UpdateContact(&p,988,1);assert(p.aura&&p.casts==2);
 p.spirit=nullptr;DarkRidersSpirits::UpdateContact(&p,988,1);assert(!p.aura); // Ariden's native despawn
 auto reset=[&]{p=Player{};spirit=Creature{};p.spirit=&spirit;};
 for(uint32 step:{0u,1u}){reset();DarkRidersSpirits::UpdateContact(&p,988,step);assert(p.aura);}
 for(int branch=0;branch<11;++branch){reset();DarkRidersSpirits::UpdateContact(&p,988,1);assert(p.aura);switch(branch){
  case 0:p.alive=false;break;case 1:p.world=false;break;case 2:p.teleport=true;break;case 3:p.map=1220;break;
  case 4:p.quest=0;break;case 5:p.phase=false;break;case 6:spirit.db=0;break;case 7:spirit.los=false;break;
  case 8:p.rewarded=true;break;case 9:p.searchSeesSpirit=false;break;case 10:p.distance=20;break;
 }DarkRidersSpirits::UpdateContact(&p,988,1);assert(!p.aura&&p.removals==1);}
 for(uint32 step=2;step<7;++step){reset();p.aura=true;DarkRidersSpirits::UpdateContact(&p,988,step);assert(!p.aura);}
 for(uint32 other:{1026u,1014u,0u}){reset();p.aura=true;DarkRidersSpirits::UpdateContact(&p,other,1);assert(p.aura&&!p.casts&&!p.removals);}
 DarkRidersSpirits::UpdateContact(nullptr,988,1);
 reset();p.quest=QUEST_STATUS_COMPLETE;DarkRidersSpirits::UpdateContact(&p,988,1);assert(p.aura);
 std::cout<<"PASS: production spirit contact, stationary aura retained across ticks, exact boundary/LOS/phase, Gateway/leave/death/abandon/despawn/stage cleanup and shared-scenario isolation\n";
}
