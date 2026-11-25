#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;
using ObjectGuid = unsigned;
enum TeamId { TEAM_ALLIANCE, TEAM_HORDE, MAX_TEAMS };
enum Team { ALLIANCE, HORDE };
#include "BattlegroundDefines.h"

namespace PvP = MS::Battlegrounds;
constexpr auto Arena = PvP::BattlegroundTypeId::ArenaAshamanesFall;
constexpr uint64 Queue = PvP::QueueOffsets::Wargame + Arena;
constexpr auto PLAYER_MAX_BATTLEGROUND_QUEUES = 3;
constexpr int MAX_LEVEL = 110;
constexpr int DISABLE_TYPE_BATTLEGROUND = 3;
namespace GameTime { int GetGameTime() { return 100; } }
namespace DisableMgr { bool IsDisabledFor(int, uint16, void*) { return false; } }
namespace WorldPackets::Battleground { struct IgnorMapInfo { }; struct BattlefieldStatusQueued { int Write() { return 0; } }; }

struct WargameRequest
{
    uint64 QueueID = Queue;
    ObjectGuid OpposingPartyMemberGUID = 1;
    int CreationDate = 41;
    bool TournamentRules = true;
};

struct Player;
struct Group
{
    struct MemberSlot { ObjectGuid Guid; };
    std::vector<MemberSlot> Slots;
    ObjectGuid GetLeaderGUID() const { return Slots.front().Guid; }
    bool IsLeader(ObjectGuid guid) const { return guid == GetLeaderGUID(); }
    std::size_t GetMembersCount() const { return Slots.size(); }
    auto const& GetMemberSlots() const { return Slots; }
    struct MemberRef
    {
        MemberRef* next() { return nullptr; }
        Player* getSource() { return nullptr; }
    };
    MemberRef* GetFirstMember() const { return nullptr; }
};

struct Player
{
    ObjectGuid Guid;
    Group* Party;
    WargameRequest* Request = nullptr;
    bool InMatch = false;
    bool AlreadyQueued = false;
    ~Player() { delete Request; }
    bool InBattleground() const { return InMatch; }
    Group* GetGroup() const { return Party; }
    ObjectGuid GetGUID() const { return Guid; }
    bool HasWargameRequest() const { return Request != nullptr; }
    WargameRequest* GetWargameRequest() const { return Request; }
    void SetWargameRequest(WargameRequest* request) { Request = request; }
    bool HasFreeBattlegroundQueueId() const { return true; }
    int GetBattlegroundQueueIndex(uint8) const { return AlreadyQueued ? 0 : PLAYER_MAX_BATTLEGROUND_QUEUES; }
    int AddBattlegroundQueueId(uint8) { return 0; }
    void SendDirectMessage(int) { }
};

struct ObjectAccessor
{
    static Player* Players[5];
    static Player* FindPlayer(ObjectGuid guid) { return guid < 5 ? Players[guid] : nullptr; }
} accessor;
Player* ObjectAccessor::Players[5] = {};
auto* const sObjectAccessor = &accessor;

struct Bracket { int RangeIndex = 1; } bracket;
struct
{
    Bracket* GetBattlegroundBracketByLevel(int, int) { return &bracket; }
} sDB2Manager;

struct Match
{
    int Starts = 0;
    int GetMapId() const { return 1552; }
    uint16 GetTypeID() const { return Arena; }
    void StartBattleground() { ++Starts; }
};
struct PartyQueue
{
    struct Invite { int JoinTime = 0; uint8 JoinType = 2; } invite;
    int Invites = 0;
    Invite* AddGroup(Player*, Group*, uint16, Bracket*, uint8, bool, bool,
        WorldPackets::Battleground::IgnorMapInfo, int, uint32) { return &invite; }
    int GetAverageQueueWaitTime(Invite*, int) const { return 0; }
    void InviteGroupToBG(Invite*, Match*, uint32) { ++Invites; }
    uint16 GenerateRandomMap(uint16) const { return Arena; }
};
struct BattlegroundMgr
{
    Match Instance;
    PartyQueue _battlegroundQueues[PvP::BattlegroundQueueTypeId::Max];
    void InitWargame(Player*, ObjectGuid, uint64, bool);
    Match* GetBattlegroundTemplate(uint16 id) { return id == Arena ? &Instance : nullptr; }
    Match* CreateNewBattleground(uint16 id, Bracket*, uint8 size, bool rated, uint16 map, bool rules, bool wargame)
    {
        return id == Arena && map == Arena && size == 2 && !rated && rules && wargame ? &Instance : nullptr;
    }
    void BuildBattlegroundStatusQueued(WorldPackets::Battleground::BattlefieldStatusQueued*, Match*, Player*, int, int, int, uint8, bool) { }
};

#include "invite.inc"

int main()
{
    Group receiver{{{1}, {2}}};
    Group challenger{{{3}, {4}}};
    Player a{1, &receiver}, b{2, &receiver}, c{3, &challenger}, d{4, &challenger};
    ObjectAccessor::Players[1] = &a;
    ObjectAccessor::Players[2] = &b;
    ObjectAccessor::Players[3] = &c;
    ObjectAccessor::Players[4] = &d;
    BattlegroundMgr manager;
    auto& queue = manager._battlegroundQueues[PvP::BattlegroundQueueTypeId::Arena2v2];

    c.Request = new WargameRequest;
    manager.InitWargame(&a, c.Guid, Queue + 1, true);
    if (!c.Request || manager.Instance.Starts)
        return 1;
    manager.InitWargame(&a, c.Guid, Queue, true);
    manager.InitWargame(&a, c.Guid, Queue, true);
    if (c.Request || manager.Instance.Starts != 1 || queue.Invites != 2)
        return 2;
    c.Request = new WargameRequest;
    manager.InitWargame(&a, c.Guid, Queue, false);
    if (c.Request || manager.Instance.Starts != 1)
        return 3;
    c.Request = new WargameRequest;
    c.Request->CreationDate = 39;
    manager.InitWargame(&a, c.Guid, Queue, true);
    if (c.Request || manager.Instance.Starts != 1)
        return 4;
    c.Request = new WargameRequest;
    d.AlreadyQueued = true;
    manager.InitWargame(&a, c.Guid, Queue, true);
    if (c.Request || manager.Instance.Starts != 1 || queue.Invites != 2)
        return 5;

    std::cout << "Wargame accepts once and rejects decline, mismatched queue, expiry and queued member.\n";
}
