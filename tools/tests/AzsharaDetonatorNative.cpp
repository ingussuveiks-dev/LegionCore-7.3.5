#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "DB2Stores.h"
#include "CreatureTextMgr.h"
#include "GameObjectModel.h"
#include "GameObjectPackets.h"
#include "QuestPackets.h"
#include "Spell.h"

void AddSC_azshara();

namespace
{
constexpr uint32 DetonationQuest = 14477;
constexpr uint32 ArmingQuest = 14476;
constexpr uint32 Detonator = 196870;
constexpr uint64 DetonatorSpawn = 181401;
constexpr uint32 Box = 184856;
constexpr uint64 BoxSpawn = 181402;

void LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeAzsharaDetonator.inc"
    );
    Require(rows && rows->GetRowCount() == 3, "The original Azshara chain is required");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(DetonationQuest + 1);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14477");
    Require(addon && addon->Fetch()[4].GetUInt32() == ArmingQuest, "The actual arming prerequisite is required");
    Quest* quest = quests[DetonationQuest];
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14477");
    if (objectives)
    {
        Require(objectives->GetRowCount() == 1 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_GAMEOBJECT &&
            objectives->Fetch()[4].GetUInt32() == Detonator && objectives->Fetch()[5].GetInt32() == 1,
            "The persisted objective must require one actual detonator use");
        quest->LoadQuestObjective(objectives->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    }

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(250498,98559) OR id=36662");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid IN(181401,181402,34634)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template AS SELECT * FROM legion_world.gameobject_template WHERE entry IN(184856,196870,197344)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=14477");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=14477");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(36662,36785)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadGameobjects();
    if (objectives)
        sCreatureTextMgr->LoadCreatureTexts();
}

void Move(LootPlayer& player, Position const& position)
{
    Map* map = player.GetMap();
    map->LoadGrid(position.GetPositionX(), position.GetPositionY());
    player.RemoveFromGrid();
    player.Relocate(position);
    map->AddToGrid(static_cast<Player*>(&player), Cell(Trinity::ComputeCellCoord(player.GetPositionX(), player.GetPositionY())));
}

Creature& LoadCreature(Map& map, uint32 entry, uint64 spawn)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data && data->id == entry && data->mapid == 1, "The real Azshara creature spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto creature = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(1, entry, spawn));
    Require(creature && creature->IsAlive(), "The real Azshara creature did not publish");
    return *creature;
}

