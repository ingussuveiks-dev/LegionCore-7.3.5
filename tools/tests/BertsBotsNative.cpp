#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "BattlePetData.h"
#include "BattlePetPackets.h"
#include "PetBattle.h"
#include "PetBattleSystem.h"
#include "QuestPackets.h"
#include "ScriptedGossip.h"

void AddSC_npc_PetBattleTrainer();

struct ObjectivesByTypeTag
{
    using Type = std::map<QuestObjectiveType, std::vector<QuestObjective>> QuestDataStoreMgr::*;
    friend Type Access(ObjectivesByTypeTag);
};
template struct FixtureAccess<ObjectivesByTypeTag, &QuestDataStoreMgr::_questObjectiveByType>;

namespace
{
constexpr uint32 Bert = 124617;
constexpr uint32 BertsBots = 47895;
constexpr uint32 FightAction = GOSSIP_ACTION_INFO_DEF + 0xABCD;
constexpr uint8 RequestLeave = 0;
constexpr uint8 CastAbility = 1;

void CheckGeometry(Map& map)
{
    for (float angle : {0.0f, 0.785398f, 1.570796f, 2.356194f, 3.141593f, 3.926991f, 4.712389f, 5.497787f})
    {
        Position actor{-5145.0f, 824.0f, map.GetHeight({}, -5145.0f, 824.0f, 291.0f, true, 6)};
        auto atDistance = [&](float d)
        {
            float x = actor.GetPositionX() + std::cos(angle) * d;
            float y = actor.GetPositionY() + std::sin(angle) * d;
            return Position{x, y, map.GetHeight({}, x, y, actor.GetPositionZ() + 2.0f, true, 5)};
        };
        auto middle = atDistance(5.0f);
        auto player = atDistance(10.0f);
        uint32 zone = 0;
        uint32 area = 0;
        map.GetZoneAndAreaId(zone, area, actor.GetPositionX(), actor.GetPositionY(), actor.GetPositionZ());
        auto hit = map.getObjectHitPos({}, false, middle, player, 0.0f);
        std::cout << "CASE geometry " << angle << " zone " << zone << " area " << area << " floors "
            << actor.GetPositionZ() << ',' << middle.GetPositionZ() << ',' << player.GetPositionZ() << " hit " << hit << '\n';
    }
}

void LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeBertsBots.inc"
    );
    Require(bool(rows), "Bert's native quest template is required");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(BertsBots + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[BertsBots] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(BertsBots, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=47895");
    Require(objective && objective->GetRowCount() == 1, "Bert must have one native objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto const& defeat = quest->GetObjectives().front();
    Require(defeat.Type == QUEST_OBJECTIVE_PET_TRAINER_DEFEAT && defeat.ObjectID == Bert && defeat.Amount == 1,
        "The original objective requires one victory over Bert");
    (sQuestDataStore->*Access(ObjectivesByTypeTag{}))[QUEST_OBJECTIVE_PET_TRAINER_DEFEAT].push_back(defeat);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_REPEATABLE);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE id=124617");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=47895");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=47895");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sBattlePetDataStore->LoadBattlePetNpcTeamMember();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_npc_PetBattleTrainer();
    sScriptMgr->SwapScriptContext(true);
}

