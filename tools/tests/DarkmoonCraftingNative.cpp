#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"

namespace
{
constexpr uint16 Faire = 75;
constexpr uint32 Island = 974;
constexpr uint32 NativeOpening = 3365;

struct ProfessionQuest
{
    uint32 QuestId;
    uint32 Giver;
    uint32 Spawn;
    uint16 Skill;
    uint32 Item;
    uint32 Spell;
    uint32 Result;
    uint32 Count;
    uint32 Object;
    uint32 Placements;
    uint32 Reagent1;
    uint32 Reagent2;
    uint32 Reagent3;
    uint32 Reagent3Count;
};

constexpr ProfessionQuest Professions[] =
{
    {29506, 14844, 2172, 171, 72043, 101988, 72044, 5, 0,      0, 19299, 1645,  0, 0},
    {29510, 14822, 2226, 333, 72018, 101985, 71979, 6, 209283,47,     0,    0,  0, 0},
    {29515, 14822, 2226, 773, 71972, 101868, 71974, 5, 0,      0, 39354,    0,  0, 0},
    {29516, 14833, 2313, 755, 72052, 102221, 72050, 5, 209287,55,     0,    0,  0, 0},
    {29517, 14841, 2140, 165, 71977, 101874, 71976, 5, 0,      0,  2320, 6260,6529,2}
};

void PrepareRows(char const* sql)
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute(sql);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
}

void LoadCrafts()
{
    auto rows = WorldDatabase.Query(
#include "NativeDarkmoonCrafting.inc"
    );
    Require(rows && rows->GetRowCount() == std::size(Professions), "Original Darkmoon profession quests are incomplete");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    for (auto const& data : Professions)
    {
        auto quest = quests[data.QuestId];
        auto objective = WorldDatabase.PQuery("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
            "FROM quest_objectives WHERE QuestID=%u", data.QuestId);
        Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
            objective->Fetch()[4].GetUInt32() == data.Result && objective->Fetch()[5].GetUInt32() == data.Count, "Original required craft quantity changed");
        quest->LoadQuestObjective(objective->Fetch());
        auto addon = WorldDatabase.PQuery("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
            "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
            "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=%u", data.QuestId);
        Require(addon && addon->Fetch()[9].GetUInt32() == data.Skill && addon->Fetch()[10].GetUInt32() == 75,
            "Original skill requirement changed");
        quest->LoadQuestTemplateAddon(addon->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
#ifdef DARKMOON_CRAFTING_BASELINE
        Require(!quest->ItemDrop[0], "The original craft result has a cleanup row");
#else
        Require(quest->ItemDrop[0] == data.Result && quest->ItemDropQuantity[0] == data.Count, "The result cleanup is missing");
#endif
        for (auto item : {data.Reagent1, data.Reagent2, data.Reagent3})
            if (item)
            {
                auto source = WorldDatabase.PQuery("SELECT c.guid FROM npc_vendor v JOIN creature c ON c.id=v.entry "
                    "WHERE v.item=%u AND v.ExtendedCost=0 LIMIT 1", item);
                Require(bool(source), "A required reagent has no real placed gold vendor");
            }
    }

    auto row = WorldDatabase.Query("SELECT occurence,length FROM game_event WHERE eventEntry=75 AND world_event=0");
    Require(bool(row), "Missing the configured Faire holiday");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(Faire + 1);
    events[Faire].occurence = row->Fetch()[0].GetUInt32();
    events[Faire].length = row->Fetch()[1].GetUInt32();

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootName = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = lootName;
    lootName += " WHERE Entry IN(40247,40250)";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootName = original;
    Require(loaded == 2, "Both genuine material loot groups must load");
    sQuestDataStore->LoadGameObjectForQuests();

    PrepareRows("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN(29506,29510,29515,29516,29517)");
    PrepareRows("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN(29506,29510,29515,29516,29517)");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    PrepareRows("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(29506,29510,29515,29516,29517)) "
        "OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(101988,101985,101863,101868,102221,101874))");
    sConditionMgr->LoadConditions();
}

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

void RequestCraft(LootPlayer& player, uint32 spell, uint32 item, bool expected)
{
    Item* input = player.GetItemByEntry(item);
    Require(input, "The actual cast item is missing");
    GameTime::UpdateGameTimers();
    auto result = player.CastSpell(&player, spell, false, input);
    if ((result == SPELL_CAST_OK) != expected)
    {
        std::cout << "CASE spell=" << spell << " cast-result=" << uint32(result) << " expected=" << expected << '\n';
        Require(false, "Native crafting admission failed");
    }
}

