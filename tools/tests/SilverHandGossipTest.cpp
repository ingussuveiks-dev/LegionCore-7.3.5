#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using uint32=uint32_t;
enum { CLASS_PALADIN=2, QUEST_STATUS_INCOMPLETE=3, UNIT_FIELD_NPC_FLAGS=1, UNIT_NPC_FLAG_NONE=0,
    UNIT_NPC_FLAG_GOSSIP=1, UNIT_STATE_DIED=8, SPELL_AURA_FEIGN_DEATH=1 };
constexpr float INTERACTION_DISTANCE=5.0f;
constexpr float PET_FOLLOW_DIST=1.0f,PET_FOLLOW_ANGLE=1.57f;
enum { MOTION_SLOT_IDLE=0 };
#ifdef SECOND_TRAVARD
constexpr uint32 ExpectedEntry=106639,ExpectedMenu=19599,ExpectedStep=4,ExpectedPath=10993806,ExpectedPoints=15;
constexpr bool SecondActor=true;
#else
constexpr uint32 ExpectedEntry=106429,ExpectedMenu=19580,ExpectedStep=1,ExpectedPath=10993804,ExpectedPoints=11;
constexpr bool SecondActor=false;
#endif
enum class GossipOptionNpc : uint8_t { None=0, Vendor=1 };
struct GossipMenuItem {GossipOptionNpc OptionNpc=GossipOptionNpc::None;uint32 Sender=0,Action=0;bool IsCoded=false;};
using GossipMenuItemContainer=std::map<uint32,GossipMenuItem>;
struct GossipMenu
{
    GossipMenuItemContainer _menuItems;
    std::map<uint32,uint32> _menuItemData;
    uint32 _menuId=ExpectedMenu;
    uint32 GetMenuId() const {return _menuId;}
    GossipMenuItem const* GetItem(uint32) const;
    uint32 GetMenuItemSender(uint32) const;
    uint32 GetMenuItemAction(uint32) const;
    void ClearMenu();
};
struct QuestMenu {void ClearMenu(){}};
struct PlayerMenu
{
    GossipMenu _gossipMenu;
    QuestMenu _questMenu;
    uint32 closes=0;
    GossipMenu& GetGossipMenu();
    uint32 GetGossipOptionSender(uint32) const;
    uint32 GetGossipOptionAction(uint32) const;
    void ClearMenus();
    void SendCloseGossip() {++closes;}
};
#include "MapLookup.inc"
#include "MenuNative.inc"
struct ObjectGuid
{
    uint32 kind=1,counter=ExpectedEntry;
    bool IsCreatureOrVehicle() const {return kind==1;}
    bool IsGameObject() const {return kind==2;}
    std::string ToString() const {return "controlled-guid";}
};
struct Player;
struct Creature;
struct GameObject;
struct Map {GameObject* GetGameObject(ObjectGuid) {return nullptr;}} liveMap,otherMap;
struct WorldObject
{
    Map* map=&liveMap;
    bool world=true,alive=true;
    uint32 instance=42;
    bool IsInWorld() const {return world;}
    bool IsAlive() const {return alive;}
    Map* GetMap() const {return map;}
    uint32 GetInstanceId() const {return instance;}
};
struct CreatureAI
{
    uint32 talks=0;
    virtual ~CreatureAI()=default;
    virtual void sGossipSelect(Player*,uint32,uint32) {}
    virtual void sGossipSelectCode(Player*,uint32,uint32,char const*) {}
    virtual void Talk(uint32 line) {assert(line==0);++talks;}
};
struct Motion
{
    uint32 paths=0,follows=0;
    Creature* followed=nullptr;
    float followAngle=0;
    std::function<void()> onMove,onFollow;
    void MovePath(uint32 path,bool repeat) {assert(path==ExpectedPath && !repeat);++paths;if(onMove)onMove();}
    void MoveFollow(Creature* actor,float distance,float angle,uint32 slot)
    {assert(distance==PET_FOLLOW_DIST && slot==MOTION_SLOT_IDLE);++follows;followed=actor;followAngle=angle;if(onFollow)onFollow();}
};
struct Creature : WorldObject
{
    struct Template {uint32 ScriptID=1;} definition;
    uint32 entry=ExpectedEntry,mapId=1539,flags=0x203,LastUsedScriptID=1;
    Creature* efrin=nullptr;
    Creature* duval=nullptr;
    CreatureAI* ai=nullptr;
    Motion motion;
    uint32 GetEntry() const {return entry;}
    uint32 GetMapId() const {return mapId;}
    bool HasFlag(uint32 field,uint32 mask) const {assert(field==UNIT_FIELD_NPC_FLAGS);return (flags&mask)!=0;}
    void RemoveFlag(uint32 field,uint32 mask) {assert(field==UNIT_FIELD_NPC_FLAGS);flags&=~mask;}
    void SetUInt32Value(uint32 field,uint32 value) {assert(field==UNIT_FIELD_NPC_FLAGS);flags=value;}
    Motion* GetMotionMaster() {return &motion;}
    CreatureAI* AI() {return ai;}
    Template* GetCreatureTemplate() {return &definition;}
    Creature* FindNearestCreature(uint32 identity,float radius,bool live)
    {assert(radius==30 && live && (identity==106371 || identity==106370));return identity==106371?efrin:duval;}
};
struct Player : WorldObject
{
    PlayerMenu* PlayerTalkClass=nullptr;
    Creature* actor=nullptr;
    bool contact=true,teleport=false,phase=true,visible=true,los=true,rewarded=false,interaction=true;
    uint32 klass=2,status=3,fallbacks=0,lastMask=999,faction=0;
    float distance=1;
    bool CanContact() const {return contact;}
    bool IsBeingTeleported() const {return teleport;}
    uint32 getClass() const {return klass;}
    bool InSamePhase(Creature*) const {return phase;}
    bool canSeeOrDetect(Creature*) const {return visible;}
    bool IsWithinLOSInMap(Creature*) const {return los;}
    bool IsWithinDistInMap(Creature*,float radius) const {return distance<=radius;}
    bool GetQuestRewardStatus(uint32 quest) const {assert(quest==42120);return rewarded;}
    uint32 GetQuestStatus(uint32 quest) const {assert(quest==42120);return status;}
    bool HasUnitState(uint32) const {return false;}
    void RemoveAurasByType(uint32) {}
    Creature* GetNPCIfCanInteractWith(ObjectGuid guid,uint32 mask)
    {lastMask=mask;return interaction && guid.counter==ExpectedEntry && distance<=INTERACTION_DISTANCE?actor:nullptr;}
    void OnGossipSelect(WorldObject*,uint32,uint32) {++fallbacks;}
};
struct Scenario {uint32 id=1092,step=ExpectedStep;uint32 GetScenarioId() const{return id;}uint32 GetCurrentStep() const{return step;}};
struct ScenarioManager
{
    bool present=true;
    Scenario scenario;
    Scenario* GetScenario(uint32 instance) {return present && instance==42?&scenario:nullptr;}
} manager;
auto* sScenarioMgr=&manager;
using WaypointPath=std::vector<uint32>;
struct WaypointManager
{
    bool present=true;
    WaypointPath path=WaypointPath(ExpectedPoints,1);
    WaypointPath const* GetPathScript(uint32 identity) {assert(identity==ExpectedPath);return present?&path:nullptr;}
} waypoints;
auto* sWaypointMgr=&waypoints;
uint32 warnings=0,debugs=0;
template<class... Values>void Warning(Values const&...){++warnings;}
template<class... Values>void Debug(Values const&...){++debugs;}
#define TC_LOG_WARN(...) Warning(__VA_ARGS__)
#define TC_LOG_DEBUG(...) Debug(__VA_ARGS__)
#define CLOSE_GOSSIP_MENU() PlayerTalkClass->SendCloseGossip()
#include "EntrancePolicy.inc"
struct EntranceAI : CreatureAI
{
    Creature* me=nullptr;
    bool firstwp=true;
#include "GossipSelect.inc"
};
struct CreatureScript
{
    virtual ~CreatureScript()=default;
    virtual bool OnGossipSelect(Player*,Creature*,uint32,uint32) {return false;}
};
struct Script : CreatureScript
{
#include "LegacyGossip.inc"
} script;
struct GameObject : WorldObject
{
    struct Info {uint32 ScriptId=1;} info;
    struct AIType {void GossipSelect(Player*,uint32,uint32){} void GossipSelectCode(Player*,uint32,uint32,char const*){}} ai;
    uint32 LastUsedScriptID=1;
    Info* GetGOInfo(){return &info;}
    AIType* AI(){return &ai;}
};
struct ScriptManager
{
    bool OnGossipSelect(Player* player,Creature* actor,uint32 sender,uint32 action){return script.OnGossipSelect(player,actor,sender,action);}
    bool OnGossipSelect(Player*,GameObject*,uint32,uint32){return false;}
    template<class Object>bool OnGossipSelectCode(Player*,Object*,uint32,uint32,char const*){return false;}
} scripts;
auto* sScriptMgr=&scripts;
struct Logger {template<class... Values>void outDiff(Values const&...){}} logger;
auto* sLog=&logger;
uint32 getMSTime(){return 0;}
uint32 GetMSTimeDiffToNow(uint32){return 0;}
namespace WorldPackets {namespace NPC {struct GossipSelectOption {ObjectGuid GossipUnit;uint32 GossipID=ExpectedMenu,GossipIndex=0;std::string PromotionCode;};}}
struct WorldSession
{
    Player* currentPlayer=nullptr;
    Player* GetPlayer(){return currentPlayer;}
    void HandleGossipSelectOption(WorldPackets::NPC::GossipSelectOption&);
};
#include "PacketNative.inc"
struct Fixture
{
    PlayerMenu menu;
    Player player;
    Creature actor,efrin,duval;
    EntranceAI ai;
    WorldSession session;
    WorldPackets::NPC::GossipSelectOption packet;
    Fixture(){manager={};waypoints={};warnings=0;player.PlayerTalkClass=&menu;player.actor=&actor;ai.me=&actor;actor.ai=&ai;actor.efrin=&efrin;actor.duval=&duval;session.currentPlayer=&player;menu._gossipMenu._menuItems[0]={};}
    void Send(){session.HandleGossipSelectOption(packet);}
};
void Invalidate(uint32 invalid,Fixture& fixture)
{
    auto& player=fixture.player;auto& actor=fixture.actor;auto& menu=fixture.menu._gossipMenu;
    switch(invalid)
    {
        case 0:player.world=false;break;case 1:player.contact=false;break;case 2:player.alive=false;break;
        case 3:player.teleport=true;break;case 4:player.klass=1;break;case 5:actor.world=false;break;
        case 6:actor.alive=false;break;case 7:actor.entry=106430;break;case 8:actor.mapId=1220;break;
        case 9:player.map=nullptr;break;case 10:player.map=&otherMap;break;case 11:player.instance=43;break;
        case 12:actor.instance=43;break;case 13:player.phase=false;break;case 14:player.visible=false;break;
        case 15:player.los=false;break;case 16:player.distance=5.01f;break;case 17:player.rewarded=true;break;
        case 18:player.status=0;break;case 19:player.status=1;break;case 20:player.status=5;break;
        case 21:manager.present=false;break;case 22:manager.scenario.id=991;break;case 23:manager.scenario.id=1037;break;
        case 24:manager.scenario.step=ExpectedStep-1;break;case 25:manager.scenario.step=ExpectedStep+1;break;case 26:manager.scenario.step=11;break;
        case 27:if(SecondActor)manager.scenario.step=0;else fixture.ai.firstwp=false;break;
        case 28:actor.flags&=~1u;break;case 29:menu._menuId=123;break;
        case 30:menu._menuItems.clear();break;case 31:menu._menuItems[0].OptionNpc=GossipOptionNpc::Vendor;break;
        case 32:menu._menuItems[0].Sender=1;break;case 33:menu._menuItems[0].Action=1;break;
        case 34:menu._menuItems[0].IsCoded=true;break;case 35:fixture.packet.GossipID=123;break;
        case 36:fixture.packet.GossipIndex=999;break;case 37:fixture.packet.PromotionCode="unexpected";break;
        case 38:fixture.session.currentPlayer=nullptr;break;case 39:fixture.packet.GossipUnit.counter=999;break;
        case 40:actor.definition.ScriptID=2;break;
    }
}
int main()
{
#ifdef BASELINE
    for(uint32 invalid:{18u,24u,27u,28u,30u,35u,36u})
    {
        Fixture fixture;Invalidate(invalid,fixture);fixture.Send();
        assert(fixture.player.lastMask==UNIT_NPC_FLAG_NONE && fixture.actor.motion.paths==1 && fixture.actor.flags==0);
    }
    Fixture repeated;repeated.Send();repeated.Send();assert(repeated.actor.motion.paths==2);
    std::cout<<"PASS pinned "<<(SecondActor?"cf8ab051 second":"357347d7 first")<<": actual packet dispatch/zero-default missing option calls old unguarded CreatureScript and starts path for7 invalid contexts; repeated packet restarts path and clears unrelated flags. Controlled world/motion, not client traversal.\n";
#else
    for(uint32 invalid=0;invalid<41;++invalid)
    {Fixture fixture;Invalidate(invalid,fixture);fixture.Send();assert(!fixture.actor.motion.paths && !fixture.ai.talks && !fixture.efrin.motion.follows && !fixture.duval.motion.follows);}
    {Fixture fixture;fixture.ai.sGossipSelect(nullptr,ExpectedMenu,0);assert(!fixture.actor.motion.paths);}
    {Fixture fixture;fixture.player.distance=5.01f;fixture.ai.sGossipSelect(&fixture.player,ExpectedMenu,0);assert(!fixture.actor.motion.paths);}
    for(bool missing:{false,true})
    {
        Fixture fixture;if(missing)waypoints.present=false;else waypoints.path.clear();fixture.Send();
        assert(warnings==1 && !fixture.actor.motion.paths && fixture.actor.flags==0x203 && fixture.menu._gossipMenu.GetItem(0));
        waypoints={};fixture.Send();assert(fixture.actor.motion.paths==1);
    }
    for(uint32 faction:{0u,1u})
    {
        Fixture fixture;fixture.player.faction=faction;fixture.player.distance=5;
        fixture.actor.motion.onMove=[&fixture]{fixture.Send();};
        fixture.efrin.motion.onFollow=[&fixture]{fixture.Send();};fixture.duval.motion.onFollow=[&fixture]{fixture.Send();};
        fixture.Send();fixture.Send();
        assert(fixture.actor.motion.paths==1 && fixture.actor.flags==0x202 && fixture.ai.firstwp);
        assert(!fixture.menu._gossipMenu.GetItem(0) && fixture.menu.closes==1);
        assert(fixture.ai.talks==uint32(SecondActor) && fixture.efrin.motion.follows==uint32(SecondActor) && fixture.duval.motion.follows==uint32(SecondActor));
        if(SecondActor){assert(fixture.efrin.motion.followed==&fixture.actor && fixture.duval.motion.followed==&fixture.actor);assert(fixture.efrin.motion.followAngle==-PET_FOLLOW_ANGLE && fixture.duval.motion.followAngle==PET_FOLLOW_ANGLE);}
    }
    {
        Fixture fixture;fixture.player.status=0;fixture.Send();assert(!fixture.actor.motion.paths && fixture.actor.flags==0x203);
        fixture.player.status=3;fixture.Send();assert(fixture.actor.motion.paths==1);
        PlayerMenu otherMenu;Player other;other.PlayerTalkClass=&otherMenu;other.actor=&fixture.actor;otherMenu._gossipMenu._menuItems[0]={};
        fixture.session.currentPlayer=&other;fixture.Send();assert(fixture.actor.motion.paths==1);
    }
    {Fixture fixture;fixture.actor.efrin=nullptr;fixture.actor.duval=nullptr;fixture.Send();assert(fixture.actor.motion.paths==1);}
    std::cout<<"PASS: actual native packet handler/menu lookup+clear and production "<<(SecondActor?"second":"first")<<" Travard AI selection;41 invalid contexts, missing/empty-path retry, direct null/5.01yd, exact5yd, reentrant follower/movement/replayed/other-player selection, original roles and unrelated flags checked. Native interaction/world/path services controlled; no physical movement, group story or client proof.\n";
#endif
}
