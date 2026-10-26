#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PetFollowConstants.inc"
enum {COMMAND_STAY,COMMAND_MOVE_TO,COMMAND_FOLLOW,RATE_TARGET_POS_RECALCULATION_RANGE};
struct ObjectGuid
{
    uint64 _high=0,_low=0;
    bool operator==(ObjectGuid const&) const;
    bool operator<(ObjectGuid const&) const;
};
#include "GuidOperators.inc"
float frand(float low,float high){return (low+high)/2.0f;}
struct CharmInfo
{
    uint32 command=COMMAND_FOLLOW;
    bool atStay=false,returning=false,attack=false,following=false;
    bool HasCommandState(uint32 value) const{return command==value;}
    bool IsAtStay() const{return atStay;}
    bool IsReturning() const{return returning;}
    bool IsCommandAttack() const{return attack;}
    bool IsFollowing() const{return following;}
    void SetIsReturning(bool value){returning=value;}
    void GetStayPosition(float& x,float& y,float& z){x=1;y=2;z=3;}
};
struct Owner;
struct MotionMaster
{
    uint32 clearCount=0,followCount=0,pointCount=0;
    float angle=float(PET_FOLLOW_ANGLE),distance=PET_FOLLOW_DIST;
    Owner* target=nullptr;
    void Clear(){++clearCount;}
    void MovePoint(uint32,float,float,float){++pointCount;}
    void MoveFollow(Owner* owner,float range,float orientation){++followCount;target=owner;distance=range;angle=orientation;}
};
struct Creature
{
    ObjectGuid guid;
    Owner* owner=nullptr;
    float followAngle=float(PET_FOLLOW_ANGLE),followDistance=PET_FOLLOW_DIST;
    CharmInfo charm;
    MotionMaster motion;
    ObjectGuid GetGUID() const{return guid;}
    uint32 GetGUIDLow() const{return uint32(guid._low);}
    void SetFollowAngle(float angle){followAngle=angle;}
    void SetFollowDistance(float distance){followDistance=distance;}
    float GetFollowAngle() const{return followAngle;}
    float GetFollowDistance() const{return followDistance;}
    CharmInfo* GetCharmInfo(){return &charm;}
    MotionMaster* GetMotionMaster(){return &motion;}
    Owner* GetCharmerOrOwner(){return owner;}
    float GetDistance(Owner*){return 10;}
};
using Minion=Creature;
struct Owner
{
    using ControlList=std::set<ObjectGuid>;
    ControlList m_Controlled;
    ObjectGuid primary;
    std::map<ObjectGuid,Creature*> world;
    ObjectGuid GetPetGUID() const{return primary;}
    void AssignFormation(Minion*,bool);
};
struct ObjectAccessor
{
    static Creature* GetCreatureOrPetOrVehicle(Owner& owner,ObjectGuid const& guid){auto found=owner.world.find(guid);return found==owner.world.end()?nullptr:found->second;}
};
struct World {float getRate(uint32){return 1;}};
World world;
World* sWorld=&world;
struct PetAI {Creature* me;void HandleReturnMovement();};
#include "Formation.inc"
#include "PetReturn.inc"
void EnterWorld(Owner& owner,Creature& creature){creature.owner=&owner;owner.world[creature.guid]=&creature;PetAI{&creature}.HandleReturnMovement();}
void CheckPair(Creature const& pet,Creature const& hati)
{
    assert(pet.followAngle==float(PET_FOLLOW_ANGLE) && hati.followAngle==float(3*PET_FOLLOW_ANGLE));
    assert(pet.motion.angle==pet.followAngle && hati.motion.angle==hati.followAngle);
    float petX=std::cos(pet.motion.angle)*pet.motion.distance,petY=std::sin(pet.motion.angle)*pet.motion.distance;
    float hatiX=std::cos(hati.motion.angle)*hati.motion.distance,hatiY=std::sin(hati.motion.angle)*hati.motion.distance;
    assert(std::hypot(petX-hatiX,petY-hatiY)>0.9f);
}
int main()
{
#ifdef BASELINE
    Owner owner;Creature pet,hati;pet.guid={10,1};hati.guid={8,2};owner.primary=pet.guid;
    owner.m_Controlled.insert(pet.guid);EnterWorld(owner,pet);owner.m_Controlled.insert(hati.guid);
    owner.AssignFormation(&hati,true);EnterWorld(owner,hati);
    assert(pet.motion.angle==hati.motion.angle && pet.followAngle==hati.followAngle);
    std::cout<<"PASS pinned6cdf37b6 counterfactual: pre-registration Hati skipped, primary/secondary both retain identical follow angle and projected follow destination\n";
#else
    for(uint64 variant=0;variant<5;++variant)
    {
        Owner owner;Creature pet,hati;pet.guid={10,1};hati.guid={8+variant%2,100+variant};owner.primary=pet.guid;
        owner.m_Controlled.insert(pet.guid);owner.AssignFormation(&pet,true);EnterWorld(owner,pet);
        float primaryAngle=pet.motion.angle;owner.m_Controlled.insert(hati.guid);owner.AssignFormation(&hati,true);EnterWorld(owner,hati);
        CheckPair(pet,hati);assert(pet.motion.angle==primaryAngle && pet.motion.followCount==1);
        uint32 petMotion=pet.motion.followCount,hatiMotion=hati.motion.followCount;
        owner.AssignFormation(&hati,true);assert(pet.motion.followCount==petMotion && hati.motion.followCount==hatiMotion);CheckPair(pet,hati);
        owner.world.erase(pet.guid);owner.m_Controlled.erase(pet.guid);owner.primary={};owner.AssignFormation(&pet,false);
        owner.primary=pet.guid;owner.m_Controlled.insert(pet.guid);owner.AssignFormation(&pet,true);
        pet.charm.returning=false;EnterWorld(owner,pet);CheckPair(pet,hati);
        owner.world.erase(hati.guid);owner.m_Controlled.erase(hati.guid);owner.AssignFormation(&hati,false);
        assert(pet.followAngle==float(PET_FOLLOW_ANGLE));
        owner.m_Controlled.insert(hati.guid);owner.AssignFormation(&hati,true);hati.charm.returning=false;EnterWorld(owner,hati);CheckPair(pet,hati);
    }
    for(uint32 count=1;count<=7;++count)
    {
        Owner owner;std::vector<Creature> summons(count);
        for(uint32 index=0;index<count;++index){summons[index].guid={8,index+1};owner.m_Controlled.insert(summons[index].guid);if(index+1<count)owner.world[summons[index].guid]=&summons[index];}
        owner.AssignFormation(&summons.back(),true);
        std::set<float> angles;
        for(auto const& summon:summons){angles.insert(summon.followAngle);assert(std::isfinite(summon.followDistance) && summon.followDistance>0 && summon.motion.followCount==0);}
        assert(angles.size()==count);
        owner.world[summons.back().guid]=&summons.back();owner.AssignFormation(&summons.back(),false);assert(summons.back().motion.clearCount==0);
    }
    {
        Owner owner;Creature pet,secondary;pet.guid={10,1};secondary.guid={8,1};owner.primary=pet.guid;
        owner.m_Controlled={secondary.guid,pet.guid,ObjectGuid{8,2}};owner.world[secondary.guid]=&secondary;
        owner.AssignFormation(&pet,true);assert(pet.followAngle==float(PET_FOLLOW_ANGLE) && secondary.followAngle==float(3*PET_FOLLOW_ANGLE));
        owner.m_Controlled.clear();owner.AssignFormation(&pet,false);
    }
    for(uint32 command:{COMMAND_STAY,COMMAND_MOVE_TO,COMMAND_FOLLOW})
    {
        Owner owner;Creature creature;creature.guid={8,1};creature.owner=&owner;creature.charm.command=command;
        owner.m_Controlled.insert(creature.guid);owner.AssignFormation(&creature,true);assert(creature.motion.clearCount==0);
        PetAI{&creature}.HandleReturnMovement();assert(creature.motion.pointCount==(command==COMMAND_STAY?1u:0u));assert(creature.motion.followCount==(command==COMMAND_FOLLOW?1u:0u));
    }
    std::cout<<"PASS actual SetMinion formation/native full-GUID order/native PetAI return: incoming pre-registration companion participates; primary pet stays first and existing follow is not restarted; Hati-like guardians use opposite side, recall/removal/re-add and1..7/no-primary/missing actors covered. Stay/move-to preserved. Projected destinations only, no actual client path/collision/combat/VFX proof\n";
#endif
}
