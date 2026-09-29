#include <array>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <set>
#include <string>
#include <vector>
#include <map>

using uint8 = uint8_t;
using int8 = int8_t;
using int32 = int32_t;
using uint32 = uint32_t;
enum Difficulty { DIFFICULTY_NORMAL = 1, DIFFICULTY_10_N = 3, DIFFICULTY_N_SCENARIO = 12,
    DIFFICULTY_NORMAL_RAID = 14, DIFFICULTY_LFR_RAID = 17 };
enum GroupFlags { GROUP_FLAG_NONE = 0, GROUP_FLAG_RAID = 2, GROUP_FLAG_LFG = 8 };
enum LootMethod { PERSONAL_LOOT = 3 };
enum ItemQualities { ITEM_QUALITY_UNCOMMON = 2 };
enum class GossipOptionNpc { None };
enum class HighGuid { Party, Player };
enum { ALLIANCE = 469, HORDE = 67, QUEST_STATUS_NONE = 0, QUEST_STATUS_COMPLETE = 1,
    QUEST_STATUS_INCOMPLETE = 3, GROUP_CATEGORY_HOME = 0, GROUP_CATEGORY_INSTANCE = 1,
    EXPANSION_WARLORDS_OF_DRAENOR = 5, EXPANSION_LEGION = 6, CONFIG_INSTANCE_IGNORE_RAID = 0,
    TRANSFER_ABORT_DIFFICULTY = 0, TRANSFER_ABORT_LOCKED_TO_DIFFERENT_INSTANCE = 1,
    GOSSIP_ACTION_INFO_DEF = 1000, GOSSIP_SENDER_MAIN = 1, TARGET_ICONS_COUNT = 8,
    GOSSIP_MAX_MENU_ITEMS = 32, SMART_EVENT_GOSSIP_SELECT = 62 };
#define TC_LOG_DEBUG(...)
#define TC_LOG_INFO(...)
#define ASSERT(value) assert(value)

