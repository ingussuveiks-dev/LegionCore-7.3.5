#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GossipDef.h"

namespace
{
constexpr uint32 Area = 5834;
constexpr uint32 GiverSpawn = 243564;
constexpr uint32 SandyFist = 29406;
constexpr std::pair<uint8, uint32> Lessons[] = {
    {CLASS_MONK, 30027}, {CLASS_MAGE, 30033}, {CLASS_HUNTER, 30034},
    {CLASS_PRIEST, 30035}, {CLASS_ROGUE, 30036}, {CLASS_SHAMAN, 30037}, {CLASS_WARRIOR, 30038}
};

void LoadLessonInputs()
{
    auto templates = WorldDatabase.Query(
#include "NativeWanderingLesson.inc"
    );
    Require(templates && templates->GetRowCount() == 15, "Missing the seven first lessons, seven old introductions or Sandy Fist");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID BETWEEN 30033 AND 30045 OR ID IN (30027,29406)");
    Require(addons && addons->GetRowCount() == 15, "Missing the actual lesson prerequisites and class masks");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
        auto const next = uint32(std::abs(quest->NextQuestID));
        if (next && next < quests.size() && quests[next])
            quests[next]->prevQuests.push_back(quest->NextQuestID < 0 ? -int32(quest->Id) : int32(quest->Id));
        if (quest->NextQuestIdChain && quest->NextQuestIdChain < quests.size() && quests[quest->NextQuestIdChain])
            quests[quest->NextQuestIdChain]->prevChainQuests.push_back(quest->Id);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives "
        "WHERE QuestID=29406 OR QuestID=30027 OR QuestID BETWEEN 30033 AND 30038");
    Require(bool(objectives), "Missing the original equipment or training objectives");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    auto oldAreaStarts = WorldDatabase.Query("SELECT COUNT(*) FROM area_queststart WHERE id=5834");
    Require(oldAreaStarts && !oldAreaStarts->Fetch()[0].GetUInt32(), "Expected the migrated area to have no automatic introduction");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=243564");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(29406,30027,30033,30034,30035,30036,30037,30038)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(29406,30027,30033,30034,30035,30036,30037,30038,30039,30040,30041,30042,30043,30044,30045)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadItemTemplates();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
}

struct LessonGiver : Creature
{
    explicit LessonGiver(Map& map) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(GiverSpawn);
        Require(data && data->id == 53566, "Missing Shang Xi's original training-grounds spawn");
        sObjectMgr->RemoveCreatureFromGrid(GiverSpawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(GiverSpawn, &map, true) && IsInWorld(), "Could not publish Shang Xi");
    }

    ~LessonGiver()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void SetStudent(LootPlayer& player, uint8 race, uint8 playerClass)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, playerClass);
    player.setFactionForRace(race);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetLevel(1);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 400);
}

void Accept(LootPlayer& player, Creature& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = giver.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
}

void CheckFirstLesson()
{
    Map map(860, 300, 0, DIFFICULTY_NONE);
    LessonGiver giver(map);
    uint64 guid = 30000;
    for (auto const& [playerClass, lesson] : Lessons)
    {
        LootPlayer student(SupplySession(), map, ++guid, giver.GetPosition());
        SetStudent(student, RACE_PANDAREN_NEUTRAL, playerClass);
        student.PrepareAreaQuest(Area);
        for (uint32 introduction = 30039; introduction <= 30045; ++introduction)
            Require(student.GetQuestStatus(introduction) == QUEST_STATUS_NONE, "An obsolete introduction auto-started");
        student.PrepareQuestMenu(giver.GetGUID());
        for (auto const& [otherClass, otherLesson] : Lessons)
            Require(student.PlayerTalkClass->GetQuestMenu().HasItem(otherLesson) == (otherClass == playerClass),
                "Shang Xi's quest menu did not offer only the correct class lesson");
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE &&
            !student.CanTakeQuest(sQuestDataStore->GetQuestTemplate(SandyFist), false),
            "The real acceptance handler failed or skipped the equipment objective");
        AbandonSupply(student, lesson);
        Accept(student, giver, lesson);
        Require(student.GetQuestStatus(lesson) == QUEST_STATUS_INCOMPLETE, "The first lesson could not be accepted again");
    }
    LootPlayer visitor(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(visitor, RACE_DRAENEI, CLASS_SHAMAN);
    visitor.PrepareQuestMenu(giver.GetGUID());
    for (auto const& lesson : Lessons)
        Require(!visitor.PlayerTalkClass->GetQuestMenu().HasItem(lesson.second), "Non-Pandaren could see a starter lesson");

    LootPlayer legacy(SupplySession(), map, ++guid, giver.GetPosition());
    SetStudent(legacy, RACE_PANDAREN_NEUTRAL, CLASS_MONK);
    legacy.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(30039), nullptr);
    Require(legacy.GetQuestStatus(30039) == QUEST_STATUS_COMPLETE, "The saved introduction cannot be completed");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = 30039;
    legacy.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(legacy.GetQuestRewardStatus(30039) && legacy.GetQuestStatus(30027) == QUEST_STATUS_INCOMPLETE,
        "The legacy turn-in did not hand off to the correct weapon lesson");
    std::cout << "PASS seven native class offers, acceptance, abandonment and retry, race restriction, and legacy turn-in\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The lesson check requires isolated characters");
        LoadLessonInputs();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFirstLesson();
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
