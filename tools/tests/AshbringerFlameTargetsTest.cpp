#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using uint64=uint64_t;
using Milliseconds=std::chrono::milliseconds;
#include "EventMapClass.inc"
#include "EventMapMethods.inc"
enum {POINT_MOTION_TYPE=8,REACT_PASSIVE=0};
struct Position {float positionX=0,positionY=0,positionZ=0,orientation=0;};
#include "FlameTargets.inc"
struct Unit {};
struct MotionMaster
{
    uint32 calls=0;
    void MovePoint(uint32 point,float positionX,float positionY,float positionZ)
    {
        assert(point==1 && positionX==-2622.49f && positionY==23.85f && positionZ==69.47f);++calls;
    }
};
struct Creature
{
    MotionMaster motion;
    bool visible=true,accepted=true;
    std::vector<Position> casts;
    void SetReactState(uint32 state){assert(state==REACT_PASSIVE);}
    MotionMaster* GetMotionMaster(){return &motion;}
    void SetVisible(bool value){visible=value;}
    bool CastSpell(Position const& position,uint32 spell,bool triggered)
    {
        assert(spell==190666 && triggered);casts.push_back(position);return accepted;
    }
};
struct ScriptedAI
{
    Creature* me;
    explicit ScriptedAI(Creature* creature):me(creature){}
    virtual ~ScriptedAI()=default;
    virtual void Reset(){}
    virtual void IsSummonedBy(Unit*){}
    virtual void MovementInform(uint32,uint32){}
    virtual void UpdateAI(uint32){}
};
#include "FlameAI.inc"
void CheckTargets(Creature const& creature,size_t first=0)
{
    for(size_t target=0;target<creature.casts.size();++target)
    {
        auto const& expected=flameTarget[first+target];
        auto const& actual=creature.casts[target];
        assert(actual.positionX==expected.positionX && actual.positionY==expected.positionY && actual.positionZ==expected.positionZ);
    }
}
int main()
{
#ifdef BASELINE
    Creature creature;npc_bs_doomguard_incineratorAI script(&creature);script.IsSummonedBy(nullptr);
    for(uint32 tick=0;tick<13;++tick)script.UpdateAI(1000);
    assert(false);
#else
    for(bool accepted:{false,true})
    {
        Creature creature;creature.accepted=accepted;npc_bs_doomguard_incineratorAI script(&creature);
        script.IsSummonedBy(nullptr);script.UpdateAI(999);assert(creature.casts.empty());script.UpdateAI(1);assert(creature.casts.size()==3);
        for(uint32 tick=1;tick<12;++tick){script.UpdateAI(1000);assert(creature.casts.size()==3*(tick+1));}
        assert(script.flameCastCount==36 && script.events.Empty() && creature.visible && creature.motion.calls==1);CheckTargets(creature);
        for(uint32 tick=0;tick<100;++tick)script.UpdateAI(60000);
        assert(creature.casts.size()==36 && script.flameCastCount==36 && script.events.Empty());
        script.MovementInform(POINT_MOTION_TYPE,1);assert(!creature.visible && script.events.Empty());
    }
    for(uint8 first:{uint8(0),uint8(1),uint8(2),uint8(33),uint8(34),uint8(35),uint8(36),uint8(255)})
    {
        Creature creature;npc_bs_doomguard_incineratorAI script(&creature);script.IsSummonedBy(nullptr);script.flameCastCount=first;
        script.UpdateAI(1000);assert(creature.casts.size()==std::min<size_t>(3,first<36?36-first:0));CheckTargets(creature,first);
        for(uint32 tick=0;tick<40;++tick)script.UpdateAI(1000);
        assert(creature.casts.size()==(first<36?36-first:0) && script.events.Empty());CheckTargets(creature,first);
    }
    for(uint32 arrival=0;arrival<=12;++arrival)
    {
        Creature creature;npc_bs_doomguard_incineratorAI script(&creature);script.IsSummonedBy(nullptr);
        for(uint32 tick=0;tick<arrival;++tick)script.UpdateAI(1000);
        script.MovementInform(POINT_MOTION_TYPE+1,1);script.MovementInform(POINT_MOTION_TYPE,2);assert(creature.visible);
        script.MovementInform(POINT_MOTION_TYPE,1);script.MovementInform(POINT_MOTION_TYPE,1);
        script.UpdateAI(60000);assert(creature.casts.size()==arrival*3 && !creature.visible && script.events.Empty());CheckTargets(creature);
    }
    {
        Creature creature;npc_bs_doomguard_incineratorAI script(&creature);script.IsSummonedBy(nullptr);
        script.UpdateAI(60000);assert(creature.casts.size()==3);script.UpdateAI(0);assert(creature.casts.size()==3);
        script.UpdateAI(999);assert(creature.casts.size()==3);script.UpdateAI(1);assert(creature.casts.size()==6);
        script.IsSummonedBy(nullptr);script.IsSummonedBy(nullptr);script.UpdateAI(1000);assert(creature.casts.size()==9 && script.flameCastCount==3 && creature.motion.calls==3);
    }
    {
        Creature creature;{npc_bs_doomguard_incineratorAI script(&creature);script.IsSummonedBy(nullptr);}
        npc_bs_doomguard_incineratorAI replacement(&creature);replacement.UpdateAI(60000);assert(creature.casts.empty());
    }
    std::cout<<"PASS actual95962 AI/native EventMap: exact36 positions/3-per1000ms/999+1/no lag catch-up; missing or late motion,13 arrival points/wrong callbacks,partial/final/exhausted indices,no uint8 wrap,repeat summon/destruction and rejected-cast policy. Geometry/casts controlled; no live encounter,client VFX or credit proof\n";
#endif
}
