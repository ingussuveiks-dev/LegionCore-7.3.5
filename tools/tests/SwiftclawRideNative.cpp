#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "ScriptsData.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"
#include <thread>

void AddSC_durotar();

namespace
{
constexpr uint32 QuestId = 24626;
constexpr uint32 Previous = 24623;
constexpr uint32 Lasso = 50053;
constexpr uint32 Rope = 70927;
constexpr uint32 Swiftclaw = 37989;
constexpr uint32 Returned = 38002;

Position SwiftclawPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct SwiftclawActor : Actor
{
    SwiftclawActor(Map& map, uint32 guid) : SwiftclawActor(map, Find(guid)) { }

    static QueryResult Find(uint32 guid)
    {
        auto rows = WorldDatabase.PQuery("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
            "WHERE map=1 AND zoneId=6453 AND guid=%u", guid);
        Require(bool(rows), "The actual Echo Isles spawn is absent");
        return rows;
    }

    SwiftclawActor(Map& map, QueryResult rows) : Actor(&map, rows->Fetch()[1].GetUInt32(), rows->Fetch()[0].GetUInt32(), SwiftclawPosition(rows->Fetch() + 2))
    {
        m_DBTableGuid = rows->Fetch()[0].GetUInt32();
        SetPhaseMask(rows->Fetch()[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish a real actor");
        if (GetVehicleKit())
            GetVehicleKit()->Install();
    }

    ~SwiftclawActor()
    {
        if (Vehicle* vehicle = GetVehicleKit())
            vehicle->RemoveAllPassengers();
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }

    void StartEncounterAI()
    {
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(AIM_Initialize(ai ? ai : new SmartAI(this)), "The native Echo Isles AI could not initialize");
    }

    void MoveTo(Position const& point)
    {
        StopMoving();
        GetMap()->CreatureRelocation(this, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
        GetMap()->MoveAllCreaturesInMoveList();
    }
};

void LoadRideData()
{
    auto rows = WorldDatabase.Query(
#include "NativeSwiftclawRide.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The original quest or predecessor is absent");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[QuestId];
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24626");
    Require(bool(addon), "The original quest prerequisites are missing");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24626 ORDER BY ID");
    Require(objectives && objectives->GetRowCount() == 2, "Original capture and return counters are missing");
    do
    {
        quest->LoadQuestObjective(objectives->Fetch());
    } while (objectives->NextRow());
    Require(quest->SourceItemId == Lasso && quest->PrevQuestID == Previous &&
        quest->GetObjectives()[0].ObjectID == Swiftclaw && quest->GetObjectives()[0].Amount == 1 &&
        quest->GetObjectives()[1].ObjectID == Returned && quest->GetObjectives()[1].Amount == 1, "Native quest metadata changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    auto item = sObjectMgr->GetItemTemplate(Lasso);
    auto rope = sSpellMgr->GetSpellInfo(Rope);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* row)
        { return row && row->SpellID == Rope && !row->Charges; }) && rope->GetEffect(EFFECT_0)->CalcRadius() == 15.0f &&
        rope->GetEffect(EFFECT_1)->MiscValue == 37995 && rope->GetEffect(EFFECT_1)->MiscValueB == 492 &&
        sSpellMgr->GetSpellInfo(70943)->GetEffect(EFFECT_0)->MiscValue == Swiftclaw &&
        sSpellMgr->GetSpellInfo(70941)->GetEffect(EFFECT_0)->MiscValue == Returned, "Original lasso, rope bunny or credit spells changed");
    auto pens = sAreaTriggerStore.LookupEntry(5675);
    Require(pens && pens->ContinentID == 1 && pens->Radius > 18.0f && pens->Radius < 19.0f, "The real Pens trigger is absent");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=24626");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24626");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(37989,38003)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=70927");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sScriptDataStore->LoadSpellScriptNames();
#ifndef SWIFTCLAW_RIDE_BASELINE
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_durotar();
    sScriptMgr->SwapScriptContext(true);
#endif
}

void SetupTroll(LootPlayer& player)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TROLL);
    player.setFactionForRace(RACE_TROLL);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetLevel(13);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    player.SetRewardedQuest(Previous);
}

