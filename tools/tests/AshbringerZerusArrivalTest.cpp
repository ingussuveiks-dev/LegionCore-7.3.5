#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using uint64=uint64_t;
using Milliseconds=std::chrono::milliseconds;
enum {POINT_MOTION_TYPE=8,SPELL_INQUISITOR_BEAM=192596,NPC_DARK_INQUISITOR=91697};
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
struct Position
{
    float positionX=0,positionY=0,positionZ=0,orientation=0;
    float GetOrientation() const{return orientation;}
};
#include "ZerusPositions.inc"
struct Creature;
struct CreatureAI {virtual ~CreatureAI()=default;virtual void MovementInform(uint32,uint32)=0;};
struct MotionMaster
{
    uint32 requested=0;
    Position destination;
    void MovePoint(uint32 point,Position const& position){requested=point;destination=position;}
};
struct Creature
{
    CreatureAI* script=nullptr;
    MotionMaster motion;
    float distance0=1,distance1=2,facing=-1;
    uint32 stopCalls=0;
    std::vector<Position> helpers;
    CreatureAI* AI(){return script;}
    void StopAttack(){++stopCalls;}
    float GetDistance(Position const& position){return &position==&zerusSumPos[0]?distance0:distance1;}
    MotionMaster* GetMotionMaster(){return &motion;}
    void SetFacingTo(float orientation){facing=orientation;}
    Creature* SummonCreature(uint32 entry,Position const& position){assert(entry==91697);helpers.push_back(position);return nullptr;}
};
struct BossAI:CreatureAI
{
    Creature* me;
    EventMap events;
    uint32 casts=0;
    explicit BossAI(Creature* creature):me(creature){creature->script=this;}
    void DoCast(Creature* target,uint32 spell,bool triggered){assert(target==me && spell==192596 && triggered);++casts;}
#include "ZerusArrival.inc"
#include "ZerusRequest.inc"
};
template<class Owner>struct PointMovementGenerator
{
    uint32 _movementId;
    void MovementInform(Owner& owner);
};
#include "PointArrival.inc"
int main(int argc,char** argv)
{
#ifdef BASELINE
    Creature creature;BossAI script(&creature);
    uint32 point=argc==2 && std::string(argv[1])=="overflow"?3:0;
    PointMovementGenerator<Creature> arrival{point};arrival.MovementInform(creature);
    if(point==0){assert(script.casts==1 && creature.helpers.size()==1 && creature.facing==-1 && script.events.GetNextEventTime()==1000);std::cout<<"PASS pinned07bd3831: native POINT8/id0 spuriously beams/summons and schedules recovery without an issued helper point\n";return 0;}
    assert(false);
#else
    for(uint32 type:{0u,8u,17u})for(uint32 point:{0u,3u,4u,256u,std::numeric_limits<uint32>::max()})
    {
        Creature creature;BossAI script(&creature);script.events.ScheduleEvent(EVENT_4,2000);
        script.MovementInform(type,point);
        assert(script.casts==0 && creature.helpers.empty() && creature.facing==-1 && script.events.GetNextEventTime()==2000);
    }
    for(uint32 type:{0u,17u})for(uint32 point:{1u,2u})
    {
        Creature creature;BossAI script(&creature);script.MovementInform(type,point);
        assert(script.casts==0 && creature.helpers.empty() && creature.facing==-1 && script.events.Empty());
    }
    for(uint32 branch=0;branch<3;++branch)
    {
        Creature creature;BossAI script(&creature);creature.distance0=1.0f+float(branch);creature.distance1=2;
        script.IssueMove();assert(creature.motion.requested==(branch?2u:1u) && creature.stopCalls==1);
        uint32 point=creature.motion.requested;PointMovementGenerator<Creature> arrival{point};arrival.MovementInform(creature);
        assert(script.casts==1 && creature.helpers.size()==1 && creature.facing==zerusSumPos[point-1].orientation);
        Position expected=zerusSumPos[1+point],actual=creature.helpers.front();
        assert(actual.positionX==expected.positionX && actual.positionY==expected.positionY && actual.positionZ==expected.positionZ && actual.orientation==expected.orientation);
        script.events.Update(999);assert(script.events.ExecuteEvent()==0);script.events.Update(1);assert(script.events.ExecuteEvent()==EVENT_4 && script.events.Empty());
        arrival.MovementInform(creature);assert(script.casts==2 && creature.helpers.size()==2);
        creature.script=nullptr;arrival.MovementInform(creature);assert(script.casts==2);
    }
    std::cout<<"PASS actual91672 request/arrival/native Point bridge/EventMap: invalid POINT IDs and other motion types produce no beam/facing/helper/timer mutation; both native distance branches retain points1/2,orientation/91697 placement and1000ms recovery. Duplicate valid callbacks retain existing replay; no request epoch/once-only/client proof\n";
#endif
}
