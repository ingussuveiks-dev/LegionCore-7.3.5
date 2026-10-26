#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using uint8=uint8_t;
using uint16=uint16_t;
using uint32=uint32_t;
using int32=int32_t;
using TaxiMask=std::array<uint8,16>;
namespace WorldPackets{namespace Taxi{struct ShowTaxiNodes;}}
struct Player;
#include "Tokenizer.inc"
#include "PlayerTaxiClass.inc"
struct ObjectManager
{
    std::map<std::pair<uint32,uint32>,std::pair<uint32,uint32>> routes;
    bool mount=true;
    void GetTaxiPath(uint32 from,uint32 to,uint32& path,uint32& cost){auto found=routes.find({from,to});path=found==routes.end()?0:found->second.first;cost=found==routes.end()?0:found->second.second;}
    uint32 GetTaxiMountDisplayId(uint32,uint32,bool){return mount?101:0;}
};
ObjectManager manager;
ObjectManager* sObjectMgr=&manager;
#include "PlayerTaxiMethods.inc"
struct TaxiPathNodeEntry
{
    struct Location {float X=0,Y=0,Z=0;} Loc;
    uint16 ContinentID=1220;
    uint8 NodeIndex=0;
};
using TaxiPathNodeList=std::vector<TaxiPathNodeEntry const*>;
std::vector<TaxiPathNodeList> sTaxiPathNodesByPath;
constexpr float SKIP_SPLINE_POINT_DISTANCE_SQ=1600;
enum {UNIT_STATE_IN_FLIGHT=1,UNIT_FIELD_FLAGS=1,PLAYER_FIELD_PLAYER_FLAGS=2,UNIT_FLAG_REMOVE_CLIENT_CONTROL=1,UNIT_FLAG_TAXI_FLIGHT=2,PLAYER_FLAGS_TAXI_BENCHMARK=4,CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING=1,FLIGHT_MOTION_TYPE=7};
struct MovementInfo {};
namespace WorldPackets{namespace Movement{struct MoveSplineDone {MovementInfo movementInfo;uint16 GetOpcode() const{return 1;}};}}
struct MovementGenerator {virtual ~MovementGenerator()=default;};
struct MotionMaster
{
    MovementGenerator* generator=nullptr;
    uint32 type=FLIGHT_MOTION_TYPE;
    uint32 GetCurrentMovementGeneratorType(){return type;}
    MovementGenerator* top(){return generator;}
};
struct Spline {uint32 index=0;uint32 currentPathIdx() const{return index;}};
struct HostileReferences {bool online=false;void setOnlineOfflineState(bool enabled){online=enabled;}};
struct Player
{
    PlayerTaxi m_taxi;
    struct {bool inHostileArea=false;} pvpInfo;
    bool flight=true,mounted=true,atTop=true;
    uint32 flags=3,benchmark=4,map=1220,zoneUpdates=0,stops=0,fallUpdates=0,petResummons=0,battlePetResummons=0,casts=0,teleports=0,criteria=0;
    int32 spent=0;
    Spline spline;
    Spline* movespline=&spline;
    MotionMaster motion;
    HostileReferences hostile;
    void ClearUnitState(uint32 state){assert(state==UNIT_STATE_IN_FLIGHT);flight=false;}
    void Dismount(){mounted=false;}
    void RemoveFlag(uint32 field,uint32 value){if(field==UNIT_FIELD_FLAGS)flags&=~value;else benchmark&=~value;}
    void StopMoving(){assert(!flight || atTop);++stops;}
    HostileReferences& getHostileRefManager(){return hostile;}
    float GetPositionZ() const{return 100;}
    float GetOrientation() const{return 0;}
    uint32 GetMapId() const{return map;}
    void SetFallInformation(uint32,float){++fallUpdates;}
    void ResummonPetTemporaryUnSummonedIfAny(){++petResummons;}
    void SummonLastSummonedBattlePet(){++battlePetResummons;}
    void GetZoneAndAreaId(uint32& zone,uint32& area){zone=1;area=2;}
    void UpdateZone(uint32,uint32){assert(!flight && !mounted && !flags && hostile.online);++zoneUpdates;}
    void CastSpell(Player* target,uint32 spell,bool triggered){assert(target==this && spell==2479 && triggered);++casts;}
    void UpdateAchievementCriteria(uint32 type,uint32){assert(type==CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING);++criteria;}
    void ModifyMoney(int32 value){spent-=value;}
    void ValidateMovementInfo(MovementInfo*){}
    MotionMaster* GetMotionMaster(){return &motion;}
    bool TeleportTo(uint32 continent,float,float,float,float){map=continent;++teleports;return true;}
    void UpdateFallInformationIfNeed(MovementInfo const&,uint16){}
    void CleanupAfterTaxiFlight(bool=true);
};
#include "PlayerCleanup.inc"
struct FlightPathMovementGenerator:MovementGenerator
{
    struct TaxiNodeChangeInfo {uint32 PathIndex;int32 Cost;};
    TaxiPathNodeList _path;
    uint32 _currentNode=0,_preloadTargetNode=0,preloads=0,events=0;
    std::deque<TaxiNodeChangeInfo> _pointsForPathSwitch;
    void LoadPath(Player&,uint32=0);
    void DoFinalize(Player&);
    bool DoUpdate(Player&,uint32);
    void DoEventIfAny(Player&,TaxiPathNodeEntry const&,bool){++events;}
    void PreloadEndGrid(){++preloads;}
    void SetCurrentNodeAfterTeleport();
    uint32 GetCurrentNode() const{return _currentNode;}
    TaxiPathNodeList const& GetPath() const{return _path;}
    void SkipCurrentNode(){++_currentNode;}
};
#include "FlightMethods.inc"
struct TaxiNodesEntry {uint32 ContinentID=1220;};
struct NodeStore
{
    TaxiNodesEntry node;
    TaxiNodesEntry const* LookupEntry(uint32){return &node;}
} sTaxiNodesStore;
struct WorldSession
{
    Player* _player;
    uint32 relocations=0;
    Player* GetPlayer(){return _player;}
    void RelocateMover(MovementInfo const&){++relocations;}
    void HandleMoveSplineDone(WorldPackets::Movement::MoveSplineDone&);
};
#include "ClientAck.inc"
struct Fixture
{
    Player player;
    FlightPathMovementGenerator flight;
    std::array<TaxiPathNodeEntry,9> nodes;
    Fixture(bool multi=false)
    {
        manager.routes={{{100,200},{1,2}},{{200,300},{2,3}}};manager.mount=true;sTaxiPathNodesByPath.assign(3,{});sTaxiNodesStore.node.ContinentID=1220;
        for(uint32 index=0;index<nodes.size();++index){nodes[index].Loc.X=float(index*100);nodes[index].NodeIndex=uint8(index);}
        sTaxiPathNodesByPath[1]={&nodes[0],&nodes[1],&nodes[2]};
        sTaxiPathNodesByPath[2]={&nodes[2],&nodes[3],&nodes[4],&nodes[5],&nodes[6]};
        assert(player.m_taxi.LoadTaxiDestinationsFromString(multi?"100 200 300 ":"100 200 ",0));
        flight.LoadPath(player);player.motion.generator=&flight;
    }
    void Arrive(){player.spline.index=uint32(flight._path.size()-1);assert(!flight.DoUpdate(player,100));assert(player.m_taxi.GetPath().size()==1 && flight._pointsForPathSwitch.empty());}
    void Finalize(){player.atTop=false;flight.DoFinalize(player);}
};
int main()
{
#ifdef BASELINE
    Fixture fixture;fixture.Arrive();fixture.Finalize();PlayerTaxi reloaded;
    assert(fixture.player.m_taxi.SaveTaxiDestinationsToString()=="200 " && !reloaded.LoadTaxiDestinationsFromString("200 ",0));
    assert(!fixture.player.flight && !fixture.player.mounted && fixture.player.zoneUpdates==0 && !fixture.player.hostile.online);
    std::cout<<"PASS pinned5ec963d5: actual path completion/native finalize without client ACK saves terminal200-only route; unchanged native loader rejects it on next login. No user character edited\n";
#else
    for(bool multi:{false,true})for(bool hostile:{false,true})for(bool ackFirst:{false,true})
    {
        Fixture fixture(multi);fixture.player.pvpInfo.inHostileArea=hostile;fixture.Arrive();
        WorldSession session{&fixture.player};WorldPackets::Movement::MoveSplineDone ack;
        if(ackFirst)session.HandleMoveSplineDone(ack);
        fixture.Finalize();PlayerTaxi reloaded;
        assert(fixture.player.m_taxi.empty() && fixture.player.m_taxi.SaveTaxiDestinationsToString().empty() && reloaded.LoadTaxiDestinationsFromString("",0));
        assert(!fixture.player.flight && !fixture.player.mounted && !fixture.player.flags && !fixture.player.benchmark && fixture.player.hostile.online);
        assert(fixture.player.zoneUpdates==1 && fixture.player.casts==(hostile?1u:0u) && fixture.player.petResummons==1 && fixture.player.battlePetResummons==1 && fixture.player.fallUpdates>=1);
        session.HandleMoveSplineDone(ack);assert(fixture.player.zoneUpdates==1 && fixture.player.casts==(hostile?1u:0u));
        if(multi)assert(fixture.player.spent==3 && fixture.player.criteria==1);
    }
    for(bool multi:{false,true})
    {
        Fixture fixture(multi);std::string saved=fixture.player.m_taxi.SaveTaxiDestinationsToString();fixture.Finalize();
        PlayerTaxi reloaded;assert(fixture.player.m_taxi.SaveTaxiDestinationsToString()==saved && reloaded.LoadTaxiDestinationsFromString(saved,1));
        assert(fixture.player.zoneUpdates==0 && fixture.player.casts==0 && fixture.player.stops==0 && !fixture.player.hostile.online);
    }
    {
        Fixture fixture;WorldSession session{&fixture.player};WorldPackets::Movement::MoveSplineDone ack;session.HandleMoveSplineDone(ack);
        assert(fixture.player.zoneUpdates==0 && fixture.player.m_taxi.GetPath().size()==2);
        sTaxiNodesStore.node.ContinentID=1;fixture.nodes[2].ContinentID=1;session.HandleMoveSplineDone(ack);
        assert(fixture.player.teleports==1 && fixture.player.m_taxi.GetPath().size()==2);fixture.Finalize();
        assert(fixture.player.zoneUpdates==0 && fixture.player.m_taxi.GetPath().size()==2);
    }
    for(uint32 blocked=0;blocked<4;++blocked)
    {
        Fixture fixture;fixture.Arrive();
        switch(blocked){case 0:fixture.flight._path.clear();break;case 1:fixture.flight._currentNode=0;break;case 2:fixture.flight._pointsForPathSwitch.push_back({5,0});break;case 3:fixture.player.m_taxi.AddTaxiDestination(300);break;}
        auto saved=fixture.player.m_taxi.SaveTaxiDestinationsToString();fixture.Finalize();assert(fixture.player.m_taxi.SaveTaxiDestinationsToString()==saved && !fixture.player.zoneUpdates);
    }
    {
        Fixture fixture;fixture.player.m_taxi.ClearTaxiDestinations();fixture.Finalize();assert(fixture.player.stops==1 && fixture.player.hostile.online && fixture.player.zoneUpdates==0);
        PlayerTaxi invalid;assert(!invalid.LoadTaxiDestinationsFromString("200 ",0));assert(!invalid.LoadTaxiDestinationsFromString("100 999 ",0));manager.mount=false;assert(!invalid.LoadTaxiDestinationsFromString("100 200 ",0));
    }
    std::cout<<"PASS actual native route builder/update/finalizer, codec/Tokenizer, Player cleanup and client ACK: completed single/multileg routes save empty without ACK; normal/late ACK, both hostile states, money and pet helpers preserved; pending/interrupted/map-transfer/invalid routes stay guarded/resumable. No actual client flight/portrait/faction/early-landing traversal proof\n";
#endif
}
