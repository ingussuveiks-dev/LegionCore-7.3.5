#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 Vision = 26320;
constexpr uint32 Rise = 26322;
constexpr uint32 Finale = 42372;
constexpr uint32 Marshal = 234;
constexpr uint32 MarshalSpawn = 107102;
constexpr uint32 King = 107574;
constexpr uint32 KingSpawn = 265840;
constexpr uint32 StageActors[] = {234, 42308, 42372, 42744, 42745, 42748, 42749, 42750, 42751, 42752, 42753, 42755, 42769, 42778, 500001};

void LoadRise()
{
    auto rows = WorldDatabase.Query(
#include "NativeWestfallRise.inc"
    );
    Require(rows && rows->GetRowCount() == 19, "Original Westfall prerequisites and phases are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(46283);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN (26322,26370) ORDER BY ID");
    Require(addon && addon->GetRowCount() == 2, "The original Brotherhood requirements are missing");
    quests[Rise]->LoadQuestTemplateAddon(addon->Fetch());
    quests[Rise]->prevQuests.push_back(Vision);
    addon->NextRow();
    quests[26370]->LoadQuestTemplateAddon(addon->Fetch());
    quests[26370]->prevQuests.push_back(Rise);
    Require(quests[Rise]->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT), "The original eyewitness event flag is required");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=0 AND "
        "((zoneId=40 AND areaId=108 AND id IN (234,42308,42372,42744,42745,42748,42749,42750,42751,42752,42753,42755,42769,42778,500001)) "
        "OR guid IN (344578,265840,11039643))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT a.* FROM legion_world.creature_addon a JOIN creature c ON a.guid=c.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_equip_template AS SELECT * FROM legion_world.creature_equip_template WHERE CreatureID IN (42750,42751)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon "
        "WHERE entry IN (234,42308,42372,42744,42745,42748,42749,42750,42751,42752,42753,42755,42769,42778,500001)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest=26322 OR (quest=26370 AND id IN (100429,107574,109466))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest=26322 AND id IN (100429,107574,109466)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text "
        "WHERE CreatureID IN (234,42308,42372,42744,42745,42748,42749,42750,42751,42752,42753,42755,42769,42778,500001)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE (source_type=0 AND entryorguid IN (234,42308,42372,42744,42745,42748,42749,42750,42751,42752,42753,42755,42769,42778,500001)) "
        "OR (source_type=9 AND entryorguid IN (23400,4237200,4274900,4275000))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions "
        "WHERE (zoneId=40 AND entry IN (1,5,6,7,8,10)) OR (zoneId=1519 AND entry BETWEEN 5 AND 12)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=40 AND SourceEntry IN (1,5,6,7,8,10)) OR (SourceGroup=1519 AND SourceEntry BETWEEN 5 AND 12))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE quest_start=26322");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE waypoints AS SELECT * FROM legion_world.waypoints WHERE entry IN (42371,42372)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadPhaseDefinitions();
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartWaypointMgr->LoadFromDB();
    sSmartScriptMgr->LoadSmartAIFromDB();
}

