#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"
#include "PerfumedGuardAnalysis.h"
#include "SpellAuraEffects.h"
#include <thread>

void AddSC_guards();

struct SpellSummaryTag
{
    using Type = void (ScriptMgr::*)();
    friend Type Access(SpellSummaryTag);
};
template struct FixtureAccess<SpellSummaryTag, &ScriptMgr::FillSpellSummary>;

namespace
{
using namespace PerfumedGuardAnalysis;

struct CityRoute
{
    uint32 quest, previous, objective, creditEntry, map, zone, giver, guard;
    uint8 race;
};

constexpr CityRoute Routes[] = {
    {AllianceQuest, 24804, 265045, 26045, 0, 1519, 38066, 68, RACE_HUMAN},
    {HordeQuest, 24805, 253050, 37558, 1, 1637, 37172, 3296, RACE_ORC}
};

Position ReadCityPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct CityActor : Actor
{
    CityActor(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(), ReadCityPosition(row + 2))
    {
        m_DBTableGuid = row[0].GetUInt32();
        SetPhaseMask(row[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish an actual city actor");
    }

    ~CityActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }

    void StartGuardAI()
    {
        auto ai = sScriptMgr->GetCreatureAI(this);
        Require(ai && AIM_Initialize(ai), "The actual guard_generic AI could not start");
    }
};

struct AnalysisGroup : Group
{
    AnalysisGroup(LootPlayer& first, LootPlayer& second) : members{&first, &second}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(first.GetGUID().GetCounter(), 0);
        for (auto player : members)
        {
            Group::MemberSlot slot;
            slot.Guid = player->GetGUID();
            slot.Class = player->getClass();
            const_cast<Group::MemberSlotList&>(GetMemberSlots()).push_back(slot);
            player->SetGroup(this, 0);
        }
    }

    ~AnalysisGroup()
    {
        for (auto player : members)
            player->SetGroup(nullptr);
    }

    LootPlayer* const members[2];
};

void LoadPerfumeData()
{
    auto rows = WorldDatabase.Query(
#include "NativePerfumedGuardAnalysis.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "The two original seasonal quests or prerequisites are missing");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24536,24655)");
    Require(addon && addon->GetRowCount() == 2, "Seasonal quest prerequisites are missing");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    } while (addon->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(24536,24655)");
    Require(objectives && objectives->GetRowCount() == 2, "The two faction-specific counters are missing");
    do
    {
        auto row = objectives->Fetch();
        quests[row[1].GetUInt32()]->LoadQuestObjective(row);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    auto item = sObjectMgr->GetItemTemplate(Analyzer);
    auto analysis = sSpellMgr->GetSpellInfo(Analysis)->GetEffect(EFFECT_0);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* row)
        { return row && row->SpellID == Analysis && !row->Charges; }) && analysis->Effect == SPELL_EFFECT_SCRIPT_EFFECT &&
        analysis->CalcValue() == Perfumed && sSpellMgr->GetSpellInfo(Perfumed)->GetEffect(EFFECT_0)->CalcValue() == RecentlyAnalyzed,
        "Original analysis spell, item or related aura metadata changed");
    for (auto const& route : Routes)
    {
        auto quest = quests[route.quest];
        uint32 spell = route.quest == HordeQuest ? HordeCredit : AllianceCredit;
        auto credit = sSpellMgr->GetSpellInfo(spell)->GetEffect(EFFECT_0);
        Require(quest->SourceItemId == Analyzer && quest->PrevQuestID == route.previous &&
            quest->GetObjectives().front().ID == route.objective && quest->GetObjectives().front().Amount == 6 &&
            quest->GetObjectives().front().ObjectID == route.creditEntry && credit->Effect == SPELL_EFFECT_KILL_CREDIT2 &&
            credit->MiscValue == int32(route.creditEntry),
            "Original quest, prerequisite, counter or native credit changed");
    }

    auto event = WorldDatabase.Query("SELECT holiday,occurence,length FROM game_event WHERE eventEntry=8 AND world_event=0");
    Require(event && event->Fetch()[0].GetUInt32() == HOLIDAY_LOVE_IS_IN_THE_AIR, "Original seasonal event is missing");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(Event + 1);
    events[Event].holiday_id = HOLIDAY_LOVE_IS_IN_THE_AIR;
    events[Event].occurence = event->Fetch()[1].GetUInt32();
    events[Event].length = event->Fetch()[2].GetUInt32();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24536,24655)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(24536,24655)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_linked_spell AS SELECT * FROM legion_world.spell_linked_spell WHERE spell_trigger=70192");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    auto linked = WorldDatabase.Query("SELECT COUNT(*) FROM spell_linked_spell WHERE spell_trigger=70192 AND spell_effect=46418 AND type=8");
#ifdef PERFUMED_GUARD_ANALYSIS_BASELINE
    Require(linked && linked->Fetch()[0].GetUInt32() == 1, "Run the counterfactual against the original linked spell data");
