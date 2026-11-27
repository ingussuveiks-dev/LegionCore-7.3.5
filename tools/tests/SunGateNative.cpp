#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObjectPackets.h"
#include "GameObjectModel.h"
#include "QuestPackets.h"

void AddSC_bloodmyst_isle();

namespace
{
constexpr uint32 MapId = 530;
constexpr uint32 SunGateQuest = 9740;
constexpr uint32 Prerequisite = 9746;
constexpr uint32 Aesom = 17844;
constexpr uint32 Gate = 182026;
constexpr uint32 Controller = 184850;
constexpr uint32 Stabilizer = 17886;

void LoadInputs()
{
    Require(!sSpellMgr->GetSpellInfo(38096), "The Legion loader must confirm that the old controller spell is absent");
    auto rows = WorldDatabase.Query(
#include "NativeSunGate.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Original Sun Gate quests are required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(Prerequisite + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9740");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Gate &&
        objective->Fetch()[5].GetUInt32() == 1, "The actual Sun Gate objective is required");
    auto quest = quests[SunGateQuest];
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=9740");
    Require(addon && addon->Fetch()[4].GetUInt32() == Prerequisite, "The actual Sun Gate prerequisite is required");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(17295,17359,17360,17361,17362)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid IN(13526,4415,4416,4417,4418)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template AS SELECT * FROM legion_world.gameobject_template WHERE entry IN(182026,184850)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=9740");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=9740");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadGameobjects();
}

void Move(LootPlayer& player, Position const& position)
{
    auto map = player.GetMap();
    map->LoadGrid(position.GetPositionX(), position.GetPositionY());
    player.RemoveFromGrid();
    player.Relocate(position);
    map->AddToGrid(static_cast<Player*>(&player), Cell(Trinity::ComputeCellCoord(player.GetPositionX(), player.GetPositionY())));
}

GameObject* LoadObject(Map& map, uint32 entry, uint64 spawn)
{
    auto data = sObjectMgr->GetGOData(spawn);
    Require(data && data->id == entry && data->mapid == MapId, "A real quest object spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto object = map.GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(MapId, entry, spawn));
    Require(object && object->IsInWorld() && object->isSpawned(), "The real quest object did not publish");
    object->Update(10000);
    return object;
}

void Use(LootPlayer& player, GameObject& object)
{
    WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE)};
    packet.Guid = object.GetGUID();
    player.GetSession()->HandleGameObjectUse(packet);
}

