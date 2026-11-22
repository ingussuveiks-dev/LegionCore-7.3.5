#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

void AddSC_bloodmyst_isle();

namespace
{
constexpr uint32 Rescue = 9670;
constexpr uint32 Web = 17680;
constexpr uint32 Researcher = 17681;

Quest const* LoadRescue()
{
    auto row = WorldDatabase.Query(
#include "NativeWebbedResearcher.inc"
    );
    Require(bool(row), "Missing Bloodmyst researcher quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Rescue] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Rescue, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9670");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Researcher && objective->Fetch()[5].GetUInt32() == 5,
        "Missing five-rescuer objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    return quest;
}

struct WebActor : Actor
{
    using Actor::Actor;

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(ai && AIM_Initialize(ai), "Original webbed-creature AI could not initialize");
    }
};

struct RescuePet : LootPet
{
    RescuePet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

void CheckRescue(Quest const* quest)
{
    auto source = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=17680 "
        "AND map=530 AND zoneId=3525 ORDER BY guid LIMIT 1");
    auto count = WorldDatabase.Query("SELECT COUNT(*) FROM creature WHERE id=17680 AND map=530 AND zoneId=3525");
    Require(source && count && count->Fetch()[0].GetUInt32() == 39, "Expected the existing Amberweb Pass placements");
    auto row = source->Fetch();
    Position point{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_queststarter s ON s.id=c.id "
        "JOIN creature_questender e ON e.id=c.id AND e.quest=s.quest WHERE s.quest=9670 AND c.id=17686 AND c.map=530")),
        "Researcher Cornelius is missing the offer or turn-in");
    Map map(530, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 9670, point);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
    owner.setFactionForRace(RACE_DRAENEI);
    owner.SetLevel(19);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    uint32 guid = 60000;
    auto breakWeb = [&](Unit& killer)
    {
        WebActor source(&map, Web, ++guid, point);
        Require(map.AddToMap(static_cast<Creature*>(&source)), "Could not publish the original webbed creature");
        auto before = owner.GetQuestObjectiveData(Rescue, Researcher);
        killer.DealDamage(&source, uint32(source.GetMaxHealth()) * 2);
        Require(source.isDead(false), "Native hit did not break the web");
        auto freed = source.FindNearestCreature(Researcher, 2.0f, true);
        bool rescued = freed != nullptr;
        if (freed)
        {
            freed->DespawnOrUnsummon();
            map.RemoveAllObjectsInRemoveList();
        }
        auto after = owner.GetQuestObjectiveData(Rescue, Researcher);
        Require(after == before + (rescued && owner.GetQuestStatus(Rescue) != QUEST_STATUS_NONE ? 1 : 0),
            "The original script credited a wrong outcome or recipient");
        return rescued;
    };
    auto findResearcher = [&](Unit& killer)
    {
        for (unsigned attempt = 0; attempt < 80; ++attempt)
            if (breakWeb(killer))
                return;
        Require(false, "No researcher appeared after eighty native outcomes");
    };
    findResearcher(owner);
    Require(!owner.GetQuestObjectiveData(Rescue, Researcher), "Researcher outside the quest gave credit");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    findResearcher(owner);
    RescuePet pet(owner, map);
    pet.SetLevel(19);
    findResearcher(pet);
    while (owner.GetQuestObjectiveData(Rescue, Researcher) < 5)
        findResearcher(pet);
    Require(owner.CanRewardQuest(quest, false), "Five successful rescues did not allow turn-in");
    AbandonSupply(owner, Rescue);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(!owner.GetQuestObjectiveData(Rescue, Researcher), "The new acceptance inherited old rescue credit");
    std::cout << "PASS native web deaths and researcher summons, player and pet-owner credit, five rescues and abandon/retry\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_bloodmyst_isle();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadRescue();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRescue(quest);
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
