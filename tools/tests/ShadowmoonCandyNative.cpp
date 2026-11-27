#define main CandyBucketsProbeMain
#include "HallowsEndCandyBucketsNative.cpp"
#undef main

#include "GameEventMgr.h"
#include "ReputationMgr.h"

#include <chrono>
#include <thread>

struct ShadowmoonSeasonTag
{
    using Type = std::unordered_map<uint32, uint16> GameEventMgr::*;
    friend Type Access(ShadowmoonSeasonTag);
};
template struct FixtureAccess<ShadowmoonSeasonTag, &GameEventMgr::_questToEventLinks>;

struct ShadowmoonReputationInitTag
{
    using Type = void (ReputationMgr::*)();
    friend Type Access(ShadowmoonReputationInitTag);
};
template struct FixtureAccess<ShadowmoonReputationInitTag, &ReputationMgr::Initialize>;

namespace
{
constexpr uint32 ShadowmoonQuest = 12409;
constexpr uint16 HallowsEnd = 12;
constexpr uint32 AldorBucket = 190115;
constexpr uint32 ScryerBucket = 190116;

void LoadShadowmoon(bool baseline)
{
    auto row = WorldDatabase.Query(
#include "NativeShadowmoonCandy.inc"
    );
    Require(bool(row), "Missing the actual shared Shadowmoon quest");
    auto quest = new Quest(row->Fetch());
    Require(quest->IsAutoComplete() && (quest->IsSeasonal() != baseline) && !quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE) &&
        quest->RewardItemId[0] == Candy && quest->RewardItemCount[0] == 1, "Wrong shared bucket quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[ShadowmoonQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(ShadowmoonQuest, quest);
    auto event = WorldDatabase.Query("SELECT eventEntry FROM game_event_seasonal_questrelation WHERE questId=12409");
    Require(event && event->GetRowCount() == 1 && event->Fetch()[0].GetUInt16() == HallowsEnd, "Wrong seasonal quest linkage");
    (sGameEventMgr->*Access(ShadowmoonSeasonTag{}))[ShadowmoonQuest] = HallowsEnd;
    auto& calendar = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    calendar.resize(HallowsEnd + 1);
    auto holiday = WorldDatabase.Query("SELECT holiday,occurence,length FROM game_event WHERE eventEntry=12 AND world_event=0");
    Require(bool(holiday), "The actual Hallow's End event is required");
    auto schedule = holiday->Fetch();
    calendar[HallowsEnd].holiday_id = HolidayIds(schedule[0].GetUInt32());
    calendar[HallowsEnd].occurence = schedule[1].GetUInt32();
    calendar[HallowsEnd].length = schedule[2].GetUInt32();

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_queststarter AS SELECT * FROM legion_world.gameobject_queststarter WHERE quest=12409");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_questender AS SELECT * FROM legion_world.gameobject_questender WHERE quest=12409");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=19 AND SourceEntry=12409");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto conditions = WorldDatabase.Query("SELECT COUNT(*) FROM conditions");
    Require(conditions && conditions->Fetch()[0].GetUInt32() == (baseline ? 0u : 6u), "Unexpected actual reputation and event gates");
    if (!baseline)
        sConditionMgr->LoadConditions(false);
    sQuestDataStore->LoadGameobjectQuestRelations();
    sQuestDataStore->LoadGameobjectInvolvedRelations();
}

