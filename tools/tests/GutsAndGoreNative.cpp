#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ItemEnchantmentMgr.h"

namespace
{
constexpr uint32 GutsQuest = 26995;
constexpr uint32 Apothecary = 44784;
constexpr uint32 Guts = 60742;
constexpr uint32 ChestEntry = 1765;
constexpr uint32 ChestLoot = 1570;
constexpr char BeastPlacementQuery[] = "SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
    "WHERE id IN (1765,1778) AND map=0 AND zoneId=130 ORDER BY id,guid";

Position GutsPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

Quest const* LoadGutsQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeGutsAndGore.inc"
    );
    Require(bool(row), "Expected the actual Guts and Gore quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[GutsQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(GutsQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26995");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Guts && objective->Fetch()[5].GetUInt32() == 6, "Expected six genuine clean guts");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=26995");
    Require(addon && !addon->Fetch()[4].GetUInt32() && addon->Fetch()[5].GetUInt32() == 26998, "Original Silverpine offer chain changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    Require(!quest->SourceItemId && !quest->PackageID && !quest->m_rewChoiceItemsCount, "Unexpected quest source or package changed the normal turn-in");
#ifdef GUTS_AND_GORE_BASELINE
    Require(!quest->ItemDrop[0] && !quest->ItemDropQuantity[0], "The original quest must reproduce missing abandonment cleanup metadata");
#else
    Require(quest->ItemDrop[0] == Guts && quest->ItemDropQuantity[0] == 6, "Native abandonment must know the same six-item requirement");
#endif

    LoadRandomEnchantmentsTable();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& chestTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto originalChest = chestTable;
    chestTable += " WHERE Entry=1570";
    auto chestRows = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    chestTable = originalChest;
    auto& beastTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalBeast = beastTable;
    beastTable += " WHERE Entry IN (1765,1778)";
    auto beastRows = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    beastTable = originalBeast;
    Require(chestRows && beastRows && LootTemplates_Gameobject.HaveLootFor(ChestLoot) &&
        LootTemplates_Creature.HaveLootFor(1765) && LootTemplates_Creature.HaveLootFor(1778), "Original chest or beast tables did not load");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE guts_starter AS SELECT * FROM creature_queststarter WHERE quest=26995");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM guts_starter");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE guts_receiver AS SELECT * FROM creature_questender WHERE quest=26995");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM guts_receiver");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

void AcceptGuts(Player& player, Creature const& giver)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = GutsQuest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    Require(player.GetQuestStatus(GutsQuest) == QUEST_STATUS_INCOMPLETE && player.HasQuestForItem(Guts), "The actual Silverpine offer did not admit clean guts");
}

bool TakeGuts(Loot& loot, LootPlayer& owner, LootPlayer& outsider)
{
    WorldPackets::Loot::LootResponse response, foreignResponse;
    loot.BuildLootResponse(response, &owner, ALL_PERMISSION);
    loot.BuildLootResponse(foreignResponse, &outsider, ALL_PERMISSION);
    Require(std::none_of(foreignResponse.Items.begin(), foreignResponse.Items.end(),
        [](auto const& row) { return row.Loot.ItemID == Guts; }), "The native quest loot exposed guts to a player without the quest");
    for (auto const& item : response.Items)
        if (item.Loot.ItemID == Guts)
        {
            auto before = owner.GetItemCount(Guts);
            owner.StoreLootItem(item.LootListID - 1, &loot);
            Require(item.Quantity == 1 && owner.GetItemCount(Guts) == before + 1, "Native gut slot did not store exactly one item");
            return true;
        }
    return false;
}

