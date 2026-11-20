#define LEGION_ROTATION_SUPPORT_ONLY
#define LEGION_ROTATION_PROGRESSION
#define LEGION_ROTATION_GARRISON_PLACEMENTS
#include "LegionWorldQuestRotationNative.cpp"

struct SaveRotation
{
    using Type = void (QuestDataStoreMgr::*)();
    friend Type Access(SaveRotation);
};
template struct FixtureAccess<SaveRotation, &QuestDataStoreMgr::SaveWorldQuest>;

struct ResetRotation
{
    using Type = void (QuestDataStoreMgr::*)();
    friend Type Access(ResetRotation);
};
template struct FixtureAccess<ResetRotation, static_cast<ResetRotation::Type>(&QuestDataStoreMgr::ResetWorldQuest)>;

template <unsigned index>
struct LoadQuestRows
{
    using Type = void (Player::*)(PreparedQueryResult);
    friend Type Access(LoadQuestRows<index>);
};
template struct FixtureAccess<LoadQuestRows<0>, &Player::_LoadQuestStatus>;
template struct FixtureAccess<LoadQuestRows<1>, &Player::_LoadQuestStatusObjectives>;
template struct FixtureAccess<LoadQuestRows<2>, &Player::_LoadQuestStatusRewarded>;
template struct FixtureAccess<LoadQuestRows<3>, &Player::_LoadWorldQuestStatus>;

struct SaveQuestRows
{
    using Type = void (Player::*)(CharacterDatabaseTransaction&);
    friend Type Access(SaveQuestRows);
};
template struct FixtureAccess<SaveQuestRows, &Player::_SaveQuestStatus>;

