#include <cassert>
#include <cstdint>
#include <vector>
#include <iostream>
using uint32=uint32_t;
enum SpellCastResult{SPELL_CAST_OK,SPELL_FAILED_BAD_TARGETS,SPELL_FAILED_ROOTED,SPELL_FAILED_OUT_OF_RANGE,SPELL_FAILED_NOPATH};
enum PathType{PATHFIND_BLANK=0,PATHFIND_NORMAL=1,PATHFIND_NOPATH=8,PATHFIND_SHORT=32};
enum{UNIT_STATE_ROOT=1};
struct Position{float m_positionX=0,m_positionY=0,m_positionZ=0;};
struct Transport{};
struct Unit{
 Transport* transport=nullptr;Position pos;bool rooted=false,water=false;float size=1;
 Transport* GetTransport()const{return transport;}Position const& GetPosition()const{return pos;}
 Unit const* ToUnit()const{return this;}float GetCollisionHeight()const{return 2;}
 bool HasUnitState(int){return rooted;}bool IsInWater(){return water;}
 float GetRelativeAngle(Unit*){return 0;}float GetObjectSize()const{return size;}
 Position GetFirstCollisionPosition(float,float){return {pos.m_positionX-size,pos.m_positionY,pos.m_positionZ};}
};
struct Point{float x=0,y=0,z=0;};Point PositionToVector3(Position const& p){return {p.m_positionX,p.m_positionY,p.m_positionZ};}
namespace Movement{
 using PointsArray=std::vector<Point>;
 enum class TransportGroundResult{NoPath,TooLong,Complete};
 TransportGroundResult next=TransportGroundResult::Complete;unsigned corridors=0;
 TransportGroundResult BuildTransportGroundPath(Transport const*,Position const& start,Position const& end,float,uint32,PointsArray& path,float){
  ++corridors;path.clear();if(next==TransportGroundResult::Complete)path={PositionToVector3(start),PositionToVector3(end)};return next;
 }
}
struct PathGenerator{
 Unit const* _source;uint32 _pointPathLimit=74;PathType _type=PATHFIND_BLANK;Movement::PointsArray _pathPoints;Point end;
 static unsigned terrain,targets;static float length;
 explicit PathGenerator(Unit* unit):_source(unit){}
 void SetStartPosition(Point){}void SetEndPosition(Point p){end=p;}void SetActualEndPosition(Point p){end=p;}
 bool CalculatePath(Position const& p){++terrain;_type=_source->GetTransport()?PATHFIND_NOPATH:PATHFIND_NORMAL;SetEndPosition(PositionToVector3(p));return _type==PATHFIND_NORMAL;}
 bool CalculatePath(float x,float y,float z,bool){return CalculatePath(Position{x,y,z});}
 bool CalculatePathToTarget(Unit const*);
 PathType GetPathType(){return _type;}float GetTotalLength(){return length;}
};
unsigned PathGenerator::terrain=0,PathGenerator::targets=0;float PathGenerator::length=12;
#include "Target.inc"
struct SpellInfo{float GetMaxRange(bool,Unit*){return 25;}};
struct Check{
 Unit* caster;Unit* target;SpellInfo info;
 Unit* GetCaster(){return caster;}Unit* GetExplTargetUnit(){return target;}SpellInfo* GetSpellInfo(){return &info;}
#include "Check.inc"
};
int main(){
 Transport ship,other;Unit player,dummy;player.transport=dummy.transport=&ship;dummy.pos={12,0,0};Check check{&player,&dummy};
#ifdef BOOST_CHARGE_BASELINE
 assert(check.CheckCast()==SPELL_FAILED_NOPATH&&PathGenerator::terrain==1&&!Movement::corridors);
 std::cout<<"PASS pinned7ad477f7 counterfactual: actual Charge script requests terrain path for two ship passengers and returns NoPath at the controlled unavailable-deck navigation boundary.\n";
#else
 for(auto result:{Movement::TransportGroundResult::Complete,Movement::TransportGroundResult::NoPath,Movement::TransportGroundResult::TooLong}){
  Movement::next=result;
  auto expected=result==Movement::TransportGroundResult::Complete?SPELL_CAST_OK:result==Movement::TransportGroundResult::TooLong?SPELL_FAILED_OUT_OF_RANGE:SPELL_FAILED_NOPATH;
  assert(check.CheckCast()==expected);
 }
 assert(PathGenerator::terrain==0&&Movement::corridors==3);
 Movement::next=Movement::TransportGroundResult::Complete;
 player.rooted=true;assert(check.CheckCast()==SPELL_FAILED_ROOTED);player.rooted=false;
 check.target=nullptr;assert(check.CheckCast()==SPELL_FAILED_BAD_TARGETS);check.target=&dummy;
 PathGenerator::length=50;assert(check.CheckCast()==SPELL_FAILED_OUT_OF_RANGE);PathGenerator::length=12;
 player.water=true;unsigned before=Movement::corridors;assert(check.CheckCast()==SPELL_CAST_OK&&Movement::corridors==before);player.water=false;
 dummy.transport=&other;assert(check.CheckCast()==SPELL_FAILED_NOPATH&&PathGenerator::terrain==1); // No common deck shortcut.
 player.transport=dummy.transport=nullptr;assert(check.CheckCast()==SPELL_CAST_OK&&PathGenerator::terrain==2);
 PathGenerator path(&player);assert(!path.CalculatePathToTarget(nullptr));assert(path.CalculatePathToTarget(&dummy)&&PathGenerator::terrain==3);
 player.transport=dummy.transport=&ship;Movement::next=Movement::TransportGroundResult::NoPath;
 assert(!path.CalculatePathToTarget(&dummy)&&path._type==PATHFIND_NOPATH&&path._pathPoints.empty());
 Movement::next=Movement::TransportGroundResult::Complete;
 assert(path.CalculatePathToTarget(&dummy)&&path._type==PATHFIND_NORMAL&&path._pathPoints.size()==2&&path.end.x==12);
 std::cout<<"PASS actual Charge CheckCast and PathGenerator target dispatch: same-deck corridor, failure/length/root/water guards, missing target, different ships and ordinary terrain preserved. Collision model, native movement and client casts tested separately.\n";
#endif
}