void CheckRise()
{
    Map map(0, 300, 0, DIFFICULTY_NONE);
    auto data = sObjectMgr->GetCreatureData(MarshalSpawn);
    Require(data, "Missing the original Sentinel Hill Marshal");
    map.LoadGrid(data->posX, data->posY);
    auto giver = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, Marshal, MarshalSpawn));
    Require(giver && giver->hasQuest(Rise), "The original Marshal questgiver was not present");
    LootPlayer witness(SupplySession(), map, 2632201, giver->GetPosition());
    LootPlayer second(SupplySession(), map, 2632202, giver->GetPosition());
    LootPlayer unaccepted(SupplySession(), map, 2632203, giver->GetPosition());
    LootPlayer distant(SupplySession(), map, 2632204, giver->GetPosition());
    for (LootPlayer* player : {&witness, &second, &unaccepted, &distant})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(player->getRace());
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetRewardedQuest(Vision);
        MoveGilneasPlayer(*player, giver->GetPosition());
    }
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver->GetGUID();
    accept.QuestID = Rise;
    witness.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(witness.GetQuestStatus(Rise) == QUEST_STATUS_INCOMPLETE, "The original Marshal did not start the real Brotherhood witness quest");
    MoveGilneasPlayer(witness, giver->GetPosition());
    Quest const* quest = sQuestDataStore->GetQuestTemplate(Rise);
    second.AddQuestAndCheckCompletion(quest, nullptr);
    distant.AddQuestAndCheckCompletion(quest, nullptr);
    MoveGilneasPlayer(second, giver->GetPosition());
    Position beyondFinale = giver->GetPosition();
    beyondFinale.m_positionX += 129.0f;
    MoveGilneasPlayer(distant, beyondFinale);

    bool sawFinale = false;
    uint32 elapsed = 0;
    for (; elapsed < 190000 && witness.GetQuestStatus(Rise) == QUEST_STATUS_INCOMPLETE; elapsed += 250)
    {
        std::list<Creature*> actors;
        for (uint32 entry : StageActors)
            witness.GetCreatureListWithEntryInGrid(actors, entry, 125.0f);
        for (Creature* actor : actors)
            if (actor->IsAlive())
            {
                sawFinale |= actor->GetEntry() == Finale && actor->isSummon();
                actor->Update(250);
            }
        Tick(witness, 250);
        Tick(second, 250);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Westfall story finale=" << sawFinale << " status=" << witness.GetQuestStatus(Rise)
        << " elapsed=" << elapsed << " phase=" << witness.GetPhases().size() << std::endl;
    Require(sawFinale && witness.GetQuestStatus(Rise) == QUEST_STATUS_COMPLETE && elapsed > 20000,
        "The configured Marshal and Hope dialogue did not reach the actual Vanessa finale");
    Require(second.GetQuestStatus(Rise) == QUEST_STATUS_COMPLETE && unaccepted.GetQuestStatus(Rise) == QUEST_STATUS_NONE &&
        distant.GetQuestStatus(Rise) == QUEST_STATUS_INCOMPLETE,
        "The finale must count for another eligible eyewitness and exclude unaccepted or distant players");

    auto kingData = sObjectMgr->GetCreatureData(KingSpawn);
    Require(kingData, "Missing the original low-level Stormwind quest receiver");
    map.LoadGrid(kingData->posX, kingData->posY);
    auto king = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, King, KingSpawn));
    Require(king && king->hasInvolvedQuest(Rise), "The original Stormwind receiver did not load the Brotherhood turn-in");
    auto campaignKing = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, 100429, 344578));
    auto alternateKing = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, 109466, 11039643));
    Require(campaignKing && alternateKing, "The real campaign receiver variants must be loaded for phase checks");
    MoveGilneasPlayer(witness, king->GetPosition());
    MoveGilneasPlayer(unaccepted, king->GetPosition());
    MoveGilneasPlayer(distant, king->GetPosition());
    Require(!unaccepted.CanRewardQuest(quest, false) && !distant.CanRewardQuest(quest, false),
        "The low-level receiver must require a real completed finale before rewarding the quest");
    std::cout << "CASE Stormwind zone=" << witness.GetZoneId() << " area=" << witness.GetAreaId()
        << " phase=" << witness.InSamePhase(king) << " interact=" << witness.CanInteractWithQuestGiver(king)
        << " reward=" << witness.CanRewardQuest(quest, false) << " phases=";
    for (uint32 phase : witness.GetPhases())
        std::cout << phase << ',';
    std::cout << std::endl;
    Require(witness.InSamePhase(king) && !witness.InSamePhase(campaignKing) && !witness.InSamePhase(alternateKing) &&
        witness.CanInteractWithQuestGiver(king) && witness.CanRewardQuest(quest, false),
        "The completed quest did not reach exactly one low-level Stormwind receiver");
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (witness.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(quest->PackageID == 2667 && choice && witness.CanRewardQuest(quest, choice, false),
        "The original reward package did not have an eligible Hunter item");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = king->GetGUID();
    reward.QuestID = Rise;
    reward.ItemChoiceID = choice;
    witness.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(witness.GetQuestRewardStatus(Rise) && witness.GetItemCount(choice) && king->hasQuest(26370),
        "The Stormwind reward did not give the selected item and expose the original return quest");
    MoveGilneasPlayer(witness, giver->GetPosition());
    MoveGilneasPlayer(witness, king->GetPosition());
    Require(witness.CanInteractWithQuestGiver(king) && witness.InSamePhase(king), "Re-entering the Keep lost the original return offer");
    accept.QuestGiverGUID = king->GetGUID();
    accept.QuestID = 26370;
    witness.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(witness.FindQuestSlot(26370) < MAX_QUEST_LOG_SIZE, "Anduin did not offer the original Return to Sentinel Hill quest");
    std::cout << "CASE shared eyewitness credit, no unaccepted or distant credit, Stormwind reward and original return acceptance\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "The isolated login pool is required");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "The isolated character pool is required");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The test must use isolated fixture characters");
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadRise();
        CheckRise();
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