namespace
{
constexpr uint32 QuestId = 46073;
constexpr uint32 FixtureGuid = 101;
constexpr uint32 InitialMoney = 20000;
constexpr uint32 RewardMoney = 1234;

PreparedQueryResult QueryPlayer(WorldSession& session, CharacterDatabaseStatements statementId, uint32 guid)
{
    auto* statement = CharacterDatabase.GetPreparedStatement(statementId);
    statement->setUInt32(0, session.GetAccountId());
    statement->setUInt64(1, guid);
    PreparedQueryResult result;
    auto query = CharacterDatabase.AsyncQuery(statement).WithPreparedCallback(
        [&](PreparedQueryResult rows) { result = std::move(rows); });
    auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!query.InvokeIfReady())
    {
        Require(std::chrono::steady_clock::now() < deadline, "Timed out waiting for a native login query");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return result;
}

void InitializeFixture(int argc, char** argv)
{
    InitializeDungeon(argc, argv, true);
    auto identity = QueryRewardFixture("SELECT DATABASE(), @@datadir");
    auto path = std::getenv("LEGION_GARRISON_FIXTURE_PATH");
    Require(identity && path && identity->Fetch()[0].GetString() == "fixture_char" &&
        std::filesystem::equivalent(std::filesystem::path(identity->Fetch()[1].GetString()), std::filesystem::path(path)),
        "Garrison checkpoints require a verified disposable character database");
    CharacterDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    CharacterDatabase.Execute("SET SESSION TRANSACTION READ WRITE");
    auto writable = QueryRewardFixture("SELECT @@tx_read_only");
    Require(writable && !writable->Fetch()[0].GetUInt32(), "Isolated worker cannot commit the checkpoints");

    DisableMgr::LoadDisables();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sQuestDataStore->LoadQuests();
    LoadRandomEnchantmentsTable();
    LoadLootTables();
    sAchievementMgr->LoadCriteriaList();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    sObjectMgr->LoadPhaseDefinitions();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowDialogue("conditions", "SourceTypeOrReferenceId=23 AND SourceGroup=7543 AND SourceEntry IN (126,142,143)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sConditionMgr->LoadConditions();
    sConversationDataStore->LoadConversationData();
    HashMapHolder<Conversation>::SetSize(20000);
    std::unordered_map<uint32, std::vector<uint32>> outdoor{{1220, {}}};
    dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe(outdoor);
    MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe(outdoor);
    sQuestDataStore->LoadWorldQuestTemplates();
}

Quest const* GarrisonQuest()
{
    auto const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    Require(quest && quest->IsWorld(), "Garrison world quest template missing");
    return quest;
}

WorldQuest& Publish(uint32 questId)
{
    auto const* quest = sQuestDataStore->GetQuestTemplate(questId);
    Require(quest, "Missing configured quest");
    auto const* update = (sQuestDataStore->*Access(RotationUpdateTag{}))(questId, quest->QuestInfoID, 0);
    Require(update && update->worldQuest &&
        (sQuestDataStore->*Access(RotationAdmissionTag{}))(update->worldQuest, update),
        "Native rotation refused a compatible quest");
    auto* published = const_cast<WorldQuest*>(
        (sQuestDataStore->*Access(RotationGenerateTag{}))(update, update->worldQuest));
    Require(published, "Native rotation could not publish the quest");
    published->Gold = RewardMoney;
    published->CurrencyID = 0;
    published->Recipe = nullptr;
    for (WorldQuestItem& item : published->ItemList)
        item = {};
    return *published;
}

void PreparePlayer(RotationPlayer& player)
{
    player.SetMovedUnit(&player);
    player.RefreshWorldQuestAreaTasks();
    player.GetPhaseMgr().NotifyStoresReloaded();
}

Position Approach(RotationLibram const& book)
{
    return {book.GetPositionX() + 1.f, book.GetPositionY(), book.GetPositionZ(), 0.f};
}

void KillObjective(RotationPlayer& player, uint32 entry, uint32 guid)
{
    RotationVictim victim(player, entry, guid);
    double damage = std::ceil(double(victim.GetHealth(&player)) * victim.GetHealthMultiplierForTarget(&player) * 1.001);
    Require(damage > 0 && damage <= std::numeric_limits<uint32>::max(), "Invalid controlled damage");
    player.DealDamage(&victim, uint32(damage), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
    Require(victim.isDead(), "Native lethal damage did not kill the quest target");
    player.PumpCallbacks(50);
}

void LoadQuestCheckpoint(RotationPlayer& player)
{
    auto load = [&](CharacterDatabaseStatements id)
    {
        return QueryPlayer(*player.GetSession(), id, player.GetGUIDLow());
    };
    (player.*Access(LoadQuestRows<0>{}))(load(CHAR_SEL_CHARACTER_QUESTSTATUS));
    (player.*Access(LoadQuestRows<1>{}))(load(CHAR_SEL_CHARACTER_QUESTSTATUS_OBJECTIVES));
    (player.*Access(LoadQuestRows<2>{}))(load(CHAR_SEL_CHARACTER_QUESTSTATUSREW));
    (player.*Access(LoadQuestRows<3>{}))(load(CHAR_SEL_CHARACTER_WORLDQUESTSTATUS));
    PreparePlayer(player);
}

void LoadPlayerCheckpoint(RotationPlayer& player)
{
    auto money = QueryRewardFixture("SELECT money FROM characters WHERE guid=101 AND name='Garrisontest'");
    Require(money && money->GetRowCount() == 1, "Expected the synthetic character row");
    player.SetMoney(money->Fetch()[0].GetUInt64());
    LoadQuestCheckpoint(player);
}

void Checkpoint(Map& map)
{
    Require(!sQuestDataStore->GetWorldQuest(GarrisonQuest()) && !QueryRewardFixture("SELECT guid FROM characters"),
        "Checkpoint must start from an empty synthetic character database");
    // Keep unrelated pool generation out of the real restart in this disposable realm.
    CharacterDatabase.DirectExecute("INSERT INTO worldstates (entry,value) VALUES (20019,UNIX_TIMESTAMP()+86400) "
        "ON DUPLICATE KEY UPDATE value=VALUES(value)");
    CharacterDatabase.DirectExecute("INSERT INTO characters (guid,account,name,race,class,gender,level,money,position_x,position_y,"
        "position_z,map,zone,taximask,specialization) VALUES (101,0,'Garrisontest',1,3,0,110,20000,-1290,2639,32.61,1220,7543,'0',253)");
    auto const& active = Publish(QuestId);
    RotationLibram first(map, 25760460);
    RotationPlayer player(map, FixtureGuid, Approach(first));
    player.UnlockBrokenShore();
    PreparePlayer(player);
    for (uint32 i = 0; i < 26; ++i)
        KillObjective(player, 117559, 190000 + i);
    KillObjective(player, 117555, 190026);
    KillObjective(player, 118720, 190027);
    first.UseThroughSession(player);
    Require(player.GetQuestObjectiveData(QuestId, 117559) == 26 && player.GetQuestObjectiveData(QuestId, 117555) == 1 &&
        player.GetQuestObjectiveData(QuestId, 118720) == 1 && player.GetQuestObjectiveData(QuestId, 269154) == 1 &&
        player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE, "Did not reach the real mixed checkpoint");
    player.SetMoney(InitialMoney);
    auto trans = CharacterDatabase.BeginTransaction();
    (player.*Access(SaveQuestRows{}))(trans);
    player.SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    (sQuestDataStore->*Access(SaveRotation{}))();
    auto saved = QueryRewardFixture("SELECT w.ResetTime,s.status,o.data FROM world_quest w "
        "JOIN character_queststatus s ON s.quest=w.QuestID AND s.guid=101 "
        "JOIN character_queststatus_objectives o ON o.quest=s.quest AND o.guid=s.guid AND o.objective=0 "
        "WHERE w.QuestID=46073");
    Require(saved && saved->Fetch()[0].GetUInt32() == active.ResetTime && saved->Fetch()[1].GetUInt8() == QUEST_STATUS_INCOMPLETE &&
        saved->Fetch()[2].GetUInt32() == 26, "Native checkpoints did not persist before restart");
    std::cout << "GARRISON checkpoint saved four objective counts and active rotation, deadline=" << active.ResetTime << std::endl;
}

void FinishCheckpoint(Map& map, bool reconnect)
{
    auto const* quest = GarrisonQuest();
    auto const* active = sQuestDataStore->GetWorldQuest(quest);
    Require(active && active->ResetTime > GameTime::GetGameTime() && active->Gold == RewardMoney &&
        sWorldStateMgr.GetWorldStateValue(12978) == 3, "Restart did not load the original active rotation");
    RotationLibram last(map, 25760461);
    RotationPlayer player(map, FixtureGuid, Approach(last));
    LoadPlayerCheckpoint(player);
    if (!reconnect)
    {
        Require(player.CanRewardWorldQuest(quest) && player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
            player.GetQuestObjectiveData(QuestId, 117559) == 26 && player.GetQuestObjectiveData(QuestId, 117555) == 1 &&
            player.GetQuestObjectiveData(QuestId, 118720) == 1 && player.GetQuestObjectiveData(QuestId, 269154) == 1 &&
            player.GetMoney() == InitialMoney, "Native login did not restore the four progress counts");
        last.UseThroughSession(player);
        auto outro = LiveConversations();
        Require(outro.size() == 1 && outro[0]->GetEntry() == 4440 && outro[0]->GetCasterGUID() == player.GetGUID(),
            "Final book did not create the personal outro after restart");
        ClearConversations(map);
    }
    Require(player.WorldQuestCompleted(QuestId) && !player.CanRewardWorldQuest(quest) &&
        player.FindQuestSlot(QuestId) == MAX_QUEST_LOG_SIZE && player.GetMoney() == InitialMoney + RewardMoney,
        "Native completion receipt or gold is incorrect after login");
    RotationLibram extra(map, 25760462);
    player.PlaceInMap(Approach(extra));
    extra.UseThroughSession(player);
    player.RewardQuest(quest, 0, &player, true);
    Require(!extra.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE) && player.GetMoney() == InitialMoney + RewardMoney,
        "A completed Garrison quest allowed repeated book use or reward");
    auto saved = QueryRewardFixture("SELECT c.money,w.resetTime,(SELECT COUNT(*) FROM character_queststatus WHERE guid=101 AND quest=46073),"
        "(SELECT COUNT(*) FROM character_queststatus_world WHERE guid=101 AND quest=46073) "
        "FROM characters c JOIN character_queststatus_world w ON c.guid=w.guid WHERE c.guid=101 AND w.quest=46073");
    Require(saved && saved->Fetch()[0].GetUInt64() == InitialMoney + RewardMoney && saved->Fetch()[1].GetUInt32() == active->ResetTime &&
        !saved->Fetch()[2].GetUInt32() && saved->Fetch()[3].GetUInt32() == 1, "Native reward persistence had duplicate or stale records");
    std::cout << (reconnect ? "GARRISON reconnect retained exactly one reward" : "GARRISON last book completed the persisted checkpoint")
              << " money=" << player.GetMoney() << " deadline=" << active->ResetTime << std::endl;
}

class WallClockSpawn : public Creature
{
public:
    WallClockSpawn(Map& map, uint32 dbGuid, uint32 entry, uint32 delay) : Creature(false)
    {
        auto row = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation,PhaseId,spawntimesecs "
            "FROM creature WHERE guid=%u AND id=%u AND map=1220 AND areaId=8551 AND spawnMask=1 AND phaseMask=1", dbGuid, entry);
        Require(row && row->Fetch()[4].GetString() == "7147" && row->Fetch()[5].GetUInt32() == delay,
            "The shipped spawn row is missing or differs");
        auto fields = row->Fetch();
        CreatureData& data = sObjectMgr->NewOrExistCreatureData(dbGuid);
        data.guid = dbGuid;
        data.id = entry;
        data.mapid = 1220;
        data.zoneId = 7543;
        data.areaId = 8551;
        data.posX = fields[0].GetFloat();
        data.posY = fields[1].GetFloat();
        data.posZ = fields[2].GetFloat();
        data.orientation = fields[3].GetFloat();
        data.PhaseID = {7147};
        data.spawntimesecs = fields[5].GetUInt32();
        map.LoadGrid(data.posX, data.posY);
        Require(LoadCreatureFromDB(dbGuid, &map) && GetRespawnDelay() == delay && HasPhaseId(7147) && IsAlive(),
            "The real spawn did not load into the zone map");
        // Keep optional combat AI from adding another death after the first respawn.
        SetReactState(REACT_PASSIVE);
    }

    ~WallClockSpawn()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

void CheckWallClockRespawns()
{
    // Broken Shore is a zone instance within the live map 1220.
    Map map(1220, 600000, 7543, DIFFICULTY_NONE);
    RotationLibram book(map, 25760460);
    RotationLibram impAnchor(map, 25760461);
    RotationLibram easternAnchor(map, 25760463);
    WallClockSpawn imp(map, 146980460, 117555, 45);
    WallClockSpawn flaz(map, 146980472, 118720, 240);
    // Each dead spawn must stay within the zone map's ordinary active cell visits.
    map.GameObjectRelocation(&impAnchor, imp.GetPositionX(), imp.GetPositionY(), imp.GetPositionZ(), 0.f);
    map.MoveAllGameObjectsInMoveList();
    book.GetZoneAndAreaId(book.m_zoneId, book.m_areaId);
    impAnchor.GetZoneAndAreaId(impAnchor.m_zoneId, impAnchor.m_areaId);
    easternAnchor.GetZoneAndAreaId(easternAnchor.m_zoneId, easternAnchor.m_areaId);
    book.setActive(true);
    impAnchor.setActive(true);
    easternAnchor.setActive(true);
    RotationPlayer player(map, 102, Approach(book));
    player.UnlockBrokenShore();
    PreparePlayer(player);
    Require(book.GetCurrentZoneID() == 7543 && impAnchor.GetCurrentZoneID() == 7543 && easternAnchor.GetCurrentZoneID() == 7543 &&
        impAnchor.GetExactDist(&imp) < 1.f &&
        player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && !player.WorldQuestCompleted(QuestId),
        "Map-time fixture requires two active anchors and an unfinished task");

    GameTime::UpdateGameTimers();
    auto const death = GameTime::GetGameTime();
    auto const start = std::chrono::steady_clock::now();
    book.UseThroughSession(player);
    Require(player.GetQuestObjectiveData(QuestId, 269154) == 1 && book.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE),
        "The real Libram interaction did not start the native cooldown");
    imp.setDeathState(JUST_DIED);
    flaz.setDeathState(JUST_DIED);
    struct ObservedSpawn
    {
        Creature& Actor;
        time_t CorpseEnd;
        time_t ReturnAt;
        bool Removed = false;
        bool Returned = false;
    };
    std::array<ObservedSpawn, 2> spawns{{
        {imp, death + imp.GetCorpseDelay(), imp.GetRespawnTime()},
        {flaz, death + flaz.GetCorpseDelay(), flaz.GetRespawnTime()}
    }};
    for (auto const& spawn : spawns)
        Require(spawn.Actor.getDeathState() == CORPSE && spawn.ReturnAt == spawn.CorpseEnd + spawn.Actor.GetRespawnDelay(),
            "The native death timer does not match the actual template and spawn");
    auto const maxRun = std::chrono::seconds(std::max(spawns[0].ReturnAt, spawns[1].ReturnAt) - death + 15);
    std::cout << "GARRISON started real map clock, corpse+respawn imp=" << spawns[0].ReturnAt - death
              << "s Flaz=" << spawns[1].ReturnAt - death << "s" << std::endl;
    time_t bookReturns = 0;
    bool bookReturned = false;
    auto previousTick = start;
    while (!spawns[0].Returned || !spawns[1].Returned || !bookReturned)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto tick = std::chrono::steady_clock::now();
        Require(tick - start <= maxRun, "Real-time grid did not reach all three respawn deadlines");
        GameTime::UpdateGameTimers();
        auto wall = GameTime::GetGameTime();
        map.Update(uint32(std::chrono::duration_cast<std::chrono::milliseconds>(tick - previousTick).count()));
        previousTick = tick;
        for (auto& spawn : spawns)
        {
            if (!spawn.Removed && spawn.Actor.getDeathState() == DEAD)
            {
                Require(wall >= spawn.CorpseEnd && wall <= spawn.CorpseEnd + 4, "Corpse removal drifted from its deadline");
                spawn.Removed = true;
                std::cout << "GARRISON corpse " << spawn.Actor.GetEntry() << " removed after " << wall - death << "s" << std::endl;
            }
            if (!spawn.Returned && spawn.Actor.IsAlive())
            {
                Require(spawn.Removed && wall >= spawn.ReturnAt && wall <= spawn.ReturnAt + 4 && spawn.Actor.HasPhaseId(7147) &&
                    spawn.Actor.GetHealth() == spawn.Actor.GetMaxHealth(), "Creature respawn drifted from its deadline");
                spawn.Returned = true;
                std::cout << "GARRISON spawn " << spawn.Actor.GetEntry() << " returned after " << wall - death << "s" << std::endl;
            }
        }
        if (!bookReturns && !book.isSpawned())
        {
            bookReturns = book.GetRespawnTime();
            Require(bookReturns == wall + 120, "Real Libram visibility interval is not 120 seconds");
            book.UseThroughSession(player);
            Require(player.GetQuestObjectiveData(QuestId, 269154) == 1, "A hidden Libram gave another credit");
            std::cout << "GARRISON book hidden after " << wall - death << "s" << std::endl;
        }
        if (bookReturns && !bookReturned && book.isSpawned())
        {
            Require(wall >= bookReturns && wall <= bookReturns + 4 && book.getLootState() == GO_READY,
                "The Libram reappeared outside its actual ready-state deadline");
            book.UseThroughSession(player);
            Require(player.GetQuestObjectiveData(QuestId, 269154) == 2, "A respawned Libram refused an eligible use");
            bookReturned = true;
            std::cout << "GARRISON book returned after " << wall - (bookReturns - 120) << "s hidden and accepted another use" << std::endl;
        }
    }
    easternAnchor.setActive(false);
    impAnchor.setActive(false);
    book.setActive(false);
}
void SaveExpiryPlayer(RotationPlayer& player)
{
    auto trans = CharacterDatabase.BeginTransaction();
    (player.*Access(SaveQuestRows{}))(trans);
    CharacterDatabase.CommitTransaction(trans);
}

