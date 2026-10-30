#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
using uint32=uint32_t;
using uint64=uint64_t;
enum {CLASS_PALADIN=2,QUEST_STATUS_INCOMPLETE=3,UNIT_FIELD_NPC_FLAGS=1,UNIT_NPC_FLAG_GOSSIP=1,UNIT_STATE_STUNNED=8,
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
struct CreatureAI {virtual ~CreatureAI()=default;virtual void UpdateLifecycle(uint32){}virtual void Reset(){}virtual void MovementInform(uint32,uint32){}virtual void LastWPReached(){}};
struct Player : WorldObject
{
    ObjectGuid guid{123,456};
    PlayerMenu menu;
    PlayerMenu* PlayerTalkClass=&menu;
    bool contact=true,alive=true,teleport=false,phase=true,visible=true,los=true,rewarded=false,gm=false,accept=true;
    uint32 klass=2,status=3,instance=42,credits=0,faction=0;
    float distance=1;
    std::function<void()> onCriteria;
    ObjectGuid GetGUID() const{return guid;}
    bool CanContact() const{return contact;}
    bool isGameMaster() const{return gm;}
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
    void UpdateAchievementCriteria(uint32 type,uint32 event);
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
    bool IsAIEnabled=true,m_AI_locked=false;uint32 states=0;
    CreatureAI* primary=nullptr;CreatureAI* i_disabledAI=nullptr;
    CreatureAI* AI(){return primary;}bool HasUnitState(uint32 bit){return (states&bit)!=0;}
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
    void Tick(uint32);
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
void Player::UpdateAchievementCriteria(uint32 type,uint32 event){assert(type==92 && event==50897);++credits;if(onCriteria)onCriteria();if(accept&&manager.present&&manager.scenario.step==4)manager.scenario.step=5;}
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
#include "silver_hand_procession.h"
namespace SilverHandCompanion {void Recover(Creature*,bool&,bool&) {}}
struct ProcessionAI : CreatureAI
{
    explicit ProcessionAI(Creature* actor):me(actor){}
    Creature* me;
    ObjectGuid _processionPlayerGuid;
    SilverHandProcessionRecovery::State _processionState;
    bool wait_heal=false,_pendingHealRecovery=false;struct Events {void Reset(){}}events;
    uint32 doors=0;
    std::vector<uint32> talks;
    std::function<void()> onTalk;
    void Talk(uint32 line){talks.push_back(line);if(onTalk)onTalk();}
    void VisualOpenDoor(){++doors;}
#include "ProcessionGossip.inc"
#include "ProcessionArrival.inc"
#include "ProcessionMovement.inc"
#if defined(RECOVERY) || defined(RECOVERY_BASELINE)
#include "ProcessionLifecycle.inc"
#include "ProcessionReset.inc"
#endif
};
#if defined(RECOVERY) || defined(RECOVERY_BASELINE)
void Creature::Tick(uint32 diff){CreatureAI* i_AI=primary;
#include "NativeLifecycle.inc"
}
template<class T>struct WaypointMovementGenerator;
template<>struct WaypointMovementGenerator<Creature>{uint32 _currentNode=14;WaypointPath* _path;void MovementInform(Creature&);};
#include "NativeWaypoint.inc"
#endif
struct Fixture
{
    Player player,other;
    Creature actor;
    ProcessionAI ai{&actor};
    Fixture(){manager={};waypoints={};warnings=0;other.guid._high=457;actor.nearest=&other;actor.primary=&ai;HashMapHolder<Player>::objects={{player.guid,&player},{other.guid,&other}};}
    void Start(){ai.sGossipSelect(&player,19599,0);assert(actor.motion.paths==1 && !player.credits && !other.credits);}
    void Arrive(){ai.MovementInform(WAYPOINT_MOTION_TYPE,14);}
#if defined(RECOVERY) || defined(RECOVERY_BASELINE)
    void Tick(uint32 diff){actor.Tick(diff);}
    void NativeArrive(){WaypointMovementGenerator<Creature> waypoint;waypoint._path=&waypoints.path;waypoint.MovementInform(actor);}
#endif
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
#if defined(RECOVERY_BASELINE)
    {Fixture f;f.Start();HashMapHolder<Player>::objects.erase(f.player.guid);f.NativeArrive();HashMapHolder<Player>::objects[f.player.guid]=&f.player;f.Tick(1000);f.Arrive();assert(f.player.credits==0&&f.ai._processionPlayerGuid.IsEmpty());}
    {Fixture f;f.Start();f.player.los=false;f.NativeArrive();f.player.los=true;f.Tick(1000);f.Arrive();assert(f.player.credits==0&&f.ai._processionPlayerGuid.IsEmpty());}
    {Fixture f;f.Start();f.player.accept=false;f.NativeArrive();assert(f.player.credits==1&&manager.scenario.step==4);f.player.accept=true;f.Tick(1000);f.Arrive();assert(f.player.credits==1&&manager.scenario.step==4&&f.ai._processionPlayerGuid.IsEmpty());}
    std::cout<<"PASS3 pinned b06f63842 expected-broken recovery counterfactuals: absent/LOS-ineligible original or rejected event permanently consumes GUID; native waypoint and lifecycle services compiled\n";
#elif defined(BASELINE)
    for(uint32 invalid:{4u,18u,19u,25u})
    {Fixture fixture;fixture.actor.nearest=&fixture.player;Invalidate(invalid,fixture);fixture.Arrive();fixture.Arrive();assert(fixture.player.credits==2 && fixture.actor.nearestLookups==2);}
    {Fixture fixture;fixture.Arrive();assert(fixture.other.credits==1 && !fixture.actor.motion.paths);}
    std::cout<<"PASS pinned8c95a0ba: real old waypoint14 attempts50897 on neighborhood player even without path start, in4 invalid contexts and twice on repeats. Controlled world/progression, not client/persisted-counter proof.\n";
#else
    for(uint32 invalid=0;invalid<28;++invalid)
    {
        Fixture fixture;fixture.Start();assert(fixture.ai._processionPlayerGuid==fixture.player.guid);Invalidate(invalid,fixture);
        fixture.Arrive();fixture.Arrive();assert(!fixture.player.credits && !fixture.other.credits);
        assert(invalid==24?fixture.ai._processionPlayerGuid.IsEmpty():fixture.ai._processionPlayerGuid==fixture.player.guid);
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
    {Fixture fixture;fixture.Start();fixture.player.status=0;fixture.Arrive();fixture.player.status=3;fixture.Arrive();assert(fixture.player.credits==1 && fixture.actor.motion.paths==1 && fixture.ai._processionPlayerGuid.IsEmpty() && fixture.actor.flags==0x202);}
    {
        Fixture fixture;auto player=std::make_unique<Player>();HashMapHolder<Player>::objects[player->guid]=player.get();
        fixture.ai.sGossipSelect(player.get(),19599,0);ObjectGuid original=player->guid;HashMapHolder<Player>::objects.erase(player->guid);player.reset();fixture.Arrive();assert(!fixture.other.credits && fixture.ai._processionPlayerGuid==original);
    }
#ifdef RECOVERY
    uint32 recovered=0;
    for(uint32 invalid:{0u,1u,2u,3u,4u,13u,14u,15u,16u,17u,18u,19u,20u,26u,27u}){
        Fixture f;f.Start();Invalidate(invalid,f);f.NativeArrive();f.Tick(10000);assert(!f.player.credits&&!f.other.credits&&f.ai._processionPlayerGuid==f.player.guid);
        f.player.world=f.player.contact=f.player.alive=f.player.phase=f.player.visible=f.player.los=true;f.player.teleport=f.player.rewarded=f.player.preDelete=false;f.player.klass=2;f.player.status=3;f.player.distance=100;HashMapHolder<Player>::objects[f.player.guid]=&f.player;
        f.Tick(0);assert(f.player.credits==1&&manager.scenario.step==5&&f.ai._processionPlayerGuid.IsEmpty()&&f.ai.talks==std::vector<uint32>({0,5}));f.Tick(1000);f.NativeArrive();assert(f.player.credits==1&&!f.other.credits);++recovered;
    }
    {Fixture f;f.Start();f.player.map=&otherMap;f.NativeArrive();f.Tick(1000);assert(!f.player.credits);f.player.map=&liveMap;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.player.instance=43;f.NativeArrive();assert(!f.player.credits);f.player.instance=42;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.ai.CompleteProcession();f.Tick(100000);assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid&&!f.ai._processionState.arrived);++recovered;}
    {Fixture f;f.Start();f.player.gm=true;f.NativeArrive();f.Tick(100000);assert(!f.player.credits&&f.ai._processionState.attempts==0&&f.ai.talks==std::vector<uint32>{0});f.player.gm=false;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.player.accept=false;f.NativeArrive();assert(f.player.credits==1&&f.ai._processionPlayerGuid==f.player.guid);f.Tick(999);assert(f.player.credits==1);f.player.accept=true;f.Tick(1);assert(f.player.credits==2&&f.ai.talks==std::vector<uint32>({0,5})&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();f.player.accept=false;f.NativeArrive();for(uint32 i=0;i<60;++i)f.Tick(1000);assert(f.player.credits==30&&f.ai._processionPlayerGuid==f.player.guid&&f.ai.talks==std::vector<uint32>({0,5}));manager.scenario.step=5;f.Tick(0);assert(f.ai._processionPlayerGuid.IsEmpty()&&f.player.credits==30);++recovered;}
    {Fixture f;f.Start();f.ai.onTalk=[&]{f.Arrive();f.Tick(0);};f.player.onCriteria=[&]{f.Arrive();f.Tick(0);};f.NativeArrive();assert(f.player.credits==1&&f.ai.talks==std::vector<uint32>({0,5})&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();f.player.onCriteria=[&]{f.player.visible=false;};bool speechVisible=false;f.ai.onTalk=[&]{speechVisible=f.player.visible;};f.NativeArrive();assert(speechVisible&&f.player.credits==1&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();f.ai.onTalk=[&]{HashMapHolder<Player>::objects.erase(f.player.guid);};f.NativeArrive();assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid&&f.ai._processionState.spoken);HashMapHolder<Player>::objects[f.player.guid]=&f.player;f.Tick(0);assert(f.player.credits==1&&f.ai.talks==std::vector<uint32>({0,5}));++recovered;}
    {Fixture f;f.Start();f.ai.onTalk=[&]{f.actor.states=UNIT_STATE_STUNNED;};f.NativeArrive();assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid);f.actor.states=0;f.Tick(0);assert(f.player.credits==1&&f.ai.talks==std::vector<uint32>({0,5}));++recovered;}
    {Fixture f;f.Start();f.ai.onTalk=[&]{f.player.gm=true;};f.NativeArrive();assert(!f.player.credits&&f.ai._processionState.attempts==0);f.player.gm=false;f.Tick(0);assert(f.player.credits==1&&f.ai.talks==std::vector<uint32>({0,5}));++recovered;}
    {Fixture f;auto owner=std::make_unique<Player>();auto replacement=std::make_unique<Player>();HashMapHolder<Player>::objects[owner->guid]=owner.get();f.ai.sGossipSelect(owner.get(),19599,0);f.ai.onTalk=[&]{HashMapHolder<Player>::objects.erase(owner->guid);owner.reset();HashMapHolder<Player>::objects[replacement->guid]=replacement.get();};f.NativeArrive();assert(!owner&&replacement->credits==1&&!f.player.credits&&!f.other.credits&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();CreatureAI temporary;f.ai.onTalk=[&]{f.actor.primary=&temporary;f.actor.i_disabledAI=&f.ai;};f.NativeArrive();assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid);f.actor.primary=&f.ai;f.actor.i_disabledAI=nullptr;f.Tick(0);assert(f.player.credits==1&&f.ai.talks==std::vector<uint32>({0,5}));++recovered;}
    {Fixture f;f.Start();f.ai.onTalk=[&]{manager.scenario.step=5;};f.NativeArrive();assert(!f.player.credits&&f.ai._processionPlayerGuid.IsEmpty()&&f.ai._processionState.completed);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.ai.Reset();f.player.contact=true;f.Tick(0);assert(f.player.credits==1&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.player.contact=true;CreatureAI temporary;f.actor.primary=&temporary;f.actor.i_disabledAI=&f.ai;f.Tick(10000);assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid);f.actor.primary=&f.ai;f.actor.i_disabledAI=nullptr;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.actor.IsAIEnabled=false;f.NativeArrive();f.Tick(10000);assert(!f.player.credits);f.actor.IsAIEnabled=true;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.actor.states=UNIT_STATE_STUNNED;f.NativeArrive();f.Tick(10000);assert(!f.player.credits);f.actor.states=0;f.Tick(0);assert(f.player.credits==1);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.actor.flags|=1;f.ai.sGossipSelect(&f.other,19599,0);assert(f.actor.motion.paths==1&&f.ai._processionPlayerGuid==f.player.guid);f.player.contact=true;f.Tick(0);assert(f.player.credits==1&&!f.other.credits);++recovered;}
    {Fixture f;f.Start();HashMapHolder<Player>::objects.erase(f.player.guid);f.NativeArrive();Player replacement;replacement.guid=f.player.guid;HashMapHolder<Player>::objects[replacement.guid]=&replacement;f.Tick(0);assert(replacement.credits==1&&!f.player.credits&&!f.other.credits&&f.ai._processionPlayerGuid.IsEmpty());++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();HashMapHolder<Player>::objects.erase(f.player.guid);f.Tick(100000);assert(!f.other.credits&&f.ai._processionPlayerGuid==f.player.guid);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();manager.scenario.step=3;f.player.contact=true;f.Tick(10000);assert(!f.player.credits&&f.ai._processionPlayerGuid==f.player.guid);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.actor.preDelete=true;f.player.contact=true;f.Tick(10000);assert(!f.player.credits);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.actor.alive=false;f.player.contact=true;f.Tick(10000);assert(!f.player.credits);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.actor.entry=106429;f.player.contact=true;f.Tick(10000);assert(!f.player.credits);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();f.actor.map=nullptr;f.player.contact=true;f.Tick(10000);assert(!f.player.credits);++recovered;}
    {Fixture f;f.Start();f.player.contact=false;f.NativeArrive();auto dropped=std::make_unique<ProcessionAI>(&f.actor);dropped->_processionPlayerGuid=f.player.guid;dropped->_processionState.arrived=true;dropped.reset();ProcessionAI replacement(&f.actor);f.actor.primary=&replacement;f.player.contact=true;replacement.MovementInform(WAYPOINT_MOTION_TYPE,14);replacement.UpdateLifecycle(10000);assert(!f.player.credits&&!f.other.credits);f.actor.primary=&f.ai;++recovered;}
    std::cout<<"PASS "<<recovered<<" additional ASAN recovery cases, actual primary/suspended lifecycle and native waypoint callback; preserved original GUID/one speech-before-native-phase event, controlled native receipt/visibility, not actual client route or persisted counters\n";
#endif
    std::cout<<"PASS: actual gossip/complete/full MovementInform and native128-bit GUID/ObjectAccessor map-world-predelete lifecycle;28 invalid arrival contexts retain original GUID except genuine advanced5, same-low/different-high neighbor/destroyed owner refusal, exact100yd, wrong motion/point, stray/repeated/reentrant callbacks, missing-path retry and unchanged door/talk hooks. Abandon/reaccept keeps observed receipt. Controlled native-event result is not persisted-counter/group/client proof; dedicated ASAN recovery host exercises lifecycle/timing.\n";
#endif
}
