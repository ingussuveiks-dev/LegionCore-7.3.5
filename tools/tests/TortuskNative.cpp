#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 Takedown = 13968;
constexpr uint32 Tortusk = 34544;

struct TakedownPet : LootPet
{
    TakedownPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
    }
};

Quest const* LoadTakedown()
{
    auto row = WorldDatabase.Query(
#include "NativeTortusk.inc"
    );
    Require(bool(row), "Missing The Tortusk Takedown quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Takedown] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Takedown, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13968");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Tortusk && objective->Fetch()[5].GetUInt32() == 1,
        "Missing the original Tortusk kill objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    auto& table = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=34544";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded > 0, "Original Tortusk loot is missing");
    return quest;
}

void CheckTortusk(Quest const* quest)
{
    auto spawn = WorldDatabase.Query("SELECT map,zoneId,areaId,spawnMask,phaseMask,position_x,position_y,position_z,orientation,spawntimesecs "
        "FROM creature WHERE id=34544");
    Require(spawn && spawn->GetRowCount() == 1, "Tortusk must have one shared spawn");
    auto fields = spawn->Fetch();
    Require(fields[0].GetUInt32() == 1 && fields[1].GetUInt32() == 17 && fields[2].GetUInt32() == 1699 &&
        fields[3].GetUInt32() == 1 && fields[4].GetUInt32() == 1 && fields[9].GetUInt32() == 120,
        "Wrong map, zone, visibility or respawn");
    auto definition = sObjectMgr->GetCreatureTemplate(Tortusk);
    Require(definition && definition->AIName == "SmartAI" && sSpellMgr->GetSpellInfo(82610), "Original aggro setup could not load");
    Position point{fields[5].GetFloat(),fields[6].GetFloat(),fields[7].GetFloat(),fields[8].GetFloat()};
    Map map(1, 300, 0, DIFFICULTY_NONE);
    map.LoadGrid(point.GetPositionX(), point.GetPositionY());
    auto floor = map.GetHeight(point.GetPositionX(), point.GetPositionY(), point.GetPositionZ() + 4.0f);
    std::cout << "CASE Thorn Hill spawn=" << point.GetPositionZ() << " floor=" << floor << std::endl;
    Require(std::fabs(floor - point.GetPositionZ()) < 0.25f, "Tortusk is not on the Legion floor");
    LootPlayer player(SupplySession(), map, Takedown, point);
    player.SetLevel(13);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 20000);
    player.setFactionForRace(RACE_ORC);
    Actor first(&map, Tortusk, 49560, point);
    Require(map.AddToMap(static_cast<Creature*>(&first)), "Could not publish the shared quest target");
    player.DealDamage(&first, uint32(first.GetMaxHealth()));
    Tick(player, 100);
    Require(first.isDead(false) && !player.GetQuestObjectiveData(Takedown, Tortusk), "A death without the quest granted progress");
    player.AddQuestAndCheckCompletion(quest, nullptr);
    Actor second(&map, Tortusk, 49561, point);
    Require(map.AddToMap(static_cast<Creature*>(&second)), "Could not repeat the shared target kill");
    TakedownPet pet(player, map);
    pet.SetLevel(13);
    pet.DealDamage(&second, uint32(second.GetMaxHealth()));
    Tick(player, 100);
    Require(second.isDead(false), "The native pet hit did not kill Tortusk");
    Require(player.GetQuestObjectiveData(Takedown, Tortusk) == 1 &&
        player.GetQuestStatus(Takedown) == QUEST_STATUS_COMPLETE, "The native pet kill did not complete the accepted objective");
    std::cout << "PASS published shared Tortusk and native quest credit from an owned pet\n";
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
        auto quest = LoadTakedown();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckTortusk(quest);
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
