#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameEventMgr.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "LegionAnniversary.h"
#include "../../src/server/scripts/World/legion_anniversary_trivia.h"
#include "Mail.h"
#include "NPCPackets.h"
#include "QuestPackets.h"

void AddSC_legion_anniversary();
void AddSC_legion_anniversary_trivia();
namespace GameTime { extern time_t GameTime; }

namespace
{
struct Route
{
    uint16 Map;
    uint8 Race;
    uint32 Historian;
    uint32 Quest;
    uint32 Letter;
    uint32 LegacyQuest;
    uint32 LegacyLetter;
    float X, Y, Z;
};

constexpr Route Alliance{0,RACE_HUMAN,110034,47252,147880,43471,139409,-8284,252,155.43f};
constexpr Route Horde{1,RACE_ORC,110035,47251,147881,43472,139410,1626,-4139,49.0071f};

void CheckGeometry(Map& map, Route const& route)
{
    map.LoadGrid(route.X, route.Y);
    uint32 zone = 0;
    uint32 area = 0;
    map.GetZoneAndAreaId(zone, area, route.X, route.Y, route.Z);
    auto floor = map.GetHeight({}, route.X, route.Y, route.Z + 2.5f, true, 7.0f);
    Require(std::abs(floor - route.Z) < 4.0f, "No matching city floor for the historian");
    std::cout << "CASE historian=" << route.Historian << " zone=" << zone << " area=" << area << " floor=" << floor << '\n';
}

void LoadSeason()
{
    auto event = WorldDatabase.PQuery("SELECT holiday,world_event,length FROM game_event WHERE eventEntry=%u", LegionAnniversary::Event);
    Require(event && event->Fetch()[0].GetUInt32() == LegionAnniversary::Event && event->Fetch()[1].GetUInt8() == GAMEEVENT_NORMAL &&
        event->Fetch()[2].GetUInt32() == 20160, "The original two-week anniversary event is missing");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(LegionAnniversary::Event + 1);
    events[LegionAnniversary::Event].holiday_id = HolidayIds(LegionAnniversary::Event);

    for (int year : {2026, 2027, 2028})
    {
        std::tm october{};
        october.tm_year = year - 1900;
        october.tm_mon = 9;
        october.tm_mday = 31;
        october.tm_isdst = -1;
        auto season = LegionAnniversary::GetSeason(std::mktime(&october));
        GameTime::GameTime = season.Start - 1;
        Require(!sGameEventMgr->CheckOneGameEvent(LegionAnniversary::Event) && sGameEventMgr->NextCheck(LegionAnniversary::Event) == 1,
            "Anniversary opened before its calendar boundary");
        GameTime::GameTime = season.Start;
        Require(sGameEventMgr->CheckOneGameEvent(LegionAnniversary::Event) && season.End - season.Start == 336 * HOUR,
            "Anniversary opens November 16 for 336 hours");
        GameTime::GameTime = season.End;
        Require(!sGameEventMgr->CheckOneGameEvent(LegionAnniversary::Event) &&
            sGameEventMgr->NextCheck(LegionAnniversary::Event) == season.NextStart - season.End,
            "Anniversary failed to close or to find next year's opening");
    }
    GameTime::GameTime = LegionAnniversary::GetSeason(GameTime::GetGameTime()).Start + HOUR;
}

void LoadWorldData()
{
    auto rows = WorldDatabase.Query(
#include "NativeLegionAnniversary.inc"
    );
    Require(rows && rows->GetRowCount() == 6, "Anniversary quest templates missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(47253);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(43323,43461)");
    Require(objectives && objectives->GetRowCount() == 2, "The native trivia objectives are required");
    do
    {
        auto quest = quests[objectives->Fetch()[1].GetUInt32()];
        quest->LoadQuestObjective(objectives->Fetch());
        Require(quest->IsDaily() && quest->GetObjectives().front().ObjectID == LegionAnniversary::TriviaCredit &&
            quest->GetObjectives().front().Amount == 1, "Each native trivia daily requires exactly one answer");
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_REPEATABLE | QUEST_SPECIAL_FLAGS_KILL);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT c.* FROM legion_world.creature c JOIN game_event_creature e ON e.guid=c.guid AND e.eventEntry=693 WHERE c.id IN (110034,110035)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN (43323,43461,43471,43472,47251,47252)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (43323,43461)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID=29788 OR ID BETWEEN 116033 AND 116272");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=20052");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=20052");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    for (auto const& question : LegionAnniversary::TriviaQuestions)
    {
        Require(sObjectMgr->GetNpcText(question.Prompt) && sBroadcastTextStore.LookupEntry(question.Prompt) &&
            question.Correct < question.Answers.size(), "Native historian question did not load");
        for (auto answer : question.Answers)
        {
            auto row = sBroadcastTextStore.LookupEntry(answer);
            Require(row && *DB2Manager::GetBroadcastTextValue(row, LOCALE_enUS), "Native historian answer did not load");
        }
    }