#else
    Require(linked && !linked->Fetch()[0].GetUInt32(), "The old unconditional credit is still linked");
#endif
    sSpellMgr->LoadSpellLinked();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    (sScriptMgr->*Access(SpellSummaryTag{}))();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_guards();
    sScriptMgr->SwapScriptContext(true);
}

void SetupCityPlayer(LootPlayer& player, CityRoute const& route)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, route.race);
    player.setFactionForRace(route.race);
    player.GetReputationMgr().LoadFromDB({});
    player.InitDisplayIds();
    player.SetLevel(20);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    player.SetRewardedQuest(route.previous);
}

void AcceptCityQuest(LootPlayer& player, CityActor& giver, uint32 quest)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver.GetGUID();
    accept.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Analyzer) == 1,
        "The original investigator did not provide the Analyzer");
}

SpellCastResult UseAnalyzer(LootPlayer& player, CityActor& target)
{
    auto item = player.GetItemByEntry(Analyzer);
    Require(item, "Missing actually carried Analyzer");
    constexpr float HalfTurn = 3.14159265358979f;
    for (float offset : {0.0f, HalfTurn / 2.0f, -HalfTurn / 2.0f, HalfTurn})
    {
        float facing = target.GetOrientation() + offset;
        Position approach{target.GetPositionX() - 1.5f * std::cos(facing), target.GetPositionY() - 1.5f * std::sin(facing),
            target.GetPositionZ(), Position::NormalizeOrientation(facing)};
        SupplyMove(player, approach);
        if (player.IsWithinLOSInMap(&target))
            break;
    }
    auto spell = sSpellMgr->GetSpellInfo(Analysis);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    GameTime::UpdateGameTimers();
    while (player.GetGlobalCooldownMgr().HasGlobalCooldown(spell) && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        GameTime::UpdateGameTimers();
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(spell), "Analyzer global cooldown exceeded three seconds");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    targets.SetUnitTarget(&target);
    auto result = player.CastSpell(targets, spell, nullptr, TRIGGERED_NONE, item);
    Tick(player, 2000);
    return result;
}