bool HuntBeast(Map& map, Field* row, LootPlayer& owner, LootPlayer& outsider)
{
    Position point = GutsPosition(row + 2);
    SupplyMove(owner, point);
    map.LoadGrid(point.GetPositionX(), point.GetPositionY());
    Actor beast(&map, row[1].GetUInt32(), row[0].GetUInt32(), point);
    Require(map.AddToMap(static_cast<Creature*>(&beast)) && beast.GetZoneId() == 130, "A placed clean beast was absent from Silverpine");
    Require(owner.DealDamage(&beast, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
        beast.isDead(false), "A native final blow did not kill the actual beast");
    return TakeGuts(beast.loot, owner, outsider);
}

void CheckGuts(Quest const* quest)
{
    auto vendor = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=44784 AND guid=369148 AND map=0 AND zoneId=130");
    auto wooden = WorldDatabase.Query("SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=1765 AND guid=39372 AND map=0 AND zoneId=267");
    auto beasts = WorldDatabase.Query(BeastPlacementQuery);
    Require(vendor && wooden && beasts && beasts->GetRowCount() == 58, "Expected the original apothecary, Hillsbrad chest and 58 actual Silverpine beasts");
    Map map(0, 505, 0, DIFFICULTY_NONE);
    Position vendorPos = GutsPosition(vendor->Fetch() + 1);
    map.LoadGrid(vendorPos.GetPositionX(), vendorPos.GetPositionY());
    Actor giver(&map, Apothecary, vendor->Fetch()[0].GetUInt32(), vendorPos);
    Require(map.AddToMap(static_cast<Creature*>(&giver)), "Could not publish the actual apothecary");
    LootPlayer owner(SupplySession(), map, 2699501, vendorPos), other(SupplySession(), map, 2699502, vendorPos);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        player->setFactionForRace(RACE_UNDEAD_PLAYER);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptGuts(owner, giver);
    auto chest = wooden->Fetch();
    Placement placement{ChestEntry, chest[0].GetUInt32(), chest[1].GetUInt32(), GutsPosition(chest + 2),
        G3D::Quat(chest[6].GetFloat(), chest[7].GetFloat(), chest[8].GetFloat(), chest[9].GetFloat())};
    SupplyMove(owner, placement.position);
    Supply stock(map, placement);
    Require(stock.GetGOInfo()->GetLootId() == ChestLoot && stock.GetZoneId() == 267, "The tested chest must be the original Hillsbrad chest");
    bool ordinaryLoot = false;
    for (uint32 roll = 0; roll < 256; ++roll)
    {
        Loot loot;
        Require(loot.FillLoot(stock.GetGOInfo()->GetLootId(), LootTemplates_Gameobject, &owner, true, false, &stock), "Could not roll the actual Hillsbrad chest loot");
        ordinaryLoot = ordinaryLoot || loot.GetLootItem(2672, 0) || loot.GetLootItem(62328, 0);
        bool acquired = TakeGuts(loot, owner, other);
#ifdef GUTS_AND_GORE_BASELINE
        if (acquired)
        {
            AbandonSupply(owner, GutsQuest);
            Require(owner.GetItemCount(Guts) == 1, "The original abandoned quest no longer retained the stray gut");
            std::cout << "EXPECTED_BROKEN the actual Hillsbrad chest supplied guts and abandoning the quest kept them\n";
            return;
        }
#else
        Require(!acquired, "The unrelated Hillsbrad chest still produced the quest item");
#endif
    }
#ifdef GUTS_AND_GORE_BASELINE
    Require(false, "No wrong-source gut dropped in 256 actual original rolls");
#else
    Require(ordinaryLoot, "The chest lost its legitimate ordinary loot");
    std::set<uint32> sources;
    for (uint32 pass = 0; pass < 3 && owner.GetItemCount(Guts) < 6; ++pass)
    {
        do
        {
            Field* row = beasts->Fetch();
            uint32 entry = row[1].GetUInt32();
            if (owner.GetItemCount(Guts) == 5 && sources.size() == 1 && sources.count(entry))
                continue;
            if (HuntBeast(map, row, owner, other))
                sources.insert(entry);
        } while (owner.GetItemCount(Guts) < 6 && beasts->NextRow());
        beasts = WorldDatabase.Query(BeastPlacementQuery);
        Require(bool(beasts), "Missing the original Silverpine beast placements for a retry");
    }
    Require(sources.size() == 2 && owner.GetItemCount(Guts) == 6 && owner.GetQuestStatus(GutsQuest) == QUEST_STATUS_COMPLETE &&
        !owner.HasQuestForItem(Guts), "Six genuine guts from both beast species should complete and cap the quest");
    SupplyMove(owner, vendorPos);
    Require(owner.CanInteractWithQuestGiver(&giver) && owner.CanRewardQuest(quest, false), "The original apothecary could not accept the six real guts");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = GutsQuest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(GutsQuest) && !owner.GetItemCount(Guts), "Actual reward kept one or more clean guts");
    AcceptGuts(other, giver);
    for (uint32 roll = 0; roll < 64 && !other.GetItemCount(Guts); ++roll)
        HuntBeast(map, beasts->Fetch(), other, owner);
    Require(other.GetItemCount(Guts) == 1, "The second player could not get a real gut before abandonment");
    AbandonSupply(other, GutsQuest);
    Require(!other.GetItemCount(Guts), "Actual abandonment kept the clean gut");
    std::cout << "PASS ordinary Hillsbrad chest loot, scoped Silverpine beasts, six actual pickups, reward and abandonment cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        auto quest = LoadGutsQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckGuts(quest);
        CharacterDatabase.Close();
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
