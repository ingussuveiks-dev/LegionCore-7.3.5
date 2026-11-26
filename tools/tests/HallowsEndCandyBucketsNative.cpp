#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GarrisonMap.h"
#include "QuestPackets.h"

namespace
{
constexpr uint32 Candy = 37586;

void LoadCandyQuests()
{
    auto rows = WorldDatabase.Query(
#include "NativeHallowsEndCandyBuckets.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "Missing actual candy quests or the unchanged comparison bucket");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        Require(quest->IsAutoComplete() && !quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE) &&
            quest->RewardItemId[0] == Candy && quest->RewardItemCount[0] == 1, "The actual candy quest requires a bucket and awards one candy bag");
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();

    // Shadow the two relation tables on the private synchronous connection. Keep the
    // legacy bucket present so both native loaders also run in the broken baseline.
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_queststarter AS SELECT * "
        "FROM legion_world.gameobject_queststarter WHERE (id,quest) IN ((189303,12286),(243912,39657),(251670,43055))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_questender AS SELECT * "
        "FROM legion_world.gameobject_questender WHERE (id,quest) IN ((189303,12286),(243912,39657),(251670,43055))");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadGameobjectQuestRelations();
    sQuestDataStore->LoadGameobjectInvolvedRelations();
}

struct CandyBucket : GameObject
{
    ~CandyBucket()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void CheckBucket(Map& map, uint32 entry, uint32 questId, uint8 race, bool expectBroken)
{
    auto placement = WorldDatabase.PQuery("SELECT g.position_x,g.position_y,g.position_z,g.orientation,"
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3 FROM gameobject g "
        "JOIN game_event_gameobject e ON g.guid=e.guid WHERE g.id=%u AND g.map=%u AND e.eventEntry=12", entry, map.GetId());
    Require(placement && placement->GetRowCount() == 1, "Missing original Hallow's End placement");
    auto row = placement->Fetch();
    Position position{row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
    auto session = new WorldSession(0, std::string("Native candy bucket"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 100, position);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.setFactionForRace(race);

    CandyBucket bucket;
    Require(bucket.Create(200, entry, &map, 1, position, G3D::Quat(row[4].GetFloat(), row[5].GetFloat(),
        row[6].GetFloat(), row[7].GetFloat()), 255, GO_STATE_READY), "Could not create the original bucket");
    WorldPackets::Quest::QuestGiverChooseReward claim{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    claim.QuestGiverGUID = bucket.GetGUID();
    claim.QuestID = questId;
    claim.ItemChoiceID = 0;

    session->HandleQuestGiverChooseReward(claim);
    Require(!player.GetItemCount(Candy) && !player.GetQuestRewardStatus(questId), "An absent bucket granted candy");
    Require(map.AddToMap(static_cast<GameObject*>(&bucket)) && bucket.hasQuest(questId) &&
        player.CanInteractWithQuestGiver(&bucket), "The original bucket did not offer its quest");
    WorldPackets::Quest::QuestGiverQueryQuest query{WorldPacket(CMSG_QUEST_GIVER_QUERY_QUEST)};
    query.QuestGiverGUID = bucket.GetGUID();
    query.QuestID = questId;
    session->HandleQuestGiverQueryQuest(query);

    Require(bucket.hasInvolvedQuest(questId) != expectBroken, "Unexpected native bucket turn-in relation");
    session->HandleQuestGiverChooseReward(claim);
    Require(player.GetQuestRewardStatus(questId) == !expectBroken && player.GetItemCount(Candy) == uint32(!expectBroken),
        "The normal reward packet did not produce the expected candy and completion");
    session->HandleQuestGiverChooseReward(claim);
    Require(player.GetItemCount(Candy) == uint32(!expectBroken), "Replaying the same choice awarded extra candy");
    std::cout << (expectBroken ? "EXPECTED_BROKEN " : "PASS ") << questId << " map " << map.GetId()
        << ": original bucket offer and native reward packet\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1153, {}}, {1159, {}}, {1220, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1153, {}}, {1159, {}}, {1220, {}}});
        LoadCandyQuests();
        auto comparison = sQuestDataStore->GetGOQuestInvolvedRelationBounds(189303);
        Require(comparison.first != comparison.second && comparison.first->second == 12286, "Unchanged comparison bucket lost its reward relation");
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Native login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "The reward packet requires a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(),
            "The reward packet requires an empty character fixture");
        sObjectMgr->LoadPlayerInfo();
        GameTime::UpdateGameTimers();
        HashMapHolder<Player>::SetSize(512);
        HashMapHolder<GameObject>::SetSize(512);
        bool expectBroken = sConfigMgr->GetBoolDefault("NativeCandyBuckets.ExpectBroken", false);
        GarrisonMap alliance(1153, 300, 1153, nullptr, ObjectGuid::Create<HighGuid::Player>(100));
        GarrisonMap horde(1159, 300, 1159, nullptr, ObjectGuid::Create<HighGuid::Player>(100));
        Map legion(1220, 300, 0, DIFFICULTY_NONE);
        CheckBucket(alliance, 243912, 39657, RACE_HUMAN, expectBroken);
        CheckBucket(horde, 243912, 39657, RACE_ORC, expectBroken);
        CheckBucket(legion, 251670, 43055, RACE_HUMAN, expectBroken);
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
