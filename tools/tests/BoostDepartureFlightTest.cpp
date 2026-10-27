#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
using uint8=uint8_t;using uint32=uint32_t;using int32=int32_t;using int8=int8_t;
#define TC_GAME_API
#include "CreatureData.h"
enum {CONFIG_CREATURE_STOP_FOR_PLAYER,REACT_PASSIVE,UNIT_FIELD_NPC_FLAGS,UNIT_NPC_FLAG_GOSSIP=1,UNIT_FIELD_NPC_FLAGS2=2,UNIT_NPC_FLAG2_STEERING=4,UNIT_STATE_ROOT=8,SPELL_AURA_HOVER=16,ALLIANCE=0,QUEST_STATUS_NONE=0,SPELL_LEAVE_TUTORIAL=219912};
enum MovementFlags:uint32{MOVEMENTFLAG_CAN_FLY=1,MOVEMENTFLAG_DISABLE_GRAVITY=2,MOVEMENTFLAG_WALKING=4,MOVEMENTFLAG_FALLING=8};
enum class AnimTier{Ground,Hover,Fly};
struct World{int getIntConfig(int){return 0;}}world;auto*sWorld=&world;
#include "MovementDefaults.inc"
struct ObjectGuid{uint32 id=0;bool IsEmpty()const{return !id;}void Clear(){id=0;}bool operator==(ObjectGuid b)const{return id==b.id;}std::string ToString()const{return std::to_string(id);}};
struct Position{float x=0,y=0,z=100;};
struct Player;struct Creature;
struct Transport{unsigned removals=0;void RemovePassenger(struct Unit*);};
struct Map{float ground=0;float GetHeight(std::set<uint32>const&,float,float,float){return ground;}};
namespace G3D{bool fuzzyGt(float a,float b){return a>b;}bool fuzzyLt(float a,float b){return a<b;}}
struct Unit{
 virtual ~Unit()=default;ObjectGuid guid;uint32 flags=0;AnimTier tier=AnimTier::Ground;Transport*ship=nullptr;
 virtual Player*ToPlayer(){return nullptr;}virtual Creature*ToCreature(){return nullptr;}
 bool HasUnitMovementFlag(uint32 f)const{return (flags&f)!=0;}void RemoveUnitMovementFlag(uint32 f){flags&=~f;}
 bool CanSwim(){return true;}ObjectGuid GetTransGUID(){return {ship?10u:0u};}bool HasFlag(int,int){return false;}
 void SetCanFly(bool v){if(v)flags|=MOVEMENTFLAG_CAN_FLY;else flags&=~uint32(MOVEMENTFLAG_CAN_FLY);}
 bool IsLevitating(){return HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);}bool IsHovering(){return false;}bool IsAlive(){return true;}bool HasUnitState(int){return false;}bool IsCreature(){return ToCreature()!=nullptr;}
 void SetAnimTier(AnimTier t){tier=t;}void SetDisableGravity(bool disable,bool updateAnimationTier=true);
 ObjectGuid GetGUID(){return guid;}Transport*GetTransport(){return ship;}
};
void Transport::RemovePassenger(Unit*p){++removals;p->ship=nullptr;}
namespace Movement{
struct Generator{uint32 NewId(){return 1;}}splineIdGen;
struct MoveSplineInit{Unit&unit;struct{uint32 splineId;bool TransformForTransport,walk;struct{bool canSwim,flying,smoothGroundPath,steering;}flags;}args;explicit MoveSplineInit(Unit&);};
#include "SplineConstructor.inc"
}
struct Motion{Creature*owner;unsigned launches=0;bool flying=false;float x=0,y=0,z=0;void MovePoint(int,float,float,float,bool);};
struct Vehicle{unsigned relocations=0;void RelocatePassengers(){++relocations;}};
struct Creature:Unit{
 CreatureMovementData movement;Map map;Position pos;Motion motion{this};Vehicle vehicle;bool kit=true,failKit=false;unsigned casts=0,despawns=0;bool m_playerMovingMe=false;
 Creature(){guid.id=2;}Creature*ToCreature()override{return this;}CreatureMovementData const&GetMovementTemplate(){return movement;}
 void UpdateMovementFlags();Map*GetMap(){return &map;}std::set<uint32>GetPhases(){return {1};}float GetPositionX(){return pos.x;}float GetPositionY(){return pos.y;}float GetPositionZ(){return pos.z;}float GetPositionZMinusOffset(){return pos.z;}
 bool IsFalling(){return HasUnitMovementFlag(MOVEMENTFLAG_FALLING);}bool HasAuraType(int){return false;}void SetHover(bool){}bool CanHover(){return false;}bool IsInWater(){return false;}void SetSwim(bool){}
 void SetReactState(int){}void RemoveFlag(int,int){}void SetFlag(int,int){}Vehicle*GetVehicleKit(){return kit?&vehicle:nullptr;}bool CreateVehicleKit(int id,uint32){assert(id==4933);return kit=!failKit;}uint32 GetEntry(){return 112565;}Position GetPosition(){return pos;}void NearTeleportTo(Position p){pos=p;}void setActive(bool){}Motion*GetMotionMaster(){return &motion;}
 void CastSpell(Creature*target,int spell,bool triggered){assert(target==this&&spell==219912&&triggered);++casts;}void DespawnOrUnsummon(int){++despawns;}
};
void Unit::SetDisableGravity(bool disable,bool updateAnimationTier){if(disable==IsLevitating())return;if(disable)flags|=MOVEMENTFLAG_DISABLE_GRAVITY;else flags&=~uint32(MOVEMENTFLAG_DISABLE_GRAVITY);
#include "AnimationTier.inc"
}
#include "UpdateMovementFlags.inc"
void Motion::MovePoint(int id,float X,float Y,float Z,bool path){assert(id==1&&!path);++launches;x=X;y=Y;z=Z;Movement::MoveSplineInit init(*owner);flying=init.args.flags.flying;}
struct Spline{bool ready=true;bool Finalized(){return ready;}};
struct Quest{};struct QuestStore{Quest q;Quest const*GetQuestTemplate(uint32){return &q;}}quests;auto*sQuestDataStore=&quests;
struct Player:Unit{
 Creature*bird=nullptr;Unit*mover=this;Unit*view=nullptr;Spline spline;Spline*movespline=&spline;unsigned enters=0,restores=0,teleports=0,adds=0;bool boarded=true;uint32 team=ALLIANCE;
 Player(){guid.id=1;}Player*ToPlayer()override{return this;}char const*GetName(){return "trainee";}void StopMoving(){}void EnterVehicle(Creature*c,int seat,bool){assert(!seat);++enters;if(boarded)bird=c;}Creature*GetVehicleBase(){return bird;}Unit*GetUnitBeingMoved(){return mover;}Unit*GetViewpoint(){return view;}
 void SetClientControl(Unit*u,bool enable){mover=u;view=u;if(enable)++restores;}void SetViewpoint(Unit*,bool){view=nullptr;}void SendMovementFlagUpdate(bool){}uint32 GetTeam(){return team;}uint32 GetQuestStatus(uint32){return 0;}bool GetQuestRewardStatus(uint32){return false;}bool CanAddQuest(Quest const*,bool){return true;}void AddQuest(Quest const*,void*){++adds;}void ExitVehicle(){bird=nullptr;}void TeleportTo(int,float,float,float,float){++teleports;}
};
struct Service{unsigned layouts=0;void ArrangeBoostActionBar(Player*){++layouts;}}service;auto*sCharacterService=&service;
struct ObjectAccessor{static Player*player;static Player*GetPlayer(Creature&,ObjectGuid g){return player&&player->guid==g?player:nullptr;}};Player*ObjectAccessor::player=nullptr;
struct ScriptedAI{Creature*me;explicit ScriptedAI(Creature*c):me(c){}virtual ~ScriptedAI()=default;virtual void PassengerBoarded(Unit*,int8,bool){}virtual void SetGUID(ObjectGuid const&,int32=0){}virtual void UpdateAI(uint32){}};
unsigned errors=0;
#define TC_LOG_INFO(...) ((void)0)
#define TC_LOG_ERROR(...) (++errors)
#include "Departure.inc"
int main(){
 // Native flag updater counterfactual: script flags alone do not survive
 // the ground-only default, including Fly -> Ground animation tier reset.
 {Creature bird;bird.SetCanFly(true);bird.SetDisableGravity(true);assert(bird.tier==AnimTier::Fly);bird.UpdateMovementFlags();assert(!bird.flags&&bird.tier==AnimTier::Ground);Movement::MoveSplineInit init(bird);assert(!init.args.flags.flying);}
 for(uint32 team:{0u,1u}){
  Creature bird;bird.movement.Flight=CreatureFlightMovementType::DisableGravity;bird.map.ground=bird.pos.z;Player p;p.team=team;ObjectAccessor::player=&p;DepartureAI ai(&bird);
  Transport ship;bird.ship=p.ship=&ship;ai.SetGUID(p.guid);ai.SetGUID(p.guid);assert(ship.removals==1);
  ai.UpdateAI(500);assert(p.enters==1&&ship.removals==2&&!bird.motion.launches);
  bird.UpdateMovementFlags();assert(!bird.flags&&bird.tier==AnimTier::Ground); // Waiting on deck must remain grounded.
  p.spline.ready=false;for(unsigned i=0;i<5;++i){ai.UpdateAI(100);bird.UpdateMovementFlags();}assert(!bird.motion.launches);
  p.spline.ready=true;ai.UpdateAI(100);assert(bird.motion.launches==1&&bird.motion.flying&&bird.tier==AnimTier::Fly&&p.mover==&bird);
  for(unsigned i=1;i<=20;++i){bird.pos.z=100+float(i);bird.UpdateMovementFlags();assert(bird.IsLevitating()&&bird.tier==AnimTier::Fly);}
  ai.UpdateAI(10000);assert(p.mover==&p&&p.restores==1&&bird.casts==1&&p.adds==1);
  ai.PassengerBoarded(&p,0,false);assert(p.restores==1); // Already restored; no second mover handoff.
  ai.UpdateAI(15000);assert(p.teleports==1&&bird.despawns==1);
 }
 {Creature bird;Player p;ObjectAccessor::player=&p;DepartureAI ai(&bird);p.boarded=false;ai.SetGUID(p.guid);ai.UpdateAI(500);for(unsigned i=0;i<100;++i)ai.UpdateAI(100);assert(!bird.motion.launches&&errors==1);ai.UpdateAI(100);assert(p.teleports==1);}
 {Creature bird;bird.kit=false;bird.failKit=true;Player p;DepartureAI ai(&bird);ai.SetGUID(p.guid);assert(ai.rider.IsEmpty()&&!ai.timer);}
 std::cout<<"PASS full production departure AI and native movement flag updater/spline constructor/animation tier branch: old ground-only flag loss, grounded boarding waits, Fly takeoff, sustained airborne tier, both teams, camera restore, unchanged self219912 and fallback/failed boarding/vehicle kit. Packet/model/queue boundaries controlled; actual client animation/portrait pending.\n";
}