void AcceptSwiftclaw(LootPlayer& player, Actor& kijara)
{
    SupplyMove(player, kijara.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = kijara.GetGUID();
    accept.QuestID = QuestId;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Lasso) == 1,
        "Kijara did not provide the original Lasso");
}

SpellCastResult LassoAfterCooldown(LootPlayer& player, Unit* target, uint32 advance)
{
    auto item = player.GetItemByEntry(Lasso);
    Require(item, "The actual Lasso is not carried");
    auto spell = sSpellMgr->GetSpellInfo(Rope);
    auto until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    GameTime::UpdateGameTimers();
    while (player.GetGlobalCooldownMgr().HasGlobalCooldown(spell) && std::chrono::steady_clock::now() < until)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        GameTime::UpdateGameTimers();
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(spell), "Lasso global cooldown exceeded three seconds");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    if (target)
        targets.SetUnitTarget(target);
    auto result = player.CastSpell(targets, spell, nullptr, TRIGGERED_NONE, item);
    Tick(player, advance);
    return result;
}

void AdvanceRide(LootPlayer& player, SwiftclawActor& raptor, uint32 elapsed)
{
    for (uint32 time = 0; time < elapsed; time += 10)
    {
        player.Unit::Update(10);
        raptor.AI()->UpdateAI(10);
    }
}