void AcceptCraft(LootPlayer& player, Creature const& giver, uint32 quest)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void BuyIngredients(LootPlayer& player, ProfessionQuest const& data, uint32 times)
{
    for (auto item : {data.Reagent1, data.Reagent2})
        if (item)
            GiveSupply(player, item, times);
    if (data.Reagent3)
        GiveSupply(player, data.Reagent3, data.Reagent3Count * times);
}

void CheckMaterials(Map& map, LootPlayer& owner, LootPlayer& other, ProfessionQuest const& data)
{
    auto rows = WorldDatabase.PQuery("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=%u AND map=974 AND zoneId=5861 ORDER BY guid", data.Object);
    Require(rows && rows->GetRowCount() == data.Placements, "One or more genuine source placements are missing");
    bool checkedClosed = false;
    do
    {
        auto row = rows->Fetch();
        Placement location{row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(), ReadPosition(row + 3),
            {row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat()}};
        SupplyMove(owner, location.position);
        SupplyMove(other, location.position);
        Supply box(map, location);
        Require(box.GetGOInfo()->GetLockId() == 1691 && !box.ActivateToQuest(&other), "The original chest or inactive marker differs");
        if (!checkedClosed)
        {
#ifdef DARKMOON_CRAFTING_BASELINE
            OpenSupply(other, box);
            Require(other.GetLootGUID() == box.GetGUID() && box.loot.isLooted(), "The original empty outside-quest loot window no longer reproduces");
            ReleaseSupply(other, box);
            return;
#else
            Require(other.CastSpell(&box, NativeOpening, false) != SPELL_CAST_OK, "The true stock still opened without the quest");
            other.SendLoot(box.GetGUID(), LOOT_SKINNING);
            Require(!other.GetLootGUID() && box.getLootState() == GO_READY, "The raw loot path bypassed the quest gate");
            checkedClosed = true;
#endif
        }
        if (owner.GetItemCount(data.Item) >= data.Count)
        {
            Require(!box.ActivateToQuest(&owner), "The marker remained active after gathering everything needed for the recipe");
            continue;
        }
        Require(box.ActivateToQuest(&owner), "The active quest did not mark actual material stock");
        OpenSupply(owner, box);
        TakeSupply(owner, box, data.Item);
        ReleaseSupply(owner, box);
    } while (rows->NextRow());
    Require(owner.GetItemCount(data.Item) == data.Count, "The original Faire stock did not deliver the required raw materials");
}

void CheckProfession(Map& map, ProfessionQuest const& data)
{
    auto actual = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=%u AND id=%u AND map=974 AND zoneId=5861", data.Spawn, data.Giver);
    Require(bool(actual), "The original Island giver is missing");
    Position position = ReadPosition(actual->Fetch());
    map.LoadGrid(position.GetPositionX(), position.GetPositionY());
    Actor giver(&map, data.Giver, data.Spawn, position);
    Require(map.AddToMap(static_cast<Creature*>(&giver)), "The actual Island giver could not be published");
    LootPlayer owner(SupplySession(), map, 3100000 + data.QuestId, position), other(SupplySession(), map, 3200000 + data.QuestId, position);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetSkill(data.Skill, 1, 75, 75);
    }
    auto quest = sQuestDataStore->GetQuestTemplate(data.QuestId);
    auto& holidays = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    Require(!owner.CanTakeQuest(quest, false), "The original Faire offer did not enforce the active event");
    holidays.insert(Faire);
    other.SetSkill(data.Skill, 1, 74, 75);
    Require(!other.CanTakeQuest(quest, false), "The original skill requirement admitted a player at 74 points");
    other.SetSkill(data.Skill, 1, 75, 75);
    BuyIngredients(other, data, 1);
    GiveSupply(other, data.Item, 1);

#ifdef DARKMOON_CRAFTING_BASELINE
    RequestCraft(other, data.Spell, data.Item, true);
    Tick(other, 6000);
    Require(other.GetItemCount(data.Result) == 1, "The old questless recipe no longer produces its result");
    AcceptCraft(owner, giver, data.QuestId);
    GiveSupply(owner, data.Result, 1);
    AbandonSupply(owner, data.QuestId);
    Require(owner.GetItemCount(data.Result) == 1, "The original abandonment now removes the crafted result");
    if (data.Object)
        CheckMaterials(map, owner, other, data);
    std::cout << "EXPECTED_BROKEN " << data.QuestId << " crafted an item without a quest and retained the output after abandoning\n";
