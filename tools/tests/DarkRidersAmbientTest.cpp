#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <cmath>
using uint32=uint32_t;
constexpr uint32 REACT_PASSIVE=0,POINT_MOTION_TYPE=8,SPELL_AURA_HOVER=1,MOVEMENTFLAG_FALLING=4;
struct Position {float x=-11005,y=-1970,z=-50,o=1;float GetPositionX()const{return x;}float GetPositionY()const{return y;}float GetPositionZ()const{return z;}float GetOrientation()const{return o;}};
enum class CreatureGroundMovementType{None,Run,Hover};
struct Movement{CreatureGroundMovementType Ground=CreatureGroundMovementType::None;int flight=1;
 bool IsGroundAllowed()const{return Ground!=CreatureGroundMovementType::None;}bool IsFlightAllowed()const{return flight!=0;}
 bool CanFly()const{return flight==2;}bool IsGravityDisabled()const{return flight==1;}};
namespace G3D{bool fuzzyGt(float x,float y){return x>y;}bool fuzzyLt(float x,float y){return x<y;}}
struct HeightMap{float ground=-50;float GetHeight(std::set<uint32> const&,float,float,float){return ground;}};
struct Motion {uint32 calls=0,id=0;float x=0,y=0,z=0,speed=0;bool path=true;
 void MovePoint(uint32 point,float a,float b,float c,bool generate,float velocity){++calls;id=point;x=a;y=b;z=c;path=generate;speed=velocity;}};
struct Creature {
 uint32 entry=100835,db=369305,map=1533,flags=33587968,reaction=2,teleports=0;Position home,pos;Motion motion;
 bool m_playerMovingMe=false,fly=false,gravity=false,hover=false,falling=false;Movement movement;HeightMap height;
 uint32 GetMapId(){return map;}uint32 GetEntry(){return entry;}uint32 GetDBTableGUIDLow(){return db;}
 void SetReactState(uint32 value){reaction=value;}void SetDisableGravity(bool value){gravity=value;}
 Position const& GetHomePosition(){return home;}float GetPositionZ(){return pos.z;}
 void NearTeleportTo(float x,float y,float z,float o){pos={x,y,z,o};++teleports;}
 Motion* GetMotionMaster(){return &motion;}
 HeightMap* GetMap(){return &height;}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZMinusOffset(){return pos.z;}
 std::set<uint32> const& GetPhases(){static std::set<uint32> s{5761};return s;}Movement const& GetMovementTemplate(){return movement;}
 bool IsFalling(){return falling;}bool HasAuraType(int){return false;}bool CanHover(){return false;}
 void SetCanFly(bool x){fly=x;}void SetHover(bool x){hover=x;}
 void RemoveUnitMovementFlag(int){falling=false;}bool IsAlive(){return true;}bool CanSwim(){return false;}bool IsInWater(){return false;}void SetSwim(bool){}
 void UpdateMovementFlags();
};
#include "Movement.inc"
struct ScriptedAI {Creature* me;explicit ScriptedAI(Creature* c):me(c){}virtual ~ScriptedAI()=default;virtual void Reset(){}virtual void MovementInform(uint32,uint32){}virtual void UpdateAI(uint32){}};
#include "Ambient.inc"
int main(){
 Creature ghost;DarkRidersAmbient::MotionAI ai(&ghost);ai.Reset();assert(ghost.gravity&&ghost.reaction==0&&ghost.flags==33587968);
 // Actual native movement admission keeps gravity off on floor/air/underground.
 for(float floor:{-60.f,-50.f,-40.f}){ghost.height.ground=floor;ghost.UpdateMovementFlags();assert(ghost.gravity&&!ghost.fly);}
 ai.UpdateAI(100);assert(!ghost.motion.calls);ai.UpdateAI(2000);assert(ghost.motion.calls==1&&!ghost.motion.path&&ghost.motion.z==50&&ghost.motion.speed==1.5f);
 ai.UpdateAI(100000);assert(ghost.motion.calls==1); // don't relaunch unfinished movement
 ai.MovementInform(99,1);ai.MovementInform(POINT_MOTION_TYPE,2);assert(!ghost.teleports);
 for(int i=0;i<4;++i){ghost.pos.z=50;ai.MovementInform(POINT_MOTION_TYPE,1);assert(ghost.pos.z==-50);ai.UpdateAI(250);}
 assert(ghost.teleports==4&&ghost.motion.calls==5);
 Creature upper;upper.pos.z=upper.home.z=49.8f;DarkRidersAmbient::MotionAI top(&upper);top.Reset();top.UpdateAI(3000);assert(upper.teleports==1&&upper.motion.z==50);
 Creature skull;skull.entry=100932;skull.pos.z=skull.home.z=10;DarkRidersAmbient::MotionAI orbit(&skull);orbit.Reset();
 std::set<std::pair<float,float>> destinations;
 for(int i=0;i<12;++i){orbit.UpdateAI(3000);destinations.insert({skull.motion.x,skull.motion.y});assert(!skull.motion.path&&skull.motion.speed==0.7f&&std::abs(skull.motion.x-skull.home.x)<1&&std::abs(skull.motion.y-skull.home.y)<1);assert(skull.motion.z>10&&skull.motion.z<11);orbit.MovementInform(POINT_MOTION_TYPE,1);}
 assert(destinations.size()==4&&!skull.teleports&&skull.flags==33587968);
 for(int mode=0;mode<3;++mode){Creature unrelated;if(mode==0)unrelated.map=0;if(mode==1)unrelated.db=0;if(mode==2)unrelated.entry=100845;DarkRidersAmbient::MotionAI excluded(&unrelated);excluded.Reset();excluded.UpdateAI(10000);assert(!unrelated.motion.calls&&!unrelated.gravity);}
 std::cout<<"PASS: production rise/restart and skull circuits, no ground path, repeated completion, native flight admission, preserved flags and map/spawn/entry isolation; spline rendering remains a client check\n";
}
