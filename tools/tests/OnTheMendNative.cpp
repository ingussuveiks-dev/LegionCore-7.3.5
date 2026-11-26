#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GossipData.h"
#include "GossipDef.h"
#include "NPCPackets.h"
#include "PetBattle.h"
#include "QuestPackets.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 Revive = 133994;

struct Actor
{
    uint32 Entry;
    uint64 Spawn;
    uint32 Menu;
    uint32 Option;
};

struct Route
{
    uint32 Map;
    uint8 Race;
    Actor Master;
    Actor Trainer;
    uint32 Learning;
    uint32 Mend;
    uint32 Credit;
    uint32 LevelUp;
    uint32 GotOne = 0;
};

constexpr Route Routes[] =
{
    {  0, RACE_HUMAN,     {6749, 102181, 9821, 2}, {63014, 189102, 66420, 0}, 31308, 31309, 64320, 31785 },
    {  0, RACE_DWARF,     {9980,  83382, 9821, 2}, {63075, 262550, 66420, 0}, 31548, 31549, 65020, 31822, 31551 },
    {  1, RACE_NIGHTELF, {10051,  77531, 9821, 2}, {63070, 262560, 66420, 0}, 31552, 31553, 65041, 31826 },
    {530, RACE_DRAENEI,  {17485,  35780, 9821, 2}, {63077, 262556, 66420, 0}, 31556, 31568, 65179, 31825 },
    {  1, RACE_ORC,       {9987,  79601, 9821, 2}, {63061, 262554, 66420, 0}, 31571, 31572, 65180, 31830 },
    {  1, RACE_TAUREN,   {10050,  93438, 9821, 2}, {63067, 262552, 66420, 0}, 31573, 31574, 65184, 31831 },
    {  0, RACE_UNDEAD_PLAYER, {10055, 135434, 100550, 1}, {63073, 262548, 66420, 0}, 31576, 31577, 65193, 31823 },
    {530, RACE_BLOODELF, {16185,  10742, 9821, 2}, {63080, 262546, 66420, 0}, 31579, 31580, 65198, 31824 },
    {  1, RACE_NIGHTELF, {10085, 120778, 9821, 2}, {63083, 262558, 66420, 0}, 31582, 31583, 65199, 31832 },
    {  1, RACE_ORC,      {45789,14208043,12106,2}, {63086,14208069,66420, 0}, 31585, 31586, 65200, 31828, 31587 },
    {  1, RACE_ORC,      {47764, 265003,14991, 1}, {63626, 264193, 14991, 1}, 31588, 31589, 65212, 31827 },
    {  0, RACE_HUMAN,    {11069, 265750, 9821, 2}, {63596, 265722, 14991, 1}, 31591, 31592, 65214, 31821 }
};

void LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeOnTheMend.inc"
    );
    Require(rows && rows->GetRowCount() == 39, "The original battle pet tutorial quests are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(31833);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives "
        "WHERE QuestID IN(31308,31309,31548,31549,31551,31552,31553,31554,31556,31568,31571,31572,31573,31574,31576,31577,31579,31580,"
        "31582,31583,31585,31586,31587,31588,31589,31591,31592,31785,31821,31822,31823,31824,31825,31826,31827,31828,31830,31831,31832)");
    Require(bool(objectives), "The original tutorial objectives are missing");
    do
    {
        auto quest = quests[objectives->Fetch()[1].GetUInt32()];
        quest->LoadQuestObjective(objectives->Fetch());
    } while (objectives->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN"
        "(31308,31309,31548,31549,31551,31552,31553,31554,31556,31568,31571,31572,31573,31574,31576,31577,31579,31580,31582,31583,31585,"
        "31586,31587,31588,31589,31591,31592,31785,31821,31822,31823,31824,31825,31826,31827,31828,31830,31831,31832)");
    Require(addons && addons->GetRowCount() == 39, "The original tutorial prerequisites are missing");
    do
    {
        auto quest = quests[addons->Fetch()[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(addons->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
        // LoadQuests also resolves each persisted prerequisite after loading the addon fields.
        if (int32 previous = addons->Fetch()[4].GetInt32())
            quest->prevQuests.push_back(previous);
    } while (addons->NextRow());

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN"
        "(102181,83382,77531,35780,79601,93438,135434,10742,120778,14208043,265003,265750,189102,262550,262560,262556,262554,262552,"
        "262548,262546,262558,14208069,264193,265722)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE id IN(63014,63061,63067,63070,63073,63075,63077,63080,63083,63086,63596,63626) AND quest IN"
        "(31308,31309,31548,31549,31551,31552,31553,31556,31568,31571,31572,31573,31574,31576,31577,31579,31580,31582,31583,31585,31586,"
        "31587,31588,31589,31591,31592,31785,31821,31822,31823,31824,31825,31826,31827,31828,31830,31831,31832)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN(31309,31549,31551,31553,31554,31568,31572,31574,31577,31580,31583,31586,31587,31589,31592,31822,31828)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 "
        "AND entryorguid IN(6749,9980,9987,10050,10051,10055,10085,11069,16185,17485,45789,47764,63014,63061,63067,63070,63073,63075,"
        "63077,63080,63083,63086,63596,63626)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID IN(13584,16996,16997,20326)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID IN(9821,12106,14991,66420,100550)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID IN(9821,12106,14991,66420,100550)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    if (WorldDatabase.Query("SELECT id FROM smart_scripts LIMIT 1"))
        sSmartScriptMgr->LoadSmartAIFromDB();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadCreatures();
}

Creature* FindActor(Map& map, Actor const& expected)
{
    auto data = sObjectMgr->GetCreatureData(expected.Spawn);
    Require(data && data->id == expected.Entry && data->mapid == map.GetId(), "Original trainer spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(map.GetId(), expected.Entry, expected.Spawn));
    Require(actor && actor->IsAlive(), "Original trainer actor did not load");
    return actor;
}

void Visit(Map& map, Creature& actor, Creature& trainer, Route const& route, bool baseline, bool missingCaptureOffer)
{
    bool const stable = actor.GetEntry() == route.Master.Entry;
    Actor const& expected = stable ? route.Master : route.Trainer;
    std::cout << "CASE quest=" << route.Mend << " entry=" << expected.Entry << std::endl;
    auto session = new WorldSession(0, std::string("Native stable observer"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 100 + actor.GetEntry(), trainer.GetPosition());
    player.SetRace(route.Race);
    player.setFactionForRace(player.getRace());
    player.SetLevel(1);
    player.learnSpell(119467, false);

    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = trainer.GetGUID();
    accept.QuestID = route.Mend;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(route.Mend) == QUEST_STATUS_NONE, "The healing quest requires Learning the Ropes");
    accept.QuestID = route.Learning;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(route.Learning) != QUEST_STATUS_NONE, "The actual trainer must offer Learning the Ropes");
    // The first pet battle is outside this probe. Continue with its saved reward receipt.
    WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    abandon.Entry = player.FindQuestSlot(route.Learning);
    session->HandleQuestLogRemoveQuest(abandon);
    player.SetRewardedQuest(route.Learning);
    accept.QuestID = route.Mend;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(route.Mend) == QUEST_STATUS_INCOMPLETE, "The actual trainer must offer the healing quest after Learning the Ropes");
    auto approach = [&](Creature& receiver)
    {
        player.RemoveFromGrid();
        player.Relocate(receiver.GetPosition());
        map.AddToGrid(static_cast<Player*>(&player), Cell(Trinity::ComputeCellCoord(player.GetPositionX(), player.GetPositionY())));
    };
    approach(actor);

    auto pet = std::make_shared<BattlePet>();
    pet->JournalID = ObjectGuid::Create<HighGuid::BattlePet>(1);
    pet->Species = 39;
    pet->Breed = 3;
    pet->Quality = 3;
    pet->Level = 1;
    pet->Slot = 0;
    pet->UpdateStats();
    Require(pet->InfoMaxHealth > 1, "Native pet stats did not load");
    pet->Health = 1;
    player._battlePets.emplace(pet->JournalID, pet);
    player.UpdateBattlePetCombatTeam();

    uint32 menu = expected.Menu;
    uint32 option = expected.Option;
    auto open = [&]()
    {
        WorldPackets::NPC::Hello packet{WorldPacket(CMSG_TALK_TO_GOSSIP)};
        packet.Unit = actor.GetGUID();
        session->HandleGossipHelloOpcode(packet);
        auto const* entry = player.PlayerTalkClass->GetGossipMenu().GetItem(option);
        Require(entry && entry->BoxMoney == 1000, "The original paid pet service must be available");
    };
    auto select = [&](uint32 selectedMenu)
    {
        WorldPackets::NPC::GossipSelectOption packet{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
        packet.GossipUnit = actor.GetGUID();
        packet.GossipID = selectedMenu;
        packet.GossipIndex = option;
        session->HandleGossipSelectOption(packet);
        Tick(player, 800);
    };

    player.SetMoney(999);
    open();
    select(menu);
    if (baseline)
    {
        Require(pet->Health == pet->InfoMaxHealth && player.GetMoney() == 999 && player.GetQuestStatus(route.Mend) == QUEST_STATUS_COMPLETE,
            "The previous SmartAI must reproduce healing and completing the quest before validating payment");
        std::cout << "EXPECTED_BROKEN Murog healed and completed the quest with only 9 silver and 99 copper\n";
        return;
    }

    Require(pet->Health == 1 && player.GetMoney() == 999 && !player.GetQuestObjectiveData(route.Mend, route.Credit),
        "Insufficient funds must leave heal, money, and credit unchanged");
    player.SetMoney(2500);
    select(menu);
    Require(pet->Health == 1 && player.GetMoney() == 2500, "A closed gossip request must not be reusable after funds change");
    open();
    select(menu + 1);
    Require(pet->Health == 1 && player.GetMoney() == 2500 && !player.GetQuestObjectiveData(route.Mend, route.Credit),
        "A stale gossip packet must leave heal, money, and credit unchanged");

    player.AddAura(143999, &player);
    Require(player.HasAura(143999), "Native heal-blocking aura is required");
    select(menu);
    Require(pet->Health == 1 && player.GetMoney() == 2500 && !player.GetQuestObjectiveData(route.Mend, route.Credit),
        "A native failed heal must leave money and credit unchanged");
    player.RemoveAurasDueToSpell(143999);

    select(menu);
    std::cout << "CASE " << actor.GetEntry() << " level=" << unsigned(player.getLevel()) << " health=" << pet->Health
        << '/' << pet->InfoMaxHealth << " money=" << player.GetMoney() << " credit=" << player.GetQuestObjectiveData(route.Mend, route.Credit)
        << " status=" << player.GetQuestStatus(route.Mend) << '\n';
    Require(pet->Health == pet->InfoMaxHealth && player.GetMoney() == 1500 && player.GetQuestObjectiveData(route.Mend, route.Credit) == int32(stable),
        "Only the named stable master's actual paid heal must credit On The Mend");
    select(menu);
    Require(player.GetMoney() == 1500, "Duplicate gossip selection must not charge twice");
    if (stable)
    {
        Require(player.GetQuestStatus(route.Mend) == QUEST_STATUS_COMPLETE, "The stable credit must finish the actual quest");
        approach(trainer);
        WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
        reward.QuestGiverGUID = trainer.GetGUID();
        reward.QuestID = route.Mend;
        session->HandleQuestGiverChooseReward(reward);
        auto paid = player.GetMoney();
        session->HandleQuestGiverChooseReward(reward);
        Require(player.IsQuestRewarded(route.Mend) && player.GetMoney() == paid, "The trainer must accept one turn-in");
        accept.QuestID = route.LevelUp;
        session->HandleQuestGiverAcceptQuest(accept);
        Require(player.GetQuestStatus(route.LevelUp) == QUEST_STATUS_INCOMPLETE, "The next original tutorial quest must be available after reward");

        if (route.GotOne)
        {
            accept.QuestID = route.GotOne;
            session->HandleQuestGiverAcceptQuest(accept);
            Require(player.GetQuestStatus(route.GotOne) == QUEST_STATUS_NONE, "Got one! requires the Level Up! reward");
            // Use the native battle-earned quest credits to exercise the following quest packets.
            pet->Level = 3;
            player.KilledMonsterCredit(65876);
            reward.QuestID = route.LevelUp;
            session->HandleQuestGiverChooseReward(reward);
            Require(player.IsQuestRewarded(route.LevelUp), "The trainer must receive Level Up!");
            session->HandleQuestGiverAcceptQuest(accept);
            if (missingCaptureOffer)
            {
                Require(player.GetQuestStatus(route.GotOne) == QUEST_STATUS_NONE, "The missing starter must block Got one! after Level Up!");
                std::cout << "EXPECTED_BROKEN missing capture offer " << route.GotOne << " from " << route.Trainer.Entry << '\n';
                return;
            }
            Require(player.GetQuestStatus(route.GotOne) == QUEST_STATUS_INCOMPLETE, "The actual trainer must offer Got one! after Level Up!");
            reward.QuestID = route.GotOne;
            session->HandleQuestGiverChooseReward(reward);
            Require(!player.IsQuestRewarded(route.GotOne), "Got one! must require capture credit");
            player.KilledMonsterCredit(65356);
            session->HandleQuestGiverChooseReward(reward);
            uint64 afterReward = player.GetMoney();
            session->HandleQuestGiverChooseReward(reward);
            Require(player.IsQuestRewarded(route.GotOne) && player.GetMoney() == afterReward, "The trainer must accept Got one! once");
            std::cout << "PASS native acceptance and one reward for Got one! " << route.GotOne << '\n';
        }
    }
    std::cout << "PASS paid native pet healing, blocked attempts and replay for " << actor.GetEntry() << '\n';
}
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        auto spell = sSpellMgr->GetSpellInfo(Revive);
        Require(spell && spell->HasEffect(SPELL_EFFECT_HEAL_BATTLEPET_PCT), "Client spell requires its actual pet heal effect");
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}, {530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}, {530, {}}});
        GameTime::UpdateGameTimers();
        HashMapHolder<Player>::SetSize(256);
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "The probe needs private account data");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private account pool failed");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Rewards require private characters");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        LoadInputs();
        bool baseline = sConfigMgr->GetBoolDefault("NativeOnTheMend.ExpectBrokenPayment", false);
        bool missingCaptureOffer = sConfigMgr->GetBoolDefault("NativeOnTheMend.ExpectMissingCaptureOffer", false);
        for (Route const& route : Routes)
        {
            if (baseline && route.Mend != 31589)
                continue;
            if (missingCaptureOffer && !route.GotOne)
                continue;

            Map map(route.Map, 300, 0, DIFFICULTY_NONE);
            auto trainer = FindActor(map, route.Trainer);
            Visit(map, *FindActor(map, route.Master), *trainer, route, baseline, missingCaptureOffer);
            if (!baseline && !missingCaptureOffer)
                Visit(map, *trainer, *trainer, route, false, false);
        }
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