void PublishBucket(CandyBucket& bucket, Map& map, uint32 entry, uint32 expectedArea)
{
    auto rows = WorldDatabase.PQuery("SELECT g.guid, g.position_x, g.position_y, g.position_z, g.orientation, "
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3 FROM gameobject g "
        "JOIN game_event_gameobject e USING (guid) WHERE g.id=%u AND g.map=530 AND e.eventEntry=12", entry);
    Require(rows && rows->GetRowCount() == 1, "Missing unique actual Hallow's End bucket placement");
    auto row = rows->Fetch();
    Position position{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
    map.LoadGrid(position.GetPositionX(), position.GetPositionY());
    uint32 zone = 0;
    uint32 area = 0;
    map.GetZoneAndAreaId(zone, area, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ());
    Require(zone == 3520 && area == expectedArea, "The actual Shadowmoon map disagrees with the bucket area");
    Require(bucket.Create(row[0].GetUInt32(), entry, &map, 1, position,
        G3D::Quat(row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat()), 100, GO_STATE_READY) &&
        map.AddToMap(static_cast<GameObject*>(&bucket)), "Failed to publish the actual world bucket");
}

void SetStanding(LootPlayer& player, int32 aldor, int32 scryer)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
    player.setFactionForRace(RACE_HUMAN);
    player.SetLevel(15);
    auto& reputation = player.GetReputationMgr();
    (reputation.*Access(ShadowmoonReputationInitTag{}))();
    Require(reputation.SetOneFactionReputation(sFactionStore.LookupEntry(932), aldor, false, false) &&
        reputation.SetOneFactionReputation(sFactionStore.LookupEntry(934), scryer, false, false), "Failed to configure real factions");
}

void Claim(LootPlayer& player, CandyBucket const& bucket)
{
    auto map = player.GetMap();
    map->PlayerRelocation(&player, bucket.GetPositionX(), bucket.GetPositionY(), bucket.GetPositionZ(), bucket.GetOrientation());
    map->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, bucket.GetPositionX(), bucket.GetPositionY(), bucket.GetPositionZ());
    WorldPackets::Quest::QuestGiverChooseReward claim{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    claim.QuestGiverGUID = bucket.GetGUID();
    claim.QuestID = ShadowmoonQuest;
    claim.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(claim);
}