struct ObjectGuid
{
    uint32 value = 0;
    template<HighGuid> static ObjectGuid Create(uint64_t value, uint32 = 0) { return {uint32(value)}; }
    void SetRawValue(std::vector<uint8> const&) { }
};
struct Field
{
    uint32 value = 0;
    uint32 GetUInt32() const { return value; }
    uint64_t GetUInt64() const { return value; }
    uint8 GetUInt8() const { return uint8(value); }
    std::vector<uint8> GetBinary() const { return {}; }
};
struct InstanceSave
{
    bool old = false;
    uint32 mask = 0;
    bool SaveIsOld() const { return old; }
    uint32 GetCompletedEncounterMask() const { return mask; }
    uint32 GetInstanceId() const { return 7; }
};
struct InstancePlayerBind { InstanceSave* save = nullptr; };
struct InstanceGroupBind { InstanceSave* save = nullptr; };
struct MapNames { std::array<std::string, 1> Str{{"Test map"}}; };
struct MapEntry
{
    uint32 ID = 1460;
    bool dungeon = true, scenario = true, garrison = false, raid = false;
    uint32 ExpansionID = EXPANSION_LEGION;
    MapNames* MapName = nullptr;
    bool IsDungeon() const { return dungeon; }
    bool IsScenario() const { return scenario; }
    bool IsGarrison() const { return garrison; }
    bool IsNonRaidDungeon() const { return dungeon && !raid; }
    bool IsRaid() const { return raid; }
};
struct InstanceTemplate { uint32 Parent = 0; };
struct MapDifficultyEntry { uint32 ID = 1; };
struct Quest { uint32 id; };
struct WorldLocation
{
    uint32 map = 1460;
    float x = 567.826f, y = 1886.94f, z = 0.737247f;
    uint32 GetMapId() const { return map; }
};
struct Corpse { uint32 GetMapId() const { return 1460; } };
struct DbcDungeon { bool raid = false; bool IsRaidType() const { return raid; } };
namespace lfg
{
    enum { LFG_STATE_DUNGEON = 5, LFG_STATE_FINISHED_DUNGEON = 6 };
    struct LFGDungeonData { uint32 map = 1460, difficulty = DIFFICULTY_N_SCENARIO; DbcDungeon* dbc; };
}
struct Group
{
    uint32 m_dbStoreId = 0;
    ObjectGuid m_guid, m_leaderGuid;
    std::string m_leaderName;
    LootMethod m_lootMethod = PERSONAL_LOOT;
    ObjectGuid m_looterGuid;
    ItemQualities m_lootThreshold = ITEM_QUALITY_UNCOMMON;
    ObjectGuid m_targetIcons[8];
    GroupFlags m_groupFlags = GROUP_FLAG_NONE;
    Difficulty m_dungeonDifficulty = DIFFICULTY_NORMAL, m_raidDifficulty = DIFFICULTY_NORMAL_RAID,
        m_legacyRaidDifficulty = DIFFICULTY_10_N;
    uint32 m_groupCategory = GROUP_CATEGORY_HOME;
    lfg::LFGDungeonData const* m_dungeon = nullptr;
    InstanceGroupBind* bind = nullptr;
    bool isLFGGroup() const { return (m_groupFlags & GROUP_FLAG_LFG) != 0; }
    bool isRaidGroup() const { return (m_groupFlags & GROUP_FLAG_RAID) != 0; }
    ObjectGuid GetGUID() const { return m_guid; }
    ObjectGuid GetLeaderGUID() const { return m_leaderGuid; }
    InstanceGroupBind* GetBoundInstance(MapEntry const*) { return bind; }
    void _initRaidSubGroupsCounter() { }
    void LoadGroupFromDB(Field*);
};
struct GossipMenuItem
{
    GossipOptionNpc OptionNpc = GossipOptionNpc::None;
    std::string Message, BoxMessage;
    bool IsCoded = false;
    uint32 Sender = 0, Action = 0, BoxMoney = 0;
};
using GossipMenuItemContainer = std::map<uint32, GossipMenuItem>;
struct GossipMenu
{
    GossipMenuItemContainer _menuItems;
    void AddMenuItem(int32, GossipOptionNpc, std::string const&, uint32, uint32, std::string const&, uint32, bool = false);
};
struct Talk
{
    uint32 closed = 0;
    GossipMenu menu;
    GossipMenu& GetGossipMenu() { return menu; }
    void SendCloseGossip() { ++closed; }
};
struct CreatureTemplate { uint32 GossipMenuId = 20487; };
struct Creature
{
    CreatureTemplate info;
    CreatureTemplate const* GetCreatureTemplate() const { return &info; }
};
struct Player
{
    bool gm = false, alive = true, vehicle = false, allowed = true;
    uint32 team = HORDE, map = 1, hordeStatus = QUEST_STATUS_INCOMPLETE, allianceStatus = QUEST_STATUS_NONE;
    int objective = 1;
    uint32 prepared = 0, shown = 0, action = 0, satisfies = 0;
    float x = 567.826f, y = 1886.94f, z = 0.737247f;
    uint32 nearTeleports = 0;
    Talk talk;
    Talk* PlayerTalkClass = &talk;
    Group* group = nullptr;
    Corpse* corpse = nullptr;
    InstancePlayerBind* bind = nullptr;
    bool isGameMaster() const { return gm; }
    uint32 GetTeam() const { return team; }
    uint32 GetQuestStatus(uint32 id) const
    {
        if (id == 40518) return hordeStatus;
        if (id == 42740) return allianceStatus;
        return QUEST_STATUS_NONE;
    }
    Difficulty GetDifficultyID(MapEntry const*) const { return DIFFICULTY_N_SCENARIO; }
    uint32 GetMapId() const { return map; }
    float GetPositionX() const { return x; }
    float GetPositionY() const { return y; }
    float GetPositionZ() const { return z; }
    float GetExactDist2d(float dx, float dy) const { return std::hypot(x - dx, y - dy); }
    void NearTeleportTo(WorldLocation const& location)
    { x = location.x; y = location.y; z = location.z; ++nearTeleports; }
    bool IsAlive() const { return alive; }
    void* GetVehicleBase() const { return vehicle ? (void*)this : nullptr; }
    int GetQuestObjectiveData(Quest const*, int8 index) const { assert(index == 1); return objective; }
    Group* GetGroup() const { return group; }
    Corpse* GetCorpse() const { return corpse; }
    char const* GetName() const { return "Test player"; }
    void SendTransferAborted(uint32, uint32, Difficulty) { }
    void ResurrectPlayer(float, bool) { alive = true; }
    void SpawnCorpseBones() { }
    void SendDirectMessage(int) { }
    bool Satisfy(void*, uint32, bool) { ++satisfies; return allowed; }
    InstancePlayerBind* GetBoundInstance(uint32, Difficulty) const { return bind; }
    uint8 GetSpecializationRoleMaskForGroup() const { return 8; }
    void PrepareGossipMenu(Creature*, uint32, bool quests) { assert(quests); ++prepared; }
    void ADD_GOSSIP_ITEM(GossipOptionNpc, char const*, uint32 sender, uint32 value)
    { assert(sender == GOSSIP_SENDER_MAIN); action = value;
        talk.menu.AddMenuItem(-1, GossipOptionNpc::None, "Return", sender, value, "", 0); }
    void SendPreparedGossip(Creature*) { ++shown; }
    static Difficulty CheckLoadedDungeonDifficultyID(Difficulty value)
    { return value == DIFFICULTY_NORMAL ? value : DIFFICULTY_NORMAL; }
    static Difficulty CheckLoadedRaidDifficultyID(Difficulty value)
    { return value == DIFFICULTY_NORMAL_RAID ? value : DIFFICULTY_NORMAL_RAID; }
    static Difficulty CheckLoadedLegacyRaidDifficultyID(Difficulty value)
    { return value == DIFFICULTY_10_N ? value : DIFFICULTY_10_N; }
};
struct Map { bool allowed = true; bool CanEnter(Player*) const { return allowed; } };
struct MapManager
{
    Map map;
    Map* FindMap(uint32, uint32) { return &map; }
    bool CanPlayerEnter(uint32, Player*, bool);
} mapManager;
auto sMapMgr = &mapManager;
struct MapStore
{
    MapEntry entry;
    bool present = true;
    MapEntry const* LookupEntry(uint32) const { return present ? &entry : nullptr; }
} sMapStore;
struct ObjectMgr
{
    InstanceTemplate instance;
    InstanceTemplate const* GetInstanceTemplate(uint32) const { return &instance; }
    uint32 GetDBCLocaleIndex() const { return 0; }
    void* GetAccessRequirement(uint32, Difficulty) const { return nullptr; }
    static bool GetPlayerNameByGUID(ObjectGuid, std::string& name) { name = "Test player"; return true; }
} objectMgr;
auto sObjectMgr = &objectMgr;
struct DB2Manager
{
    MapDifficultyEntry data;
    bool present = true;
    uint32 condition = 0;
    MapDifficultyEntry const* GetMapDifficultyData(uint32, Difficulty) const { return present ? &data : nullptr; }
    MapDifficultyEntry const* GetDownscaledMapDifficultyData(uint32, Difficulty&) const { return present ? &data : nullptr; }
    uint32 GetPlayerConditionForMapDifficulty(uint32) const { return condition; }
} sDB2Manager;
struct ConditionMgr { bool allowed = true; bool IsPlayerMeetingCondition(Player*, uint32) { return allowed; } } conditionMgr;
auto sConditionMgr = &conditionMgr;
struct World { bool getBoolConfig(uint32) const { return false; } } world;
auto sWorld = &world;
namespace WorldPackets { namespace Chat {
    struct ChatNotInParty { uint32 SlashCmd; int Write() const { return 0; } };
} }
namespace WorldPackets { namespace Misc {
    struct AreaTriggerNoCorpse { int Write() const { return 0; } };
} }
struct QuestDataStore
{
    Quest alliance{42740}, horde{40518};
    Quest const* GetQuestTemplate(uint32 id) const
    {
        if (id == 42740) return &alliance;
        if (id == 40518) return &horde;
        return nullptr;
    }
} questDataStore;
auto sQuestDataStore = &questDataStore;
struct GroupMgr { uint32 GenerateGroupId() const { return 9; } } groupMgr;
auto sGroupMgr = &groupMgr;
struct LFGMgr
{
    DbcDungeon dbc;
    lfg::LFGDungeonData dungeon{1460, DIFFICULTY_N_SCENARIO, &dbc};
    uint32 id = 908, state = lfg::LFG_STATE_DUNGEON, joins = 0, teleports = 0;
    void _LoadFromDB(Field* fields, ObjectGuid) { id = fields[16].GetUInt32(); state = fields[17].GetUInt8(); }
    uint32 GetDungeon(ObjectGuid) const { return id; }
    lfg::LFGDungeonData const* GetLFGDungeon(uint32 value, uint32 = 0) const { return value == 908 ? &dungeon : nullptr; }
    uint32 GetState(ObjectGuid, uint32) const { return state; }
    uint32 GetQueueId(ObjectGuid) const { return 908; }
    void TeleportPlayer(Player*, bool out) { assert(!out); ++teleports; }
    void JoinLfg(Player*, uint8 roles, std::set<uint32> const& slots)
    { assert(roles == 8 && slots == std::set<uint32>{908}); ++joins; }
} lfgMgr;
auto sLFGMgr = &lfgMgr;