void CheckRide()
{
    Map map(1, 597, 0, DIFFICULTY_NONE);
    SwiftclawActor kijara(map, 80622), raptor(map, 80616), pen(map, 240751), moraya(map, 80851);
    raptor.StartEncounterAI();
    pen.StartEncounterAI();
    auto seat = raptor.GetVehicleKit();
    Require(seat && seat->GetVehicleInfo()->ID == 349 && seat->HasEmptySeat(0), "The original driver seat is unavailable");
    LootPlayer owner(SupplySession(), map, 2462601, kijara.GetPosition()), second(SupplySession(), map, 2462602, kijara.GetPosition());
    for (auto player : {&owner, &second})
        SetupTroll(*player);
    AcceptSwiftclaw(owner, kijara);
    Position const original = raptor.GetPosition();
    SupplyMove(owner, original);
#ifdef SWIFTCLAW_RIDE_BASELINE
    raptor.AI()->SpellHit(&owner, sSpellMgr->GetSpellInfo(Rope));
    Require(!owner.GetVehicle() &&
        owner.GetQuestObjectiveData(QuestId, Swiftclaw) == 1, "The original SmartAI no longer reproduces capture credit before a real passenger");
    Tick(owner, 100);
    if (owner.GetVehicle())
        owner.ExitVehicle();
    raptor.MoveTo(pen.GetPosition());
    SupplyMove(owner, pen.GetPosition());
    pen.AI()->UpdateAI(6000);
    Require(!owner.GetVehicle() && owner.GetQuestObjectiveData(QuestId, Returned) == 1,
        "The original controller no longer reproduces return progress without a rider");
    std::cout << "EXPECTED_BROKEN original SmartAI capture before boarding and pens credit without a passenger\n";
#else
    GiveSupply(second, Lasso, 1);
    SupplyMove(second, original);
    Require(LassoAfterCooldown(second, &raptor, 20) != SPELL_CAST_OK && !seat->GetPassenger(0), "A stale Lasso claimed the actual raptor");
    Require(LassoAfterCooldown(owner, &kijara, 0) != SPELL_CAST_OK && !owner.GetQuestObjectiveData(QuestId, Swiftclaw),
        "The Lasso accepted an unrelated target");
    Require(LassoAfterCooldown(owner, nullptr, 0) == SPELL_CAST_OK, "The original area cast failed to reserve a real target");
    AdvanceRide(owner, raptor, 100);
    Require(!owner.GetVehicle() && !owner.GetQuestObjectiveData(QuestId, Swiftclaw), "The delayed ride began before its native delay");
    AbandonSupply(owner, QuestId);
    AdvanceRide(owner, raptor, 250);
    Require(!owner.GetVehicle() && !owner.GetItemCount(Lasso) && !raptor.HasAura(46598, owner.GetGUID()),
        "Abandonment during the delayed cast still mounted its old player");
    AcceptSwiftclaw(owner, kijara);
    SupplyMove(owner, original);

    Require(LassoAfterCooldown(owner, nullptr, 0) == SPELL_CAST_OK && !owner.GetQuestObjectiveData(QuestId, Swiftclaw),
        "The native area cast failed or credited before boarding");
    uint32 const castAndBoard = sSpellMgr->GetSpellInfo(Rope)->CalcCastTime(&owner) + 350;
    AdvanceRide(owner, raptor, castAndBoard);
    std::cout << "CASE Swiftclaw elapsed=" << castAndBoard << " capture=" << owner.GetQuestObjectiveData(QuestId, Swiftclaw)
        << " native ride aura=" << raptor.HasAura(46598, owner.GetGUID()) << " driver=" << (seat->GetPassenger(0) == &owner)
        << " vehicle=" << (owner.GetVehicleBase() == &raptor) << " entry=" << raptor.GetEntry() << '\n';
    Require(seat->GetPassenger(0) == &owner && owner.GetVehicleBase() == &raptor && owner.GetQuestObjectiveData(QuestId, Swiftclaw) == 1,
        "The native boarding event did not acknowledge capture");
    Require(!owner.GetQuestObjectiveData(QuestId, Returned), "Capture also gave the pen objective");
    raptor.AI()->SetData(1, 1);
    Require(!owner.GetQuestObjectiveData(QuestId, Returned), "Data outside the original pen granted return");

    AbandonSupply(owner, QuestId);
    raptor.AI()->UpdateAI(100);
    Require(!owner.GetVehicle() && !owner.GetItemCount(Lasso), "Abandonment kept vehicle control or the Lasso");
    raptor.MoveTo(original);
    AcceptSwiftclaw(owner, kijara);
    SupplyMove(owner, original);
    Require(LassoAfterCooldown(owner, &raptor, 0) == SPELL_CAST_OK, "A new quest attempt could not cast the Lasso");
    AdvanceRide(owner, raptor, castAndBoard);
    Require(seat->GetPassenger(0) == &owner, "A new quest attempt could not retake the actual raptor");
    owner.ExitVehicle();
    raptor.MoveTo(pen.GetPosition());
    pen.AI()->UpdateAI(6000);
    Require(!owner.GetQuestObjectiveData(QuestId, Returned), "The pens credited a former rider without the real seat");
    raptor.MoveTo(original);
    SupplyMove(owner, original);
    Require(LassoAfterCooldown(owner, &raptor, 0) == SPELL_CAST_OK, "Dismount prevented a legitimate Lasso cast");
    AdvanceRide(owner, raptor, castAndBoard);
    Require(seat->GetPassenger(0) == &owner, "Dismount prevented a legitimate recapture");
    raptor.MoveTo(pen.GetPosition());
    Require(owner.IsInAreaTriggerRadius(5675), "The ridden raptor never reached the original pen");
    pen.AI()->UpdateAI(6000);
    Require(!owner.GetVehicle() && owner.GetQuestObjectiveData(QuestId, Returned) == 1 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE,
        "The native pen controller did not acknowledge and unload its actual rider");
    Require(!second.GetQuestObjectiveData(QuestId, Swiftclaw) && !second.GetQuestObjectiveData(QuestId, Returned), "The quest advanced on an uninvolved player");

    SupplyMove(owner, moraya.GetPosition());
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    uint32 choice = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *choices)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(choice && owner.CanRewardQuest(quest, choice, false), "Moraya's original class package was not available");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = moraya.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Lasso), "Moraya did not remove the Lasso at the actual reward");
    std::cout << "PASS actual Kijara, item, native seat, abandonment and recapture, placed pen controller and Moraya reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Swiftclaw checks require isolated characters");
        LoadRideData();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRide();
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
