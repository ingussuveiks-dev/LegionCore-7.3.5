#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "QuestPackets.h"

void AddSC_go_scripts();

namespace
{
constexpr uint32 SelectedQuests[] = {13909, 28325, 29360, 29418};

void LoadSupplyInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeQuestSupplyChests.inc"
    );
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    Require(rows && rows->GetRowCount() == std::size(SelectedQuests), "Missing supply quest templates");
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives "
        "WHERE QuestID IN(13909,28325,29360,29418)");
    Require(objectives && objectives->GetRowCount() == std::size(SelectedQuests), "Missing exact delivery objectives");
    do
    {
        auto fields = objectives->Fetch();
        auto quest = quests[fields[1].GetUInt32()];
        quest->LoadQuestObjective(fields);
    } while (objectives->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(13909,28325,29360,29418)");
    Require(addons && addons->GetRowCount() == std::size(SelectedQuests), "Missing supply quest addon inputs");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (addons->NextRow());
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry IN(195080,35870,39791,40266)";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 4, "Expected four actual exclusive loot tables");
}

struct Placement
{
    uint32 entry;
    uint32 guid;
    uint32 respawn;
    Position position;
    G3D::Quat rotation;
};

std::vector<Placement> FindSupplies(uint32 map, uint32 zone, uint32 item, uint64 expected)
{
    auto rows = WorldDatabase.PQuery(
        "SELECT g.id,g.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,g.rotation0,g.rotation1,g.rotation2,g.rotation3 "
        "FROM gameobject g JOIN gameobject_template t ON t.entry=g.id WHERE g.map=%u AND g.zoneId=%u AND t.questItem1=%u "
        "AND g.id IN(195042,195080,207089,207090,208874,208876,209326,209327) ORDER BY g.id,g.guid", map, zone, item);
    Require(rows && rows->GetRowCount() == expected, "Unexpected original supply placements");
    std::vector<Placement> result;
    do
    {
        auto row = rows->Fetch();
        result.push_back({row[0].GetUInt32(),row[1].GetUInt32(),row[2].GetUInt32(),
            {row[3].GetFloat(),row[4].GetFloat(),row[5].GetFloat(),row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(),row[8].GetFloat(),row[9].GetFloat(),row[10].GetFloat())});
    } while (rows->NextRow());
    return result;
}

