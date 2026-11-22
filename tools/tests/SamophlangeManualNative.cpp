#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 ReadTheManual = 14006;
constexpr uint32 LegacyManual = 3924;
constexpr uint32 Prerequisite = 14004;
constexpr uint32 Sputtervalve = 3442;
constexpr uint32 Cover = 11147;
constexpr uint32 Page = 11148;
constexpr uint32 Manual = 11149;
constexpr uint32 Assemble = 13714;
constexpr char PagePlacementQuery[] = "SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
    "WHERE id IN (3283,3286) AND map=1 AND zoneId=17 AND areaId=381 ORDER BY id,guid";

Position ManualPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

void LoadManualQuests()
{
    auto rows = WorldDatabase.Query(
#include "NativeSamophlangeManual.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "Expected the old manual, the prerequisite and Read the Manual");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    for (uint32 questId : {ReadTheManual, LegacyManual})
    {
        auto quest = quests[questId];
        auto objective = WorldDatabase.PQuery("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
            "FROM quest_objectives WHERE QuestID=%u", questId);
        Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
            objective->Fetch()[4].GetUInt32() == Manual && objective->Fetch()[5].GetUInt32() == 1, "Expected the actual completed manual item");
        quest->LoadQuestObjective(objective->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    }
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14006");
    auto quest = quests[ReadTheManual];
    Require(addon && addon->Fetch()[4].GetUInt32() == Prerequisite && addon->Fetch()[5].GetUInt32() == 29094 &&
        !quest->SourceItemId && quest->ItemDrop[0] == Cover && quest->ItemDropQuantity[0] == 1 &&
        quest->ItemDrop[1] == Page && quest->ItemDropQuantity[1] == 5, "Original quest sequence and component roles changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quest->prevQuests.push_back(quest->PrevQuestID);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry IN (3283,3286,9336)";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded && LootTemplates_Creature.HaveLootFor(3283) && LootTemplates_Creature.HaveLootFor(3286) && LootTemplates_Creature.HaveLootFor(9336),
        "The actual cover and page tables failed to load");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE manual_starter AS SELECT * FROM creature_queststarter WHERE quest=14006");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM manual_starter");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE manual_receiver AS SELECT * FROM creature_questender WHERE quest=14006");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM manual_receiver");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE manual_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceEntry=11147");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM manual_conditions");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sConditionMgr->LoadConditions();
}

void AcceptManual(Player& player, Creature const& giver)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = ReadTheManual;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

bool TakeComponent(LootPlayer& player, LootPlayer& outsider, Map& map, Field* row, uint32 component)
{
    Position point = ManualPosition(row + 2);
    SupplyMove(player, point);
    map.LoadGrid(point.GetPositionX(), point.GetPositionY());
    Actor source(&map, row[1].GetUInt32(), row[0].GetUInt32(), point);
    Require(map.AddToMap(static_cast<Creature*>(&source)) && source.GetAreaId() == 381, "An actual component source is absent from Boulder Lode Mine");
    Require(player.DealDamage(&source, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) && source.isDead(false),
        "Could not defeat an actual component source");
    Loot& loot = source.loot;
    WorldPackets::Loot::LootResponse response, inactiveResponse;
    loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    loot.BuildLootResponse(inactiveResponse, &outsider, ALL_PERMISSION);
    Require(std::none_of(inactiveResponse.Items.begin(), inactiveResponse.Items.end(),
        [component](auto const& row) { return row.Loot.ItemID == component; }), "The quest component was exposed to a player without the quest");
    for (auto const& item : response.Items)
        if (item.Loot.ItemID == component)
        {
            auto before = player.GetItemCount(component);
            player.StoreLootItem(item.LootListID - 1, &loot);
            Require(item.Quantity == 1 && player.GetItemCount(component) == before + 1, "An actual component slot did not store exactly one item");
            return true;
        }
    return false;
}

void CheckManual()
{
    auto master = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE guid=86025 AND id=3442 AND map=1");
    auto cover = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=86061 AND id=9336 AND map=1 AND zoneId=17 AND areaId=381");
    auto pageSources = WorldDatabase.Query(PagePlacementQuery);
    Require(master && cover && pageSources && pageSources->GetRowCount() == 39, "Missing Sputtervalve, Boss Copperplug or the 39 page sources");
    Map map(1, 504, 0, DIFFICULTY_NONE);
    Position masterPos = ManualPosition(master->Fetch() + 1);
    map.LoadGrid(masterPos.GetPositionX(), masterPos.GetPositionY());
    Actor giver(&map, Sputtervalve, master->Fetch()[0].GetUInt32(), masterPos);
    Require(map.AddToMap(static_cast<Creature*>(&giver)), "Could not publish the actual Sputtervalve");
    LootPlayer owner(SupplySession(), map, 1400601, masterPos), other(SupplySession(), map, 1400602, masterPos);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    GiveSupply(other, Cover, 1);
    GiveSupply(other, Page, 5);
#ifdef SAMOPHLANGE_MANUAL_BASELINE
    Require(other.CastSpell(&other, Assemble, false, other.GetItemByEntry(Cover)) == SPELL_CAST_OK, "The original out-of-quest recipe did not start");
    Tick(other, 2400);
    Require(other.GetItemCount(Manual) == 1 && other.GetQuestStatus(ReadTheManual) == QUEST_STATUS_NONE,
        "The original assembly did not create a manual outside the quest");
    other.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(LegacyManual), nullptr);
    AbandonSupply(other, LegacyManual);
    Require(other.GetItemCount(Manual) == 1, "The original abandoned legacy quest no longer retained its manual");
    std::cout << "EXPECTED_BROKEN the native cover and pages made a manual without either quest and abandonment retained it\n";
    return;
