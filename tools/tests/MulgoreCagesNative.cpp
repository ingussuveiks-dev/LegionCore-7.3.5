#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "CreatureTextMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameObjectModel.h"
#include "GameTime.h"

void AddSC_mulgore();

namespace
{
constexpr uint32 QuestId = 24852;
constexpr uint32 CageEntry = 202112;
constexpr uint32 CaptiveEntry = 38345;

Quest const* LoadQuests()
{
    auto rows = WorldDatabase.Query(
#include "NativeMulgoreCagesQuest.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "Missing the cage quest or its neighbors in the chain");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=24852");
    Require(objectives && objectives->GetRowCount() == 1, "Missing the cage credit");
    auto quest = quests[QuestId];
    quest->LoadQuestObjective(objectives->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->Objectives.front().Type == QUEST_OBJECTIVE_GAMEOBJECT && quest->Objectives.front().ObjectID == CageEntry &&
        quest->Objectives.front().Amount == 4 && !quest->SourceItemId, "This quest should require four cage actions and no item");
    auto addons = WorldDatabase.Query("SELECT ID,PrevQuestID FROM quest_template_addon WHERE ID IN(24852,14458)");
    Require(addons && addons->GetRowCount() == 2, "Missing the Rite of Strength or Go to Adana link");
    do
    {
        auto fields = addons->Fetch();
        quests[fields[0].GetUInt32()]->prevQuests.push_back(fields[1].GetInt32());
    } while (addons->NextRow());
    return quest;
}

struct Placement
{
    uint32 cage;
    uint32 captive;
    uint32 respawn;
    Position cagePosition;
    Position captivePosition;
    G3D::Quat rotation;
};

std::vector<Placement> LoadPlacements()
{
    auto rows = WorldDatabase.Query(
        "SELECT g.guid,c.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,"
        "c.position_x,c.position_y,c.position_z,c.orientation,g.rotation0,g.rotation1,g.rotation2,g.rotation3 "
        "FROM gameobject g JOIN creature c ON c.id=38345 AND c.map=g.map AND c.spawntimesecs=g.spawntimesecs "
        "AND ABS(c.position_x-g.position_x)<1 AND ABS(c.position_y-g.position_y)<1 WHERE g.id=202112 AND g.map=1 ORDER BY g.guid");
    Require(rows && rows->GetRowCount() == 10, "Missing an exact outdoor cage and occupant pairing");
    std::vector<Placement> result;
    do
    {
        auto fields = rows->Fetch();
        result.push_back({fields[0].GetUInt32(), fields[1].GetUInt32(), fields[2].GetUInt32(),
            {fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat()},
            {fields[7].GetFloat(), fields[8].GetFloat(), fields[9].GetFloat(), fields[10].GetFloat()},
            G3D::Quat(fields[11].GetFloat(), fields[12].GetFloat(), fields[13].GetFloat(), fields[14].GetFloat())});
    } while (rows->NextRow());
    return result;
}

struct Cage : GameObject
{
    Cage(Map& map, Placement const& placement)
    {
        m_DBTableGuid = placement.cage;
        Require(Create(placement.cage, CageEntry, &map, 1, placement.cagePosition, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Cage could not enter its native map");
        SetRespawnDelayTime(placement.respawn);
        Update(10000);
        Require(getLootState() == GO_READY && GetGOInfo()->GetLockId() == 1691 && GetGOInfo()->GetAutoCloseTime() == 60 &&
            GetGOInfo()->goober.spell == 71725, "Cage lock, use spell or close timing changed");
    }

    ~Cage()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void FinishOpenTimer()
    {
        Require(m_cooldownTime == GameTime::GetGameTime() + 60, "The actual opening used a different close delay");
        m_cooldownTime = GameTime::GetGameTime() - 1;
        Update(10000);
        Update(10000);
    }

    void FinishRespawnTimer()
    {
        Require(m_respawnTime > GameTime::GetGameTime(), "Expected the configured shared respawn wait");
        // Advance only the clock prerequisite. Let the real GO update rearm the cage.
        m_respawnTime = GameTime::GetGameTime();
        Update(10000);
    }
};

struct Brave : Actor
{
    Brave(Map& map, Placement const& placement) : Actor(&map, CaptiveEntry, placement.captive, placement.captivePosition)
    {
        m_DBTableGuid = placement.captive;
    }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(), "Native Captured Brave AI was not initialized");
    }
};

WorldSession* Session()
{
    return new WorldSession(0, "Quilboar cage native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void MoveToCage(LootPlayer& player, Placement const& placement, float distance = 2.25f)
{
    float exit = placement.captivePosition.GetOrientation();
    Position const& base = placement.cagePosition;
    player.GetMap()->PlayerRelocation(&player, base.GetPositionX() + distance * std::cos(exit),
        base.GetPositionY() + distance * std::sin(exit), base.GetPositionZ(), exit + 3.14159f);
}

void Accept(LootPlayer& player, Quest const* quest)
{
    Require(player.CanTakeQuest(quest, false), "Cage quest is not available after Rite of Strength");
    player.AddQuestAndCheckCompletion(quest, nullptr);
}

void CheckCages(Quest const* quest)
{
    auto placements = LoadPlacements();
    Placement const& first = placements.front();
    Map map(1, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 557, first.cagePosition), stranger(Session(), map, 558, first.cagePosition);
    for (auto player : {&owner, &stranger})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
        player->setFactionForRace(RACE_TAUREN);
        player->SetLevel(2);
        MoveToCage(*player, first);
        Require(!player->CanTakeQuest(quest, false), "Quest can be accepted before Rite of Strength");
        player->SetRewardedQuest(14452);
    }
    Accept(owner, quest);
#ifdef MULGORE_CAGES_BASELINE
    Cage empty(map, first);
    empty.Use(&owner);
    Require(owner.GetQuestObjectiveData(QuestId, CageEntry) == 1, "Old empty cage did not award credit");
    Placement const& next = placements[1];
    MoveToCage(owner, next);
    Cage occupied(map, next);
    Brave brave(map, next);
    Require(map.AddToMap(static_cast<Creature*>(&brave)), "Missing original captive");
    occupied.Use(&owner);
    for (uint32 elapsed = 0; elapsed < 9000; elapsed += 100)
    {
        brave.Update(100);
        map.MoveAllCreaturesInMoveList();
    }
    Require(owner.GetQuestObjectiveData(QuestId, CageEntry) == 2 && owner.canSeeOrDetect(&brave) && brave.GetExactDist(&next.captivePosition) < 0.1f,
        "Old cage no longer leaves its occupant behind after credit");
    std::cout << "EXPECTED_BROKEN the empty cage gave credit and the occupied cage left its real captive inside\n";
#else
    auto next = sQuestDataStore->GetQuestTemplate(14458);
    bool checkedReward = false;
    for (Placement const& placement : placements)
    {
        if (owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)
        {
            uint32 choice = 0;
            if (auto const rewards = sDB2Manager.GetQuestPackageItems(quest->PackageID))
                for (QuestPackageItemEntry const* reward : *rewards)
                    if (owner.CanSelectQuestPackageItem(reward))
                    {
                        choice = reward->ItemID;
                        break;
                    }
            if (!choice)
                if (auto const fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
                    choice = fallback->front()->ItemID;
            Require(choice && owner.CanRewardQuest(quest, choice, false) && !owner.CanTakeQuest(next, false),
                "Four openings must admit a valid reward choice before the next quest");
            checkedReward = true;
            owner.RemoveActiveQuest(QuestId);
            owner.SetQuestSlot(0, 0);
            Accept(owner, quest);
        }
        MoveToCage(owner, placement);
        MoveToCage(stranger, placement);
        Cage cage(map, placement);
        uint32 const before = owner.GetQuestObjectiveData(QuestId, CageEntry);
        Require(cage.ActivateToQuest(&owner) && !cage.ActivateToQuest(&stranger), "Cage marker does not follow the player's incomplete quest");
        cage.Use(&owner);
        Require(cage.GetGoState() == GO_STATE_READY && owner.GetQuestObjectiveData(QuestId, CageEntry) == before,
            "An empty cage can open or give quest credit");
        bool firstPair = placement.cage == first.cage;
        if (firstPair)
        {
            Placement otherOccupant = placement;
            otherOccupant.captive = placements[1].captive;
            Cage otherGate(map, placements[1]);
            Brave nearby(map, otherOccupant);
            Require(map.AddToMap(static_cast<Creature*>(&nearby)) && owner.canSeeOrDetect(&nearby), "Unable to place a visible unrelated Captured Brave");
            cage.Use(&owner);
            Require(cage.GetGoState() == GO_STATE_READY && owner.GetQuestObjectiveData(QuestId, CageEntry) == before,
                "An unrelated nearby captive made the empty cage usable");
        }
        auto brave = std::make_unique<Brave>(map, placement);
        Require(map.AddToMap(static_cast<Creature*>(brave.get())) && stranger.canSeeOrDetect(brave.get()), "Missing the shared initial captive");
        cage.Use(&stranger);
        MoveToCage(owner, placement, 25.0f);
        cage.Use(&owner);
        Require(cage.GetGoState() == GO_STATE_READY && owner.GetQuestObjectiveData(QuestId, CageEntry) == before,
            "A nearby stranger or distant eligible player opened the cage");
        if (firstPair)
        {
            MoveToCage(owner, placement, 4.0f);
            Placement obstacle = placement;
            obstacle.cage = 9000000;
            float exit = placement.captivePosition.GetOrientation();
            obstacle.cagePosition.Relocate(placement.cagePosition.GetPositionX() + 2.0f * std::cos(exit),
                placement.cagePosition.GetPositionY() + 2.0f * std::sin(exit),
                placement.cagePosition.GetPositionZ(), placement.cagePosition.GetOrientation());
            Cage separateBars(map, obstacle);
            cage.Use(&owner);
            Require(cage.GetGoState() == GO_STATE_READY && owner.GetQuestObjectiveData(QuestId, CageEntry) == before,
                "Ignoring the target bars also ignored a separate physical cage");
        }
        MoveToCage(owner, placement);
        std::cout << "CASE cage=" << placement.cage << " range=" << cage.IsWithinDistInMap(&owner, cage.GetInteractionDistance())
            << " los=" << owner.IsWithinLOSInMap(&cage) << " cage_visible=" << owner.canSeeOrDetect(&cage)
            << " brave_visible=" << owner.canSeeOrDetect(brave.get()) << " native_brave=" << (map.GetCreature(brave->GetGUID()) == brave.get())
            << " native_cage=" << (ObjectAccessor::GetGameObject(*brave, cage.GetGUID()) == &cage) << " state=" << unsigned(cage.getLootState()) << std::endl;
        cage.Use(&owner);
        Require(cage.GetGoState() == GO_STATE_ACTIVE && owner.GetQuestObjectiveData(QuestId, CageEntry) == before + 1,
            "Native world use did not open the cage for exactly one credit");
        cage.Use(&owner);
        Require(owner.GetQuestObjectiveData(QuestId, CageEntry) == before + 1, "Repeated use consumed the same open cage twice");
        float distance = 0.0f;
        for (uint32 elapsed = 0; elapsed < 9000 && stranger.canSeeOrDetect(brave.get()); elapsed += 100)
        {
            brave->Update(100);
            map.MoveAllCreaturesInMoveList();
            distance = std::max(distance, brave->GetExactDist(&placement.captivePosition));
        }
        std::cout << "CASE cage=" << placement.cage << " captive=" << placement.captive << " moved=" << distance
            << " hidden=" << !stranger.canSeeOrDetect(brave.get()) << std::endl;
        Require(distance > 1.0f && !stranger.canSeeOrDetect(brave.get()) && !owner.canSeeOrDetect(brave.get()),
            "The actual captive did not escape and disappear for both players");
        cage.FinishOpenTimer();
        brave->AI()->UpdateAI(1000);
        Require(!cage.isSpawned(), "The native cage did not enter its shared respawn wait");
        Require(!stranger.canSeeOrDetect(brave.get()), "A captive reappeared ahead of the native cage");
        if (firstPair)
        {
            brave.reset();
            brave = std::make_unique<Brave>(map, placement);
            Require(map.AddToMap(static_cast<Creature*>(brave.get())) && !stranger.canSeeOrDetect(brave.get()),
                "Recreating the original captive during cage respawn exposed it prematurely");
        }
        cage.FinishRespawnTimer();
        brave->AI()->UpdateAI(1000);
        Require(cage.isSpawned() && cage.GetGoState() == GO_STATE_READY && stranger.canSeeOrDetect(brave.get()) &&
            brave->GetExactDist(&placement.captivePosition) < 0.1f, "Native rearm did not restore the original shared captive at home");
        if (owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)
        {
            Require(!cage.ActivateToQuest(&owner), "A completed quest still activates the cage marker");
            cage.Use(&owner);
            Require(cage.GetGoState() == GO_STATE_READY, "A completed quest can open another cage");
        }
        if (firstPair)
        {
            Accept(stranger, quest);
            cage.Use(&stranger);
            Require(stranger.GetQuestObjectiveData(QuestId, CageEntry) == 1 && owner.GetQuestObjectiveData(QuestId, CageEntry) == 1,
                "A second player could not use the respawned cage or changed the first player's credit");
            stranger.RemoveActiveQuest(QuestId);
            stranger.SetQuestSlot(0, 0);
        }
    }
    Require(checkedReward, "The native sequence never admitted a valid reward choice");
    std::cout << "PASS ten native cage pairs, exact occupants, per-player credit, visible movement, shared rearm and reward eligibility\n";
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
        LoadGameObjectModelList(argv[2]);
        auto quest = LoadQuests();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sCreatureTextMgr->LoadCreatureTexts();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifndef MULGORE_CAGES_BASELINE
        AddSC_mulgore();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckCages(quest);
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
