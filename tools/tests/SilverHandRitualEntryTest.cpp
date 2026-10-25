#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
enum {CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3,TYPEID_PLAYER=4,MOTION_SLOT_IDLE=0};
constexpr float PET_FOLLOW_DIST=1.0f,PET_FOLLOW_ANGLE=1.57f;
struct ObjectGuid
{
    uint64 _low,_high;
    ObjectGuid();
    ObjectGuid(uint64 low,uint64 high):_low(low),_high(high){}
    void Clear();
    bool IsEmpty() const;
    bool operator<(ObjectGuid const& other) const{return _high<other._high || (_high==other._high && _low<other._low);}
};
#include "GuidLifecycle.inc"
struct Map {} liveMap,otherMap;
struct WorldObject
{
    virtual ~WorldObject()=default;
    Map* map=&liveMap;
    bool world=true,preDelete=false;
    Map* GetMap() const{return map;}
    bool IsInWorld() const{return world;}
    bool IsPreDelete() const{return preDelete;}
};
struct Player;
struct Creature;
struct Unit : WorldObject
{
    virtual Player* ToPlayer(){return nullptr;}
    virtual uint32 GetTypeId() const{return 0;}
};
struct Player : Unit
{
    ObjectGuid guid{123,456};
    bool contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,faction=0;
    float distance=1;
    Player* ToPlayer() override{return this;}
    uint32 GetTypeId() const override{return TYPEID_PLAYER;}
    ObjectGuid GetGUID() const{return guid;}
    bool CanContact() const{return contact;}
    bool IsAlive() const{return alive;}
    bool IsBeingTeleported() const{return teleport;}
    uint32 getClass() const{return klass;}
    uint32 GetInstanceId() const{return instance;}
    bool InSamePhase(Creature*) const{return phase;}
    bool canSeeOrDetect(Creature*) const{return visible;}
    bool IsWithinLOSInMap(Creature*) const{return los;}
    bool IsWithinDistInMap(Creature*,float radius) const{return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const{assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const{assert(quest==42120);return status;}
};
struct Motion
{
    uint32 paths=0,follows=0;
    float followAngle=0;
    Creature* followed=nullptr;
    std::function<void()> onMove,onFollow;
    void MovePath(uint32 identity,bool repeat){assert(identity==10993807 && !repeat);++paths;if(onMove)onMove();}
    void MoveFollow(Creature* actor,float distance,float angle,uint32 slot)
    {assert(distance==PET_FOLLOW_DIST && slot==MOTION_SLOT_IDLE);++follows;followed=actor;followAngle=angle;if(onFollow)onFollow();}
};
struct Creature : WorldObject
{
    bool alive=true;
    uint32 entry=106676,mapId=1539,instance=42;
    Creature* efrin=nullptr;
    Creature* duval=nullptr;
    Motion motion;
    std::vector<std::function<void()>> pending;
    bool IsAlive() const{return alive;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return mapId;}
    uint32 GetInstanceId() const{return instance;}
    bool IsWithinDistInMap(Unit* who,float radius) const{return static_cast<Player*>(who)->distance<=radius;}
    Motion* GetMotionMaster(){return &motion;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool live)
    {assert(radius==30 && live && (identity==106371 || identity==106370));return identity==106371?efrin:duval;}
    void AddDelayedEvent(uint32 delay,std::function<void()> callback){assert(delay==1000);pending.push_back(std::move(callback));}
    void Run(){auto callbacks=std::move(pending);pending.clear();for(auto& callback:callbacks)callback();}
};
template<class Object>struct HashMapHolder
{
    static std::map<ObjectGuid,Object*> objects;
    static Object* Find(ObjectGuid guid){auto found=objects.find(guid);return found==objects.end()?nullptr:found->second;}
};
template<class Object>std::map<ObjectGuid,Object*> HashMapHolder<Object>::objects;
class ObjectAccessor
{
public:
#include "WorldPlayer.inc"
#include "MapPlayer.inc"
    static Player* GetPlayer(WorldObject const&,ObjectGuid);
};
#include "AccessorPlayer.inc"
struct Scenario {uint32 id=1092,step=5;uint32 GetScenarioId() const{return id;}uint32 GetCurrentStep() const{return step;}};
struct ScenarioManager
{
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance){return present && instance==42?&scenario:nullptr;}
} manager;
auto* sScenarioMgr=&manager;
using WaypointPath=std::vector<uint32>;
struct WaypointManager
{
    bool present=true;
    WaypointPath path=WaypointPath(22,1);
    WaypointPath const* GetPathScript(uint32 identity){assert(identity==10993807);return present?&path:nullptr;}
} waypoints;
auto* sWaypointMgr=&waypoints;
uint32 warnings=0;
template<class... Values>void Warning(Values const&...){++warnings;}
#define TC_LOG_WARN(...) Warning(__VA_ARGS__)
#include "RitualPolicy.inc"
struct RitualAI
{
    Creature* me;
    bool _introDone=false;
    uint32 talks=0;
    void Talk(uint32 line){assert(line==0);++talks;}
#include "RitualLos.inc"
};
struct Fixture
{
    Player player,other;
    Creature actor,efrin,duval;
    RitualAI ai{&actor};
    Fixture(){manager={};waypoints={};warnings=0;other.guid._high=457;actor.efrin=&efrin;actor.duval=&duval;HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};}
};
void Invalidate(uint32 invalid,Fixture& fixture)
{
    auto& player=fixture.player;auto& actor=fixture.actor;
    switch(invalid)
    {
        case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.alive=false;break;
        case 3:player.teleport=true;break;case 4:player.klass=1;break;case 5:actor.world=false;break;
        case 6:actor.alive=false;break;case 7:actor.entry=106639;break;case 8:actor.mapId=1220;break;
        case 9:player.map=nullptr;break;case 10:player.map=&otherMap;break;case 11:player.instance=43;break;
        case 12:actor.instance=43;break;case 13:player.phase=false;break;case 14:player.visible=false;break;
        case 15:player.los=false;break;case 16:player.distance=20.01f;break;case 17:player.rewarded=true;break;
        case 18:player.status=0;break;case 19:player.status=1;break;case 20:player.status=5;break;
        case 21:manager.present=false;break;case 22:manager.scenario.id=991;break;case 23:manager.scenario.step=4;break;
        case 24:manager.scenario.step=9;break;case 25:manager.scenario.step=11;break;
    }
}
int main()
{
#ifdef BASELINE
    for(uint32 invalid:{0u,10u,18u,25u})
    {Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.player);Invalidate(invalid,fixture);fixture.actor.Run();assert(fixture.actor.motion.paths==1 && fixture.ai._introDone && fixture.ai.talks==1);}
    std::cout<<"PASS pinned952c2ad5: actual old1s callback starts10993807 after original logout/foreign map/abandon/completed scenario without revalidating a recipient. Controlled services, not client proof.\n";