void CheckCity(CityRoute const& route)
{
    auto giverRow = WorldDatabase.PQuery("SELECT c.guid,c.id,c.position_x,c.position_y,c.position_z,c.orientation,c.phaseMask FROM creature c "
        "JOIN game_event_creature e ON e.guid=c.guid WHERE c.id=%u AND c.map=%u AND c.zoneId=%u AND e.eventEntry=8", route.giver, route.map, route.zone);
    auto guardRows = WorldDatabase.PQuery("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
        "WHERE id=%u AND map=%u AND zoneId=%u AND phaseMask=1 ORDER BY guid", route.guard, route.map, route.zone);
    Require(giverRow && guardRows && guardRows->GetRowCount() >= 6, "The two investigators or enough actual capital guards are missing");
    Map map(route.map, 594 + route.map, 0, DIFFICULTY_NONE);
    CityActor giver(map, giverRow->Fetch());
    CityActor first(map, guardRows->Fetch());
    LootPlayer owner(SupplySession(), map, route.quest * 100 + 1, giver.GetPosition());
    LootPlayer observer(SupplySession(), map, route.quest * 100 + 2, giver.GetPosition());
    for (auto player : {&owner, &observer})
        SetupCityPlayer(*player, route);

    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(Event);
    first.StartGuardAI();
    Require(first.HasAura(Perfumed), "The live holiday guard did not receive native perfume");
    AcceptCityQuest(owner, giver, route.quest);
    auto progress = [&](LootPlayer& player) { return player.GetQuestObjectiveData(route.quest, route.creditEntry); };
#ifdef PERFUMED_GUARD_ANALYSIS_BASELINE
    auto result = UseAnalyzer(owner, first);
    Require(result == SPELL_CAST_OK && first.HasAura(Perfumed) && !first.HasAura(RecentlyAnalyzed), "Original analysis no longer reproduces its failed aura transition");
    if (route.quest == HordeQuest)
        Require(!progress(owner), "Original Horde analysis unexpectedly produced Horde credit");
    std::cout << "EXPECTED_BROKEN " << route.quest << " accepted an analysis without changing the guard aura, objective=" << progress(owner) << '\n';
#else
    Require(UseAnalyzer(owner, giver) != SPELL_CAST_OK && !progress(owner), "The investigator was treated as a treatable guard");
    GiveSupply(observer, Analyzer, 1);
    Require(UseAnalyzer(observer, first) != SPELL_CAST_OK && first.HasAura(Perfumed), "A stale Analyzer advanced an inactive player");
    Require(owner.CastSpell(&first, Analysis, true) != SPELL_CAST_OK && first.HasAura(Perfumed), "A direct spell without the item was admitted");

    auto firstUse = UseAnalyzer(owner, first);
    std::cout << "First analysis quest=" << route.quest << " cast=" << uint32(firstUse) << " count=" << progress(owner)
        << " perfumed=" << first.HasAura(Perfumed) << " recently analyzed=" << first.HasAura(RecentlyAnalyzed)
        << " team=" << owner.GetTeam() << " owner zone=" << owner.GetZoneId() << " guard zone=" << first.GetZoneId()
        << " friendly=" << first.IsFriendlyTo(&owner) << " phase=" << owner.InSamePhase(&first) << " map=" << owner.IsInMap(&first)
        << " los=" << owner.IsWithinLOSInMap(&first) << " active=" << ActiveQuest(&owner) << '\n';
    Require(firstUse == SPELL_CAST_OK && progress(owner) == 1 && !first.HasAura(Perfumed) && owner.GetItemCount(Analyzer) == 1,
        "The first real guard did not produce exactly one native credit");
    auto recently = first.GetAura(RecentlyAnalyzed);
    Require(recently && recently->GetMaxDuration() > 0, "Missing the native Recently Analyzed duration");
    Require(UseAnalyzer(owner, first) != SPELL_CAST_OK && progress(owner) == 1, "A recently analyzed guard produced repeat credit");
    first.AI()->Reset();
    Require(!first.HasAura(Perfumed) && first.HasAura(RecentlyAnalyzed), "Reset bypassed the Recently Analyzed aura");
    recently->SetDuration(1);
    first.Unit::Update(10);
    first.Unit::Update(10);
    first.AI()->UpdateAI(1000);
    Require(first.HasAura(Perfumed) && !first.HasAura(RecentlyAnalyzed), "Native aura expiry did not refresh the guard");

    AcceptCityQuest(observer, giver, route.quest);
    Require(UseAnalyzer(observer, first) == SPELL_CAST_OK && progress(observer) == 1, "A second player could not analyze after native expiry");
    AbandonSupply(observer, route.quest);
    Require(!observer.GetItemCount(Analyzer), "Abandoning left a quest Analyzer in the bag");
    active.erase(Event);
    first.AI()->UpdateAI(1000);
    Require(!first.HasAura(Perfumed) && !first.HasAura(RecentlyAnalyzed) && UseAnalyzer(owner, first) != SPELL_CAST_OK,
        "The ended holiday left or accepted an analysis aura");
    active.insert(Event);
    first.AI()->UpdateAI(1000);
    Require(first.HasAura(Perfumed), "The holiday starting while a guard was loaded did not apply perfume");

    AcceptCityQuest(observer, giver, route.quest);
    {
        Require(guardRows->NextRow(), "Missing the second original guard");
        CityActor second(map, guardRows->Fetch());
        second.StartGuardAI();
        SupplyMove(observer, second.GetPosition());
        AnalysisGroup group(owner, observer);
        Require(UseAnalyzer(owner, second) == SPELL_CAST_OK && progress(owner) == 2 && progress(observer) == 1,
            "The native cast did not advance both nearby eligible group members exactly once");
        Require(UseAnalyzer(observer, second) != SPELL_CAST_OK && progress(owner) == 2 && progress(observer) == 1,
            "The group received repeat credit from a recently analyzed guard");
    }
    AbandonSupply(observer, route.quest);

    {
        auto wrongZone = WorldDatabase.PQuery("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
            "WHERE id=%u AND map=%u AND zoneId<>%u AND phaseMask=1 ORDER BY guid LIMIT 1", route.guard, route.map, route.zone);
        Require(bool(wrongZone), "Missing the same guard template outside the correct city");
        CityActor outside(map, wrongZone->Fetch());
        outside.StartGuardAI();
        Require(!outside.HasAura(Perfumed) && UseAnalyzer(owner, outside) != SPELL_CAST_OK && progress(owner) == 2,
            "The same guard template outside the city accepted an analysis");
    }

    for (uint32 count = 3; count <= 6; ++count)
    {
        Require(guardRows->NextRow(), "The original guard list ended early");
        CityActor guard(map, guardRows->Fetch());
        guard.StartGuardAI();
        Require(UseAnalyzer(owner, guard) == SPELL_CAST_OK && progress(owner) == count && guard.HasAura(RecentlyAnalyzed),
            "An actual city guard did not provide its exact quest credit");
    }
    Require(owner.GetQuestStatus(route.quest) == QUEST_STATUS_COMPLETE && UseAnalyzer(owner, first) != SPELL_CAST_OK,
        "A completed quest still allowed the Analyzer");
    SupplyMove(owner, giver.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = route.quest;
    reward.QuestGiverGUID = giver.GetGUID();
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(route.quest) && !owner.GetItemCount(Analyzer), "The native reward did not remove the Analyzer");
    std::cout << "PASS " << route.quest << " six actual guards, native aura, holiday transition, abandonment and investigator reward\n";
#endif
    active.erase(Event);
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Air Analysis checks require isolated characters");
        LoadPerfumeData();
        HashMapHolder<Player>::SetSize(32);
        sWorld->setFloatConfig(CONFIG_GROUP_XP_DISTANCE, 100.0f);
        GameTime::UpdateGameTimers();
        for (auto const& route : Routes)
            CheckCity(route);
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
