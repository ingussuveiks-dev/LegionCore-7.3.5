#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Introduction = 29521;
constexpr uint32 DryFur = 29661;
constexpr uint32 NextQuest = 29676;
constexpr uint32 BellCredit = 55077;
constexpr uint32 BellEntry = 209608;
constexpr uint32 BellSpawn = 102318;
constexpr uint32 AysaSpawn = 244270;
constexpr uint32 BellSpell = 102843;

void LoadPools()
{
    auto rows = WorldDatabase.Query(
#include "NativeSingingPools.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "Missing the Singing Pools quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    auto rowsAddon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID IN (29521,29661,29676)");
    Require(rowsAddon && rowsAddon->GetRowCount() == 3, "Missing the original Pools prerequisites");
    do
    {
        auto row = rowsAddon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (rowsAddon->NextRow());

    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29661");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == BellCredit,
        "The original Training Bell objective is missing");
    quests[DryFur]->LoadQuestObjective(objective->Fetch());
    quests[DryFur]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=244270");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (29521,29661,29676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (29521,29661,29676)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_WanderingIsland();
    sScriptMgr->SwapScriptContext(true);
}

struct Bell : GameObject
{
    explicit Bell(Map& map)
    {
        auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
            "FROM gameobject WHERE guid=102318 AND id=209608 AND map=860");
        Require(bool(row), "Missing the placed Training Bell");
        auto data = row->Fetch();
        Position point(data[0].GetFloat(), data[1].GetFloat(), data[2].GetFloat(), data[3].GetFloat());
        G3D::Quat rotation{data[4].GetFloat(), data[5].GetFloat(), data[6].GetFloat(), data[7].GetFloat()};
        m_DBTableGuid = BellSpawn;
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Require(Create(BellSpawn, BellEntry, &map, data[8].GetUInt32(), point, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && GetGOInfo()->spellCaster.spell == BellSpell, "Could not load the native Bell");
    }

    ~Bell()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

struct Teacher : Creature
{
    explicit Teacher(Map& map) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(AysaSpawn);
        Require(data && data->id == 54975, "The original Singing Pools Aysa is missing");
        sObjectMgr->RemoveCreatureFromGrid(AysaSpawn, data);
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(AysaSpawn, &map, true), "Could not load Aysa at the Pools");
    }

    ~Teacher()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void AcceptQuest(Player& player, Creature& teacher, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = teacher.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
}

void RewardQuest(Player& player, Creature& teacher, uint32 quest, uint32 choice = 0)
{
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = teacher.GetGUID();
    request.QuestID = quest;
    request.ItemChoiceID = choice;
    player.GetSession()->HandleQuestGiverChooseReward(request);
}

void Ring(LootPlayer& player, Bell& bell, bool report)
{
    if (report)
    {
        WorldPacket wire(CMSG_GAME_OBJ_REPORT_USE);
        wire << bell.GetGUID();
        WorldPackets::GameObject::GameObjReportUse request(std::move(wire));
        request.Read();
        player.GetSession()->HandleGameobjectReportUse(request);
    }
    else
    {
        WorldPacket wire(CMSG_GAME_OBJ_USE);
        wire << bell.GetGUID();
        WorldPackets::GameObject::GameObjectUse request(std::move(wire));
        request.Read();
        player.GetSession()->HandleGameObjectUse(request);
    }
    Tick(player, 3000);
}

void CheckBell()
{
    Map map(860, 663, 0, DIFFICULTY_NONE);
    Teacher aysa(map);
    Bell bell(map);
    LootPlayer pupil(SupplySession(), map, 2966101, aysa.GetPosition());
    pupil.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    pupil.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    pupil.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    pupil.InitDisplayIds();
    pupil.GetReputationMgr().LoadFromDB({});
    pupil.SetLevel(10);
    pupil.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        pupil.SetFloatValue(field, 1.0f);
    pupil.SetRewardedQuest(29423);
    pupil.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(Introduction), nullptr);
    RewardQuest(pupil, aysa, Introduction);
    Require(pupil.IsQuestRewarded(Introduction), "Aysa did not accept the Singing Pools introduction");
    if (pupil.GetQuestStatus(DryFur) == QUEST_STATUS_NONE)
        AcceptQuest(pupil, aysa, DryFur);
    Require(pupil.GetQuestStatus(DryFur) == QUEST_STATUS_INCOMPLETE, "Aysa did not admit the training lesson");

#ifndef SINGING_POOLS_BASELINE
    Ring(pupil, bell, false);
    Require(!pupil.GetQuestObjectiveData(DryFur, BellCredit), "Remote ordinary use completed the lesson");
    MoveGilneasPlayer(pupil, bell.GetPosition());
    Ring(pupil, bell, false);
    Require(pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1, "Ordinary use lost the native bell spell");
    AbandonSupply(pupil, DryFur);
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    AcceptQuest(pupil, aysa, DryFur);
    Require(pupil.GetQuestStatus(DryFur) == QUEST_STATUS_INCOMPLETE && !pupil.GetQuestObjectiveData(DryFur, BellCredit),
        "The next attempt kept the previous bell credit");
#endif
    MoveGilneasPlayer(pupil, bell.GetPosition());
    Ring(pupil, bell, true);
    auto report = pupil.GetQuestObjectiveData(DryFur, BellCredit);
    std::cout << "CASE Training Bell report credit=" << report << std::endl;
#ifdef SINGING_POOLS_BASELINE
    Ring(pupil, bell, false);
    Require(!report && pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1,
        "Expected only ordinary use to ring the original bell");
    std::cout << "EXPECTED_BROKEN report-use skipped the normal bell spell" << std::endl;
#else
    Require(report == 1, "Report-use did not ring the bell");
    Ring(pupil, bell, false);
    Require(pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1 && pupil.GetQuestStatus(DryFur) == QUEST_STATUS_COMPLETE,
        "Duplicate use changed the completed objective");
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    auto quest = sQuestDataStore->GetQuestTemplate(DryFur);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (pupil.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    Require(choice, "No suitable native belt reward");
    RewardQuest(pupil, aysa, DryFur, choice);
    Require(pupil.IsQuestRewarded(DryFur) && pupil.GetItemCount(choice) == 1, "Aysa did not issue the selected belt");
    if (pupil.GetQuestStatus(NextQuest) == QUEST_STATUS_NONE)
        AcceptQuest(pupil, aysa, NextQuest);
    Require(pupil.GetQuestStatus(NextQuest) != QUEST_STATUS_NONE, "Aysa did not admit Finding an Old Friend");
    std::cout << "PASS native Aysa arrival, Training Bell and next quest" << std::endl;
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Singing Pools check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadPools();
        CheckBell();
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