void CheckExpiry(Map& map)
{
    auto const* quest = GarrisonQuest();
    auto* active = const_cast<WorldQuest*>(sQuestDataStore->GetWorldQuest(quest));
    Require(active && active->ResetTime > GameTime::GetGameTime(), "Expiry check requires the existing Garrison rotation");
    uint32 const oldStart = active->StartTime;
    RotationLibram onlineBook(map, 25760460);
    RotationLibram offlineBook(map, 25760461);
    RotationLibram expiredBook(map, 25760462);
    RotationPlayer online(map, 102, Approach(onlineBook));
    RotationPlayer delayed(map, 105, Approach(onlineBook));
    delayed.UnlockBrokenShore();
    PreparePlayer(delayed);
    KillObjective(delayed, 117555, 190039);
    Require(delayed.GetQuestObjectiveData(QuestId, 117555) == 1, "The delayed-cleanup fixture lacks its old kill credit");
    online.UnlockBrokenShore();
    PreparePlayer(online);
    onlineBook.UseThroughSession(online);
    Require(online.GetQuestObjectiveData(QuestId, 269154) == 1, "The online fixture lacks its real partial quest progress");
    SaveExpiryPlayer(online);
    {
        RotationPlayer disconnected(map, 103, Approach(offlineBook));
        disconnected.UnlockBrokenShore();
        PreparePlayer(disconnected);
        offlineBook.UseThroughSession(disconnected);
        Require(disconnected.GetQuestObjectiveData(QuestId, 269154) == 1, "The offline fixture lacks its partial progress");
        SaveExpiryPlayer(disconnected);
    }
    auto pending = QueryRewardFixture("SELECT COUNT(*) FROM character_queststatus WHERE guid IN (102,103) AND quest=46073 AND status=3");
    Require(pending && pending->Fetch()[0].GetUInt32() == 2, "Both unfinished characters must be persisted before expiry");

    GameTime::UpdateGameTimers();
    active->ResetTime = GameTime::GetGameTime();
    CharacterDatabase.DirectExecute("UPDATE world_quest SET ResetTime=UNIX_TIMESTAMP() WHERE QuestID=46073");
    online.PlaceInMap(Approach(expiredBook));
    expiredBook.UseThroughSession(online);
    online.KillCreditGO(269154, expiredBook.GetGUID());
    KillObjective(online, 117555, 190040);
    bool const afterDeadlineBlocked = online.GetQuestObjectiveData(QuestId, 269154) == 1 &&
        online.GetQuestObjectiveData(QuestId, 117555) == 0 && !expiredBook.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);

    (sQuestDataStore->*Access(ResetRotation{}))();
    Require(!sQuestDataStore->GetWorldQuest(quest) && sWorldStateMgr.GetWorldStateValue(12978) == 0,
        "The actual scheduled reset did not withdraw the expired rotation");
    auto const& felskorn = Publish(45744);
    Require(felskorn.QuestID == 45744 && sWorldStateMgr.GetWorldStateValue(12979) == 1,
        "The compatible replacement event did not publish");
    // The replacement's admission needs the native modifier tree. This fixture
    // leaves unrelated combat achievement tracking off.
    online.GetAchievementMgr()->m_canUpdateAchiev = 1;
    sQuestDataStore->QueueWorldQuestTaskRefreshes();
    online.PumpCallbacks(150);
    online.PumpCallbacks(150);
    online.GetPhaseMgr().NotifyStoresReloaded();
    online.GetAchievementMgr()->m_canUpdateAchiev = 0;
    KillObjective(online, 117555, 190041);
    expiredBook.UseThroughSession(online);

    bool const removed = online.GetQuestStatus(QuestId) == QUEST_STATUS_NONE && online.FindQuestSlot(QuestId) == MAX_QUEST_LOG_SIZE;
    bool const progressed = online.GetQuestObjectiveData(QuestId, 117555) != 0 ||
        online.GetQuestObjectiveData(QuestId, 269154) != 0 || expiredBook.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE);
    auto saved = QueryRewardFixture("SELECT (SELECT COUNT(*) FROM character_queststatus WHERE guid IN (102,103) AND quest=46073),"
        "(SELECT COUNT(*) FROM character_queststatus_objectives WHERE guid IN (102,103) AND quest=46073)");
    auto const statusRows = saved->Fetch()[0].GetUInt32();
    auto const objectiveRows = saved->Fetch()[1].GetUInt32();
    std::cout << "GARRISON expiry immediateGuard=" << afterDeadlineBlocked << " removed=" << removed
              << " staleProgress=" << progressed << " oldStatusRows=" << statusRows << " oldObjectiveRows=" << objectiveRows
              << " Felskorn=" << unsigned(online.GetQuestStatus(45744)) << std::endl;
    Require(afterDeadlineBlocked && removed && !progressed && !statusRows && !objectiveRows,
        "Expired Garrison progress or quest state outlived the rotation");
    Require(online.GetQuestStatus(45744) == QUEST_STATUS_INCOMPLETE && online.HasPhaseId(7148) && !online.HasPhaseId(7147),
        "The same stationary player did not enter the actual replacement phase");
    Require(!online.WorldQuestCompleted(QuestId) && !online.CanRewardWorldQuest(quest),
        "Expiry granted or retained a completion reward for an unfinished player");

    // Model a last save during disconnect that reached the DB after expiry
    // cleanup. The persisted old rotation must still fail login admission.
    CharacterDatabase.DirectExecute(("INSERT INTO character_queststatus (guid,account,quest,status,timer) "
        "VALUES (103,0,46073,3," + std::to_string(oldStart) + ")").c_str());
    CharacterDatabase.DirectExecute("INSERT INTO character_queststatus_objectives (guid,account,quest,objective,data) VALUES (103,0,46073,4,1)");
    {
        RotationPlayer relogWithoutTask(map, 103, Approach(expiredBook));
        LoadQuestCheckpoint(relogWithoutTask);
        Require(relogWithoutTask.GetQuestStatus(QuestId) == QUEST_STATUS_NONE && !relogWithoutTask.GetQuestObjectiveData(QuestId, 269154),
            "An offline expired row reappeared after login while the old task was inactive");
    }
    sQuestDataStore->ResetWorldQuest(45744);
    auto const& next = Publish(QuestId);
    Require(next.StartTime > oldStart, "Expected a new native rotation generation");
    PreparePlayer(delayed);
    delayed.PumpCallbacks(300);
    Require(delayed.CanRewardWorldQuest(quest) && delayed.FindQuestSlot(QuestId) < MAX_QUEST_LOG_SIZE &&
        !delayed.GetQuestObjectiveData(QuestId, 117555),
        "Delayed cleanup either removed a fresh attempt or carried old progress forward");
    RotationPlayer relogWithNewTask(map, 103, Approach(expiredBook));
    LoadQuestCheckpoint(relogWithNewTask);
    Require(relogWithNewTask.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        !relogWithNewTask.GetQuestObjectiveData(QuestId, 269154) && relogWithNewTask.CanRewardWorldQuest(quest),
        "A previous rotation's saved rows contaminated the replacement Garrison rotation");
    expiredBook.UseThroughSession(relogWithNewTask);
    Require(relogWithNewTask.GetQuestObjectiveData(QuestId, 269154) == 1, "The fresh eligible attempt lost normal book credit");

    // Older realm rows used the ordinary save timestamp rather than the
    // rotation start. A save legitimately made during this rotation can resume.
    CharacterDatabase.DirectExecute(("INSERT INTO character_queststatus (guid,account,quest,status,timer) "
        "VALUES (104,0,46073,3," + std::to_string(next.StartTime + 1) + ")").c_str());
    CharacterDatabase.DirectExecute("INSERT INTO character_queststatus_objectives (guid,account,quest,objective,data) VALUES (104,0,46073,4,1)");
    RotationPlayer legacy(map, 104, Approach(expiredBook));
    LoadQuestCheckpoint(legacy);
    Require(legacy.CanRewardWorldQuest(quest) && legacy.GetQuestObjectiveData(QuestId, 269154) == 1,
        "The same rotation's legacy save was lost");
    std::cout << "GARRISON absent and replaced rotation rejected stale login progress; a fresh task and its legacy save retained credit\n";
}
}

int main(int argc, char** argv)
{
    try
    {
        InitializeFixture(argc, argv);
        auto stage = std::getenv("LEGION_GARRISON_STAGE");
        Require(stage, "Missing explicit Garrison checkpoint stage");
        std::string_view mode(stage);
        if (mode == "respawn")
            CheckWallClockRespawns();
        else
        {
            Map map(1220, 300, 0, DIFFICULTY_NONE);
            if (mode == "checkpoint")
                Checkpoint(map);
            else if (mode == "finish" || mode == "reconnect")
                FinishCheckpoint(map, mode == "reconnect");
            else if (mode == "expiry")
                CheckExpiry(map);
            else
                Require(false, "Unrecognized checkpoint stage");
        }
        CharacterDatabase.Close();
        LoginDatabase.Close();
        WorldDatabase.Close();
        HotfixDatabase.Close();
        MySQL::Library_End();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