    auto& lootTable = LootTemplates_Item.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry=147876";
    Require((LootTemplates_Item.*Access(LoadLootTableTag{}))() == 6, "Anniversary gift loot did not load");
    lootTable = original;
    auto conditions = WorldDatabase.Query("SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=5 AND SourceGroup=147876");
    Require(conditions && conditions->GetRowCount() == 10, "Gift requires native faction, quest and level gates");
    do
    {
        auto f = conditions->Fetch();
        auto condition = new Condition;
        condition->SourceType = ConditionSourceType(f[0].GetInt32());
        condition->SourceGroup = f[1].GetUInt32();
        condition->SourceEntry = f[2].GetUInt32();
        condition->SourceId = f[3].GetInt32();
        condition->ElseGroup = f[4].GetUInt32();
        condition->ConditionType = ConditionTypes(f[5].GetInt32());
        condition->ConditionTarget = f[6].GetUInt8();
        condition->ConditionValue1 = f[7].GetUInt32();
        condition->ConditionValue2 = f[8].GetUInt32();
        condition->ConditionValue3 = f[9].GetUInt32();
        condition->NegativeCondition = f[10].GetBool();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition) &&
            (sConditionMgr->*Access(AttachLootConditionTag{}))(condition, LootTemplates_Item.GetLootForConditionFill(condition->SourceGroup)),
            "Native loot rejected a gift condition");
    } while (conditions->NextRow());
}

void CheckTrivia(LootPlayer& player, Creature& historian, Route const& route)
{
    uint32 const quest = route.Race == RACE_HUMAN ? LegionAnniversary::AllianceTrivia : LegionAnniversary::HordeTrivia;
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(LegionAnniversary::Event);
    auto* session = player.GetSession();
    WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};
    hello.Unit = historian.GetGUID();
    auto& menu = player.PlayerTalkClass->GetGossipMenu();
    auto getAsk = [&]() -> int32
    {
        session->HandleGossipHelloOpcode(hello);
        for (auto const& entry : menu.GetMenuItems())
            if (entry.second.Sender == LegionAnniversary::Event)
                return entry.first;
        return -1;
    };
    auto select = [&](uint32 id, uint32 index)
    {
        WorldPackets::NPC::GossipSelectOption packet{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
        packet.GossipUnit = historian.GetGUID();
        packet.GossipID = id;
        packet.GossipIndex = index;
        session->HandleGossipSelectOption(packet);
    };
    auto openQuestion = [&]() -> LegionAnniversary::TriviaQuestion const&
    {
        auto ask = getAsk();
        Require(ask >= 0, "Active faction daily must offer the question");
        select(menu.GetMenuId(), ask);
        auto const& list = LegionAnniversary::TriviaQuestions;
        auto question = std::find_if(list.begin(), list.end(), [&](auto const& q) { return q.Prompt == menu.GetMenuId(); });
        Require(question != list.end() && menu.GetMenuItems().size() == 4, "The native gossip must publish one of 43 four-option questions");
        for (uint8 i = 0; i != 4; ++i)
            Require(menu.GetItem(i) && menu.GetItem(i)->Action == question->Answers[i], "The sent answer order changed");
        return *question;
    };

    Require(getAsk() < 0 && player.PlayerTalkClass->GetQuestMenu().HasItem(quest), "The historian should offer the daily before allowing an answer");
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = historian.GetGUID();
    accept.QuestID = quest;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "The native historian could not offer its daily");

    auto const& wrong = openQuestion();
    select(wrong.Prompt + 1, wrong.Correct);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "A stale question packet granted credit");
    select(wrong.Prompt, (wrong.Correct + 1) % 4);
    select(wrong.Prompt, wrong.Correct);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "A wrong answer or a replay granted credit");

    auto const& expired = openQuestion();
    active.erase(LegionAnniversary::Event);
    select(expired.Prompt, expired.Correct);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "A correct answer outside the season granted credit");
    active.insert(LegionAnniversary::Event);
    auto const otherRace = route.Race == RACE_HUMAN ? RACE_ORC : RACE_HUMAN;
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, otherRace);
    Require(getAsk() < 0, "The other faction could answer this historian's question");
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, route.Race);

    auto const& right = openQuestion();
    select(right.Prompt, right.Correct);
    select(right.Prompt, right.Correct);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE && player.GetQuestObjectiveData(quest, LegionAnniversary::TriviaCredit) == 1,
        "The real correct choice must issue exactly one native objective credit");
    uint32 badges = player.GetCurrency(1166);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = historian.GetGUID();
    reward.QuestID = quest;
    session->HandleQuestGiverChooseReward(reward);
    session->HandleQuestGiverChooseReward(reward);
    auto daily = sQuestDataStore->GetQuestTemplate(quest);
    Require(player.IsQuestDailyRewarded(quest) && player.GetCurrency(1166) == badges + 25 && !player.CanTakeQuest(daily, false),
        "The historian should pay exactly 25 badges and lock the daily until reset");
    player.DailyReset();
    Require(player.CanTakeQuest(daily, false), "The native daily reset did not renew admission");
    active.erase(LegionAnniversary::Event);
    std::cout << "PASS " << quest << " 43 localized-authoring rows, wrong/replayed/stale answer, faction, season, victory and daily reset\n";
}

