#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "QuestPackets.h"

void AddSC_ashenvale();

namespace
{
constexpr uint32 QuestId = 26454;
constexpr uint32 Prerequisite = 26453;
constexpr uint32 ChestEntry = 181916;
constexpr uint32 Wood = 24081;
constexpr uint32 Opening = 3365;

Quest const* LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeAshenvaleFelWoodQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing the fel wood quest or its prerequisite");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (rows->NextRow());
    auto objective = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=26454");
    auto addon = WorldDatabase.Query("SELECT PrevQuestID FROM quest_template_addon WHERE ID=26454");
    auto receiver = WorldDatabase.Query("SELECT id FROM creature_questender WHERE quest=26454");
    Require(objective && objective->GetRowCount() == 1 && addon && addon->Fetch()[0].GetInt32() == Prerequisite &&
        receiver && receiver->GetRowCount() == 1 && receiver->Fetch()[0].GetUInt32() == 17310,
        "Missing the real prerequisite, objective or recipient");
    auto quest = quests[QuestId];
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quest->prevQuests.push_back(addon->Fetch()[0].GetInt32());
    Require(quest->Objectives.front().Type == QUEST_OBJECTIVE_ITEM && quest->Objectives.front().ObjectID == Wood &&
        quest->Objectives.front().Amount == 5 && !quest->SourceItemId, "The quest should require exactly five collected wood");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry=181916";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded == 1 && LootTemplates_Gameobject.HaveQuestLootFor(ChestEntry), "Missing the original quest-only chest loot");
    return quest;
}

struct Placement
{
    uint32 guid;
    uint32 respawn;
    Position position;
    G3D::Quat rotation;
};

std::vector<Placement> Locations()
{
    auto spawns = WorldDatabase.Query(
        "SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=181916 AND map=1 AND zoneId=331 AND phaseMask=1 ORDER BY guid");
    Require(spawns && spawns->GetRowCount() == 15, "Missing original Satyrnaar fel wood placements");
    std::vector<Placement> result;
    do
    {
        auto row = spawns->Fetch();
        result.push_back({row[0].GetUInt32(), row[1].GetUInt32(), {row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()},
            G3D::Quat(row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat())});
    } while (spawns->NextRow());
    return result;
}

