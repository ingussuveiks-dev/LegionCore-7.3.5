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
constexpr uint32 OnTheMend = 31592;
constexpr uint32 Credit = 65214;
constexpr uint32 Revive = 133994;
constexpr uint32 Jenova = 11069;
constexpr uint32 Audrey = 63596;
constexpr uint32 LearningTheRopes = 31591;
constexpr uint32 LevelUp = 31821;

void LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeOnTheMend.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The original battle pet tutorial quests are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(LevelUp + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID IN(31592,31821)");
    Require(objectives && objectives->GetRowCount() == 2, "The original tutorial objectives are missing");
    do
    {
        auto quest = quests[objectives->Fetch()[1].GetUInt32()];
        quest->LoadQuestObjective(objectives->Fetch());
    } while (objectives->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(31592,31821)");
    Require(addons && addons->GetRowCount() == 2, "The original tutorial prerequisites are missing");
    do
    {
        auto quest = quests[addons->Fetch()[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(addons->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    } while (addons->NextRow());

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE id IN(11069,63596) AND map=0");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(31592,31821)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=31592");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid IN(11069,63596) AND source_type=0");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID IN(13584,20326)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID IN(9821,14991)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID IN(9821,14991)");
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

Creature* FindActor(Map& map, uint64 spawn, uint32 entry)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data && data->id == entry && data->mapid == 0, "Original trainer spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(0, entry, spawn));
    Require(actor && actor->IsAlive(), "Original trainer actor did not load");
    return actor;
}

void Visit(Map& map, Creature& actor, Creature& trainer, bool baseline)
{
    auto session = new WorldSession(0, std::string("Native stable observer"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 100 + actor.GetEntry(), trainer.GetPosition());
    player.setFactionForRace(player.getRace());
    player.SetLevel(1);
    player.learnSpell(119467, false);
    player.SetRewardedQuest(LearningTheRopes);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = trainer.GetGUID();
    accept.QuestID = OnTheMend;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(OnTheMend) == QUEST_STATUS_INCOMPLETE, "The actual trainer must offer the healing quest after Learning the Ropes");
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

    bool stable = actor.GetEntry() == Jenova;
    uint32 menu = stable ? 9821 : 14991;
    uint32 option = stable ? 2 : 1;
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
        Require(pet->Health == pet->InfoMaxHealth && player.GetMoney() == 999 && player.GetQuestStatus(OnTheMend) == QUEST_STATUS_COMPLETE,
            "The previous SmartAI must reproduce healing and completing the quest before validating payment");
        std::cout << "EXPECTED_BROKEN Jenova healed and completed the quest with only 9 silver and 99 copper\n";
        return;
    }

    Require(pet->Health == 1 && player.GetMoney() == 999 && !player.GetQuestObjectiveData(OnTheMend, Credit),
        "Insufficient funds must leave heal, money, and credit unchanged");
    player.SetMoney(2500);
    select(menu);
    Require(pet->Health == 1 && player.GetMoney() == 2500, "A closed gossip request must not be reusable after funds change");
    open();
    select(menu + 1);
    Require(pet->Health == 1 && player.GetMoney() == 2500 && !player.GetQuestObjectiveData(OnTheMend, Credit),
        "A stale gossip packet must leave heal, money, and credit unchanged");

    player.AddAura(143999, &player);
    Require(player.HasAura(143999), "Native heal-blocking aura is required");
    select(menu);
    Require(pet->Health == 1 && player.GetMoney() == 2500 && !player.GetQuestObjectiveData(OnTheMend, Credit),
        "A native failed heal must leave money and credit unchanged");
    player.RemoveAurasDueToSpell(143999);

    select(menu);
    std::cout << "CASE " << actor.GetEntry() << " level=" << unsigned(player.getLevel()) << " health=" << pet->Health
        << '/' << pet->InfoMaxHealth << " money=" << player.GetMoney() << " credit=" << player.GetQuestObjectiveData(OnTheMend, Credit)
        << " status=" << player.GetQuestStatus(OnTheMend) << '\n';
    Require(pet->Health == pet->InfoMaxHealth && player.GetMoney() == 1500 && player.GetQuestObjectiveData(OnTheMend, Credit) == int32(stable),
        "Only Jenova's actual paid heal must credit On The Mend");
    select(menu);
    Require(player.GetMoney() == 1500, "Duplicate gossip selection must not charge twice");
    if (stable)
    {
        Require(player.GetQuestStatus(OnTheMend) == QUEST_STATUS_COMPLETE, "The stable credit must finish the actual quest");
        approach(trainer);
        WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
        reward.QuestGiverGUID = trainer.GetGUID();
        reward.QuestID = OnTheMend;
        session->HandleQuestGiverChooseReward(reward);
        auto paid = player.GetMoney();
        session->HandleQuestGiverChooseReward(reward);
        Require(player.IsQuestRewarded(OnTheMend) && player.GetMoney() == paid, "Audrey must accept one turn-in");
        accept.QuestID = LevelUp;
        session->HandleQuestGiverAcceptQuest(accept);
        Require(player.GetQuestStatus(LevelUp) == QUEST_STATUS_INCOMPLETE, "The next original tutorial quest must be available after reward");
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
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
        {
            Map map(0, 300, 0, DIFFICULTY_NONE);
            auto trainer = FindActor(map, 265722, Audrey);
            Visit(map, *FindActor(map, 265750, Jenova), *trainer, baseline);
            if (!baseline)
                Visit(map, *trainer, *trainer, false);
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
