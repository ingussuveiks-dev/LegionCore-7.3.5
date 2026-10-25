#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
enum {CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_GOSSIP=1,
    MOTION_SLOT_IDLE=0,CRITERIA_TYPE_SCRIPT_EVENT_2=92,WAYPOINT_MOTION_TYPE=2,POINT_MOTION_TYPE=8};
constexpr float INTERACTION_DISTANCE=5.0f,PET_FOLLOW_DIST=1.0f,PET_FOLLOW_ANGLE=1.57f;
struct ObjectGuid
{
    uint64 _low,_high;
    ObjectGuid();
    ObjectGuid(uint64 low,uint64 high):_low(low),_high(high){}
    void Clear();
    bool IsEmpty() const;
    bool operator<(ObjectGuid const& other) const {return _high<other._high || (_high==other._high && _low<other._low);}
    bool operator==(ObjectGuid const& other) const {return _high==other._high && _low==other._low;}
};
#include "GuidLifecycle.inc"
struct Map {} liveMap,otherMap;
struct WorldObject
{
    Map* map=&liveMap;
    bool world=true,preDelete=false;
    Map* GetMap() const {return map;}
    bool IsInWorld() const {return world;}
    bool IsPreDelete() const {return preDelete;}
};
enum class GossipOptionNpc {None=0};
struct GossipMenuItem {GossipOptionNpc OptionNpc=GossipOptionNpc::None;uint32 Sender=0,Action=0;bool IsCoded=false;};
struct GossipMenu
{
    GossipMenuItem item;
    bool present=true;
    GossipMenuItem const* GetItem(uint32 index) const {return present && !index?&item:nullptr;}
    uint32 GetMenuId() const {return 19599;}
};
struct PlayerMenu
{
    GossipMenu menu;
    GossipMenu& GetGossipMenu(){return menu;}
    void ClearMenus(){menu.present=false;}
    void SendCloseGossip(){}
};
struct Creature;
struct Player : WorldObject
{
    ObjectGuid guid{123,456};
    PlayerMenu menu;
    PlayerMenu* PlayerTalkClass=&menu;
    bool contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false;
    uint32 klass=2,status=3,instance=42,credits=0,faction=0;
    float distance=1;
    std::function<void()> onCriteria;
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
    void UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92 && event==50897);++credits;if(onCriteria)onCriteria();}
};
struct Motion
{
    uint32 paths=0;
    void MovePath(uint32 path,bool repeat){assert(path==10993806 && !repeat);++paths;}
    void MoveFollow(Creature*,float distance,float,uint32 slot){assert(distance==PET_FOLLOW_DIST && slot==MOTION_SLOT_IDLE);}
};
struct Creature : WorldObject
{
    bool alive=true;
    uint32 entry=106639,mapId=1539,instance=42,flags=0x203,nearestLookups=0;
    Player* nearest=nullptr;
    Motion motion;
    bool IsAlive() const{return alive;}
    uint32 GetEntry() const{return entry;}
    uint32 GetMapId() const{return mapId;}
    uint32 GetInstanceId() const{return instance;}
    bool HasFlag(uint32 field,uint32 mask) const{assert(field==UNIT_FIELD_NPC_FLAGS);return (flags&mask)!=0;}
    void RemoveFlag(uint32 field,uint32 mask){assert(field==UNIT_FIELD_NPC_FLAGS);flags&=~mask;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool live){assert((identity==106371 || identity==106370) && radius==30 && live);return nullptr;}
    Player* FindNearestPlayer(float radius){assert(radius==100);++nearestLookups;return nearest;}
    Motion* GetMotionMaster(){return &motion;}
    float GetPositionX() const{return 0;}
    float GetPositionY() const{return 0;}
    float GetPositionZ() const{return 0;}
    float GetOrientation() const{return 0;}
    void SetHomePosition(float,float,float,float){}
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
struct Scenario {uint32 id=1092,step=4;uint32 GetScenarioId() const{return id;}uint32 GetCurrentStep() const{return step;}};
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
    WaypointPath path=WaypointPath(15,1);
    WaypointPath const* GetPathScript(uint32 identity){assert(identity==10993806);return present?&path:nullptr;}
} waypoints;
auto* sWaypointMgr=&waypoints;
uint32 warnings=0;
template<class... Values>void Warning(Values const&...){++warnings;}
#define TC_LOG_WARN(...) Warning(__VA_ARGS__)
#define CLOSE_GOSSIP_MENU() PlayerTalkClass->SendCloseGossip()
#include "ProcessionPolicy.inc"
struct ProcessionAI
{
    Creature* me;
    ObjectGuid _processionPlayerGuid;
    uint32 doors=0;
    std::vector<uint32> talks;
    void Talk(uint32 line){talks.push_back(line);}
    void VisualOpenDoor(){++doors;}
#include "ProcessionGossip.inc"
#include "ProcessionArrival.inc"
#include "ProcessionMovement.inc"
};
struct Fixture
{
    Player player,other;
    Creature actor;
    ProcessionAI ai{&actor};
    Fixture(){manager={};waypoints={};warnings=0;other.guid._high=457;actor.nearest=&other;HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};}
    void Start(){ai.sGossipSelect(&player,19599,0);assert(actor.motion.paths==1 && !player.credits && !other.credits);}
    void Arrive(){ai.MovementInform(WAYPOINT_MOTION_TYPE,14);}
};
void Invalidate(uint32 invalid,Fixture& fixture)
{
    auto& player=fixture.player;auto& actor=fixture.actor;
    switch(invalid)
    {
        case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.alive=false;break;
        case 3:player.teleport=true;break;case 4:player.klass=1;break;case 5:actor.world=false;break;
        case 6:actor.alive=false;break;case 7:actor.entry=106430;break;case 8:actor.mapId=1220;break;
        case 9:player.map=nullptr;break;case 10:player.map=&otherMap;break;case 11:player.instance=43;break;
        case 12:actor.instance=43;break;case 13:player.phase=false;break;case 14:player.visible=false;break;
        case 15:player.los=false;break;case 16:player.distance=100.01f;break;case 17:player.rewarded=true;break;
        case 18:player.status=0;break;case 19:player.status=1;break;case 20:player.status=5;break;
        case 21:manager.present=false;break;case 22:manager.scenario.id=991;break;case 23:manager.scenario.step=3;break;
        case 24:manager.scenario.step=5;break;case 25:manager.scenario.step=11;break;
        case 26:player.preDelete=true;break;case 27:HashMapHolder<Player>::objects.erase(player.guid);break;
    }
}
int main()
{
#ifdef BASELINE
    for(uint32 invalid:{4u,18u,19u,25u})
    {Fixture fixture;fixture.actor.nearest=&fixture.player;Invalidate(invalid,fixture);fixture.Arrive();fixture.Arrive();assert(fixture.player.credits==2 && fixture.actor.nearestLookups==2);}
    {Fixture fixture;fixture.Arrive();assert(fixture.other.credits==1 && !fixture.actor.motion.paths);}
    std::cout<<"PASS pinned8c95a0ba: real old waypoint14 attempts50897 on neighborhood player even without path start, in4 invalid contexts and twice on repeats. Controlled world/progression, not client/persisted-counter proof.\n";
#else
    for(uint32 invalid=0;invalid<28;++invalid)
    {
        Fixture fixture;fixture.Start();assert(fixture.ai._processionPlayerGuid==fixture.player.guid);Invalidate(invalid,fixture);
        fixture.Arrive();fixture.Arrive();assert(!fixture.player.credits && !fixture.other.credits && fixture.ai._processionPlayerGuid.IsEmpty());
        assert(fixture.ai.talks==std::vector<uint32>{0} && !fixture.actor.nearestLookups && fixture.actor.flags==0x202);
    }
    for(uint32 faction:{0u,1u})
    {
        Fixture fixture;fixture.player.faction=faction;fixture.Arrive();assert(fixture.ai.talks.empty());
        fixture.Start();fixture.player.distance=100;fixture.ai.MovementInform(POINT_MOTION_TYPE,14);fixture.ai.MovementInform(WAYPOINT_MOTION_TYPE,13);
        assert(!fixture.player.credits && fixture.ai._processionPlayerGuid==fixture.player.guid);
        fixture.ai.MovementInform(WAYPOINT_MOTION_TYPE,7);fixture.ai.MovementInform(WAYPOINT_MOTION_TYPE,11);
        fixture.player.onCriteria=[&fixture]{fixture.Arrive();fixture.ai.sGossipSelect(&fixture.player,19599,0);};
        fixture.Arrive();fixture.Arrive();assert(fixture.player.credits==1 && !fixture.other.credits && !fixture.actor.nearestLookups);
        assert(fixture.ai.talks==std::vector<uint32>({0,3,4,5}) && fixture.ai.doors==1 && fixture.actor.motion.paths==1 && fixture.ai._processionPlayerGuid.IsEmpty());
    }
    for(bool missing:{false,true})
    {Fixture fixture;if(missing)waypoints.present=false;else waypoints.path.clear();fixture.ai.sGossipSelect(&fixture.player,19599,0);fixture.Arrive();assert(!fixture.player.credits && !fixture.other.credits && fixture.ai._processionPlayerGuid.IsEmpty());waypoints={};fixture.Start();fixture.Arrive();assert(fixture.player.credits==1);}
    {Fixture fixture;fixture.Start();fixture.player.status=0;fixture.Arrive();fixture.player.status=3;fixture.Arrive();assert(!fixture.player.credits && fixture.actor.motion.paths==1 && fixture.ai._processionPlayerGuid.IsEmpty() && fixture.actor.flags==0x202);}
    {
        Fixture fixture;auto player=std::make_unique<Player>();HashMapHolder<Player>::objects[player->guid]=player.get();
        fixture.ai.sGossipSelect(player.get(),19599,0);HashMapHolder<Player>::objects.erase(player->guid);player.reset();fixture.Arrive();assert(!fixture.other.credits && fixture.ai._processionPlayerGuid.IsEmpty());
    }
    std::cout<<"PASS: actual gossip/complete/full MovementInform and native128-bit GUID/ObjectAccessor map-world-predelete lifecycle;28 invalid arrival contexts, same-low/different-high neighbor, destroyed player, exact100yd, wrong motion/point, stray/repeated/reentrant callbacks, missing-path retry and unchanged door/talk hooks. No progress on request or neighborhood substitution; controlled motion/criteria/faction services are not native persisted-counter, accepted-motion recovery or client proof.\n";
#endif
}