void Run(bool baseline)
{
    GameTime::UpdateGameTimers();
    Map map(MapId, 300, 0, DIFFICULTY_NONE);
    auto data = sObjectMgr->GetCreatureData(17295);
    Require(data && data->id == Aesom, "Aesom's original spawn data is missing");
    map.LoadGrid(data->posX, data->posY);
    auto aesom = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(MapId, Aesom, 17295));
    Require(aesom && aesom->IsAlive(), "Aesom's original spawn did not publish");
    auto session = new WorldSession(0, "Native Sun Gate", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS,
        0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 101, aesom->GetPosition());
    player.SetRace(RACE_DRAENEI);
    player.setFactionForRace(player.getRace());
    player.SetLevel(16);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = aesom->GetGUID();
    accept.QuestID = SunGateQuest;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_NONE, "The actual quest must still require Limits of Physical Exhaustion");
    player.SetRewardedQuest(Prerequisite);
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_INCOMPLETE, "Aesom could not offer the quest after its actual prerequisite");

    auto gate = LoadObject(map, Gate, 13526);
    std::array<GameObject*, 4> controllers{};
    for (size_t index = 0; index < controllers.size(); ++index)
        controllers[index] = LoadObject(map, Controller, 4415 + index);
    Move(player, gate->GetPosition());
    Use(player, *gate);
    if (baseline)
    {
        Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_COMPLETE &&
            std::all_of(controllers.begin(), controllers.end(), [](GameObject const* object)
            {
                return object->isSpawned() && object->getLootState() == GO_READY;
            }), "The previous server must reproduce completion with all four controllers still ready");
        std::cout << "EXPECTED_BROKEN The original Sun Gate completes while all four controllers remain active\n";
        return;
    }

    Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_INCOMPLETE, "The gate must refuse an early click");
    Require(gate->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE), "The gate must start locked for the client");
    Use(player, *controllers.front());
    Require(controllers.front()->getLootState() == GO_READY, "The first controller was usable from beyond its range");
    auto observerSession = new WorldSession(0, "Native Sun Gate observer", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS,
        0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer observer(observerSession, map, 102, controllers.front()->GetPosition());
    observer.SetRace(RACE_DRAENEI);
    observer.setFactionForRace(observer.getRace());
    observer.SetLevel(16);
    Use(observer, *controllers.front());
    Require(controllers.front()->getLootState() == GO_READY, "A player without the active quest used an actual controller");

    constexpr std::array<uint64, 4> stabilizerSpawns{17361, 17360, 17359, 17362};
    auto turnOff = [&](GameObject& controller)
    {
        Move(player, controller.GetPosition());
        Use(player, controller);
        Require(controller.getLootState() == GO_ACTIVATED, "The real controller use did not activate its goober");
        controller.Update(10000);
        controller.Update(10000);
        Require(!controller.isSpawned(), "The actual used controller did not despawn");
    };
    for (size_t index = 0; index < controllers.size(); ++index)
    {
        auto stabilizer = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(MapId, Stabilizer, stabilizerSpawns[index]));
        Require(stabilizer && stabilizer->IsAlive(), "The paired stabilizer must remain alive until its own controller is used");
        turnOff(*controllers[index]);
        Require(!stabilizer->IsAlive(), "The used controller did not stop its actual paired stabilizer");
        Use(player, *controllers[index]);
        Require(!controllers[index]->isSpawned(), "A duplicate use improperly changed a disabled controller");
        if (index + 1 < controllers.size())
        {
            Move(player, gate->GetPosition());
            Use(player, *gate);
            Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_INCOMPLETE, "The gate accepted only some of its four controllers");
        }
        if (index == 0)
        {
            WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
            abandon.Entry = player.FindQuestSlot(SunGateQuest);
            session->HandleQuestLogRemoveQuest(abandon);
            Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_NONE, "The actual quest attempt could not be abandoned");
            Move(player, controllers[1]->GetPosition());
            Use(player, *controllers[1]);
            Require(controllers[1]->getLootState() == GO_READY, "Abandoning retained authority to use another controller");
            Move(player, aesom->GetPosition());
            session->HandleQuestGiverAcceptQuest(accept);
            Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_INCOMPLETE && controllers[1]->isSpawned(),
                "The quest could not be taken again during the shared controller cycle");
        }
    }

    gate->Update(2000);
    Require(!gate->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE), "Four deactivated controllers did not unlock the gate");
    controllers.front()->Respawn();
    controllers.front()->Update(10000);
    gate->Update(2000);
    Require(controllers.front()->isSpawned() && gate->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE),
        "A respawned controller did not relock the gate");
    Move(player, gate->GetPosition());
    Use(player, *gate);
    Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_INCOMPLETE, "The gate credited after a controller respawned");
    turnOff(*controllers.front());
    gate->Update(2000);
    Move(observer, gate->GetPosition());
    Use(observer, *gate);
    Require(gate->getLootState() == GO_READY, "The nonparticipant consumed an unlocked gate");
    Move(player, gate->GetPosition());
    Use(player, *gate);
    Require(player.GetQuestStatus(SunGateQuest) == QUEST_STATUS_COMPLETE, "The gate use did not complete after the four controllers");
    Use(player, *gate);
    Require(player.GetQuestObjectiveData(SunGateQuest, Gate) == 1, "A second gate use exceeded the native objective count");
    Move(player, aesom->GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = aesom->GetGUID();
    reward.QuestID = SunGateQuest;
    session->HandleQuestGiverChooseReward(reward);
    uint64 afterReward = player.GetMoney();
    session->HandleQuestGiverChooseReward(reward);
    Require(player.IsQuestRewarded(SunGateQuest) && player.GetMoney() == afterReward, "Aesom must accept the actual quest turn-in once");
    std::cout << "PASS inactive and remote rejection, abandon/reaccept, four native controller uses, respawn relock and one Aesom reward\n";
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
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_bloodmyst_isle();
        sScriptMgr->SwapScriptContext(true);
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "The probe needs a private login pool");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "The probe needs private character data");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        HashMapHolder<Player>::SetSize(128);
        HashMapHolder<GameObject>::SetSize(128);
        LoadInputs();
        Run(sConfigMgr->GetBoolDefault("NativeSunGate.ExpectBroken", false));
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
