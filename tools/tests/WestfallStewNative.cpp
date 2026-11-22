#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "ConditionMgr.h"
#include "GilneasQuestNative.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"

void AddSC_westfall();

namespace
{
constexpr uint32 QuestId = 26271;
constexpr uint32 Prerequisite = 26266;
constexpr uint32 Stew = 57991;
constexpr uint32 Offer = 79450;
constexpr uint32 FullBelly = 79451;
constexpr uint32 Pot = 42617;
constexpr uint32 HopeSpawn = 107100;
constexpr uint32 ResidentSpawns[] = {106870, 106869, 106878, 106882};

struct WestfallPlacement : Creature
{
    WestfallPlacement(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "An original Sentinel Hill actor is absent");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "An actual Sentinel Hill actor did not enter the world");
    }

    ~WestfallPlacement()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void LoadWestfallStewInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeWestfallStew.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "The original Westfall quest chain or phase quest is missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());

    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(26271,26289)");
    Require(addon && addon->GetRowCount() == 2, "Missing actual Westfall prerequisite rows");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=26271");
    auto quest = quests[QuestId];
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Pot &&
        objective->Fetch()[5].GetUInt32() == 20 && quest->PrevQuestID == Prerequisite && quest->SourceItemId == Stew,
        "The real Westfall quest objective, previous quest or item changed");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto stew = sObjectMgr->GetItemTemplate(Stew);
    Require(stew && std::any_of(stew->Effects.begin(), stew->Effects.end(), [](ItemEffectEntry const* effect)
        { return effect && effect->SpellID == Offer && !effect->Charges; }) && sSpellMgr->GetSpellInfo(FullBelly)->GetDuration() == 120000,
        "Expected the native reusable stew item and two-minute Full Belly aura");
    std::cout << "CASE native Westfall pot duration=" << sSpellMgr->GetSpellInfo(Offer)->GetDuration()
        << " cast time=" << sSpellMgr->GetSpellInfo(Offer)->CalcCastTime() << std::endl;
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
#ifdef WESTFALL_STEW_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(107100,106870,106869,106878,106882)");
#else
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid=107100 OR "
        "(map=0 AND zoneId=40 AND areaId=108 AND MovementType=0 AND id IN(42383,42384,42386,42391,42400) AND PhaseId IN('','3417'))");
#endif
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_addon AS SELECT addon.* FROM legion_world.creature_addon addon JOIN creature spawn ON spawn.guid=addon.guid");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN(42383,42384,42386,42391,42400,42575,42617)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(26271,26289)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=26271");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_text AS SELECT * FROM legion_world.creature_text WHERE CreatureID IN(42383,42384,42386,42391,42400)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(42383,42384,42386,42391,42400)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(79450,79451)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId=40 AND entry IN(1,6,7)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=40 AND SourceEntry IN(6,7)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatureAddons();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sCreatureTextMgr->LoadCreatureTexts();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sObjectMgr->LoadPhaseDefinitions();
    sConditionMgr->LoadConditions();
#ifndef WESTFALL_STEW_BASELINE
    sScriptDataStore->LoadSpellScriptNames();
#endif
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_westfall();
    sScriptMgr->SwapScriptContext(true);
}

void AcceptStew(LootPlayer& owner, Creature& hope)
{
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    owner.setFactionForRace(RACE_HUMAN);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(12);
    owner.SetRewardedQuest(Prerequisite);
    owner.SetFloatValue(UNIT_FIELD_MOD_TIME_RATE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_CASTING_SPEED, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_SPELL_HASTE, 1.0f);
    owner.SetFloatValue(UNIT_FIELD_MOD_HASTE, 1.0f);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    MoveGilneasPlayer(owner, hope.GetPosition());
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    std::cout << "CASE Hope admission flags=" << hope.GetUInt32Value(UNIT_FIELD_NPC_FLAGS)
        << " relation=" << hope.hasQuest(QuestId) << " start=" << owner.CanTakeQuest(quest, false)
        << " previous=" << owner.GetQuestRewardStatus(Prerequisite) << " phase=" << owner.InSamePhase(&hope)
        << " los=" << owner.IsWithinLOSInMap(&hope) << " faction=" << owner.getFaction() << ',' << hope.getFaction() << std::endl;
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = QuestId;
    accept.QuestGiverGUID = hope.GetGUID();
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    std::cout << "CASE actual Hope result status=" << owner.GetQuestStatus(QuestId) << " item=" << owner.GetItemCount(Stew) << std::endl;
    Require(owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(Stew) == 1,
        "The actual Hope Saldean did not provide the quest and stew");
}

