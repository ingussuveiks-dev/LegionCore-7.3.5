// Full extracted SummonCreepings,actual Position and Unit callback queue.
// Spawn requests are observed;this is not registered NPC combat/client testing.
#include "ActorQueueLifetimeNative.h"
#include <cmath>
#include <set>
static uint32 wallChoice=0,wallPassage=6,wallDirection=0,wallTalks=0;
struct WallRequest{Position at;uint32 entry,lifetime;TempSummonType type;};
static std::vector<WallRequest> wallRequests;
static uint32 CordanaRand(uint32 low,uint32 high){
 if(low==0&&high==3)return wallChoice;
 if(low==6&&high==18)return wallPassage;
 Require(low==0&&high==2,"Unexpected production random interval");return wallDirection?1:0;
}
static void CordanaRequest(uint32 entry,float x,float y,float z,float o,TempSummonType type,uint32 lifetime){wallRequests.push_back({{x,y,z,o},entry,lifetime,type});}
static constexpr uint8 SAY_EMOTE_CREEPING=10;
#include "CordanaWalls.inc"
static void CheckWave(uint32 wall){
#ifdef CORDANA_WALL_BASELINE
 if(wall%2){Require(wallRequests.empty(),"Pinned horizontal-wall counterfactual missing");return;}
#endif
 Require(wallRequests.size()==20,"Wall must retain25authored slots minus one five-slot passage");
 auto const& endpoints=creeping_walls.at(uint8(wall));auto const& start=endpoints.first;auto const& end=endpoints.second;
 bool vertical=start.GetPositionX()==end.GetPositionX();std::set<uint32> slots;
 for(auto const& request:wallRequests){
  Require(request.entry==100336&&request.lifetime==18000&&request.type==TEMPSUMMON_TIMED_DESPAWN,"Original summon contract changed");
  Require(std::abs(request.at.GetPositionZ()-start.GetPositionZ())<0.001f&&std::abs(request.at.GetOrientation()-start.GetOrientation())<0.001f,"Original Z/orientation changed");
  float distance=vertical?start.GetPositionY()-request.at.GetPositionY():start.GetPositionX()-request.at.GetPositionX();
  auto slot=uint32(std::lround(distance/2.5f));
  Require(std::abs(distance-slot*2.5f)<0.001f&&slot<25&&slots.insert(slot).second,"Wall spacing,extent or duplicate spawn");
  Require(vertical?std::abs(request.at.GetPositionX()-start.GetPositionX())<0.001f:std::abs(request.at.GetPositionY()-start.GetPositionY())<0.001f,"Wall crossed its authored axis");
  Require(slot+1<wallPassage-2||slot+1>wallPassage+2,"Authored passage filled");
 }
 for(uint32 slot=0;slot<25;++slot)Require(bool(slots.count(slot))==(slot+1<wallPassage-2||slot+1>wallPassage+2),"Missing wall segment outside the passage");
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);
 dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1600,{}}});MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1600,{}}});
 MapInstanced parent(1600,300);InstanceMap map(1600,300,9999781,DIFFICULTY_N_SCENARIO,&parent);Actor actor(&map,299,24101,{50,803,805,0});Require(map.AddToMap(static_cast<Creature*>(&actor)),"Native wall queue actor insertion");
 uint32 cases=0;
 for(auto difficulty:{DIFFICULTY_HEROIC,DIFFICULTY_MYTHIC_DUNGEON})for(uint8 phase:{uint8(1),uint8(2)})for(wallChoice=0;wallChoice<4;++wallChoice)for(wallPassage=6;wallPassage<=18;++wallPassage)for(wallDirection=0;wallDirection<2;++wallDirection){
  actor.KillAllDelayedEvents();PumpQueue(actor,0);wallRequests.clear();wallTalks=0;WallsFrame frame(&actor,difficulty);frame.SummonCreepings(phase);
  Require(wallTalks==1,"Authored phase emote changed");PumpQueue(actor,0);Require(wallRequests.empty(),"Wall spawned before its first deadline");
  uint32 waves=phase==1?(difficulty==DIFFICULTY_HEROIC?2:4):(difficulty==DIFFICULTY_HEROIC?1:2);uint32 interval=phase==1?(difficulty==DIFFICULTY_HEROIC?17000:8000):1;
  for(uint32 wave=0;wave<waves;++wave){
   if(wave){wallRequests.clear();PumpQueue(actor,interval-1);Require(wallRequests.empty(),"Wave spawned before authored interval");PumpQueue(actor,1);}else PumpQueue(actor,1);
   uint32 wall=(wallChoice+4+(wallDirection?wave:4-wave))%4;CheckWave(wall);
  }
  auto settled=wallRequests.size();PumpQueue(actor,60000);Require(wallRequests.size()==settled,"Extra wave after authored wave count");++cases;
 }
 Require(cases==416,"Expected all directions,phases,difficulties and13passage positions");
 wallChoice=0;wallPassage=12;wallDirection=0;wallRequests.clear();{WallsFrame frame(&actor,DIFFICULTY_HEROIC);frame.SummonCreepings(1);actor.KillAllDelayedEvents();PumpQueue(actor,60000);Require(wallRequests.empty(),"Native cancellation no longer cancels walls");}
 {WallsFrame frame(&actor,DIFFICULTY_MYTHIC_DUNGEON);frame.SummonCreepings(1);}PumpQueue(actor,60000);Require(wallRequests.empty(),"Expired scheduling owner entered wall payload");
 {WallsFrame frame(&actor,DIFFICULTY_HEROIC);frame.SummonCreepings(0);PumpQueue(actor,60000);Require(wallRequests.empty(),"Unknown phase spawned a wall");}
 CloseAltheaNative();std::cout<<
#ifdef CORDANA_WALL_BASELINE
 "PASS expected-broken pinned Cordana method:horizontal waves emit no requests;vertical controls and native deadlines retained\n";
#else
 "PASS Cordana416full-method native queue cases:all four walls,phase1/2,heroic/mythic,13passages,both rotation directions,spacing/count/orientation/deadlines,cancellation and expired owner. Spawn boundary observed;NPC combat/client traversal pending\n";
#endif
 return 0;
 }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