void CheckGiftLoot(LootPlayer& player, Route const& route, bool currentLetter, bool legacyLetter)
{
    Loot loot;
    Require(loot.FillLoot(LegionAnniversary::Gift, LootTemplates_Item, &player, true), "Gift cannot open its 26972 loot");
    WorldPackets::Loot::LootResponse packet;
    loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
    std::set<uint32> items;
    for (auto const& entry : packet.Items)
        Require(entry.Quantity == 1 && items.insert(entry.Loot.ItemID).second, "Gift yielded an invalid item quantity");
    std::set<uint32> expected{147877};
    if (currentLetter)
        expected.insert(route.Letter);
    if (legacyLetter)
        expected.insert(route.LegacyLetter);
    Require(items == expected, "Gift delivered the wrong faction, level or previously accepted letter");
    Require(packet.Currencies.size() == 1 && packet.Currencies.front().CurrencyID == 1166 && packet.Currencies.front().Quantity == 200,
        "Gift must grant exactly 200 Timewarped Badges");
    Require(packet.Write()->size(), "Gift loot packet did not serialize");
}

void CheckRoute(Map& map, Route const& route)
{
    auto row = WorldDatabase.PQuery("SELECT guid FROM creature WHERE id=%u", route.Historian);
    Require(row && row->GetRowCount() == 1, "The seasonal historian spawn is missing");
    auto actor = std::make_unique<Creature>();
    map.LoadGrid(route.X, route.Y);
    Require(actor->LoadCreatureFromDB(row->Fetch()[0].GetUInt64(), &map, true), "The real historian could not load");
    Creature* historian = actor.release();
    Require(std::abs(historian->GetPositionZ() - map.GetHeight({}, route.X, route.Y, route.Z + 2.5f, true, 7.0f)) < 0.05f,
        "Historian must stand on the original 26972 city floor");

    auto session = new WorldSession(0, std::string("Native anniversary observer"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 9000000 + route.Quest + uint64(GetCurrentProcessId()) * 100000, historian->GetPosition());
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, route.Race);
    player.setFactionForRace(route.Race);
    player.SetLevel(20);
    player.SetInventorySlotCount(16);
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    auto guid = player.GetGUIDLow();
    auto giftMail = [guid]()
    {
        return CharacterDatabase.PQuery("SELECT m.id,m.sender,mi.item_guid FROM mail m JOIN mail_items mi ON mi.mail_id=m.id"
            " JOIN item_instance i ON i.guid=mi.item_guid AND i.itemEntry=147876 WHERE m.receiver=" UI64FMTD, guid);
    };
    Require(!giftMail(), "Private probe character already has event mail; use a fresh fixture character");
    sScriptMgr->OnPlayerLogin(&player);
    Require(!giftMail(), "The inactive event issued a gift");
    active.insert(LegionAnniversary::Event);
    auto season = LegionAnniversary::GetSeason(GameTime::GetGameTime());
    GameTime::GameTime = season.End;
    sScriptMgr->OnPlayerLogin(&player);
    Require(!giftMail(), "The event issued a gift past its calendar boundary");
    GameTime::GameTime = season.Start;
    sScriptMgr->OnPlayerLogin(&player);
    sScriptMgr->OnPlayerLogin(&player);
    auto received = giftMail();
    Require(received && received->GetRowCount() == 1 && received->Fetch()[1].GetUInt64() == route.Historian,
        "The native mail transaction must commit one gift from the correct historian");
    player._LoadMail();
    auto mail = player.GetMail(received->Fetch()[0].GetUInt32());
    auto item = player.GetMItem(received->Fetch()[2].GetUInt64());
    Require(mail && item && item->GetEntry() == LegionAnniversary::Gift, "Native mailbox did not load its serialized gift attachment");
    active.erase(LegionAnniversary::Event);

    CheckGiftLoot(player, route, true, true);
    player.SetLevel(59);
    CheckGiftLoot(player, route, true, true);
    player.SetLevel(60);
    CheckGiftLoot(player, route, true, false);
    player.SetLevel(20);

    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = historian->GetGUID();
    for (auto [quest, letter] : {std::make_pair(route.Quest, route.Letter), std::make_pair(route.LegacyQuest, route.LegacyLetter)})
    {
        ItemPosCountVec slots;
        Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, slots, letter, 1) == EQUIP_ERR_OK, "Gift letter did not fit in inventory");
        auto starter = player.StoreNewItem(slots, letter, true);
        std::cout << "CASE letter=" << letter << " slot=" << slots.front().pos << " quest="
            << (starter ? starter->GetTemplate()->GetStartQuestID() : 0) << " status=" << player.GetQuestStatus(quest) << '\n';
        Require(starter && starter->GetTemplate()->GetStartQuestID() == quest, "The 26972 letter must start the matching quest");
        accept.QuestGiverGUID = starter->GetGUID();
        accept.QuestID = quest;
        session->HandleQuestGiverAcceptQuest(accept);
        Require(player.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE, "Native starter letter did not accept and complete the objective-free quest");
        reward.QuestID = quest;
        session->HandleQuestGiverChooseReward(reward);
        session->HandleQuestGiverChooseReward(reward);
        Require(player.IsQuestRewarded(quest), "Native historian could not turn in its letter quest");
    }
    CheckGiftLoot(player, route, false, false);
    std::cout << "PASS " << route.Historian << " season, mailbox, item loot, letter offers and both historian rewards\n";
    CheckTrivia(player, *historian, route);
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
        GameTime::UpdateGameTimers();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}},{1,{}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}},{1,{}}});
        bool geometryOnly = sConfigMgr->GetBoolDefault("NativeAnniversary.Geometry", false);
        if (!geometryOnly)
        {
            auto config = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
            Require(config.substr(config.find_last_of(';') + 1) == "fixture_char", "Mail and rewards require private characters");
            CharacterDatabase.SetConnectionInfo(config, 1, 1);
            Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
            LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
            Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
            sObjectMgr->SetHighestGuids();
            HashMapHolder<Player>::SetSize(256);
            LoadSeason();
            LoadWorldData();
            sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
            AddSC_legion_anniversary();
            AddSC_legion_anniversary_trivia();
            sScriptMgr->SwapScriptContext(true);
        }
        for (Route const& route : {Alliance, Horde})
        {
            Map map(route.Map, 300, 9999995 + route.Map, DIFFICULTY_NONE);
            CheckGeometry(map, route);
            if (!geometryOnly)
                CheckRoute(map, route);
        }
        if (!geometryOnly)
        {
            CharacterDatabase.Close();
            LoginDatabase.Close();
        }
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
