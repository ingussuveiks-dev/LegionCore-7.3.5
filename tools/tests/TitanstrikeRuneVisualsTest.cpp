#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;
unsigned choice=0;
uint32 urand(uint32 lo,uint32 hi){assert(choice>=lo&&choice<=hi);return choice;}
int32 HacksMorphesOne[31]={},HacksMorphesTwo[31]={};
constexpr uint32 CREATURE_FLAG_EXTRA_TRIGGER=128;
constexpr int SPELL_AURA_HOVER=1,MOVEMENTFLAG_FALLING=4;
struct CreatureTemplate {
 uint32 Entry=106322,flags_extra=0;int32 Modelid[4]={69560,11686,0,0};
 int32 GetRandomValidModelId()const;uint32 GetFirstInvisibleModel()const{return 11686;}
};
struct CreatureData {uint32 displayid=0;};
struct ObjectMgr {static int32 ChooseDisplayId(uint32,CreatureTemplate const*,CreatureData const*);};
#include "RandomModel.inc"
#include "ChooseDisplay.inc"
// Use the production movement enum/data declarations as well as its dispatcher.
using int64=int64_t;
#define TC_GAME_API
template<class E>int AsUnderlyingType(E e){return int(e);}
struct World{int getIntConfig(int){return 0;}}world;
auto sWorld=&world;
constexpr int CONFIG_CREATURE_STOP_FOR_PLAYER=0;
#include "MovementData.inc"
#include "MovementCtor.inc"
namespace G3D {bool fuzzyGt(float a,float b){return a>b;}bool fuzzyLt(float a,float b){return a<b;}}
struct Map {float floor=-36.5f;float GetHeight(std::set<uint32> const&,float,float,float){return floor;}};
struct Creature {
 bool m_playerMovingMe=false,falling=false,gravity=false,fly=false,hover=false;Map map;float z=-28.5f;CreatureMovementData movement;
 Map* GetMap(){return &map;}std::set<uint32> const& GetPhases(){static std::set<uint32> p{6985};return p;}
 float GetPositionX(){return 4890;}float GetPositionY(){return 315;}float GetPositionZMinusOffset(){return z;}
 CreatureMovementData const& GetMovementTemplate(){return movement;}bool IsFalling(){return falling;}bool HasAuraType(int){return false;}
 void SetCanFly(bool b){fly=b;}void SetDisableGravity(bool b){gravity=b;}void SetHover(bool b){hover=b;}
 bool IsAlive(){return true;}bool CanHover(){return false;}void RemoveUnitMovementFlag(int){falling=false;}
 bool CanSwim(){return false;}bool IsInWater(){return false;}void SetSwim(bool){}void UpdateMovementFlags();
};
#include "Movement.inc"
int main(int argc,char** argv){
 assert(argc==2);std::ifstream rows(argv[1]);assert(rows);uint32 entry,display,ground,flight;float z;unsigned count=0,runes=0;
 CreatureTemplate native;CreatureData random;
 choice=1;assert(ObjectMgr::ChooseDisplayId(0,&native,&random)==11686); // actual old invisible choice
 Creature old;old.movement.Flight=CreatureFlightMovementType::CanFly;old.UpdateMovementFlags();assert(old.fly&&!old.gravity);
 while(rows>>entry>>display>>ground>>flight>>z){
  Creature actor;actor.z=z;actor.movement.Ground=static_cast<CreatureGroundMovementType>(ground);actor.movement.Flight=static_cast<CreatureFlightMovementType>(flight);
  for(float height:{-50.f,z,z+1.f}){actor.map.floor=height;actor.UpdateMovementFlags();assert(actor.gravity&&!actor.fly&&!actor.hover);}
  if(entry!=64367){++runes;native.Entry=entry;native.Modelid[0]=69560+(entry-106322);CreatureData pinned{display};
   for(choice=0;choice<2;++choice)assert(ObjectMgr::ChooseDisplayId(0,&native,&pinned)==int32(native.Modelid[0]));
  }++count;
 }
 assert(count==16&&runes==3);
 std::cout<<"PASS: actual SQL16 actor rows through native model selection and movement flags/data; invisible alternate reproduced, all3 explicit rune displays stable, gravity disabled above/on/below sampled floor. Renderer and collision geometry remain client boundaries.\n";
}