// Each include is extracted from the production source by the PowerShell runner.
#include "CanPlayerEnter.inc"
#include "LoadGroup.inc"
#include "GossipMenuAdd.inc"
#include "BrokenShoreReturn.inc"

struct SmartScriptHolder
{
    struct Event
    {
        struct Gossip { uint32 sender = 20487, action = 0, cooldownMin = 0, cooldownMax = 0; } gossip;
    } event;
};
struct SmartGossipFixture
{
    uint32 skipActions = 0;
    void ProcessAction(SmartScriptHolder&, Player*, uint32, uint32) { ++skipActions; }
    void RecalcTimer(SmartScriptHolder&, uint32, uint32) { }
    void Dispatch(SmartScriptHolder& e, Player* unit, uint32 var0, uint32 var1)
    {
        switch (SMART_EVENT_GOSSIP_SELECT)
        {
            #include "SmartGossipSelect.inc"
        }
    }
};

struct EntranceFixture
{
    uint32 step = 0;
    WorldLocation graveyard;
    uint32 getScenarionStep() const { return step; }
    WorldLocation const* GetClosestGraveYard(float, float, float) const { return &graveyard; }
    #include "RestoreScenarioEntrance.inc"
};

int main()
{
    Player player;
    auto boostQuest = [](Player* player)
    {
        #include "BoostBrokenShoreQuest.inc"
        return questId;
    };
    assert(boostQuest(&player) == 40518); // Horde: Eitrigg, then Fate of the Horde.
    player.team = ALLIANCE;
    assert(boostQuest(&player) == 42740); // Alliance: Genn, then The Fallen Lion.
    player.team = HORDE;
    assert(sMapMgr->CanPlayerEnter(1460, &player, true));
    assert(player.satisfies == 1); // Recovery must still run access requirements.
    assert(!sMapMgr->CanPlayerEnter(1500, &player, true));
    player.hordeStatus = QUEST_STATUS_COMPLETE;
    assert(!sMapMgr->CanPlayerEnter(1460, &player, true));
    assert(sMapMgr->CanPlayerEnter(1460, &player, false));
    player.hordeStatus = QUEST_STATUS_INCOMPLETE;
    player.team = ALLIANCE;
    assert(!sMapMgr->CanPlayerEnter(1460, &player, true));
    player.allianceStatus = QUEST_STATUS_INCOMPLETE;
    assert(sMapMgr->CanPlayerEnter(1460, &player, true));
    player.allowed = false;
    assert(!sMapMgr->CanPlayerEnter(1460, &player, true));
    player.allowed = true;
    sDB2Manager.condition = 1;
    conditionMgr.allowed = false;
    assert(!sMapMgr->CanPlayerEnter(1460, &player, true));
    conditionMgr.allowed = true;
    sDB2Manager.condition = 0;
    sDB2Manager.present = false;
    assert(!sMapMgr->CanPlayerEnter(1460, &player, true));
    sDB2Manager.present = true;
    player.gm = true;
    assert(sMapMgr->CanPlayerEnter(1500, &player, true));
    player.gm = false;

    Field fields[19];
    fields[0].value = 1;
    fields[1].value = PERSONAL_LOOT;
    fields[3].value = ITEM_QUALITY_UNCOMMON;
    fields[12].value = GROUP_FLAG_LFG;
    fields[13].value = DIFFICULTY_N_SCENARIO;
    fields[14].value = DIFFICULTY_NORMAL_RAID;
    fields[15].value = 37; // Storage ID must never be interpreted as legacy difficulty.
    fields[16].value = 908;
    fields[17].value = lfg::LFG_STATE_DUNGEON;
    fields[18].value = DIFFICULTY_10_N;
    Group group;
    group.LoadGroupFromDB(fields);
    assert(group.m_dbStoreId == 37 && group.m_dungeon == &lfgMgr.dungeon);
    assert(group.m_groupCategory == GROUP_CATEGORY_INSTANCE);
    assert(group.m_dungeonDifficulty == DIFFICULTY_N_SCENARIO);
    assert(group.m_legacyRaidDifficulty == DIFFICULTY_10_N);
    lfgMgr.dbc.raid = true;
    lfgMgr.dungeon.difficulty = DIFFICULTY_LFR_RAID;
    Group raid;
    raid.LoadGroupFromDB(fields);
    assert(raid.m_raidDifficulty == DIFFICULTY_LFR_RAID);
    lfgMgr.dbc.raid = false;
    lfgMgr.dungeon.difficulty = DIFFICULTY_N_SCENARIO;

    Creature creature;
    player.team = HORDE;
    assert(SendBrokenShoreReturnMenu(&player, &creature));
    assert(player.prepared == 1 && player.shown == 1);
    assert(player.talk.menu._menuItems.at(BrokenShoreReturnOption).Action == BrokenShoreReturnAction);
    // Holgar's account-quest condition can hide database option 0. The return
    // option must not reuse it: SmartAI runs BEFORE the C++ gossip handler.
    SmartGossipFixture smartAI;
    SmartScriptHolder skipIntro;
    uint32 returnIndex = player.talk.menu._menuItems.begin()->first;
    smartAI.Dispatch(skipIntro, &player, 20487, returnIndex);
    assert(smartAI.skipActions == 0);
    assert(returnIndex == BrokenShoreReturnOption);
    // The database's real skip option retains index 0 when it is visible.
    player.talk.menu.AddMenuItem(0, GossipOptionNpc::None, "Skip introduction", 0, 0, "", 0);
    assert(SendBrokenShoreReturnMenu(&player, &creature));
    assert(player.talk.menu._menuItems.size() == 2);
    smartAI.Dispatch(skipIntro, &player, 20487, BrokenShoreReturnOption);
    assert(smartAI.skipActions == 0);
    smartAI.Dispatch(skipIntro, &player, 20487, 0);
    assert(smartAI.skipActions == 1);
    assert(!HandleBrokenShoreReturn(&player, GOSSIP_SENDER_MAIN, 0));
    assert(lfgMgr.joins == 0 && lfgMgr.teleports == 0);
    player.objective = 0;
    assert(!CanReturnToBrokenShore(&player));
    assert(HandleBrokenShoreReturn(&player, GOSSIP_SENDER_MAIN, BrokenShoreReturnAction));
    assert(lfgMgr.joins == 0); // A forged/expired selection cannot start a scenario.
    player.objective = 1;
    player.hordeStatus = QUEST_STATUS_COMPLETE;
    assert(!CanReturnToBrokenShore(&player));
    player.hordeStatus = QUEST_STATUS_INCOMPLETE;
    player.map = 1460;
    assert(!CanReturnToBrokenShore(&player));
    player.map = 1;
    player.alive = false;
    assert(!CanReturnToBrokenShore(&player));
    player.alive = true;
    player.vehicle = true;
    assert(!CanReturnToBrokenShore(&player));
    player.vehicle = false;
    assert(HandleBrokenShoreReturn(&player, GOSSIP_SENDER_MAIN, BrokenShoreReturnAction));
    assert(lfgMgr.joins == 1 && lfgMgr.teleports == 0);
    player.group = &group;
    assert(HandleBrokenShoreReturn(&player, GOSSIP_SENDER_MAIN, BrokenShoreReturnAction));
    assert(lfgMgr.joins == 1 && lfgMgr.teleports == 1); // Reuse the live instance.
    lfgMgr.state = lfg::LFG_STATE_FINISHED_DUNGEON;
    assert(HandleBrokenShoreReturn(&player, GOSSIP_SENDER_MAIN, BrokenShoreReturnAction));
    assert(lfgMgr.joins == 2 && lfgMgr.teleports == 1);

    EntranceFixture entrance;
    player.x = 2.39286f;
    player.y = 1.694546f;
    entrance.RestoreScenarioEntrance(&player);
    assert(player.nearTeleports == 0); // The initial ship arrival still owns boarding.
    entrance.step = 1;
    entrance.RestoreScenarioEntrance(&player);
    assert(player.nearTeleports == 1 && player.x == entrance.graveyard.x);
    player.x += 25.0f;
    entrance.RestoreScenarioEntrance(&player);
    assert(player.nearTeleports == 1); // Relogging on the beach keeps the saved spot.
    player.x = 2.39286f;
    player.y = 1.694546f;
    entrance.step = 5;
    entrance.graveyard.x = 900.0f;
    entrance.RestoreScenarioEntrance(&player);
    assert(player.nearTeleports == 2 && player.x == 900.0f);
}
