#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <set>

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;

enum TeamId { TEAM_ALLIANCE, TEAM_HORDE, MAX_TEAMS };
enum Team { ALLIANCE, HORDE };
#include "BattlegroundDefines.h"

#define TC_LOG_ERROR(...) std::abort()

struct BattlemasterListEntry
{
    uint16 MapID[2];
    uint8 MinPlayers;
    uint8 InstanceType;
};

struct
{
    std::map<uint16, BattlemasterListEntry> rows;
    BattlemasterListEntry const* LookupEntry(uint16 id) const { return &rows.at(id); }
} sBattlemasterListStore;

struct BattlegroundMgr
{
    std::map<uint16, uint8> weights[MS::Battlegrounds::IternalPvpTypes::Max];
    std::map<uint16, uint8>* GetSelectionWeight(uint8 type) { return &weights[type]; }
    bool isTesting() const { return false; }
} manager;

BattlegroundMgr* const sBattlegroundMgr = &manager;
uint32 roll;
uint32 urand(uint32 low, uint32 high) { return std::clamp(roll, low, high); }

struct BattlegroundQueue
{
    struct GroupQueueInfo
    {
        struct { uint16 map[2]; } ignore;
    };

    struct Pool
    {
        std::list<GroupQueueInfo*> SelectedGroups;
        uint32 Count = 0;
        uint32 GetPlayerCount() const { return Count; }
    } m_SelectionPools[MAX_TEAMS];

    uint16 GenerateRandomMap(uint16 bgTypeId);
};

#include "select.inc"

int main()
{
    namespace BG = MS::Battlegrounds;
    namespace Type = BG::BattlegroundTypeId;
    auto& rows = sBattlemasterListStore.rows;
    rows[Type::RatedBattleground] = {{529, 1105}, 5, BG::PvpInstanceType::Battleground};
    rows[Type::BattlegroundRandom] = {{529, 489}, 5, BG::PvpInstanceType::Battleground};
    rows[Type::ArenaAll] = {{1672, 617}, 0, BG::PvpInstanceType::Arena};
    rows[Type::ArenaBlackrookHold] = {{1504, 0}, 0, BG::PvpInstanceType::Arena};
    rows[Type::BattlegroundWarsongGulch] = {{489, 0}, 5, BG::PvpInstanceType::Battleground};
    rows[Type::BattlegroundRatedEyeOfTheStorm] = {{968, 0}, 5, BG::PvpInstanceType::Battleground};
    rows[Type::BattlegroundEyeOfTheStorm] = {{566, 0}, 8, BG::PvpInstanceType::Battleground};
    rows[Type::BattlegroundSeethingShore] = {{1803, 0}, 10, BG::PvpInstanceType::Battleground};

    manager.weights[BG::IternalPvpTypes::RatedBattleground] = {
        {Type::BattlegroundWarsongGulch, 1}, {Type::BattlegroundRatedEyeOfTheStorm, 1}, {Type::BattlegroundSeethingShore, 1}
    };
    manager.weights[BG::IternalPvpTypes::Battleground] = {{Type::BattlegroundEyeOfTheStorm, 1}};
    manager.weights[BG::IternalPvpTypes::Arena] = {{Type::ArenaBlackrookHold, 1}};

    BattlegroundQueue queue;
    if (queue.GenerateRandomMap(Type::RatedBattleground) != Type::None ||
        queue.GenerateRandomMap(Type::ArenaAll) != Type::ArenaBlackrookHold)
        return 1;

    queue.m_SelectionPools[TEAM_ALLIANCE].Count = 10;
    queue.m_SelectionPools[TEAM_HORDE].Count = 10;
    std::set<uint16> picks;
    for (roll = 0; roll < 3; ++roll)
        picks.insert(queue.GenerateRandomMap(Type::RatedBattleground));
    if (picks != std::set<uint16>{Type::BattlegroundWarsongGulch, Type::BattlegroundRatedEyeOfTheStorm, Type::BattlegroundSeethingShore})
        return 2;
    if (queue.GenerateRandomMap(Type::BattlegroundRandom) != Type::BattlegroundEyeOfTheStorm)
        return 3;

    BattlegroundQueue::GroupQueueInfo bans{{489, 1803}};
    queue.m_SelectionPools[TEAM_HORDE].SelectedGroups.push_back(&bans);
    if (queue.GenerateRandomMap(Type::RatedBattleground) != Type::BattlegroundRatedEyeOfTheStorm)
        return 4;

    manager.weights[BG::IternalPvpTypes::RatedBattleground].erase(Type::BattlegroundRatedEyeOfTheStorm);
    if (queue.GenerateRandomMap(Type::RatedBattleground) != Type::BattlegroundWarsongGulch)
        return 5;

    std::cout << "Rated roster, native variants, arena and random pools, veto and fallback passed.\n";
}