struct WoodChest : GameObject
{
    WoodChest(Map& map, Placement const& placement)
    {
        m_DBTableGuid = placement.guid;
        Require(Create(placement.guid, ChestEntry, &map, 1, placement.position, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Fel wood chest did not enter Ashenvale");
        SetRespawnDelayTime(placement.respawn);
        Update(10000);
        Require(getLootState() == GO_READY && GetGOInfo()->GetLockId() == 57 &&
            GetGOInfo()->GetLootId() == ChestEntry && !IsPersonal(), "Native lock, loot or sharing changed");
    }

    ~WoodChest()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }

    void Rearm(uint32 expected)
    {
        Require(getLootState() == GO_JUST_DEACTIVATED, "Native loot release did not consume its used chest");
        Update(10000);
        Require(!isSpawned() && m_respawnTime == GameTime::GetGameTime() + expected, "Original shared respawn delay changed");
        m_respawnTime = GameTime::GetGameTime();
        Update(10000);
        Require(isSpawned() && getLootState() == GO_READY, "Native update did not rearm the chest");
    }
};

WorldSession* Session()
{
    return new WorldSession(0, "Ashenvale fel wood native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void MoveTo(Player& player, Position const& position)
{
    player.GetMap()->PlayerRelocation(&player, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), position.GetOrientation());
}

void Open(LootPlayer& player, WoodChest& chest)
{
    auto result = player.CastSpell(&chest, Opening, false);
    std::cout << "CASE Opening result=" << unsigned(result) << " state=" << unsigned(chest.getLootState()) << std::endl;
    Tick(player, 6000);
}

void Release(Player& player, WoodChest const& chest)
{
    WorldPackets::Loot::LootRelease packet{WorldPacket(CMSG_LOOT_RELEASE)};
    packet.Unit = chest.GetGUID();
    player.GetSession()->HandleLootRelease(packet);
}

void LootWood(Player& player, WoodChest& chest, uint8 lootListId)
{
    WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
    packet.Loot.push_back({chest.loot.GetGUID(), lootListId});
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
}

bool Take(LootPlayer& player, WoodChest& chest, LootPlayer* bystander = nullptr)
{
    Open(player, chest);
    Require(player.GetLootGUID() == chest.GetGUID() && chest.getLootState() == GO_ACTIVATED, "Real Opening never entered the shared loot path");
    WorldPackets::Loot::LootResponse response;
    chest.loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    if (response.Items.empty())
    {
        Release(player, chest);
        return false;
    }
    Require(response.Items.size() == 1 && response.Items.front().Loot.ItemID == Wood && response.Items.front().Quantity == 1,
        "The chest awarded a different item or count");
    uint8 lootListId = response.Items.front().LootListID;
    uint32 before = player.GetItemCount(Wood);
    if (bystander)
    {
        Open(*bystander, chest);
        Require(!bystander->GetLootGUID(), "The second eligible player opened an empty window on an in-use chest");
        Release(*bystander, chest);
        Require(chest.getLootState() == GO_ACTIVATED && player.GetLootGUID() == chest.GetGUID() && !chest.loot.isLooted(),
            "A second session closed the eligible player's loot");
        for (uint8 position = INVENTORY_SLOT_ITEM_START; position < INVENTORY_SLOT_ITEM_END; ++position)
        {
            ItemPosCountVec destinations{{uint16(INVENTORY_SLOT_BAG_0 << 8 | position), 1}};
            Require(player.StoreNewItem(destinations, 2589, true), "Could not fill the ordinary inventory slots");
        }
        LootWood(player, chest, lootListId);
        Require(!player.GetItemCount(Wood) && !chest.loot.isLooted() && player.GetLootGUID() == chest.GetGUID(),
            "Full bags gave credit or consumed the available loot");
        player.DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, true);
    }
    LootWood(player, chest, lootListId);
    Require(player.GetItemCount(Wood) == before + 1 && player.GetQuestObjectiveData(QuestId, Wood) == int32(before + 1),
        "Loot did not add exactly one carried wood and one objective receipt");
    if (bystander)
        player.DestroyItemCount(2589, INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START, true);
    Release(player, chest);
    return true;
}

void Check(Quest const* quest)
{
    auto placements = Locations();
    Map map(1, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 784, placements.front().position);
    LootPlayer other(Session(), map, 785, placements.front().position);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->setFactionForRace(RACE_NIGHTELF);
        player->SetLevel(19);
        Require(!player->CanTakeQuest(quest, false), "Quest was offered before A Helping Hand");
        player->SetRewardedQuest(Prerequisite);
    }
    Require(owner.CanTakeQuest(quest, false), "A Helping Hand did not unlock the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    {
        WoodChest chest(map, placements.front());
        Require(chest.ActivateToQuest(&owner) && !chest.ActivateToQuest(&other), "Native quest highlight has incorrect admission");
        Open(other, chest);
#ifdef ASHENVALE_FEL_WOOD_BASELINE
        Require(chest.getLootState() == GO_ACTIVATED && chest.loot.isLooted() && other.GetLootGUID() == chest.GetGUID() &&
            !other.GetItemCount(Wood), "Original Opening did not open an empty shared chest without the quest");
        Release(other, chest);
        chest.Rearm(placements.front().respawn);
        Release(other, chest);
        Require(chest.getLootState() == GO_JUST_DEACTIVATED, "Original release did not consume a chest with no open loot");
        std::cout << "EXPECTED_BROKEN the unaccepted quest opened empty loot and a repeated release consumed the rearmed chest\n";
#else
        Require(chest.getLootState() == GO_READY && !other.GetLootGUID(), "Unaccepted quest changed the shared loot state");
        Release(other, chest);
        Require(chest.getLootState() == GO_READY && chest.isSpawned(), "Release without an open window consumed the chest");
#endif
    }
#ifndef ASHENVALE_FEL_WOOD_BASELINE
    other.AddQuestAndCheckCompletion(quest, nullptr);
    {
        auto const& placement = placements.front();
        WoodChest chest(map, placement);
        Open(owner, chest);
        Require(owner.GetLootGUID() == chest.GetGUID(), "The first eligible player could not open the chest");
        Position away = placement.position;
        away.m_positionX += 15.0f;
        MoveTo(owner, away);
        Open(other, chest);
        Require(!owner.GetLootGUID() && other.GetLootGUID() == chest.GetGUID() && chest.getLootState() == GO_ACTIVATED,
            "An opener who left interaction distance kept the chest unavailable");
        MoveTo(owner, placement.position);
        Release(owner, chest);
        Require(chest.getLootState() == GO_ACTIVATED, "The departed opener was able to close the next player's loot");
        Release(other, chest);
        chest.Rearm(placement.respawn);
    }
    for (auto const& placement : placements)
    {
        MoveTo(owner, placement.position);
        MoveTo(other, placement.position);
        WoodChest chest(map, placement);
        if (owner.GetItemCount(Wood) < 5)
        {
            Require(chest.ActivateToQuest(&owner) && !owner.CanRewardQuest(quest, false), "Early quest admission or highlight changed");
            if (Take(owner, chest, owner.GetItemCount(Wood) ? nullptr : &other))
            {
                chest.Rearm(placement.respawn);
                Release(owner, chest);
                Require(chest.getLootState() == GO_READY, "A queued release consumed a rearmed chest");
            }
        }
        else
        {
            Require(!chest.ActivateToQuest(&owner), "A complete quest is still highlighted");
            Open(owner, chest);
            Require(chest.getLootState() == GO_READY && !owner.GetLootGUID(), "Complete quest can open unused fel wood");
        }
    }
    Require(owner.GetItemCount(Wood) == 5 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE &&
        owner.CanRewardQuest(quest, false), "Five real fel wood loot receipts did not allow reward");
    Actor receiver(&map, 17310, 3000, owner.GetPosition());
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "The actual quest recipient could not be instantiated");
    owner.RewardQuest(quest, 0, &receiver, false);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Wood), "Native reward left wood behind");
    MoveTo(owner, placements.front().position);
    MoveTo(other, placements.front().position);
    WoodChest chest(map, placements.front());
    Open(owner, chest);
    Require(chest.isSpawned() && chest.getLootState() == GO_READY && !chest.ActivateToQuest(&owner) &&
        !owner.GetLootGUID(), "Rewarded quest kept chest activation or opened a fresh chest");
    bool taken = false;
    for (uint32 attempt = 0; attempt < 5 && !taken; ++attempt)
    {
        taken = Take(other, chest);
        chest.Rearm(placements.front().respawn);
    }
    Require(taken && !owner.GetItemCount(Wood), "A second eligible player could not loot the shared chest after rearm");
    Open(other, chest);
    Require(other.GetLootGUID() == chest.GetGUID(), "Could not open the stock before abandonment");
    WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    abandon.Entry = other.FindQuestSlot(QuestId);
    other.GetSession()->HandleQuestLogRemoveQuest(abandon);
    LootWood(other, chest, 1);
    Require(!other.GetItemCount(Wood) && other.GetQuestStatus(QuestId) == QUEST_STATUS_NONE,
        "Abandon did not clean up collected wood or a stale loot slot bypassed the quest check");
    Release(other, chest);
    chest.Rearm(placements.front().respawn);
    Open(other, chest);
    Require(!other.GetItemCount(Wood) && !other.GetLootGUID() && chest.getLootState() == GO_READY && !chest.ActivateToQuest(&other),
        "Abandon left fel wood or chest access");
    Require(other.CanTakeQuest(quest, false), "Abandoning blocked reacceptance");
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(chest.ActivateToQuest(&other), "Reacceptance did not restore the chest highlight");
    std::cout << "PASS fifteen real chest placements, native Opening and release, shared rearm, bag cleanup, reward and reacceptance\n";
#endif
}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoadGameObjectModelList(argv[2]);
        auto quest = LoadInputs();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_ashenvale();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef ASHENVALE_FEL_WOOD_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward requires a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
        sObjectMgr->LoadPlayerInfo();
#endif
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        Check(quest);
#ifndef ASHENVALE_FEL_WOOD_BASELINE
        CharacterDatabase.Close();
#endif
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