GameObject& LoadObject(Map& map, uint32 entry, uint64 spawn)
{
    auto data = sObjectMgr->GetGOData(spawn);
    Require(data && data->id == entry && data->mapid == 1, "The real Azshara object spawn is missing");
    map.LoadGrid(data->posX, data->posY);
    auto object = map.GetGameObject(ObjectGuid::Create<HighGuid::GameObject>(1, entry, spawn));
    Require(object && object->isSpawned(), "The real Azshara object did not publish");
    object->Update(10000);
    return *object;
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
    Map map(1, 300, 0, DIFFICULTY_NONE);
    auto& smooks = LoadCreature(map, 36785, 250498);
    auto session = new WorldSession(0, "Native Azshara detonator", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS,
        0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer player(session, map, 101, smooks.GetPosition());
    player.SetRace(RACE_ORC);
    player.setFactionForRace(player.getRace());
    player.SetLevel(20);
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = smooks.GetGUID();
    accept.QuestID = DetonationQuest;
    session->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_NONE, "Smooks bypassed Rigged to Blow");
    player.SetRewardedQuest(ArmingQuest);
    session->HandleQuestGiverAcceptQuest(accept);
    if (baseline)
    {
        Require(!sObjectMgr->GetGOData(DetonatorSpawn) && player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_COMPLETE,
            "The previous data must reproduce automatic completion with the detonator absent");
        std::cout << "EXPECTED_BROKEN Smooks completes the quest on acceptance while the original detonator is absent\n";
        return;
    }

    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_INCOMPLETE, "Acceptance bypassed real detonation");
    auto& button = LoadObject(map, Detonator, DetonatorSpawn);
    auto& box = LoadObject(map, Box, BoxSpawn);
    auto& gorek = LoadCreature(map, 36662, 3101072);
    Require(gorek.IsWithinDistInMap(&button, 5.0f) && sCreatureTextMgr->TextExist(36662, 0) &&
        sCreatureTextMgr->TextExist(36662, 1), "Gorek and the two Legion broadcast lines are missing");
    Require(button.GetGoType() == GAMEOBJECT_TYPE_BUTTON && button.GetGOInfo()->GetLockId() == 43 &&
        button.GetGOInfo()->button.conditionID1 == 11721, "The actual button and its client lock metadata must be retained");
    auto& kroum = LoadCreature(map, 36728, 98559);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = DetonationQuest;
    reward.QuestGiverGUID = kroum.GetGUID();
    auto quest = sQuestDataStore->GetQuestTemplate(DetonationQuest);
    reward.ItemChoiceID = quest->RewardChoiceItemId[0];
    if (!reward.ItemChoiceID)
        if (auto package = sDB2Manager.GetQuestPackageItems(quest->PackageID))
            for (auto item : *package)
                if (player.CanSelectQuestPackageItem(item))
                {
                    reward.ItemChoiceID = item->ItemID;
                    break;
                }
    if (!reward.ItemChoiceID)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            reward.ItemChoiceID = fallback->front()->ItemID;
    Move(player, kroum.GetPosition());
    session->HandleQuestGiverChooseReward(reward);
    Require(!player.IsQuestRewarded(DetonationQuest), "Kroum rewarded the quest before actual detonation");
    player.CompleteQuest(DetonationQuest);
    session->HandleQuestGiverChooseReward(reward);
    Require(!player.IsQuestRewarded(DetonationQuest), "An old COMPLETE without the GO receipt bypassed reward validation");
    Use(player, button);
    Require(!player.GetQuestObjectiveData(DetonationQuest, Detonator) && button.getLootState() == GO_READY,
        "Remote use altered the button or added a receipt");
    player.IncompleteQuest(DetonationQuest);
    Move(player, button.GetPosition());
    Use(player, box);
    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_INCOMPLETE, "The decoration replaced the original button");

    WorldPackets::Quest::QuestLogRemoveQuest abandon{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    abandon.Entry = player.FindQuestSlot(DetonationQuest);
    session->HandleQuestLogRemoveQuest(abandon);
    Use(player, button);
    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_NONE && button.getLootState() == GO_READY,
        "An abandoned quest consumed the original button");
    Move(player, smooks.GetPosition());
    session->HandleQuestGiverAcceptQuest(accept);
    Move(player, button.GetPosition());
    WorldPackets::GameObject::GameObjReportUse report{WorldPacket(CMSG_GAME_OBJ_REPORT_USE)};
    report.Guid = button.GetGUID();
    session->HandleGameobjectReportUse(report);
    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_INCOMPLETE, "Reporting use was treated as an interaction");
    player.CompleteQuest(DetonationQuest);
    Use(player, button);
    Require(player.GetQuestStatus(DetonationQuest) == QUEST_STATUS_COMPLETE && button.getLootState() == GO_ACTIVATED &&
        player.GetQuestObjectiveData(DetonationQuest, Detonator) == 1, "The real button did not recover and complete the older attempt");
    Use(player, button);
    player.m_Events.Update(5500);
    Require(player.GetQuestObjectiveData(DetonationQuest, Detonator) == 1, "Duplicate use exceeded the native counter");

    auto otherSession = new WorldSession(0, "Native Azshara second participant", {}, SEC_PLAYER, 6, 0, "Win",
        LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    LootPlayer other(otherSession, map, 102, smooks.GetPosition());
    other.SetRace(RACE_ORC);
    other.setFactionForRace(other.getRace());
    other.SetLevel(20);
    other.SetRewardedQuest(ArmingQuest);
    otherSession->HandleQuestGiverAcceptQuest(accept);
    Move(other, button.GetPosition());
    Use(other, button);
    Require(other.GetQuestStatus(DetonationQuest) == QUEST_STATUS_INCOMPLETE, "A busy button granted a second receipt");
    button.ResetDoorOrButton();
    button.Update(10000);
    auto lock = sLockStore.LookupEntry(button.GetGOInfo()->GetLockId());
    Require(lock, "The original button has no DB2 lock");
    auto lockMode = std::find(std::begin(lock->Index), std::end(lock->Index), LOCKTYPE_OPEN_KNEELING);
    Require(lockMode != std::end(lock->Index) && lock->Type[lockMode - std::begin(lock->Index)] == LOCK_KEY_SKILL,
        "The actual button must retain the native kneeling lock");
    Require(other.CastSpell(&button, 3365, false) != SPELL_CAST_OK, "An unrelated Opening cast matched the kneeling lock");
    Require(other.CastSpell(&button, 6478, false) == SPELL_CAST_OK, "The native Opening Kneeling cast did not start");
    Tick(other, 6000);
    Require(other.GetQuestStatus(DetonationQuest) == QUEST_STATUS_COMPLETE &&
        other.GetQuestObjectiveData(DetonationQuest, Detonator) == 1, "A second eligible player could not use the original lock after reset");

    Move(player, kroum.GetPosition());
    Require(reward.ItemChoiceID && player.CanInteractWithQuestGiver(&kroum) && player.CanRewardQuest(quest, reward.ItemChoiceID, false),
        "The fixture could not offer the actual package reward");
    auto rewards = player.GetItemCount(reward.ItemChoiceID);
    session->HandleQuestGiverChooseReward(reward);
    uint64 const money = player.GetMoney();
    session->HandleQuestGiverChooseReward(reward);
    Require(player.IsQuestRewarded(DetonationQuest) && player.GetMoney() == money &&
        player.GetItemCount(reward.ItemChoiceID) == rewards + 1, "Kroum must accept the actual reward choice exactly once");
    std::cout << "PASS prerequisite, button packets, stale COMPLETE recovery, two separate uses through native lock reset, one Kroum reward\n";
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
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_azshara();
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
        Run(sConfigMgr->GetBoolDefault("NativeAzsharaDetonator.ExpectBroken", false));
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