void CheckBattle(Map& map)
{
    LoadQuest();
    auto row = WorldDatabase.Query("SELECT guid FROM creature WHERE id=124617 AND map=0");
    Require(row && row->GetRowCount() == 1, "Bert's Train Depot spawn is required");
    auto spawn = row->Fetch()[0].GetUInt64();
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data && data->id == Bert, "The real Bert spawn did not load");
    map.LoadGrid(data->posX, data->posY);
    auto actor = std::make_unique<Creature>();
    Require(actor->LoadCreatureFromDB(spawn, &map, true), "Bert's original template could not be published");
    Creature& bert = *actor.release();

    auto session = new WorldSession(0, std::string("Native Bert observer"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 100, bert.GetPosition());
    player.setFactionForRace(player.getRace());
    player.learnSpell(119467, false);
    auto pet = std::make_shared<BattlePet>();
    pet->JournalID = ObjectGuid::Create<HighGuid::BattlePet>(1);
    pet->Species = 39;
    pet->Slot = 0;
    pet->Breed = 3;
    pet->Quality = 3;
    pet->Level = 1;
    pet->Health = 1000;
    player._battlePets.emplace(pet->JournalID, pet);
    player.UpdateBattlePetCombatTeam();
    Require(player.GetUnlockedPetBattleSlot() == 1, "The battle pet training spell must unlock one slot");

    sScriptMgr->OnGossipSelect(&player, &bert, GOSSIP_SENDER_MAIN, FightAction);
    Require(!player._petBattleId, "A player without the quest could open Bert's fight");
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = bert.GetGUID();
    accept.QuestID = BertsBots;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(BertsBots) == QUEST_STATUS_INCOMPLETE, "Bert did not offer or accept his quest");

    for (uint8 level : {1, 2, 4, 10, 25})
    {
        player.GetMotionMaster()->Clear();
        player.Relocate(bert.GetPosition());
        pet->Level = level;
        pet->UpdateAbilities();
        pet->Health = 1000;
        bert.AI()->sGossipHello(&player);
        auto option = player.PlayerTalkClass->GetGossipMenu().GetItem(0);
        Require(option && option->Action == FightAction, "Bert's real fight option is missing for an accepted player");
        sScriptMgr->OnGossipSelect(&player, &bert, GOSSIP_SENDER_MAIN, FightAction);
        auto battle = sPetBattleSystem->GetBattle(player._petBattleId);
        if (!battle)
        {
            PetBattleRequest request{};
            request.RequesterGuid = player.GetGUID();
            request.OpponentGuid = bert.GetGUID();
            request.TeamPosition[PETBATTLE_TEAM_1] = bert.GetPosition();
            request.TeamPosition[PETBATTLE_PVE_TEAM_ID] = bert.GetPosition();
            request.PetBattleCenterPosition = bert.GetPosition();
            request.RequestType = PETBATTLE_TYPE_PVE;
            std::cout << "CASE denied at level=" << unsigned(level) << " native admission="
                << unsigned(sPetBattleSystem->CanPlayerEnterInPetBattle(&player, &request)) << " team slot="
                << bool(player.GetBattlePetCombatTeam()[0]) << " trainer id=" << bert._petBattleId.ToString() << '\n';
        }
        Require(battle && battle->PveBattleType == PVE_PETBATTLE_TRAINER, "The real trainer battle did not open");
        Require(!player.GetQuestObjectiveData(BertsBots, Bert), "Opening the battle gave trainer credit");
        auto& team = *battle->Teams[PETBATTLE_PVE_TEAM_ID];
        Require(team.OwnerGuid == bert.GetGUID() && team.TeamPetCount == 3, "The real Bert battle must have three opponents");
        uint32 const expected[3][3] = {{282,1917,0}, {1919,282,0}, {1922,1921,1920}};
        for (uint8 i = 0; i != 3; ++i)
        {
            auto const& opponent = *team.TeamPets[i];
            Require(opponent.Species == 2068u - i && opponent.Level == level && opponent.Quality == 4 &&
                opponent.DisplayModelID && opponent.Breed == 3, "Bert's level, order, quality, breed or models are incorrect");
            for (uint8 ability = 0; ability != 3; ++ability)
            {
                uint8 threshold = ability == 0 ? 1 : ability == 1 ? 2 : 4;
                Require(opponent.Abilities[ability] == (level >= threshold ? expected[i][ability] : 0),
                    "Bert's native level-gated abilities changed");
            }
        }
        battle->Begin();
        WorldPackets::BattlePet::PetBattleInput input{WorldPacket(CMSG_PET_BATTLE_INPUT)};
        input.Round = battle->Turn - 1;
        input.MoveType = level == 2 ? RequestLeave : CastAbility;
        input.IgnoreAbandonPenalty = true;
        session->HandlePetBattleInput(input);
        if (level == 4)
            battle->Finish(PETBATTLE_TEAM_1, true, true); // Counterfactual interrupted victory.
        else
        {
            if (level == 10)
            {
                WorldPackets::Quest::QuestLogRemoveQuest remove{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
                remove.Entry = player.FindQuestSlot(BertsBots);
                session->HandleQuestLogRemoveQuest(remove);
                session->HandleQuestGiverAcceptQuest(accept);
                Require(player.GetQuestStatus(BertsBots) == QUEST_STATUS_INCOMPLETE, "Bert's quest could not be accepted again");
            }
            if (level != 2)
            {
                auto loser = level == 1 ? PETBATTLE_TEAM_1 : PETBATTLE_PVE_TEAM_ID;
                auto& losingTeam = *battle->Teams[loser];
                auto& winningTeam = *battle->Teams[!loser];
                for (auto const& member : losingTeam.TeamPets)
                    if (member)
                        battle->Kill(winningTeam.ActivePetID, member->ID, 0);
                battle->Update(1000);
            }
        }
        std::cout << "CASE level=" << unsigned(level) << " status=" << battle->BattleStatus << " winner=" << battle->WinnerTeamId
            << " credit=" << player.GetQuestObjectiveData(BertsBots, Bert) << '\n';
        Require(!player._petBattleId && player.GetQuestObjectiveData(BertsBots, Bert) == (level == 25),
            "Only the non-aborted player victory can grant trainer credit");
    }
    Require(player.GetQuestStatus(BertsBots) == QUEST_STATUS_COMPLETE, "The real trainer finish did not complete the objective");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = bert.GetGUID();
    reward.QuestID = BertsBots;
    session->HandleQuestGiverChooseReward(reward);
    session->HandleQuestGiverChooseReward(reward);
    Require(player.IsQuestRewarded(BertsBots) && player.GetItemCount(151638) == 1,
        "Bert's native reward must issue one Leprous Sack of Pet Supplies");
    std::cout << "PASS Bert's offer, levels 1/2/4/10/25, loss/forfeit/interruption/reaccept, victory and one daily reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        GameTime::UpdateGameTimers();
        {
            Map map(0, 300, 9999997, DIFFICULTY_NONE);
            map.LoadGrid(-5145.0f, 824.0f);
            if (sConfigMgr->GetBoolDefault("NativeBert.Geometry", false))
                CheckGeometry(map);
            else
            {
                auto connections = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
                Require(connections.substr(connections.find_last_of(';') + 1) == "fixture_char", "Rewards require private characters");
                CharacterDatabase.SetConnectionInfo(connections, 1, 1);
                Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
                LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
                Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
                HashMapHolder<Player>::SetSize(256);
                CheckBattle(map);
            }
        }
        if (!sConfigMgr->GetBoolDefault("NativeBert.Geometry", false))
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
