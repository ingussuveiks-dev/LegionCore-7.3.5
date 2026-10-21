// Executes extracted production state changes, queue packets and group cleanup.
// World/map/database facades record effects; they do not simulate client UI.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
using uint8 = uint8_t; using uint32 = uint32_t; using int32 = int32_t;
struct ObjectGuid
{
    uint32 value = 0;
    static const ObjectGuid Empty;
    bool IsParty() const { return value >= 100; }
    bool IsEmpty() const { return !value; }
    bool operator<(ObjectGuid b) const { return value < b.value; }
    bool operator==(ObjectGuid b) const { return value == b.value; }
    bool operator!=(ObjectGuid b) const { return !(*this == b); }
};
const ObjectGuid ObjectGuid::Empty{};
using GuidSet = std::set<ObjectGuid>;
using LfgDungeonSet = std::set<uint32>;
#define TC_LOG_DEBUG(...)
#define TC_LOG_INFO(...)
#define TC_LOG_ERROR(...)
namespace GameTime { int32 GetGameTime() { return 123456; } }
namespace WorldPackets { namespace LFG {
enum class RideType { None, Battlegrounds, Lfg };
struct RideTicket { ObjectGuid RequesterGuid; uint32 Id=0; RideType Type=RideType::None; int32 Time=0; };
struct QueueStatusUpdate
{
    RideTicket Ticket;
    std::vector<ObjectGuid> SuspendedPlayers;
    std::vector<uint32> Slots;
    uint32 RequestedRoles=0;
    uint8 SubType=0, Reason=0;
    bool IsParty=false, NotifyUI=false, Joined=false, LfgJoined=false, Queued=false;
    QueueStatusUpdate const* Write() const { return this; }
};
} }
struct Group;
struct Player
{
    ObjectGuid guid{1}; Group* group=nullptr; uint32 map=0; bool horde=true;
    std::set<uint32> rewarded; bool timewalk=true;
    std::vector<WorldPackets::LFG::QueueStatusUpdate> packets;
    ObjectGuid GetGUID() const { return guid; }
    Group* GetGroup() const { return group; }
    uint32 GetMapId() const { return map; }
    std::string GetName() const { return "Fixture"; }
    uint32 GetTeam() const { return horde ? 67 : 469; }
    bool GetQuestRewardStatus(uint32 q) const { return rewarded.count(q)!=0; }
    void SendDirectMessage(WorldPackets::LFG::QueueStatusUpdate const* p) { packets.push_back(*p); }
    void EnterInTimeWalk(void*) { timewalk=false; }
};
Player* online;
namespace ObjectAccessor { Player* FindPlayer(ObjectGuid g) { return online && online->guid==g ? online : nullptr; } }
struct Group
{
    ObjectGuid guid{100}; Player* player=nullptr;
    uint32 members=1; bool lfg=true, disbanded=false;
    ObjectGuid GetGUID() const { return guid; }
    ObjectGuid GetLeaderGUID() const { return player->guid; }
    uint32 GetMembersCount() const { return members; }
    bool isLFGGroup() const { return lfg; }
    void Disband();
};
namespace lfg {
#include "States.inc"
#include "UpdateTypes.inc"
constexpr uint8 LFG_QUEUE_DUNGEON=1;
struct LfgUpdateData {
    LfgUpdateType updateType; LfgState state=LFG_STATE_NONE; LfgDungeonSet dungeons;
    LfgUpdateData(LfgUpdateType t,LfgDungeonSet d):updateType(t),dungeons(d){}
};
struct LfgPlayerData {
    LfgState m_State=LFG_STATE_NONE, m_OldState=LFG_STATE_NONE;
    uint8 m_Roles=0; LfgDungeonSet m_SelectedDungeons; std::recursive_mutex m_lock;
    ObjectGuid group, lfgGroup; WorldPackets::LFG::RideTicket ticket;
    void SetState(LfgState);
};
struct LfgGroupData {
    LfgState m_State=LFG_STATE_NONE, m_OldState=LFG_STATE_NONE;
    uint32 m_Dungeon=0, queue=0, m_KicksLeft=3;
    GuidSet players;
    void SetState(LfgState);
    uint32 GetDungeon() const { return m_Dungeon; }
    uint32 GetQueueId() const { return queue; }
    GuidSet const& GetPlayers() const { return players; }
};
constexpr uint32 LFG_GROUP_MAX_KICKS=3;
struct LFGDungeonsEntry { bool scenario=true; bool IsScenario() const { return scenario; } };
struct LFGDungeonData { LFGDungeonsEntry* dbc; uint32 id; int32 map; };
struct LfgQueueData { uint8 subType=0; };
struct LFGQueue { void RemoveFromQueue(ObjectGuid) {} };
struct ProposalPlayer { int accept=0; };
struct Proposal { std::map<ObjectGuid,ProposalPlayer> players; };
constexpr int LFG_ANSWER_DENY=0;
class LFGMgr {
public:
    std::recursive_mutex m_lock;
    std::map<ObjectGuid,LfgGroupData> GroupsStore;
    std::map<ObjectGuid,std::map<uint32,LfgPlayerData>> PlayersStore;
    std::map<ObjectGuid,std::set<uint32>> PlayerDungeons, GroupDungeons;
    std::map<uint32,LFGDungeonData> catalog;
    std::map<uint32,Proposal> ProposalsStore;
    LFGQueue queueFacade;
    uint32 finishCalls=0;
    uint32 GetDungeon(ObjectGuid g) { return GroupsStore[g].m_Dungeon; }
    uint32 GetQueueId(ObjectGuid g) { return GroupsStore[g].queue; }
    uint32 GetQueueId(ObjectGuid g,uint32) { return GetQueueId(g); }
    uint32 GetQueueId(uint32 d) { return d; }
    LFGDungeonData const* GetLFGDungeon(uint32 d) { auto it=catalog.find(d);return it==catalog.end()?nullptr:&it->second; }
    LFGDungeonData const* GetLFGDungeon(uint32 d,uint32) { return GetLFGDungeon(d); }
    uint32 GetLFGDungeonEntry(uint32 d) { return d|0x04000000; }
    LfgState GetState(ObjectGuid g,uint32 q) { return g.IsParty()?GroupsStore[g].m_State:PlayersStore[g][q].m_State; }
    void SetState(ObjectGuid g,LfgState s,uint32 q) { if(g.IsParty()) GroupsStore[g].SetState(s);else PlayersStore[g][q].SetState(s); }
    LfgDungeonSet const& GetSelectedDungeons(ObjectGuid g,uint32 q) { return PlayersStore[g][q].m_SelectedDungeons; }
    void SetSelectedDungeons(ObjectGuid g,LfgDungeonSet const& d,uint32 q) { PlayersStore[g][q].m_SelectedDungeons=d; }
    ObjectGuid GetGroup(ObjectGuid g,uint32 q) { return g.IsParty()?g:PlayersStore[g][q].group; }
    void SetGroup(ObjectGuid g,ObjectGuid v,uint32 q) { PlayersStore[g][q].group=v; }
    void SetLfgGroup(ObjectGuid g,ObjectGuid v,uint32 q) { PlayersStore[g][q].lfgGroup=v; }
    void AddPlayerToGroup(ObjectGuid g,ObjectGuid p) { GroupsStore[g].players.insert(p); }
    GuidSet const& GetPlayers(ObjectGuid g) { return GroupsStore[g].players; }
    ObjectGuid GetLeader(ObjectGuid) { return online->guid; }
    bool HasQueue(ObjectGuid g) { return PlayerDungeons.count(g) && !PlayerDungeons[g].empty(); }
    auto GetTicket(ObjectGuid g,uint32 q) -> WorldPackets::LFG::RideTicket const* {
        auto p=PlayersStore.find(g);if(p==PlayersStore.end())return nullptr;
        auto t=p->second.find(q);return t==p->second.end()?nullptr:&t->second.ticket;
    }
    void SetTicket(ObjectGuid g,WorldPackets::LFG::RideTicket const& t,uint32 q) { PlayersStore[g][q].ticket=t; }
    uint8 GetRoles(ObjectGuid g,uint32 q) { return PlayersStore[g][q].m_Roles; }
    void SendLfgUpdatePlayer(ObjectGuid g,LfgUpdateData const& d,bool s=false) { SendUpdateStatus(g,d,s); }
    void SendLfgUpdateParty(ObjectGuid g,LfgUpdateData const& d) { SendUpdateStatus(g,d,false); }
    void RemoveFromQueue(ObjectGuid,uint32);
    void RemoveFromGroupQueue(ObjectGuid,uint32);
    LFGQueue& GetQueue(ObjectGuid,uint32) { return queueFacade; }
    void UpdateRoleCheck(ObjectGuid) {}
    void RemoveProposal(std::map<uint32,Proposal>::iterator it,LfgUpdateType) { ProposalsStore.erase(it); }
    void SetupGroupMember(ObjectGuid,ObjectGuid);
    void RemoveGroupData(ObjectGuid);
    void LeaveLfg(ObjectGuid,uint32=0);
    void SendUpdateStatus(ObjectGuid,LfgUpdateData const&,bool);
    void CleanupFinishedSoloScenario(Player*,uint32=0);
    void FinishDungeon(ObjectGuid,uint32) { ++finishCalls; }
    void FinishState(ObjectGuid gguid,uint32 queueId) {
#include "FinishState.inc"
    }
};
#include "PlayerSetState.inc"
#include "GroupSetState.inc"
#include "Setup.inc"
#include "RemoveQueue.inc"
#include "RemoveGroupQueue.inc"
#include "RemoveGroup.inc"
#include "Leave.inc"
#include "Packet.inc"
#include "Cleanup.inc"
}
lfg::LFGMgr* sLFGMgr;
#include "ScenarioTypes.inc"
struct ScenarioRewardDispatch {
    struct Entry { ScenarioType Type=SCENARIO_TYPE_DEFAULT; } entry;
    struct Challenge { void Complete() {} } challenge;
    Entry* _scenarioEntry=&entry;
    Challenge* _challenge=nullptr;
    lfg::LFGDungeonData const* dungeonData=nullptr;
    void Reward(ObjectGuid groupGuid) {
#include "RewardDispatch.inc"
    }
};
void Group::Disband() { sLFGMgr->RemoveGroupData(guid); disbanded=true;player->group=nullptr; }
constexpr uint32 ALLIANCE=469;
namespace LegionIntroSequence {
#include "LegacyCleanup.inc"
}
struct Fixture {
    lfg::LFGMgr mgr; Player player; Group group;
    lfg::LFGDungeonsEntry entry;
    Fixture(uint32 slot=908,int32 map=1460) {
        online=&player;sLFGMgr=&mgr;player.group=&group;group.player=&player;
        mgr.catalog.emplace(slot,lfg::LFGDungeonData{&entry,slot,map});
        auto& data=mgr.GroupsStore[group.guid];data.m_Dungeon=slot;data.queue=slot;data.SetState(lfg::LFG_STATE_DUNGEON);
        mgr.SetupGroupMember(player.guid,group.guid);
    }
    void removal(uint32 slot) {
        assert(!player.packets.empty());auto const& p=player.packets.back();
        assert(p.Reason==lfg::LFG_UPDATETYPE_REMOVED_FROM_QUEUE);
        assert(!p.LfgJoined && !p.Queued);
        assert(p.Ticket.Id==slot && p.Ticket.Type==WorldPackets::LFG::RideType::Lfg);
        assert(p.Slots==std::vector<uint32>{slot|0x04000000});
    }
};
int main() {
    using namespace lfg;
    // Boost itself has no LFG group. Broken Shore creates the first one.
    { LFGMgr m;Player p;online=&p;sLFGMgr=&m;m.CleanupFinishedSoloScenario(&p);assert(!p.group && p.packets.empty()); }
    // Unfinished exits/relogs/restarts keep the return group and a valid ticket.
    { Fixture f;auto before=*f.mgr.GetTicket(f.player.guid,908);f.mgr.SetupGroupMember(f.player.guid,f.group.guid);
      assert(f.mgr.GetTicket(f.player.guid,908)->Time==before.Time);
      f.mgr.CleanupFinishedSoloScenario(&f.player);assert(!f.group.disbanded); }
    // Completion packet still addresses 908 after native state clears selection.
    { Fixture f;f.mgr.FinishState(f.group.guid,908);assert(f.player.packets.back().Ticket.Id==908);
      assert(f.player.packets.back().Slots==std::vector<uint32>{908|0x04000000});
      f.player.map=1460;f.mgr.CleanupFinishedSoloScenario(&f.player);assert(!f.group.disbanded);
      f.player.map=1;f.mgr.CleanupFinishedSoloScenario(&f.player);assert(f.group.disbanded);f.removal(908);
      assert(!f.mgr.HasQueue(f.player.guid));assert(f.mgr.GroupDungeons.empty()); }
    { Fixture f;f.mgr.PlayerDungeons[f.player.guid].insert(777);
      f.mgr.SetState(f.group.guid,LFG_STATE_FINISHED_DUNGEON,908);
      f.mgr.CleanupFinishedSoloScenario(&f.player);f.removal(908);
      assert(f.mgr.PlayerDungeons[f.player.guid]==std::set<uint32>{777});assert(!f.player.timewalk); }
    // Removal must preserve the slot/ticket and leave unrelated queues alone.
    { Fixture f;f.mgr.PlayerDungeons[f.player.guid].insert(777);
      f.mgr.LeaveLfg(f.player.guid,908);f.removal(908);assert(f.mgr.HasQueue(f.player.guid));
      assert(f.mgr.PlayerDungeons[f.player.guid]==std::set<uint32>{777}); }
    { Fixture f;f.mgr.SetState(f.player.guid,LFG_STATE_FINISHED_DUNGEON,908);
      f.mgr.LeaveLfg(f.player.guid,908);f.removal(908); }
    // A stale one-member Broken Shore group is retired only for a rewarded intro.
    for(bool horde:{false,true}) {
      Fixture f;f.player.horde=horde;
      LegionIntroSequence::CleanupCompletedScenario(&f.player);assert(!f.group.disbanded);
      f.player.rewarded.insert(horde?40518:42740);
      // Reproduce legacy state 3 ignored by _LoadFromDB, and login-hook order.
      f.mgr.GroupsStore[f.group.guid].SetState(LFG_STATE_NONE);
      f.mgr.GroupsStore[f.group.guid].m_Dungeon=908;
      f.mgr.PlayerDungeons.clear();f.mgr.PlayersStore.clear();f.mgr.GroupsStore[f.group.guid].players.clear();
      LegionIntroSequence::CleanupCompletedScenario(&f.player);assert(f.group.disbanded);f.removal(908);
      LegionIntroSequence::CleanupCompletedScenario(&f.player); // idempotent
    }
    // Do not remove ordinary parties, multiplayer groups, dungeons or active maps.
    for(int branch=0;branch<5;++branch) {
      Fixture f;f.player.rewarded.insert(40518);
      if(branch==0)f.group.lfg=false;
      if(branch==1)f.group.members=2;
      if(branch==2)f.entry.scenario=false;
      if(branch==3)f.player.map=1460;
      if(branch==4)f.mgr.catalog[908].map=1498;
      LegionIntroSequence::CleanupCompletedScenario(&f.player);assert(!f.group.disbanded);
    }
    // Later completed scenarios use the same generic exit path, without quest IDs.
    for(auto pair:{std::pair<uint32,int32>{1183,1522},{1229,1600},{1250,1498},{1185,1533}}) {
      Fixture f(pair.first,pair.second);f.player.map=pair.second;
      f.mgr.FinishState(f.group.guid,pair.first);f.mgr.CleanupFinishedSoloScenario(&f.player);assert(!f.group.disbanded);
      f.player.map=1220;f.mgr.CleanupFinishedSoloScenario(&f.player);assert(f.group.disbanded);f.removal(pair.first);
    }
    // Execute Scenario::Reward's native dispatch, including Broken Shore type 4.
    for(auto type:{SCENARIO_TYPE_DEFAULT,SCENARIO_TYPE_BOOST_TUTORIAL,SCENARIO_TYPE_LEGION_INVASION}) {
      Fixture f;ScenarioRewardDispatch s;s.entry.Type=type;s.dungeonData=&f.mgr.catalog[908];
      s.Reward(f.group.guid);assert(f.mgr.finishCalls==1);
      s.dungeonData=nullptr;s.Reward(f.group.guid);assert(f.mgr.finishCalls==1);
      auto unrelated=f.mgr.catalog[908];unrelated.id=1183;s.dungeonData=&unrelated;
      s.Reward(f.group.guid);assert(f.mgr.finishCalls==1);
    }
    std::cout<<"PASS: native queue packets, completion/exit/relog lifecycle, legacy intro recovery, both factions and unrelated-group boundaries\n";
}
