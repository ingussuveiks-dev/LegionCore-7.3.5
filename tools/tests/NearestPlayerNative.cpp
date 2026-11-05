// Real WorldObject/Player predicates, world-grid visitors and independent
// Creature selector. Positions/life/grid transitions are controlled endpoints.
#define ALTHEA_INTRO_NATIVE_SUPPORT_ONLY
#include "AltheaIntroNative.cpp"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectVisitors.hpp"
#include "BeforeNearest.inc"
static Player* Search(WorldObject* source,float range,bool alive=true){
#ifdef NEAREST_PLAYER_BASELINE
 return BeforeNearest(source,range,alive);
#else
 return source->FindNearestPlayer(range,alive);
#endif
}
static void Place(NativePlayer& player,Position const& position){
 player.RemoveFromGrid();player.Relocate(position);auto map=player.GetMap();
 map->LoadGrid(position.GetPositionX(),position.GetPositionY());
 map->AddToGrid(static_cast<Player*>(&player),Cell(Trinity::ComputeCellCoord(position.GetPositionX(),position.GetPositionY())));
}
int main(int argc,char**argv){try{
 InitializeAltheaGuardsNative(argc,argv);uint32 cases=0,broken=0;
 for(uint32 race:{uint32(RACE_HUMAN),uint32(RACE_ORC)})for(uint32 mode=0;mode<12;++mode){
  MapInstanced parent(1620,300);InstanceMap map(1620,300,9935000+cases,DIFFICULTY_N_SCENARIO,&parent);auto first=Session(),second=Session();
  {Position pose{-8822.94f,657.59f,96.5f,0};NativePlayer a(first,map,101,pose),b(second,map,102,pose);
   for(auto p:{&a,&b}){p->SetByteValue(UNIT_FIELD_BYTES_0,0,race);p->setFactionForRace(race);}
   Actor source(&map,108177,31001,pose);Require(map.AddToMap(static_cast<Creature*>(&source)),"Native search source missing");
   auto offset=[&](NativePlayer& player,float x,float z=0){auto p=pose;p.m_positionX+=x;p.m_positionZ+=z;Place(player,p);};
   auto selected=Search(&source,15);Require(selected==&a||selected==&b,"Initial grid fixture has no Player");
   auto earlier=static_cast<NativePlayer*>(selected);auto later=earlier==&a?&b:&a;
   Player* expected=nullptr;float range=15;bool alive=true;bool counterfactual=false;
   if(mode==0){offset(*earlier,12);offset(*later,5);expected=later;counterfactual=true;}
   if(mode==1){offset(a,10);offset(b,400);range=1;Require(!source.IsWithinDistInMap(&a,range),"Radius witness not outside requested range");counterfactual=true;}
   if(mode==2){float visibility=source.GetVisibilityRange();offset(a,visibility+20);offset(b,visibility+200);range=visibility+40;Require(!source.IsWithinDistInMap(&a,visibility)&&source.IsWithinDistInMap(&a,range),"Visibility/radius witness differs");expected=&a;counterfactual=true;}
   if(mode==3){offset(a,3);offset(b,12);a.Unit::setDeathState(JUST_DIED);alive=false;expected=&a;counterfactual=true;Require(source.SelectNearestPlayer(15)==&b,"Existing Creature selector lost live-only contract");}
   if(mode==4){offset(a,3);offset(b,12);a.Unit::setDeathState(JUST_DIED);b.Unit::setDeathState(JUST_DIED);alive=false;expected=&a;counterfactual=true;Require(!source.SelectNearestPlayer(15),"Existing Creature selector admits corpses");}
   if(mode==5){offset(a,3);a.SetPhaseMask(2,false);offset(b,12);expected=&b;}
   if(mode==6){offset(*earlier,0,20);offset(*later,5);range=10;expected=later;counterfactual=true;}
   if(mode==7){offset(a,3);offset(b,12);a.Unit::setDeathState(JUST_DIED);b.SetGameMaster(true);expected=&b;Require(source.FindNearestPlayer(15)==&b,"Default live-only/GM-neutral selection changed");}
   if(mode==8){offset(a,10);offset(b,-10);auto found=Search(&source,15);Require(found&&(found==&a||found==&b)&&source.GetDistance(found)==source.GetDistance(&a),"Equal-distance tie not a nearest Player");expected=found;}
   if(mode==9){offset(*earlier,5);offset(*later,12);Require(Search(earlier,15)==earlier,"Player source cannot select itself at distance0");expected=earlier;}
   if(mode==10){offset(a,8);offset(b,12);InstanceMap other(1620,300,9945000+cases,DIFFICULTY_N_SCENARIO,&parent);auto third=Session();{NativePlayer foreign(third,other,103,pose);Require(!source.IsWithinDistInMap(&foreign,15),"Foreign map is not foreign");Require(Search(&source,15)==&a,"Search crossed actual Map boundary");}third->SetPlayer(nullptr);delete third;expected=&a;}
   if(mode==11){offset(a,3);offset(b,12);a.Unit::RemoveFromWorld();a.RemoveFromGrid();Require(Search(&source,15)==&b,"Removed grid Player remains searchable");a.Relocate(pose);map.AddToGrid(static_cast<Player*>(&a),Cell(Trinity::ComputeCellCoord(pose.GetPositionX(),pose.GetPositionY())));a.Unit::AddToWorld();expected=&a;}
   auto found=Search(&source,range,alive);
#ifdef NEAREST_PLAYER_BASELINE
   if(counterfactual){Require(found!=expected,"Pinned nearest-player counterfactual no longer reproduces");++broken;std::cout<<"COUNTERFACTUAL race="<<race<<" mode="<<mode<<" pinned first/visibility/ignored-alive selection differs from native nearest/range/life contract\n";}
   else if(mode!=10&&mode!=11)Require(found==expected,"Pinned control differs");
#else
   Require(found==expected,"Actual native nearest-player result differs");
   if(alive)Require(found==source.SelectNearestPlayer(range),"Default native Creature selector disagrees");
   if(found)Require(found->GetMap()==&map&&source.IsWithinDistInMap(found,range)&&(!alive||found->IsAlive()),"Native result violates map/radius/life");
#endif
   std::cout<<"CASE race="<<race<<" mode="<<mode<<" range="<<range<<" alive="<<alive<<" selected="<<(found?found->GetGUID().GetCounter():0)<<'\n';
  }for(auto s:{first,second}){s->SetPlayer(nullptr);delete s;}++cases;
 }
 std::cout<<"PASS "<<cases<<" native nearest-player contexts; "<<broken<<" pinned counterfactuals, earlier poses/life/grid admission controlled, no client or whole-caller claim\n";CloseAltheaNative();return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
