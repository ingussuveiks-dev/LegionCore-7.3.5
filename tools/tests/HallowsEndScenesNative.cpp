#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "GameEventMgr.h"

void AddSC_npcs_special();

namespace
{
void LoadSceneQuests()
{
    auto rows = WorldDatabase.Query(
#include "NativeHallowsEndScenesQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 4, "Missing scene quest templates");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID, QuestID, Type, StorageIndex, ObjectID, Amount, Flags, Flags2, TaskStep, Description, Bugged "
        "FROM quest_objectives WHERE QuestID IN (29399,29425) ORDER BY QuestID,StorageIndex");
    Require(bool(objectives) && objectives->GetRowCount() == 2, "Missing original Shopping Around objectives");
    do
    {
        auto row = objectives->Fetch();
        Quest* quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());
}

struct Placement
{
    uint32 Entry;
    Position Spot;
};

Placement ReadPlacement(uint64 guid)
{
    auto rows = WorldDatabase.PQuery("SELECT id,position_x,position_y,position_z,orientation FROM creature WHERE guid=" UI64FMTD, guid);
    Require(bool(rows), "Missing an existing merchant or ritual actor placement");
    auto row = rows->Fetch();
    return {row[0].GetUInt32(), Position(row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat())};
}

struct StoryActor : Creature
{
    ~StoryActor()
    {
        if (IsInWorld())
        {
            CombatStop(true);
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        }
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint64 guid, Placement const& placement)
    {
        Position const& pos = placement.Spot;
        auto const runtimeGuid = sObjectMgr->GetGenerator<HighGuid::Creature>()->Generate();
        Require(Create(runtimeGuid, &map, 1, placement.Entry, 0, ALLIANCE, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation()),
            "Native story actor could not be created");
        m_DBTableGuid = guid;
        SetHomePosition(pos);
        Require(map.AddToMap(static_cast<Creature*>(this)), "Story actor could not enter the world grid");
        Require(map.GetCreature(GetGUID()) == this, "Story actor GUID was not registered for native lookup");
    }
};

void Accept(LootPlayer& player, uint32 quest)
{
    player.SetQuestStatus(quest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, quest);
}

void Cancel(LootPlayer& player, uint32 quest)
{
    player.RemoveActiveQuest(quest);
    player.SetQuestSlot(0, 0);
}

void Faction(LootPlayer& player, uint8 race)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.setFactionForRace(race);
}

void CheckShopping(WorldSession* session, uint32 mapId, uint32 quest, uint64 investigatorGuid, uint64 merchantGuid, uint8 race)
{
    Placement investigatorPlacement = ReadPlacement(investigatorGuid);
    Placement merchantPlacement = ReadPlacement(merchantGuid);
    Map map(mapId, 300, 9999991 + mapId, DIFFICULTY_NONE);
    LootPlayer witness(session, map, 100, investigatorPlacement.Spot);
    Faction(witness, race);
    StoryActor investigator, merchant;
    investigator.Spawn(map, investigatorGuid, investigatorPlacement);
    merchant.Spawn(map, merchantGuid, merchantPlacement);
    auto& holiday = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    holiday.insert(12);

    Accept(witness, quest);
    investigator.AI()->MoveInLineOfSight_Safe(&witness);
    for (uint8 line = 0; line < 4; ++line)
        investigator.AI()->UpdateAI(2400);
    Require(witness.GetQuestObjectiveData(quest, 54022) == 0, "Shopping credited the witness before the actual buyer was revealed");
    Cancel(witness, quest);
    Accept(witness, quest);
    investigator.AI()->UpdateAI(2400);
    Require(witness.GetQuestObjectiveData(quest, 54022) == 0, "An old Shopping attempt survived cancellation");

    investigator.AI()->MoveInLineOfSight_Safe(&witness);
    for (uint8 line = 0; line < 7; ++line)
        investigator.AI()->UpdateAI(2400);
    Require(witness.GetQuestObjectiveData(quest, 54022) == 0, "Credit overtook the final Shopping line");
    investigator.AI()->UpdateAI(2400);
    Tick(witness, 100);
    Require(witness.GetQuestObjectiveData(quest, 54022) == 1 && witness.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE,
        "The full dialogue did not deliver native Shopping Around credit");
    holiday.erase(12);
    std::cout << "PASS seven native Shopping speeches, final credit and cancellation for " << quest << '\n';
}

