#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
using uint32=uint32_t;using int32=int32_t;
template<class T>using Optional=std::optional<T>;
#include "MovementFlags.inc"
#include "MoveTypes.inc"
constexpr int TYPEID_UNIT=3,TYPEID_PLAYER=4,UNIT_FIELD_FLAGS=0,UNIT_FLAG_PLAYER_CONTROLLED=8;
constexpr int UNIT_STATE_NOT_MOVE=1,UNIT_STATE_FOLLOW_MOVE=2,PATHFIND_NOPATH=1;
namespace G3D{struct Vector3{float x=0,y=0,z=0;};}
struct Position
{
    float m_positionX=0,m_positionY=0,m_positionZ=0;
    float GetPositionX()const{return m_positionX;}float GetPositionY()const{return m_positionY;}float GetPositionZ()const{return m_positionZ;}
    void GetPosition(float&x,float&y,float&z)const{x=m_positionX;y=m_positionY;z=m_positionZ;}
    float GetExactDistSq(Position const* p)const{return std::pow(m_positionX-p->m_positionX,2)+std::pow(m_positionY-p->m_positionY,2)+std::pow(m_positionZ-p->m_positionZ,2);}
    static float NormalizeOrientation(float a){while(a<0)a+=float(2*M_PI);return std::fmod(a,float(2*M_PI));}
};
namespace Movement
{
#include "SelectSpeed.inc"
struct MoveSpline
{
    struct Flags{bool done=false;}splineflags;
    struct Path{int last(){return 2;}}spline;
    bool walk=false;int point_Idx=0,time_passed=0;
    bool Finalized()const{return splineflags.done;}bool isWalking()const{return walk;}
    int Duration(){return 1000;}void _Finalize();
};
#include "Finalize.inc"
}
struct MovementInfo{uint32 flags=0;bool HasMovementFlag(uint32 f)const{return (flags&f)!=0;}};
struct Creature;struct Pet;
struct Unit:Position
{
    int guid=1,master=0,type=TYPEID_PLAYER;bool inWorld=true,alive=true,combat=false,vehicle=false,controlled=false,guardian=false,pet=false,gm=false,sameMap=true,casting=false;
    uint32 states=0;float orientation=0,objectSize=0.5f,reach=0.5f;
    float speeds[MAX_MOVE_TYPE]={2.5f,7,4.5f,4.722222f,2.5f,3.14159f,7,4.5f,3.14159f};
    Movement::MoveSpline splineStorage;Movement::MoveSpline* movespline=&splineStorage;MovementInfo m_movementInfo;
    int stops=0,launches=0,informs=0;float launchedVelocity=0,lastAngle=0;bool launchedWalk=false,pathSuccess=true,noPath=false;
    G3D::Vector3 destination;
    virtual ~Unit()=default;virtual Creature* ToCreature(){return nullptr;}virtual Pet* ToPet(){return nullptr;}
    Unit* ToUnit(){return this;}Unit* ToPlayer(){return this;}
    bool IsInWorld()const{return inWorld;}bool IsAlive()const{return alive;}bool isInCombat()const{return combat;}
    bool IsVehicle()const{return vehicle;}bool HasFlag(int,int)const{return controlled;}
    bool isPet()const{return pet;}bool isGuardian()const{return guardian;}bool isGameMaster()const{return gm;}
    int GetGUID()const{return guid;}int GetOwnerGUID()const{return master;}int GetCharmerOrOwnerGUID()const{return master;}int GetCritterGUID()const{return 88;}
    int GetTypeId()const{return type;}uint32 GetUnitMovementFlags()const{return m_movementInfo.flags;}
    float GetSpeed(UnitMoveType t)const{return speeds[t];}float GetObjectSize()const{return objectSize;}float GetCombatReach()const{return reach;}
    bool IsInMap(Unit*)const{return sameMap;}bool HasUnitState(uint32 s)const{return (states&s)!=0;}bool IsMovementPreventedByCasting()const{return casting;}
    void AddUnitState(uint32 s){states|=s;}void ClearUnitState(uint32 s){states&=~s;}void StopMoving(){++stops;movespline->splineflags.done=true;}
    bool IsStopped(){return movespline->Finalized();}void SetFacingTo(float a){orientation=a;}
    Position GetPosition()const{return *this;}float GetOrientation()const{return orientation;}float ToAbsoluteAngle(float a)const{return Position::NormalizeOrientation(a+orientation);}
    float GetExactDistSq(float x,float y,float z)const{return std::pow(m_positionX-x,2)+std::pow(m_positionY-y,2)+std::pow(m_positionZ-z,2);}
    float GetDistance2d(float x,float y)const{return std::hypot(m_positionX-x,m_positionY-y);}
    void MovePositionToFirstCollision(Position& p,float distance,float angle)
    {lastAngle=angle;p.m_positionX+=std::cos(angle+orientation)*distance;p.m_positionY+=std::sin(angle+orientation)*distance;}
    void UpdateAllowedPositionZ(float&,float&,float&){}
};
struct Creature:Unit
{
    Creature(){type=TYPEID_UNIT;}Creature* ToCreature()override{return this;}
};
struct Pet:Creature
{
    Pet(){pet=guardian=true;}Pet* ToPet()override{return this;}
};
struct PathGenerator
{
    Unit* owner;G3D::Vector3 end;explicit PathGenerator(Unit* u):owner(u){}
    void Clear(){}bool CalculatePath(float x,float y,float z,bool){end={x,y,z};return owner->pathSuccess;}
    int GetPathType(){return owner->noPath?PATHFIND_NOPATH:0;}
    G3D::Vector3 GetActualEndPosition(){return end;}std::vector<G3D::Vector3> GetPath(){return {end};}
};
namespace Movement
{
struct MoveSplineInit
{
    Unit& owner;bool walk=false;Optional<float> speed;G3D::Vector3 end;
    explicit MoveSplineInit(Unit& u):owner(u){walk=u.m_movementInfo.HasMovementFlag(MOVEMENTFLAG_WALKING);}
    void MovebyPath(std::vector<G3D::Vector3> const& p){end=p.back();}void SetWalk(bool v){walk=v;}void SetVelocity(float v){speed=v;}
    void Launch(){++owner.launches;owner.launchedWalk=walk;owner.launchedVelocity=speed?*speed:owner.GetSpeed(walk?MOVE_WALK:MOVE_RUN);owner.destination=end;owner.movespline->walk=walk;owner.movespline->splineflags.done=false;}
};
}
struct Timer{int left;Timer(int v=0):left(v){}void Reset(int v){left=v;}void Update(uint32 d){left-=d;}bool Passed(){return left<=0;}int GetExpiry(){return left;}};
using TimeTrackerSmall=Timer;
template<class T>struct FollowMovementGenerator
{
    Unit* followedTarget;std::unique_ptr<PathGenerator> _path;Optional<Position> _lastTargetPosition,_lastPredictedPosition;
    Timer _recheckPredictedDistanceTimer;bool _recheckPredictedDistance=false;float _range=3,_angle=float(M_PI);
    explicit FollowMovementGenerator(Unit* u):followedTarget(u){}
    Unit* GetTarget()const{return followedTarget;}bool IsTargetValid()const{return followedTarget!=nullptr;}
    bool EnableWalking()const;bool DoUpdate(T&,uint32);bool PositionOkay(Unit*,bool,bool&,uint32);
    void MovementInform(Creature& owner){++owner.informs;}
};
#include "MoveType.inc"
#include "Walking.inc"
#include "Velocity.inc"
#include "Predict.inc"
#include "PositionOkay.inc"
#include "Update.inc"
bool Near(float a,float b){return std::abs(a-b)<0.001f;}
int main()
{
    Unit player;player.guid=50;player.m_movementInfo.flags=MOVEMENTFLAG_FORWARD;
    // The actual spline completion method retains 'walk'. Reproduce a finished
    // scripted walk followed by client-controlled running without a new spline.
    player.movespline->walk=true;player.movespline->_Finalize();assert(player.movespline->isWalking());
    Pet pet;pet.master=50;pet.m_positionX=-2;
    assert(GetFollowTargetMoveType(&player)==MOVE_RUN);
    assert(Near(*GetVelocity(&pet,&player,{0,0,0},true),7));
    assert(Near(PredictPosition(&player).m_positionX,3.5f));
    player.movespline->splineflags.done=false;
    pet.m_positionX=-1;
    assert(GetFollowTargetMoveType(&player)==MOVE_WALK&&Near(*GetVelocity(&pet,&player,{0,0,0},true),2.5f));
    assert(Near(PredictPosition(&player).m_positionX,1.25f));
    player.movespline->_Finalize();
    player.m_movementInfo.flags=MOVEMENTFLAG_FORWARD|MOVEMENTFLAG_WALKING;
    assert(GetFollowTargetMoveType(&player)==MOVE_WALK);
    player.m_movementInfo.flags=MOVEMENTFLAG_SWIMMING|MOVEMENTFLAG_FORWARD;assert(GetFollowTargetMoveType(&player)==MOVE_SWIM);
    player.m_movementInfo.flags=MOVEMENTFLAG_FLYING|MOVEMENTFLAG_FORWARD;assert(GetFollowTargetMoveType(&player)==MOVE_FLIGHT);
    player.m_movementInfo.flags=MOVEMENTFLAG_BACKWARD;assert(GetFollowTargetMoveType(&player)==MOVE_RUN_BACK);
    player.m_movementInfo.flags=MOVEMENTFLAG_FORWARD;pet.m_positionX=-20;
    assert(*GetVelocity(&pet,&player,{0,0,0},true)>7); // Existing catch-up remains active.
    pet.combat=true;assert(!GetVelocity(&pet,&player,{0,0,0},true));pet.combat=false;
    pet.vehicle=true;assert(!GetVelocity(&pet,&player,{0,0,0},true));pet.vehicle=false;
    pet.controlled=true;assert(!GetVelocity(&pet,&player,{0,0,0},true));pet.controlled=false;
    Creature stranger;stranger.master=999;assert(!GetVelocity(&stranger,&player,{0,0,0},false));
    for(float boost:{1.f,1.5f,2.f}){player.speeds[MOVE_RUN]=7*boost;pet.m_positionX=-2;assert(Near(*GetVelocity(&pet,&player,{0,0,0},true),7*boost));}
    player.speeds[MOVE_RUN]=7;pet.m_positionX=-20;
    pet.m_movementInfo.flags=MOVEMENTFLAG_WALKING; // Previous pet walk must not leak into the next spline.
    FollowMovementGenerator<Creature> follow(&player);
    assert(follow.DoUpdate(pet,100));assert(pet.launches==1&&!pet.launchedWalk&&pet.launchedVelocity>7);
    assert(Near(player.lastAngle,float(M_PI)));assert(Near(pet.destination.x,-3.5f));
    follow.DoUpdate(pet,100);assert(Near(pet.destination.x,0));
    // Explicit follow angle, not numeric follow range, determines the destination.
    FollowMovementGenerator<Creature> side(&player);side._angle=float(M_PI/2);side.DoUpdate(pet,100);
    assert(Near(player.lastAngle,float(M_PI/2))&&Near(pet.destination.y,3.5f));
    // Root/casting/map/death and inaccessible paths retain native stopping gates.
    pet.states|=UNIT_STATE_NOT_MOVE;int before=pet.launches;side.DoUpdate(pet,100);assert(pet.launches==before&&pet.stops);
    pet.states=0;pet.casting=true;side.DoUpdate(pet,100);assert(pet.launches==before);pet.casting=false;
    player.inWorld=false;assert(!side.DoUpdate(pet,100));player.inWorld=true;
    pet.sameMap=false;assert(!side.DoUpdate(pet,100));pet.sameMap=true;
    pet.alive=false;assert(!side.DoUpdate(pet,100));pet.alive=true;
    pet.pathSuccess=false;FollowMovementGenerator<Creature> noPath(&player);noPath.DoUpdate(pet,100);assert(pet.launches==before);
    std::cout<<"PASS: native completed-walk lifecycle, follow speed/prediction/full update and angle dispatch; run/walk/swim/flight/backwards, boosts, catch-up, combat, possession, casting/root/map/death/path boundaries. Client follow smoothness remains pending.\n";
}