#else
    for(uint32 invalid=0;invalid<26;++invalid)for(bool delayed:{false,true})
    {
        Fixture fixture;if(delayed)fixture.ai.MoveInLineOfSight(&fixture.player);Invalidate(invalid,fixture);
        if(delayed)fixture.actor.Run();else fixture.ai.MoveInLineOfSight(&fixture.player);
        assert(!fixture.actor.motion.paths && !fixture.efrin.motion.follows && !fixture.duval.motion.follows && !fixture.ai.talks && !fixture.ai._introDone && fixture.actor.pending.empty());
    }
    for(uint32 step:{5u,6u,7u,8u})for(uint32 faction:{0u,1u})
    {
        Fixture fixture;manager.scenario.step=step;fixture.player.faction=faction;fixture.player.distance=20;Unit pet,guardian;
        fixture.ai.MoveInLineOfSight(nullptr);fixture.ai.MoveInLineOfSight(&pet);fixture.ai.MoveInLineOfSight(&guardian);assert(!fixture.ai._introDone);
        fixture.actor.motion.onMove=[&fixture]{fixture.ai.MoveInLineOfSight(&fixture.player);};
        fixture.efrin.motion.onFollow=[&fixture]{fixture.ai.MoveInLineOfSight(&fixture.player);};
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.ai.MoveInLineOfSight(&fixture.other);assert(fixture.actor.pending.size()==1 && !fixture.ai.talks);
        fixture.actor.Run();fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run();
        assert(fixture.actor.motion.paths==1 && fixture.ai.talks==1 && fixture.efrin.motion.follows==1 && fixture.duval.motion.follows==1);
        assert(fixture.efrin.motion.followed==&fixture.actor && fixture.duval.motion.followed==&fixture.actor && fixture.efrin.motion.followAngle==-PET_FOLLOW_ANGLE && fixture.duval.motion.followAngle==PET_FOLLOW_ANGLE);
    }
    for(bool preDelete:{false,true})
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.player);
        if(preDelete)fixture.player.preDelete=true;else HashMapHolder<Player>::objects.erase(fixture.player.guid);
        fixture.actor.Run();assert(!fixture.ai._introDone && !fixture.actor.motion.paths);
        fixture.ai.MoveInLineOfSight(&fixture.other);fixture.actor.Run();assert(fixture.actor.motion.paths==1);
    }
    for(bool missing:{false,true})
    {Fixture fixture;if(missing)waypoints.present=false;else waypoints.path.clear();fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run();assert(!fixture.ai._introDone && !fixture.ai.talks && warnings==1);waypoints={};fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run();assert(fixture.actor.motion.paths==1);}
    {Fixture fixture;fixture.actor.efrin=nullptr;fixture.actor.duval=nullptr;fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run();assert(fixture.actor.motion.paths==1);}
    {Fixture fixture;auto player=std::make_unique<Player>();HashMapHolder<Player>::objects[player->guid]=player.get();fixture.ai.MoveInLineOfSight(player.get());HashMapHolder<Player>::objects.erase(player->guid);player.reset();fixture.actor.Run();assert(!fixture.ai._introDone && !fixture.actor.motion.paths);}
    std::cout<<"PASS: actual LOS/helper and native128-bit GUID/ObjectAccessor lifecycle;26 invalid admission/delayed contexts, null/pet/guardian, native5..8 phase envelope,20yd, same-low/different-high neighbor, deleted/predelete original, missing/empty-path retry, missing companions and reentrant/duplicate start checked. Controlled world/motion/faction labels, not later credits, accepted-motion recovery or client proof.\n";
#endif
}