#ifdef WESTFALL_STEW_BASELINE
void CheckWestfallStew()
{
    sObjectMgr->RemoveCreatureFromGrid(HopeSpawn, sObjectMgr->GetCreatureData(HopeSpawn));
    for (uint32 spawn : ResidentSpawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 618, 0, DIFFICULTY_NONE);
    WestfallPlacement hope(map, HopeSpawn), citizen(map, ResidentSpawns[0]), transient(map, ResidentSpawns[1]),
        drifter(map, ResidentSpawns[2]), secondCitizen(map, ResidentSpawns[3]);
    LootPlayer owner(SupplySession(), map, 2627101, hope.GetPosition()), observer(SupplySession(), map, 2627102, hope.GetPosition());
    AcceptStew(owner, hope);
    Position point(-10334.5f, 973.5f, 32.2f, 0.0f);
    MoveGilneasPlayer(owner, point);
    MoveGilneasPlayer(observer, point);
    bool falseFullBelly = citizen.HasAura(FullBelly);
    auto result = owner.CastSpell(&owner, Offer, false, owner.GetItemByEntry(Stew));
    Tick(owner, 6000);
    auto pot = owner.FindNearestCreature(Pot, 10.0f, true);
    std::cout << "CASE original stew cast result=" << uint32(result) << " summon list=" << owner.GetSummonList(Pot)->size()
        << " area=" << owner.GetAreaId() << " phase=" << owner.HasPhaseId(3417) << std::endl;
    Require(result == SPELL_CAST_OK && pot, "The original item failed to deploy its actual stew pot");
    auto credit = owner.GetQuestObjectiveData(QuestId, Pot);
    std::cout << "CASE original stew pot visible=" << observer.canSeeOrDetect(pot) << " credit=" << credit
        << " native Full Belly=" << citizen.HasAura(FullBelly) << ',' << transient.HasAura(FullBelly) << ','
        << drifter.HasAura(FullBelly) << ',' << secondCitizen.HasAura(FullBelly) << std::endl;
    Require(observer.canSeeOrDetect(pot) && credit == 1 && falseFullBelly && !drifter.HasAura(FullBelly) &&
        !secondCitizen.HasAura(FullBelly), "Original public pot, ignored residents or falsely pre-applied Full Belly could not be reproduced");
    auto proof = pot->GetGUID();
    AbandonSupply(owner, QuestId);
    Require(!owner.GetItemCount(Stew) && map.GetCreature(proof), "Original abandonment no longer retained the active pot");
    std::cout << "EXPECTED_BROKEN public stew pot only credited one resident who already carried Full Belly and outlived the quest\n";
}
#else
SpellCastResult UseStew(LootPlayer& owner)
{
    auto result = owner.CastSpell(&owner, Offer, false, owner.GetItemByEntry(Stew));
    Tick(owner, 6000);
    return result;
}

