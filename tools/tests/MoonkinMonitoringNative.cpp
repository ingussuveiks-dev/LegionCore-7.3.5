#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "GameEventMgr.h"
#include "QuestPackets.h"

void AddSC_MoonkinFestival();

namespace
{
constexpr uint32 Festival = 299;
constexpr uint32 Monitoring = 47430;
constexpr uint32 Astralaire = 122241;
constexpr uint32 Pewkew = 122256;
constexpr uint32 Dramock = 122390;

void LoadFestival()
{
    auto row = WorldDatabase.Query(
#include "NativeMoonkinMonitoring.inc"
    );
    Require(bool(row), "The original Moonkin Monitoring template is required");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(Monitoring + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Monitoring] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Monitoring, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=47430 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 3, "The three native observation objectives are required");
    do
    {
        Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER, "Observation uses the native creature-credit objective");
        quest->LoadQuestObjective(objectives->Fetch());
    } while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto event = WorldDatabase.Query("SELECT COUNT(*) FROM game_event_creature WHERE eventEntry=299 "
        "AND guid IN(146703497,146703557,146703559,146703560)");
    Require(event && event->Fetch()[0].GetUInt32() == 4, "Missing one of the four original event actors");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE id IN(122134,122241,122256,122390)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=47430");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=47430");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text "
        "WHERE CreatureID IN(122134,122241,122256,122390)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sCreatureTextMgr->LoadCreatureTexts();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_MoonkinFestival();
    sScriptMgr->SwapScriptContext(true);
}

Creature& FestivalActor(Map& map, uint32 guid, uint32 entry)
{
    auto data = sObjectMgr->GetCreatureData(guid);
    Require(data && data->id == entry, "The matching event placement is required");
    map.LoadGrid(data->posX, data->posY);
    auto actor = std::make_unique<Creature>();
    Require(actor->LoadCreatureFromDB(guid, &map, true) && actor->IsAlive() && actor->AI(),
        "The matching event actor did not load");
    return *actor.release();
}

void MoveObserver(LootPlayer& player, Position const& point)
{
    player.GetMap()->PlayerRelocation(&player, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
}

void Accept(LootPlayer& player, Creature& giver)
{
    MoveObserver(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver.GetGUID();
    accept.QuestID = Monitoring;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(Monitoring) == QUEST_STATUS_INCOMPLETE, "Makkaw did not accept an eligible player");
}

void Approach(LootPlayer& player, Creature& actor)
{
    MoveObserver(player, actor.GetPosition());
    actor.AI()->MoveInLineOfSight_Safe(&player);
}

void Advance(Creature& actor, uint32 duration)
{
    for (uint32 time = 0; time < duration; time += 100)
    {
        actor.Unit::Update(100);
        actor.AI()->UpdateAI(100);
    }
}

void VerifyMonitoring(bool expectBroken)
{
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(Festival);
    Map map(1, 300, 9999997, DIFFICULTY_NONE);
    auto& giver = FestivalActor(map, 146703497, 122134);
    auto& astralaire = FestivalActor(map, 146703557, Astralaire);
    auto& pewkew = FestivalActor(map, 146703559, Pewkew);
    auto& dramock = FestivalActor(map, 146703560, Dramock);
    auto first = new WorldSession(0, std::string("Native moonkin owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto second = new WorldSession(0, std::string("Native moonkin visitor"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer owner(first, map, 100, giver.GetPosition());
    LootPlayer visitor(second, map, 101, giver.GetPosition());
    for (auto player : {&owner, &visitor})
        player->setFactionForRace(player->getRace());

    Accept(owner, giver);
    MoveObserver(owner, pewkew.GetPosition());
    Advance(astralaire, 35000);
    Require(!owner.GetQuestObjectiveData(Monitoring, Astralaire), "An absent player received dance observation");
    MoveObserver(visitor, astralaire.GetPosition());
    Approach(owner, astralaire);
    Advance(astralaire, 37000);
    if (expectBroken)
    {
        Require(!owner.GetQuestObjectiveData(Monitoring, Astralaire), "The old failed dance retry did not reproduce");
        std::cout << "EXPECTED_BROKEN Astralaire did not start for the accepted player returning alone\n";
        return;
    }
    Require(owner.GetQuestObjectiveData(Monitoring, Astralaire) == 1 && !visitor.GetQuestObjectiveData(Monitoring, Astralaire),
        "Only the accepted observer should receive the dance credit");

    Approach(owner, pewkew);
    Advance(pewkew, 10000);
    WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    abandon.Entry = owner.FindQuestSlot(Monitoring);
    first->HandleQuestLogRemoveQuest(abandon);
    Accept(owner, giver);
    MoveObserver(owner, pewkew.GetPosition());
    Advance(pewkew, 22000);
    Require(!owner.GetQuestObjectiveData(Monitoring, Pewkew), "An abandoned observation completed a later acceptance");
    Advance(pewkew, 10000);
    Require(owner.GetQuestObjectiveData(Monitoring, Pewkew) == 1, "The accepted attempt could not retry the lunar-strike observation");

    Accept(visitor, giver);
    Approach(owner, dramock);
    Advance(dramock, 10000);
    active.erase(Festival);
    Advance(dramock, 31000);
    Require(!owner.GetQuestObjectiveData(Monitoring, Dramock), "The event shutdown did not cancel observation");
    active.insert(Festival);
    Approach(owner, dramock);
    Advance(dramock, 24000);
    Approach(visitor, dramock);
    Advance(dramock, 8000);
    Require(owner.GetQuestObjectiveData(Monitoring, Dramock) == 1 && !visitor.GetQuestObjectiveData(Monitoring, Dramock),
        "An observer arriving for the end received the full napping credit");
    Advance(dramock, 31000);
    Require(visitor.GetQuestObjectiveData(Monitoring, Dramock) == 1, "The late observer could not start the next performance");

    Approach(owner, astralaire);
    Advance(astralaire, 35000);
    Require(owner.GetQuestStatus(Monitoring) == QUEST_STATUS_COMPLETE, "Three actual observations did not finish the quest");
    MoveObserver(owner, giver.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = Monitoring;
    first->HandleQuestGiverChooseReward(reward);
    Require(owner.IsQuestRewarded(Monitoring), "Makkaw did not process the native quest reward");
    active.erase(Festival);
    std::cout << "PASS three original observations, missed start, abandon/reaccept, late arrival, and Makkaw reward\n";
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
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        auto connections = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(connections.substr(connections.find_last_of(';') + 1) == "fixture_char", "The native probe requires private characters");
        CharacterDatabase.SetConnectionInfo(connections, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        HashMapHolder<Player>::SetSize(256);
        GameTime::UpdateGameTimers();
        LoadFestival();
        VerifyMonitoring(sConfigMgr->GetBoolDefault("NativeMoonkinMonitoring.ExpectBroken", false));
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