#else
    RequestCraft(other, data.Spell, data.Item, false);
    AcceptCraft(owner, giver, data.QuestId);
    Require(owner.GetQuestStatus(data.QuestId) == QUEST_STATUS_INCOMPLETE, "The actual profession trainer did not start the quest");
    if (data.QuestId == 29506)
        RequestCraft(owner, data.Spell, data.Item, false);
    BuyIngredients(owner, data, data.Count);
    if (data.QuestId == 29506)
    {
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < owner.GetInventoryEndSlot(); ++slot)
            if (!owner.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                ItemPosCountVec destination;
                destination.emplace_back(uint16(INVENTORY_SLOT_BAG_0 << 8) | slot, 1);
                Require(owner.StoreNewItem(destination, 2589, true), "The full-bag test could not fill the native backpack");
            }
        RequestCraft(owner, data.Spell, data.Item, false);
        Require(owner.GetItemCount(data.Item) == 1 && owner.GetItemCount(data.Reagent1) == data.Count &&
            owner.GetItemCount(data.Reagent2) == data.Count && !owner.GetItemCount(data.Result), "A full backpack consumed its real recipe inputs");
        owner.DestroyItemCount(2589, owner.GetItemCount(2589), true);
    }
    if (data.Object)
        CheckMaterials(map, owner, other, data);
    else if (data.QuestId == 29515)
    {
        RequestCraft(owner, 101863, 71971, true);
        Tick(owner, 6000);
        Require(owner.GetItemCount(data.Item) == 1 && !owner.GetItemCount(71971), "The original herbs did not become one reusable Prophetic Ink");
    }

    holidays.erase(Faire);
    RequestCraft(owner, data.Spell, data.Item, false);
    holidays.insert(Faire);
    Require(owner.CastSpell(&owner, data.Spell, false) != SPELL_CAST_OK, "A direct spell bypassed its actual recipe item");
    Require(owner.CastSpell(&other, data.Spell, false, owner.GetItemByEntry(data.Item)) == SPELL_CAST_OK,
        "The native self-recipe did not normalize the selected player to the actual caster");
    Tick(owner, 6000);
    Require(owner.GetItemCount(data.Result) == 1 && !other.GetItemCount(data.Result), "The self recipe produced its result for a different player");

    for (uint32 count = 1; count < data.Count; ++count)
    {
        RequestCraft(owner, data.Spell, data.Item, true);
        Tick(owner, 6000);
        Require(owner.GetItemCount(data.Result) == count + 1 && !other.GetItemCount(data.Result), "Quest recipe did not make exactly one native result");
    }
    Require(owner.GetQuestStatus(data.QuestId) == QUEST_STATUS_COMPLETE, "Genuine crafting did not complete the objective");
    SupplyMove(owner, position);
    holidays.erase(Faire);
    Require(!owner.CanRewardQuest(quest, false), "A completed profession quest allowed an off-season reward");
    holidays.insert(Faire);
    if (data.QuestId == 29515)
    {
        AbandonSupply(owner, data.QuestId);
        Require(!owner.GetItemCount(71972) && !owner.GetItemCount(data.Result), "Abandoning fortunes left the reusable ink or finished result");
        AcceptCraft(owner, giver, data.QuestId);
        BuyIngredients(owner, data, data.Count);
        RequestCraft(owner, 101863, 71971, true);
        Tick(owner, 6000);
        for (uint32 count = 0; count < data.Count; ++count)
        {
            RequestCraft(owner, data.Spell, data.Item, true);
            Tick(owner, 6000);
        }
        Require(owner.GetQuestStatus(data.QuestId) == QUEST_STATUS_COMPLETE, "The newly accepted inscription quest did not reset its progress");
    }
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = data.QuestId;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.FindQuestSlot(data.QuestId) == MAX_QUEST_LOG_SIZE && !owner.GetItemCount(data.Result) && !owner.GetItemCount(data.Item),
        "Native profession reward left its recipe or objective item in the bag");
    std::cout << "PASS profession " << data.QuestId << " real item spell, inventory and quest progression\n";
#endif
    holidays.erase(Faire);
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{Island, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{Island, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Doing turn-ins requires an isolated character schema");
        LoadCrafts();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        {
            Map map(Island, 575, 0, DIFFICULTY_NONE);
            for (auto const& quest : Professions)
                CheckProfession(map, quest);
        }
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
