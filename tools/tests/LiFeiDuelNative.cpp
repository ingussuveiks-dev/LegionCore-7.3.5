#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Trial = 29421;
constexpr uint32 TeacherEntry = 54135;
constexpr uint32 DuelEntry = 54734;

Quest const* LoadTrial()
{
    auto row = WorldDatabase.Query(
#include "NativeLiFeiDuel.inc"
    );
    Require(bool(row), "Missing Only the Worthy Shall Pass template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Trial] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Trial, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29421");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == DuelEntry && objective->Fetch()[5].GetUInt32() == 1,
        "Missing original duel objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature_queststarter s JOIN creature_questender e ON e.id=s.id AND e.quest=s.quest "
        "JOIN quest_template_addon a ON a.ID=s.quest WHERE s.quest=29421 AND s.id=54135 AND a.PrevQuestID=29664 AND a.NextQuestID=29422")),
        "Original brazier prerequisite, turn-in or next quest changed");
    sObjectMgr->LoadItemTemplates();
    sCreatureTextMgr->LoadCreatureTexts();
    return quest;
}

struct DuelPet : LootPet
{
    DuelPet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

Creature* DuelFor(Creature& teacher, Player& player)
{
    std::list<Creature*> actors;
    teacher.GetCreatureListWithEntryInGrid(actors, DuelEntry, 40.0f);
    Creature* result = nullptr;
    for (auto actor : actors)
        if (actor->HasQuestObjectScope() && !actor->IsQuestObjectExpired() && actor->IsInPersonnalVisibilityList(player.GetGUID()))
        {
            Require(!result, "Duplicate duel for one quest attempt");
            result = actor;
        }
    return result;
}

void CheckDuel(Quest const* quest)
{
    auto spawn = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=54135 AND map=860 AND areaId=5849");
    Require(spawn && spawn->GetRowCount() == 1, "Original cavern teacher missing");
    auto data = spawn->Fetch();
    Position point{data[1].GetFloat(),data[2].GetFloat(),data[3].GetFloat(),data[4].GetFloat()};
    Map map(860, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 29421, point), second(SupplySession(), map, 29422, point);
    for (auto player : {&owner, &second})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
        player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
        player->SetLevel(8);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    }
    Actor teacher(&map, TeacherEntry, data[0].GetUInt32(), point);
    Require(map.AddToMap(static_cast<Creature*>(&teacher)), "Original teacher could not enter the cave");
    auto accept = [&](LootPlayer& player) { sScriptMgr->OnQuestAccept(&player, &teacher, quest); };
    accept(owner);
    Require(!DuelFor(teacher, owner), "A player without the quest received a duel");
    for (auto player : {&owner, &second})
    {
        player->AddQuestAndCheckCompletion(quest, nullptr);
        accept(*player);
        accept(*player);
    }
    auto firstDuel = DuelFor(teacher, owner);
    auto otherDuel = DuelFor(teacher, second);
    Require(firstDuel && otherDuel && firstDuel != otherDuel && firstDuel->GetAreaId() == 5849 &&
        !firstDuel->IsInPersonnalVisibilityList(second.GetGUID()) && !otherDuel->IsInPersonnalVisibilityList(owner.GetGUID()),
        "The shared teacher did not create independent private duels");
    second.DealDamage(firstDuel, uint32(firstDuel->GetMaxHealth()) * 2);
    Require(!owner.GetQuestObjectiveData(Trial, DuelEntry) && firstDuel->IsAlive(), "A bystander could win the first duel");
    auto old = firstDuel->GetGUID();
    AbandonSupply(owner, Trial);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(old) && !otherDuel->IsQuestObjectExpired(), "Abandon canceled a wrong duel or retained the old one");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    accept(owner);
    firstDuel = DuelFor(teacher, owner);
    Require(firstDuel && firstDuel->GetGUID() != old, "Fresh acceptance could not start the duel");
    DuelPet pet(owner, map);
    pet.SetLevel(8);
    pet.DealDamage(firstDuel, uint32(firstDuel->GetMaxHealth()) * 2);
    pet.DealDamage(firstDuel, uint32(firstDuel->GetMaxHealth()) * 2);
    Require(firstDuel->IsAlive() && firstDuel->getFaction() == 35 && owner.CanRewardQuest(quest, false) &&
        owner.GetQuestObjectiveData(Trial, DuelEntry) == 1 && !second.GetQuestObjectiveData(Trial, DuelEntry),
        "Owned pet's finishing hit did not produce one private surrender");
    otherDuel->AI()->KilledUnit(&owner);
    Require(second.GetQuestStatus(Trial) == QUEST_STATUS_INCOMPLETE && owner.CanRewardQuest(quest, false),
        "A death notification for a bystander changed an unrelated quest");
    otherDuel->Kill(&second, false);
    Require(!second.IsAlive() && second.GetQuestStatus(Trial) == QUEST_STATUS_FAILED, "Duel defeat did not fail its owner's quest");
    AbandonSupply(owner, Trial);
    AbandonSupply(second, Trial);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS real private Li Fei duels, invalid attacker rejection, abandon/retake, pet surrender and native owner defeat\n";
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
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_WanderingIsland();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadTrial();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckDuel(quest);
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