WorldSession* ShadowmoonSession()
{
    return new WorldSession(0, std::string("Native Shadowmoon candy"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void CheckShadowmoon(bool baseline)
{
    Map map(530, 792, 0, DIFFICULTY_NONE);
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    CandyBucket scryer;
    PublishBucket(scryer, map, ScryerBucket, 3938);
    Require(scryer.hasQuest(ShadowmoonQuest) && scryer.hasInvolvedQuest(ShadowmoonQuest), "The actual Scryer bucket lost its shared quest");
    LootPlayer aldorPlayer(ShadowmoonSession(), map, 1240901, scryer.GetPosition());
    SetStanding(aldorPlayer, 500, -3500);
    if (baseline)
    {
        auto missing = WorldDatabase.Query("SELECT COUNT(*) FROM gameobject WHERE id=190115");
        auto offer = sQuestDataStore->GetGOQuestRelationBounds(AldorBucket);
        Require(missing && missing->Fetch()[0].GetUInt32() == 0 && offer.first == offer.second, "Missing old Aldor placement or offer counterfactual");
        active.insert(HallowsEnd);
        Claim(aldorPlayer, scryer);
        Require(aldorPlayer.GetItemCount(Candy) == 1 && aldorPlayer.GetQuestRewardStatus(ShadowmoonQuest) &&
            !aldorPlayer.IsQuestSeasonalRewarded(ShadowmoonQuest), "Old reward path no longer admits an unfriendly player or stores a permanent reward");
        active.erase(HallowsEnd);
        std::cout << "EXPECTED_BROKEN Aldor bucket missing and the Scryer bucket ignored reputation\n";
        return;
    }

    CandyBucket aldor;
    PublishBucket(aldor, map, AldorBucket, 3754);
    Require(aldor.hasQuest(ShadowmoonQuest) && aldor.hasInvolvedQuest(ShadowmoonQuest), "The actual Aldor bucket did not offer the shared quest");
    Claim(aldorPlayer, aldor);
    Require(!aldorPlayer.GetItemCount(Candy) && aldorPlayer.GetQuestDialogStatus(&aldor) == QuestGiverStatus::None,
        "Inactive Hallow's End exposed the seasonal reward");
    active.insert(HallowsEnd);
    auto& schedule = const_cast<GameEventData&>(sGameEventMgr->GetEventMap()[HallowsEnd]);
    schedule.start = GameTime::GetGameTime() - 120;
    sWorld->ResetEventSeasonalQuests(HallowsEnd);
    auto occurrence = CharacterDatabase.Query("SELECT value FROM worldstates WHERE entry=20028");
    Require(occurrence && occurrence->Fetch()[0].GetUInt32() == uint32(schedule.start),
        "The real Hallow's End occurrence was not recorded");
    Claim(aldorPlayer, scryer);
    Require(!aldorPlayer.GetItemCount(Candy) && aldorPlayer.GetQuestDialogStatus(&scryer) == QuestGiverStatus::None,
        "Unfriendly Scryer standing exposed the reward");
    Claim(aldorPlayer, aldor);
    Require(aldorPlayer.GetItemCount(Candy) == 1 && aldorPlayer.IsQuestSeasonalRewarded(ShadowmoonQuest), "Eligible Aldor claim did not record the seasonal reward");

    LootPlayer scryerPlayer(ShadowmoonSession(), map, 1240902, aldor.GetPosition());
    SetStanding(scryerPlayer, -3500, 0);
    Claim(scryerPlayer, aldor);
    Require(!scryerPlayer.GetItemCount(Candy), "Unfriendly Aldor standing granted the reward");
    Claim(scryerPlayer, scryer);
    Require(scryerPlayer.GetItemCount(Candy) == 1 && scryerPlayer.IsQuestSeasonalRewarded(ShadowmoonQuest), "Neutral Scryer claim was not rewarded");

    LootPlayer neutral(ShadowmoonSession(), map, 1240903, aldor.GetPosition());
    SetStanding(neutral, 0, 0);
    Claim(neutral, aldor);
    Claim(neutral, aldor);
    Claim(neutral, scryer);
    Require(neutral.GetItemCount(Candy) == 1 && neutral.IsQuestSeasonalRewarded(ShadowmoonQuest),
        "Repeated or alternate bucket claim bypassed the shared seasonal record");
    auto storedReward = []() {
        return CharacterDatabase.Query("SELECT quest FROM character_queststatus_seasonal WHERE guid=1240903 AND quest=12409 AND event=12");
    };
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!storedReward() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Require(bool(storedReward()), "The actual selected reward did not reach seasonal persistence");
    sWorld->ResetEventSeasonalQuests(HallowsEnd);
    Require(bool(storedReward()), "Reapplying the same occurrence cleared the shared reward");
    schedule.start = GameTime::GetGameTime() - 10;
    sWorld->ResetEventSeasonalQuests(HallowsEnd);
    Require(!storedReward(), "A new occurrence kept the previous shared reward");
    neutral.ResetSeasonalQuestStatus(HallowsEnd);
    Require(!neutral.IsQuestSeasonalRewarded(ShadowmoonQuest) && neutral.GetQuestStatus(ShadowmoonQuest) == QUEST_STATUS_NONE,
        "The online reset kept the previous quest status");
    auto candyTemplate = sObjectMgr->GetItemTemplate(Candy);
    Require(candyTemplate, "The actual candy bag is required");
    auto previousCandy = neutral.GetItemCount(Candy);
    if (candyTemplate->GetMaxCount() == 1)
    {
        neutral.DestroyItemCount(Candy, 1, true);
        previousCandy = 0;
    }
    auto quest = sQuestDataStore->GetQuestTemplate(ShadowmoonQuest);
    Require(neutral.CanTakeQuest(quest, false), "The next occurrence still blocks the normal offer");
    Require(neutral.CanRewardQuest(quest, 0, false), "The next occurrence still blocks the native reward admission");
    Claim(neutral, scryer);
    Require(neutral.GetItemCount(Candy) == previousCandy + 1 && neutral.IsQuestSeasonalRewarded(ShadowmoonQuest),
        "The native next-season reset did not make the shared reward available again");

    LootPlayer historical(ShadowmoonSession(), map, 1240904, aldor.GetPosition());
    SetStanding(historical, 0, 0);
    historical.SetRewardedQuest(ShadowmoonQuest);
    Claim(historical, aldor);
    Require(!historical.GetItemCount(Candy), "A historical permanent reward was lost during the transition");
    active.erase(HallowsEnd);
    std::cout << "PASS both original Shadowmoon buckets, rank and holiday rejection, and one shared seasonal reward\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth" &&
            characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Private Shadowmoon fixture is required");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        bool baseline = sConfigMgr->GetBoolDefault("NativeShadowmoonCandy.Baseline", false);
        LoadShadowmoon(baseline);
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckShadowmoon(baseline);
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
