#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "AlliedRaceCityBoard.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include "PhaseMgr.h"

void AddSC_go_scripts();

struct PhaseConditionsTag
{
    using Type = PhaseDefinitionConditionContainer ConditionMgr::*;
    friend Type Access(PhaseConditionsTag);
};
template struct FixtureAccess<PhaseConditionsTag, &ConditionMgr::PhaseDefinitionsConditionStore>;

struct CityEndersTag
{
    using Type = QuestRelations QuestDataStoreMgr::*;
    friend Type Access(CityEndersTag);
};
template struct FixtureAccess<CityEndersTag, &QuestDataStoreMgr::_creatureQuestInvolvedRelations>;

namespace
{
struct CityRoute
{
    uint32 quest, prerequisite, race, map, zone, boardSpawn, emissarySpawn, phase;
};

constexpr CityRoute Routes[] = {
    {50303, 49933, RACE_NIGHTBORNE, 1, 1637, 107487, 14556699, 4001},
    {50305, 49788, RACE_VOID_ELF, 0, 1519, 108162, 14556703, 4000},
    {50313, 49772, RACE_LIGHTFORGED_DRAENEI, 0, 1519, 108162, 14556705, 4001},
    {50319, 49773, RACE_HIGHMOUNTAIN_TAUREN, 1, 1637, 107487, 14556701, 4000}
};

void LoadRoutes()
{
    auto quests = WorldDatabase.Query(
#include "NativeAlliedCityQuest.inc"
    );
    Require(quests && quests->GetRowCount() == 8, "Missing a city quest or prerequisite");
    auto& vector = sQuestDataStore->*Access(QuestVectorTag{});
    vector.resize(50320);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(vector.size());
    do
    {
        auto quest = new Quest(quests->Fetch());
        vector[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    } while (quests->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN (50303,50305,50313,50319)");
    Require(addons && addons->GetRowCount() == 4, "Missing quest prerequisites");
    do
    {
        auto fields = addons->Fetch();
        Quest* quest = vector[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (50303,50305,50313,50319)");
    Require(objectives && objectives->GetRowCount() == 4, "Missing city discovery objectives");
    do
    {
        auto fields = objectives->Fetch();
        auto quest = vector[fields[1].GetUInt32()];
        quest->LoadQuestObjective(fields);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_KILL);
    } while (objectives->NextRow());

    auto receivers = WorldDatabase.Query("SELECT id,quest FROM creature_questender WHERE quest IN (50303,50305,50313,50319)");
    Require(receivers && receivers->GetRowCount() == 4, "Missing capital quest receiver relations");
    do
    {
        auto row = receivers->Fetch();
        (sQuestDataStore->*Access(CityEndersTag{})).emplace(row[0].GetUInt32(), row[1].GetUInt32());
    } while (receivers->NextRow());

    sObjectMgr->LoadPhaseDefinitions();
    auto conditions = WorldDatabase.Query(
        "SELECT SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1 "
        "FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup IN (1519,1637) AND SourceEntry IN (400,401)"
        " AND NegativeCondition=0");
    Require(conditions && conditions->GetRowCount() == 4, "Capital phase gates differ");
    auto& store = sConditionMgr->*Access(PhaseConditionsTag{});
    do
    {
        auto row = conditions->Fetch();
        auto condition = new Condition;
        condition->SourceType = CONDITION_SOURCE_TYPE_PHASE_DEFINITION;
        condition->SourceGroup = row[0].GetUInt32();
        condition->SourceEntry = row[1].GetUInt32();
        condition->ElseGroup = row[2].GetUInt32();
        condition->ConditionType = ConditionTypes(row[3].GetUInt32());
        condition->ConditionTarget = row[4].GetUInt8();
        condition->ConditionValue1 = row[5].GetUInt32();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition),
            "Native capital phase condition admission failed");
        store[condition->SourceGroup][condition->SourceEntry].push_back(condition);
    } while (conditions->NextRow());
}

struct Board : GameObject
{
    ~Board()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 entry, uint32 spawn, uint32 mask, Position const& pos)
    {
        m_DBTableGuid = spawn;
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        Require(Create(guid, entry, &map, mask, pos, G3D::Quat(0, 0, 0, 1), 100, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && GetDBTableGUIDLow() == spawn, "Missing capital board");
        Update(100);
    }
};

WorldSession* Session()
{
    return new WorldSession(0, std::string("Capital board probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void Configure(LootPlayer& player, CityRoute route)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, uint8(route.race));
    player.setFactionForRace(uint8(route.race));
    player.SetLevel(20);
    player.m_zoneId = route.zone;
    player.SetPhaseMask(1, false);
    player.GetPhaseMgr().Recalculate();
}

void CheckRoute(CityRoute route)
{
    auto boardRow = WorldDatabase.PQuery("SELECT id,phaseMask,position_x,position_y,position_z,orientation FROM gameobject WHERE map=%u AND guid=%u", route.map, route.boardSpawn);
    auto emissaryRow = WorldDatabase.PQuery("SELECT id,phaseMask,PhaseId,position_x,position_y,position_z,orientation FROM creature WHERE map=%u AND guid=%u", route.map, route.emissarySpawn);
    Require(boardRow && emissaryRow, "Missing city placement");
    auto board = boardRow->Fetch();
    auto emissary = emissaryRow->Fetch();
    Position boardPos{board[2].GetFloat(), board[3].GetFloat(), board[4].GetFloat(), board[5].GetFloat()};
    Position emissaryPos{emissary[3].GetFloat(), emissary[4].GetFloat(), emissary[5].GetFloat(), emissary[6].GetFloat()};
    uint32 boardEntry = board[0].GetUInt32();
    Map map(route.map, 300, 9999900 + route.race, DIFFICULTY_NONE);
    LootPlayer owner(Session(), map, 110, boardPos), other(Session(), map, 111, boardPos);
    Configure(owner, route);
    Configure(other, route);
    Actor receiver(&map, emissary[0].GetUInt32(), 200, emissaryPos);
    receiver.SetPhaseMask(emissary[1].GetUInt32(), false);
    receiver.SetPhaseId({uint32(std::stoul(emissary[2].GetString()))}, false);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Native phased emissary did not spawn");
    auto quest = sQuestDataStore->GetQuestTemplate(route.quest);
    Require(receiver.hasInvolvedQuest(route.quest), "The phased actor is not the registered turn-in");
    Require(quest->Objectives.size() == 1 && quest->MinLevel == 20 && !owner.CanTakeQuest(quest, false), "Missing race, level or prerequisite gate");
    auto& rewarded = const_cast<RewardedQuestSet&>(owner.getRewardedQuests());
    rewarded.insert(route.prerequisite);
    owner.SetLevel(19);
    Require(!owner.CanTakeQuest(quest, false), "Underlevel acceptance");
    Configure(owner, route);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
    Require(!owner.CanTakeQuest(quest, false), "Foreign-race acceptance");
    Configure(owner, route);
    Require(owner.CanTakeQuest(quest, false) && !owner.InSamePhase(&receiver), "Eligible player cannot accept in the ordinary capital phase");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    owner.GetPhaseMgr().Recalculate();

#ifdef ALLIED_CITY_BASELINE
    Require(quest->Objectives.front().Amount == 0 && owner.GetQuestStatus(route.quest) == QUEST_STATUS_COMPLETE &&
        owner.InSamePhase(&receiver), "Baseline did not reproduce skipped board discovery");
    std::cout << "EXPECTED_BROKEN quest=" << route.quest << " completed and showed the receiver before any board visit\n";
#else
    Require(quest->Objectives.front().Type == QUEST_OBJECTIVE_GAMEOBJECT && quest->Objectives.front().Amount == 1 &&
        uint32(quest->Objectives.front().ObjectID) == boardEntry && owner.GetQuestStatus(route.quest) == QUEST_STATUS_INCOMPLETE &&
        !owner.InSamePhase(&receiver) && !owner.CanRewardQuest(quest, false), "Quest completed or phased in the receiver without visiting the board");
    Board capitalBoard, copy;
    capitalBoard.Spawn(map, boardEntry, route.boardSpawn, board[1].GetUInt32(), boardPos);
    copy.Spawn(map, boardEntry, 777, board[1].GetUInt32(), boardPos);
    Require(capitalBoard.ActivateToQuest(&owner) && !capitalBoard.ActivateToQuest(&other), "The active board marker is missing or leaked");
    copy.Use(&owner);
    copy.AI()->GossipHello(&owner, false);
    capitalBoard.Use(&other);
    Require(!owner.GetQuestObjectiveData(route.quest, boardEntry) && !other.GetQuestObjectiveData(route.quest, boardEntry),
        "Credit from a copy or to a bystander");
    map.PlayerRelocation(&owner, boardPos.GetPositionX() + 32, boardPos.GetPositionY(), boardPos.GetPositionZ(), boardPos.GetOrientation());
    capitalBoard.Use(&owner);
    Require(!owner.GetQuestObjectiveData(route.quest, boardEntry), "Out-of-range board gave credit");
    map.PlayerRelocation(&owner, boardPos.GetPositionX(), boardPos.GetPositionY(), boardPos.GetPositionZ(), boardPos.GetOrientation());
    owner.CompleteQuest(route.quest);
    Require(!owner.CanRewardQuest(quest, false), "Saved zero-credit quest can be turned in");
    AlliedRaceCityBoard::RestoreIncompleteQuests(owner);
    Require(owner.GetQuestStatus(route.quest) == QUEST_STATUS_INCOMPLETE, "Saved zero-credit quest was not reopened");
    if (route.race == RACE_NIGHTBORNE || route.race == RACE_VOID_ELF)
        capitalBoard.Use(&owner);
    else
        Require(!capitalBoard.AI()->GossipHello(&owner, false), "Report-use consumed the ordinary board menu");
    capitalBoard.Use(&owner);
    AlliedRaceCityBoard::RestoreIncompleteQuests(owner);
    owner.GetPhaseMgr().Recalculate();
    Require(owner.HasPhaseId(route.phase) && owner.InSamePhase(&receiver) && !other.InSamePhase(&receiver) &&
        owner.GetQuestObjectiveData(route.quest, boardEntry) == 1 && owner.CanRewardQuest(quest, 0, false) &&
        !capitalBoard.ActivateToQuest(&owner), "The actual board failed to reveal the correct receiver exactly once");
    map.PlayerRelocation(&owner, emissaryPos.GetPositionX(), emissaryPos.GetPositionY(), emissaryPos.GetPositionZ(), emissaryPos.GetOrientation());
    map.PlayerRelocation(&other, emissaryPos.GetPositionX(), emissaryPos.GetPositionY(), emissaryPos.GetPositionZ(), emissaryPos.GetOrientation());
    Require(owner.CanInteractWithQuestGiver(&receiver) && !other.CanInteractWithQuestGiver(&receiver),
        "Native questgiver interaction denies the eligible player or admits the bystander");
    owner.RemoveActiveQuest(route.quest);
    owner.SetQuestSlot(0, 0);
    owner.GetPhaseMgr().Recalculate();
    Require(!owner.InSamePhase(&receiver) && !owner.CanRewardQuest(quest, false), "Abandon left receiver or reward active");
    map.PlayerRelocation(&owner, boardPos.GetPositionX(), boardPos.GetPositionY(), boardPos.GetPositionZ(), boardPos.GetOrientation());
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    capitalBoard.Use(&owner);
    Require(owner.CanRewardQuest(quest, false), "Reaccepted quest did not recover real board progress");
    owner.RemoveActiveQuest(route.quest);
    owner.SetQuestSlot(0, 0);
    rewarded.insert(route.quest);
    owner.GetPhaseMgr().Recalculate();
    Require(!owner.InSamePhase(&receiver) && !owner.CanTakeQuest(quest, false), "Rewarded quest still exposes the receiver or can be retaken");
    std::cout << "PASS quest=" << route.quest << " admission, actual board, persisted recovery, phases, repeat, abandon and reward eligibility\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        LoadRoutes();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifndef ALLIED_CITY_BASELINE
        AddSC_go_scripts();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(128);
        HashMapHolder<GameObject>::SetSize(128);
        GameTime::UpdateGameTimers();
        for (CityRoute route : Routes)
            CheckRoute(route);
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
