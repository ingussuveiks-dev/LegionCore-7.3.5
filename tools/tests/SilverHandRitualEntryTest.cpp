#include <cassert>
#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
using FunctionList=std::multimap<uint64,std::function<void()>>;
#include "FunctionClass.inc"
#include "FunctionMethods.inc"
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
struct CreatureAI
{
    virtual ~CreatureAI()=default;
    virtual void Reset(){}
    virtual void UpdateLifecycle(uint32){}
};
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
    bool m_AI_locked=false,IsAIEnabled=true;
    CreatureAI* i_AI=nullptr;
    CreatureAI* i_disabledAI=nullptr;
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
    CreatureAI* AI(){return i_AI;}
    bool AIM_Destroy();
    void Run(uint32 diff=1000)
    {
        auto callbacks=std::move(pending);pending.clear();for(auto& callback:callbacks)callback();
#include "LifecycleBridge.inc"
    }
};
uint32 debugMessages=0;
#define TC_LOG_DEBUG(...) ++debugMessages
#define ASSERT(value,...) assert(value)
#include "DestroyAI.inc"
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
struct EventMap {void Reset(){}};
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct RitualAI : CreatureAI
{
    Creature* me;
    bool _introDone=false;
    bool wait_heal=false,_pendingHealRecovery=false;
    FunctionProcessor _entryFunctions;
    uint64 _entryGeneration=0;
    bool _entryPending=false;
    EventMap events;
    uint32 talks=0;
    void CompleteRitualArrival(){}
    explicit RitualAI(Creature* actor):me(actor){actor->i_AI=this;}
    void Talk(uint32 line){assert(line==0);++talks;}
#include "RitualLos.inc"
#include "RitualReset.inc"
#include "RitualLifecycle.inc"
};
struct Fixture
{
    Player player,other;
    Creature actor,efrin,duval;
    RitualAI ai{&actor};
    Fixture(){manager={};waypoints={};warnings=0;other.guid._high=457;actor.efrin=&efrin;actor.duval=&duval;HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};}
    uint32 Pending(){return uint32(actor.pending.size())+ai._entryFunctions.Size()+ai._entryFunctions.SizeQueue();}
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
#elif defined(LIFETIME_BASELINE)
    Fixture fixture;
    alignas(RitualAI) unsigned char storage[sizeof(RitualAI)];
    auto original=new(storage) RitualAI(&fixture.actor);
    original->MoveInLineOfSight(&fixture.player);assert(fixture.actor.pending.size()==1);
    original->~RitualAI();auto replacement=new(storage) RitualAI(&fixture.actor);
    fixture.actor.Run();assert(fixture.actor.motion.paths==1 && replacement->talks==1 && !replacement->_introDone);
    replacement->~RitualAI();fixture.actor.i_AI=nullptr;
    std::cout<<"PASS pinned4bc7b7e6: actual Unit-owned1s callback survives destruction/reconstruction of AI at same address and starts an unsolicited path on replacement AI. Controlled transparent same-type replacement avoids dereferencing freed storage; not a live crash/client reproduction.\n";
#else
    for(uint32 invalid=0;invalid<26;++invalid)for(bool delayed:{false,true})
    {
        Fixture fixture;if(delayed)fixture.ai.MoveInLineOfSight(&fixture.player);Invalidate(invalid,fixture);
        if(delayed)fixture.actor.Run();else fixture.ai.MoveInLineOfSight(&fixture.player);
        assert(!fixture.actor.motion.paths && !fixture.efrin.motion.follows && !fixture.duval.motion.follows && !fixture.ai.talks && !fixture.ai._introDone && fixture.Pending()==0);
    }
    for(uint32 step:{5u,6u,7u,8u})for(uint32 faction:{0u,1u})
    {
        Fixture fixture;manager.scenario.step=step;fixture.player.faction=faction;fixture.player.distance=20;Unit pet,guardian;
        fixture.ai.MoveInLineOfSight(nullptr);fixture.ai.MoveInLineOfSight(&pet);fixture.ai.MoveInLineOfSight(&guardian);assert(!fixture.ai._introDone);
        fixture.actor.motion.onMove=[&fixture]{fixture.ai.MoveInLineOfSight(&fixture.player);};
        fixture.efrin.motion.onFollow=[&fixture]{fixture.ai.MoveInLineOfSight(&fixture.player);};
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.ai.MoveInLineOfSight(&fixture.other);assert(fixture.Pending()==1 && fixture.actor.pending.empty() && !fixture.ai.talks);
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
    for(uint32 mode:{0u,1u,2u,3u})
    {
        Fixture fixture;CreatureAI temporary;
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run(999);
        assert(!fixture.actor.motion.paths && fixture.ai._entryPending);
        if(mode==1 || mode==3){fixture.actor.i_AI=mode==1?&temporary:nullptr;fixture.actor.i_disabledAI=&fixture.ai;}
        if(mode==2)fixture.actor.i_disabledAI=&fixture.ai;
        fixture.actor.motion.onMove=[&]{assert(fixture.actor.m_AI_locked);fixture.ai.Reset();};
        fixture.actor.Run(1);assert(fixture.actor.motion.paths==1 && !fixture.ai._entryPending && fixture.ai._introDone);
        fixture.actor.Run(1000);assert(fixture.actor.motion.paths==1);
        fixture.actor.i_disabledAI=nullptr;
    }
    {
        Fixture fixture;fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run(500);fixture.ai.Reset();
        assert(!fixture.ai._entryPending && !fixture.ai._introDone && fixture.ai._entryGeneration==1);
        fixture.ai.MoveInLineOfSight(&fixture.other);fixture.actor.Run(500);
        assert(!fixture.actor.motion.paths && fixture.ai._entryPending && fixture.ai._introDone);
        fixture.actor.Run(500);assert(fixture.actor.motion.paths==1 && !fixture.ai._entryPending);
        fixture.ai.Reset();assert(fixture.ai._introDone && fixture.ai._entryGeneration==1);
        fixture.ai.MoveInLineOfSight(&fixture.player);fixture.actor.Run();assert(fixture.actor.motion.paths==1);
    }
    {
        Fixture fixture;auto original=new RitualAI(&fixture.actor);original->MoveInLineOfSight(&fixture.player);
        assert(fixture.actor.pending.empty() && fixture.actor.AIM_Destroy() && !fixture.actor.i_AI);
        fixture.actor.Run();assert(!fixture.actor.motion.paths);
        auto replacement=std::make_unique<RitualAI>(&fixture.actor);
        fixture.actor.Run();assert(!fixture.actor.motion.paths);
        replacement->MoveInLineOfSight(&fixture.player);
        fixture.actor.motion.onMove=[&]{auto before=debugMessages;assert(!fixture.actor.AIM_Destroy() && debugMessages==before+1);};
        fixture.actor.Run();assert(fixture.actor.motion.paths==1);
        fixture.actor.i_AI=nullptr;
    }
    std::cout<<"PASS: actual LOS/helper/reset/lifecycle, native FunctionProcessor/GUID/ObjectAccessor/locked Creature bridge/AIM_Destroy;26 invalid initial/delayed contexts, native5..8/20yd, missing-path/companions/deleted-player and reentrant admission retained. AI-owned999+1ms queue, primary/disabled/alias, stale Reset generation with fresh pending retry, accepted Reset without replay, native destruction/replacement and locked destruction rejection checked. Controlled world/motion/faction labels and companion-heal stub, not later credits, accepted-motion recovery or client proof.\n";
#endif
}