#else
    Require(other.CastSpell(&other, Assemble, false, other.GetItemByEntry(Cover)) != SPELL_CAST_OK, "The real cover worked without either quest");
    AcceptManual(owner, giver);
    Require(owner.GetQuestStatus(ReadTheManual) == QUEST_STATUS_NONE, "Sputtervalve bypassed the actual prerequisite");
    owner.SetRewardedQuest(Prerequisite);
    AcceptManual(owner, giver);
    Require(owner.GetQuestStatus(ReadTheManual) == QUEST_STATUS_INCOMPLETE && !owner.GetItemCount(Cover) &&
        owner.HasQuestForItem(Cover) && owner.HasQuestForItem(Page), "The genuine quest did not open both component sources");
    Require(TakeComponent(owner, other, map, cover->Fetch(), Cover), "The chief engineer's guaranteed native cover did not drop");
    Require(owner.CastSpell(&owner, Assemble, false, owner.GetItemByEntry(Cover)) != SPELL_CAST_OK, "The cover assembled before five pages were available");

    std::set<uint32> pageKinds;
    for (uint32 pass = 0; pass < 3 && owner.GetItemCount(Page) < 5; ++pass)
    {
        do
        {
            Field* row = pageSources->Fetch();
            uint32 entry = row[1].GetUInt32();
            if (owner.GetItemCount(Page) == 4 && pageKinds.size() == 1 && pageKinds.count(entry))
                continue;
            if (TakeComponent(owner, other, map, row, Page))
                pageKinds.insert(entry);
        } while (owner.GetItemCount(Page) < 5 && pageSources->NextRow());
        pageSources = WorldDatabase.Query(PagePlacementQuery);
        Require(bool(pageSources), "Missing the original page placements for a retry");
    }
    Require(owner.GetItemCount(Page) == 5 && pageKinds.size() == 2 && !owner.HasQuestForItem(Page),
        "Five native pages did not roll from both placed author types or cap the collection");
    Require(owner.CastSpell(&owner, Assemble, false, owner.GetItemByEntry(Page)) != SPELL_CAST_OK, "A page impersonated the cover");
    Require(owner.CastSpell(&owner, Assemble, false, owner.GetItemByEntry(Cover)) == SPELL_CAST_OK, "The authentic manual could not start assembly");
    Tick(owner, 1000);
    Require(owner.GetItemCount(Page) == 5 && owner.GetItemCount(Cover) == 1 && !owner.GetItemCount(Manual), "The native two-second assembly consumed materials early");
    Tick(owner, 1400);
    Require(!owner.GetItemCount(Page) && !owner.GetItemCount(Cover) && owner.GetItemCount(Manual) == 1 && owner.GetQuestStatus(ReadTheManual) == QUEST_STATUS_COMPLETE,
        "The genuine assembly did not consume exactly the cover and five pages and complete its item objective");
    GiveSupply(owner, Cover, 1);
    GiveSupply(owner, Page, 5);
    SupplyMove(owner, masterPos);
    auto quest = sQuestDataStore->GetQuestTemplate(ReadTheManual);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (QuestPackageItemEntry const* item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(quest->PackageID == 1727 && choice && owner.CanInteractWithQuestGiver(&giver) && owner.CanRewardQuest(quest, choice, false),
        "Expected Sputtervalve's actual choice reward");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = ReadTheManual;
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(ReadTheManual) && !owner.GetItemCount(Manual) && !owner.GetItemCount(Cover) && !owner.GetItemCount(Page),
        "Sputtervalve left a quest item after actual reward");

    other.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(LegacyManual), nullptr);
    Require(other.CastSpell(&other, Assemble, false, other.GetItemByEntry(Cover)) == SPELL_CAST_OK, "An existing accepted old manual was no longer usable");
    Tick(other, 2400);
    Require(other.GetItemCount(Manual) == 1 && other.GetQuestStatus(LegacyManual) == QUEST_STATUS_COMPLETE, "The legitimate legacy attempt did not keep its native recipe");
    AbandonSupply(other, LegacyManual);
    Require(!other.GetItemCount(Manual), "Abandoning the only manual quest left its existing completed manual");
    other.SetRewardedQuest(Prerequisite);
    AcceptManual(other, giver);
    GiveSupply(other, Cover, 1);
    GiveSupply(other, Page, 5);
    Sleep(1800);
    GameTime::UpdateGameTimers();
    Require(other.CastSpell(&other, Assemble, false, other.GetItemByEntry(Cover)) == SPELL_CAST_OK, "The new manual could not start before cancellation");
    Tick(other, 1000);
    AbandonSupply(other, ReadTheManual);
    Require(!other.GetItemCount(Cover) && !other.GetItemCount(Page) && !other.GetItemCount(Manual), "Abandoning the manual retained components or output");
    AcceptManual(other, giver);
    GiveSupply(other, Cover, 1);
    GiveSupply(other, Page, 5);
    Tick(other, 1800);
    Require(!other.GetItemCount(Manual) && other.GetItemCount(Cover) == 1 && other.GetItemCount(Page) == 5, "A stale cast consumed the fresh attempt's ingredients");
    other.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(LegacyManual), nullptr);
    AbandonSupply(other, LegacyManual);
    Require(other.GetItemCount(Cover) == 1 && other.GetItemCount(Page) == 5 && other.GetQuestStatus(ReadTheManual) == QUEST_STATUS_INCOMPLETE,
        "Abandoning one manual quest removed the other active quest's components");
    AbandonSupply(other, ReadTheManual);
    Require(!other.GetItemCount(Cover) && !other.GetItemCount(Page), "Abandoning the last manual quest kept shared components");
    std::cout << "PASS actual native cover, both page sources, accepted manual and legacy assembly, cancelled retry and Sputtervalve reward\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        LoadManualQuests();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckManual();
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