void CheckWestfallStew()
{
    auto rows = WorldDatabase.Query("SELECT guid FROM creature WHERE id IN(42383,42384,42386,42391,42400) ORDER BY guid");
    Require(rows && rows->GetRowCount() >= 56, "The original Sentinel Hill residents are missing");
    std::vector<uint32> spawns;
    do
        spawns.push_back(rows->Fetch()[0].GetUInt32());
    while (rows->NextRow());
    sObjectMgr->RemoveCreatureFromGrid(HopeSpawn, sObjectMgr->GetCreatureData(HopeSpawn));
    for (uint32 spawn : spawns)
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 618, 0, DIFFICULTY_NONE);
    WestfallPlacement hope(map, HopeSpawn);
    std::vector<std::unique_ptr<WestfallPlacement>> residents;
    for (uint32 spawn : spawns)
        residents.push_back(std::make_unique<WestfallPlacement>(map, spawn));
    LootPlayer owner(SupplySession(), map, 2627101, hope.GetPosition()), observer(SupplySession(), map, 2627102, hope.GetPosition());
    AcceptStew(owner, hope);
    AcceptStew(observer, hope);
    Require(UseStew(owner) == SPELL_FAILED_BAD_TARGETS, "The stew could be placed at Hope without a nearby hungry resident");

    Position point(-10334.5f, 973.5f, 32.2f, 0.0f);
    MoveGilneasPlayer(owner, point);
    MoveGilneasPlayer(observer, point);
    Require(owner.HasPhaseId(3417) && observer.HasPhaseId(3417), "The fixture is not in the actual Sentinel Hill starting phase");
    Require(owner.CastSpell(&owner, Offer, false) == SPELL_FAILED_BAD_TARGETS, "The spell could put down a pot independently of its item");
    Require(observer.CastSpell(&observer, Offer, false, observer.GetItemByEntry(Stew)) == SPELL_CAST_OK,
        "The real stew cast could not start before the abandonment case");
    AbandonSupply(observer, QuestId);
    AcceptStew(observer, hope);
    MoveGilneasPlayer(observer, point);
    Tick(observer, 6000);
    Require(observer.GetSummonList(Pot)->empty() && !observer.GetQuestObjectiveData(QuestId, Pot),
        "A cast on the abandoned attempt carried over to its replacement quest");

    auto firstCast = UseStew(owner);
    auto firstCredit = owner.GetQuestObjectiveData(QuestId, Pot);
    auto pot = owner.FindNearestCreature(Pot, 10.0f, true);
    std::cout << "CASE first actual Westfall pot cast=" << uint32(firstCast) << " credit=" << firstCredit
        << " nearby pot=" << (pot != nullptr) << " area=" << owner.GetAreaId() << std::endl;
    Require(firstCast == SPELL_CAST_OK && pot && pot->HasQuestObjectScope() && owner.canSeeOrDetect(pot) && !observer.canSeeOrDetect(pot),
        "The native offering did not make an owner-only pot");
    auto firstPotGuid = pot->GetGUID();
    uint32 fed = 0;
    std::set<uint32> fedKinds;
    for (auto& resident : residents)
        if (auto aura = resident->GetAura(FullBelly, resident->GetGUID()))
        {
            ++fed;
            fedKinds.insert(resident->GetEntry());
            Require(aura->GetMaxDuration() == 120000, "Full Belly did not keep its native two-minute cooldown");
        }
    std::cout << "CASE initial Full Belly=" << fed << " distinct entries=" << fedKinds.size() << std::endl;
    Require(firstCredit == fed && fedKinds.count(42383) && fedKinds.count(42384) && fedKinds.count(42386) && fedKinds.count(42391),
        "Native Full Belly and credits did not reach the real residents in range");
    Require(!observer.GetQuestObjectiveData(QuestId, Pot), "The personal offering credited an unrelated player");

    auto retryResult = UseStew(observer);
    auto repeatCredit = observer.GetQuestObjectiveData(QuestId, Pot);
    std::cout << "CASE second nearby player cast=" << uint32(retryResult) << " credit=" << repeatCredit << std::endl;
    Require(!repeatCredit, "Another player could count already fed residents");
    AbandonSupply(observer, QuestId);
    Require(!observer.GetItemCount(Stew), "Abandonment retained the offered stew item");
    auto activePot = map.GetCreature(firstPotGuid);
    Require(activePot, "The native pot was removed before its lifetime elapsed");
    activePot->Update(30001);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(firstPotGuid), "The pot did not expire after its native lifetime");

    for (auto& resident : residents)
    {
        if (owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE)
            break;
        if (resident->HasAura(FullBelly) || !owner.InSamePhase(resident.get()))
            continue;
        MoveGilneasPlayer(owner, resident->GetNearPosition(2.0f, 0.0f));
        auto cooldown = owner.GetSpellCooldownDelay(Offer);
        if (cooldown > 0.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(uint32(cooldown * 1000) + 60));
        UseStew(owner);
    }
    Require(owner.GetQuestObjectiveData(QuestId, Pot) == 20 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE,
        "The native quest did not reach twenty real feedings");
    Require(owner.GetItemCount(Stew) == 1 && UseStew(owner) != SPELL_CAST_OK,
        "Reaching the objective consumed the stew early or left its quest action available");
    MoveGilneasPlayer(owner, hope.GetNearPosition(2.0f, 0.0f));
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = hope.GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    map.RemoveAllObjectsInRemoveList();
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Stew) && owner.GetSummonList(Pot)->empty() &&
        owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(26289), false), "Hope did not complete and clean up the native quest");
    std::cout << "PASS actual Westfall food, shared Full Belly, twenty live residents, personal pot, expiry and Hope reward\n";
}
#endif
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Westfall Stew checks require isolated characters");
        LoadWestfallStewInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckWestfallStew();
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