struct Supply : GameObject
{
    Supply(Map& map, Placement const& placement) : respawn(placement.respawn)
    {
        m_DBTableGuid = placement.guid;
        Require(Create(placement.guid, placement.entry, &map, 1, placement.position, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish original supply object");
        SetRespawnDelayTime(respawn);
        Update(10000);
        Require(getLootState() == GO_READY && GetGoType() == GAMEOBJECT_TYPE_CHEST && !IsPersonal(),
            "Supply object did not initialize as an original shared chest");
    }

    ~Supply()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }

    void Rearm()
    {
        Update(10000);
        Require(!isSpawned() && m_respawnTime == GameTime::GetGameTime() + respawn, "Native stock timer changed");
        m_respawnTime = GameTime::GetGameTime();
        Update(10000);
        Require(isSpawned() && getLootState() == GO_READY, "Native stock did not respawn");
    }

    uint32 const respawn;
};

WorldSession* SupplySession()
{
    return new WorldSession(0, "Native supply chest probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void SupplyMove(Player& player, Position const& point)
{
    player.GetMap()->PlayerRelocation(&player, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
}

void OpenSupply(LootPlayer& player, Supply& chest)
{
    auto lock = sLockStore.LookupEntry(chest.GetGOInfo()->GetLockId());
    Require(lock, "Original chest has no DB2 lock");
    for (uint8 index = 0; index < MAX_LOCK_CASE; ++index)
        if (lock->Type[index] == LOCK_KEY_SKILL && (lock->Index[index] == LOCKTYPE_OPEN || lock->Index[index] == LOCKTYPE_OPEN_KNEELING))
        {
            uint32 spell = lock->Index[index] == LOCKTYPE_OPEN ? 3365 : 6478;
            Require(player.CastSpell(&chest, spell, false) == SPELL_CAST_OK, "The native lock opening did not start");
            Tick(player, 6000);
            return;
        }
    std::cout << "CASE unexpected lock=" << chest.GetGOInfo()->GetLockId() << " " << unsigned(lock->Type[0]) << " " << lock->Index[0] << std::endl;
    Require(false, "Unhandled DB2 opening category");
}

void ReleaseSupply(LootPlayer& player, Supply& chest)
{
    WorldPackets::Loot::LootRelease packet{WorldPacket(CMSG_LOOT_RELEASE)};
    packet.Unit = chest.GetGUID();
    player.GetSession()->HandleLootRelease(packet);
}

bool TakeSupply(LootPlayer& player, Supply& chest, uint32 item)
{
    Require(player.GetLootGUID() == chest.GetGUID() && chest.getLootState() == GO_ACTIVATED, "Eligible player never opened its native loot");
    WorldPackets::Loot::LootResponse response;
    chest.loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    if (response.Items.empty())
        return false; // Preserve the real loot chance.
    Require(response.Items.size() == 1 && response.Items.front().Loot.ItemID == item, "Supply table gave a different item");
    auto before = player.GetItemCount(item);
    WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
    packet.Loot.push_back({chest.loot.GetGUID(), response.Items.front().LootListID});
    player.GetSession()->HandleAutostoreLootItemOpcode(packet);
    Require(player.GetItemCount(item) == before + response.Items.front().Quantity, "Advertised supply loot was not stored");
    return true;
}

void AbandonSupply(LootPlayer& player, uint32 quest)
{
    WorldPackets::Quest::QuestLogRemoveQuest packet{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    packet.Entry = player.FindQuestSlot(quest);
    Require(packet.Entry != MAX_QUEST_LOG_SIZE, "Missing active quest to abandon");
    player.GetSession()->HandleQuestLogRemoveQuest(packet);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_NONE, "Could not abandon supply quest");
}

void GiveSupply(LootPlayer& player, uint32 item, uint32 count)
{
    ItemPosCountVec slots;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, slots, item, count) == EQUIP_ERR_OK && player.StoreNewItem(slots, item, true),
        "Could not prepare the existing items for the last loot roll");
}

void CheckSupplies(uint32 questId, uint32 mapId, uint32 zone, uint32 expected, uint8 race)
{
    Quest const* quest = sQuestDataStore->GetQuestTemplate(questId);
    auto item = uint32(quest->Objectives.front().ObjectID);
    auto amount = uint32(quest->Objectives.front().Amount);
    auto placements = FindSupplies(mapId, zone, item, expected);
    Map map(mapId, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 900 + questId, placements.front().position);
    LootPlayer other(SupplySession(), map, 1100 + questId, placements.front().position);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
        player->setFactionForRace(race);
        player->SetLevel(20);
    }
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    std::set<uint32> entries;
    bool collected = false;
    for (auto const& placement : placements)
    {
        Supply stock(map, placement);
        auto first = entries.insert(placement.entry).second;
        Require(stock.ActivateToQuest(&owner) && !stock.ActivateToQuest(&other), "Wrong quest marker");
        if (!first)
            continue;
        SupplyMove(other, placement.position);
        OpenSupply(other, stock);
#ifdef SUPPLY_CHESTS_BASELINE
        Require(other.GetLootGUID() == stock.GetGUID() && stock.getLootState() == GO_ACTIVATED && stock.loot.isLooted(),
            "Original chest no longer opens empty loot for a character without the quest");
        ReleaseSupply(other, stock);
        stock.Rearm();
#else
        Require(!other.GetLootGUID() && stock.getLootState() == GO_READY, "A character without the quest activated shared stock");
        ReleaseSupply(other, stock);
        Require(stock.getLootState() == GO_READY, "An unopened stock could be consumed");
#endif
        SupplyMove(owner, placement.position);
        OpenSupply(owner, stock);
        collected |= TakeSupply(owner, stock, item);
        ReleaseSupply(owner, stock);
        stock.Rearm();
        // Reset the test inventory between different chest variants.
        owner.DestroyItemCount(item, owner.GetItemCount(item), true);
    }
    Require(collected, "Expected at least one actual loot receipt");
    auto const& first = placements.front();
    Supply stock(map, first);
    SupplyMove(owner, first.position);
    bool taken = false;
    for (uint32 trial = 0; trial < 6 && !taken; ++trial)
    {
        OpenSupply(owner, stock);
        taken = TakeSupply(owner, stock, item);
        ReleaseSupply(owner, stock);
        stock.Rearm();
    }
    Require(taken, "No actual item was available for the abandonment regression");
    AbandonSupply(owner, questId);
#ifdef SUPPLY_CHESTS_BASELINE
    Require(owner.GetItemCount(item), "Baseline did not reproduce quest loot left in bags");
    std::cout << "EXPECTED_BROKEN " << questId << " empty shared opening and retained abandoned supply\n";
#else
    Require(!owner.GetItemCount(item), "Abandonment retained quest loot");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    if (questId == 29360)
        GiveSupply(owner, item, amount - 1);
    for (uint32 trial = 0; trial < 30 && owner.GetItemCount(item) < amount; ++trial)
    {
        OpenSupply(owner, stock);
        TakeSupply(owner, stock, item);
        ReleaseSupply(owner, stock);
        stock.Rearm();
        if (questId == 29360 && owner.GetItemCount(item) == amount && trial < 15)
            owner.DestroyItemCount(item, 1, true);
    }
    std::cout << "CASE " << questId << " carried=" << owner.GetItemCount(item) << " required=" << amount
        << " progress=" << owner.GetQuestObjectiveData(questId, item) << " status=" << unsigned(owner.GetQuestStatus(questId)) << '\n';
    Require(owner.GetItemCount(item) >= amount && owner.CanRewardQuest(quest, false), "Real supply loot did not permit reward");
    if (questId == 29360)
        Require(owner.GetItemCount(item) > amount, "Expected a real last stack with spare flies");
    OpenSupply(owner, stock);
    Require(!owner.GetLootGUID() && stock.getLootState() == GO_READY && !stock.ActivateToQuest(&owner), "Completed quest still activates stock");
    uint32 choice = quest->RewardChoiceItemId[0];
    if (!choice && quest->PackageID)
        if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
            for (auto entry : *choices)
                if (owner.CanSelectQuestPackageItem(entry))
                {
                    choice = entry->ItemID;
                    break;
                }
    owner.RewardQuest(quest, choice, nullptr, false);
    Require(!owner.GetItemCount(item) && owner.IsQuestRewarded(questId), "Native reward kept supply or did not finish the quest");
    OpenSupply(owner, stock);
    Require(!owner.GetLootGUID() && stock.getLootState() == GO_READY, "A rewarded quest still opens stock");
    std::cout << "PASS " << questId << " " << placements.size() << " placements, " << entries.size()
        << " templates, native lock, legal loot, reopen, abandonment and reward cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}, {860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}, {860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoadSupplyInputs();
#ifndef SUPPLY_CHESTS_BASELINE
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_go_scripts();
        sScriptMgr->SwapScriptContext(true);
#endif
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
#ifndef SUPPLY_CHESTS_BASELINE
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward writes require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
        sObjectMgr->LoadPlayerInfo();
#endif
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSupplies(13909, 1, 148, 26, RACE_NIGHTELF);
        CheckSupplies(28325, 0, 267, 16, RACE_UNDEAD_PLAYER);
        CheckSupplies(29360, 0, 1497, 31, RACE_UNDEAD_PLAYER);
        CheckSupplies(29418, 860, 5736, 17, RACE_PANDAREN_NEUTRAL);
#ifndef SUPPLY_CHESTS_BASELINE
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