void CheckReturn(WorldSession* first, WorldSession* other, uint32 mapId, uint32 quest, uint64 receiverGuid, uint64 employerGuid, uint8 race, uint32 ritual)
{
    auto receiverPlacement = ReadPlacement(receiverGuid);
    auto employerPlacement = ReadPlacement(employerGuid);
    Map map(mapId, 300, 9999980 + mapId, DIFFICULTY_NONE);
    LootPlayer player(first, map, 100, receiverPlacement.Spot);
    LootPlayer onlooker(other, map, 101, receiverPlacement.Spot);
    Faction(player, race);
    Faction(onlooker, race);
    StoryActor receiver, alternateEmployer;
    receiver.Spawn(map, receiverGuid, receiverPlacement);
    StoryActor* employer = &receiver;
    if (employerGuid != receiverGuid)
    {
        alternateEmployer.Spawn(map, employerGuid, employerPlacement);
        employer = &alternateEmployer;
    }
    auto& holiday = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    holiday.insert(12);
    auto* roster = player.GetSummonList(employer->GetEntry());
    Quest const* questTemplate = sQuestDataStore->GetQuestTemplate(quest);
    Require(!sScriptMgr->OnQuestReward(&player, &receiver, questTemplate, 0) && roster->empty(), "Unrewarded turn-in started a ritual");
    const_cast<RewardedQuestSet&>(player.getRewardedQuests()).insert(quest);
    Require(!sScriptMgr->OnQuestReward(&player, &receiver, questTemplate, 0), "Ritual consumed the native questgiver callback");
    if (!ritual)
    {
        Require(roster->empty(), "Giving the crate directly to Edgar must not start the alternate ritual");
        holiday.erase(12);
        std::cout << "PASS direct Edgar handoff has no extra ritual\n";
        return;
    }
    Require(roster->size() == 1, "Rewarded handoff did not create a personal actor");
    auto actorGuid = roster->front();
    auto actor = map.GetCreature(actorGuid);
    Require(actor && player.canSeeOrDetect(actor) && !onlooker.canSeeOrDetect(actor) &&
        !player.canSeeOrDetect(employer) && onlooker.canSeeOrDetect(employer), "Ritual replaced a public actor for the wrong player");
    Require(!actor->GetUInt32Value(UNIT_FIELD_NPC_FLAGS) && employer->GetUInt32Value(UNIT_FIELD_NPC_FLAGS), "Cosmetic actor exposed the native quest menu");
    sScriptMgr->OnQuestReward(&player, &receiver, questTemplate, 0);
    Require(roster->size() == 1, "Duplicate callback started another ritual");

    const_cast<RewardedQuestSet&>(onlooker.getRewardedQuests()).insert(quest);
    sScriptMgr->OnQuestReward(&onlooker, &receiver, questTemplate, 0);
    auto* secondRoster = onlooker.GetSummonList(employer->GetEntry());
    Require(secondRoster->size() == 1, "The first player's copy blocked a second eligible turn-in");
    auto secondActor = map.GetCreature(secondRoster->front());
    Require(secondActor && onlooker.canSeeOrDetect(secondActor) && !player.canSeeOrDetect(secondActor), "The second copy crossed private visibility");
    secondActor->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    Require(secondRoster->empty() && onlooker.canSeeOrDetect(employer), "Cleaning the second scene did not restore its public actor");

    uint32 originalDisplay = actor->GetDisplayId();
    bool sawAura = false;
    bool sawTransform = false;
    bool sawCollapse = false;
    float const initialDistance = actor->GetExactDist(&receiver);
    float closest = initialDistance;
    uint32 elapsed = 0;
    for (; elapsed < 41000; elapsed += 100)
    {
        Creature* next = map.GetCreature(actorGuid);
        if (!next)
            break;
        next->Update(100);
        map.MoveAllCreaturesInMoveList();
        closest = std::min(closest, next->GetExactDist(&receiver));
        if (next->HasAura(ritual))
        {
            sawAura = true;
            if (next->GetDisplayId() != originalDisplay)
                sawTransform = true;
        }
        if (sawTransform && !next->HasAura(ritual) && next->getStandState() == UNIT_STAND_STATE_DEAD)
            sawCollapse = true;
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "RITUAL receiver=" << receiverPlacement.Entry << " start=" << initialDistance << " closest=" << closest << " elapsed=" << elapsed
        << " aura=" << sawAura << " transform=" << sawTransform
        << " collapse=" << sawCollapse << " pending=" << roster->size() << '\n';
    Require(sawTransform && sawCollapse && roster->empty() && player.canSeeOrDetect(employer), "The actual ritual, collapse or private actor cleanup did not complete");
    holiday.erase(12);
    std::cout << "PASS What Now recipient " << receiverPlacement.Entry << ", native transform " << ritual << ", personal scene and cleanup\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        LoadSceneQuests();
        sCreatureTextMgr->LoadCreatureTexts();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_npcs_special();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        auto first = new WorldSession(0, std::string("Holiday witness fixture"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto other = new WorldSession(0, std::string("Holiday onlooker fixture"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckShopping(first, 0, 29399, 202611212, 265761, RACE_HUMAN);
        CheckShopping(first, 1, 29425, 202611213, 264716, RACE_ORC);
        CheckReturn(first, other, 0, 29411, 384230, 384230, RACE_HUMAN, 100671);
        CheckReturn(first, other, 0, 29411, 202611272, 384231, RACE_HUMAN, 100671);
        CheckReturn(first, other, 1, 29428, 384254, 384254, RACE_ORC, 0);
        CheckReturn(first, other, 1, 29428, 202611273, 384255, RACE_ORC, 100792);
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
