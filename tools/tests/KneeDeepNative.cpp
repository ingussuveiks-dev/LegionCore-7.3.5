#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "MotionMaster.h"
#include "QuestItemObjectUse.h"
#include <thread>

namespace
{
using namespace KneeDeep;
constexpr uint32 BatQuest = 24920;
constexpr uint32 VengeanceQuest = 24903;
constexpr uint32 FollowUp = 24602;
constexpr uint32 VerminLayer = 2097152;

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct TunnelActor : Actor
{
    TunnelActor(Map& map, Field* row) : Actor(&map, row[1].GetUInt32(), row[0].GetUInt32(), ReadPosition(row + 2))
    {
        m_DBTableGuid = row[0].GetUInt32();
        SetPhaseMask(row[6].GetUInt32(), false);
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish an actual Gilneas actor");
    }
};

void LoadTunnelData()
{
    auto rows = WorldDatabase.Query(
#include "NativeKneeDeep.inc"
    );
    Require(rows && rows->GetRowCount() == 4, "The original Genn, bat, tunnel and next quest data are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new ::Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(24678,24602) ORDER BY ID");
    Require(addon && addon->GetRowCount() == 2, "Original tunnel quest prerequisites missing");
    do
    {
        auto row = addon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addon->NextRow());
    auto objective = WorldDatabase.Query("SELECT COUNT(*) FROM quest_objectives WHERE QuestID=24678");
    Require(objective && !objective->Fetch()[0].GetUInt32() && quests[KneeDeep::Quest]->PrevQuestID == BatQuest &&
        quests[KneeDeep::Quest]->SourceItemId == Torch && quests[FollowUp]->PrevQuestID == KneeDeep::Quest, "Wrong original quest chain or Torch");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    auto item = sObjectMgr->GetItemTemplate(Torch);
    auto effect = sSpellMgr->GetSpellInfo(KneeDeep::Swing)->GetEffect(EFFECT_0);
    Require(item && std::any_of(item->Effects.begin(), item->Effects.end(), [](ItemEffectEntry const* row)
        { return row && row->SpellID == KneeDeep::Swing && !row->Charges; }) && effect->Effect == SPELL_EFFECT_DUMMY &&
        effect->TargetB.GetTarget() == TARGET_UNIT_SRC_AREA_ENTRY && effect->CalcRadius() == 15.0f, "Original reusable 15-yard torch spell changed");
    auto counts = WorldDatabase.Query("SELECT COUNT(*) FROM creature WHERE map=654 AND phaseMask=2097152 AND areaId IN(4758,4790,4727) AND id IN(37889,37891,37892)");
    Require(counts && counts->Fetch()[0].GetUInt32() == 60, "The original sixty placed vermin are missing");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24678,24602)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=24678");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=69486");
#ifdef KNEE_DEEP_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE (zoneId=4714 AND entry IN(1,2)) OR (zoneId=4755 AND entry=1)");
#else
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId IN(4714,4755)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE (SourceTypeOrReferenceId=13 AND SourceEntry=70631) "
        "OR (SourceTypeOrReferenceId=23 AND ((SourceGroup=4755 AND SourceEntry=3) OR (SourceGroup=4714 AND SourceEntry IN(3,4))))");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadPhaseDefinitions();
    sSpellMgr->LoadSpellAreas();
#ifndef KNEE_DEEP_BASELINE
    sConditionMgr->LoadConditions();
#endif
}

void MoveAndRephase(LootPlayer& player, Position const& point)
{
    SupplyMove(player, point);
    player.GetMap()->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void AcceptKneeDeep(LootPlayer& player, Actor& genn)
{
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = genn.GetGUID();
    accept.QuestID = KneeDeep::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
}

SpellCastResult SwingTorch(LootPlayer& player)
{
    auto item = player.GetItemByEntry(Torch);
    Require(item, "The actual Torch was not carried");
    auto spell = sSpellMgr->GetSpellInfo(KneeDeep::Swing);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    GameTime::UpdateGameTimers();
    while (player.GetGlobalCooldownMgr().HasGlobalCooldown(spell) && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        GameTime::UpdateGameTimers();
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(spell), "Torch global cooldown exceeded three seconds");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    auto result = player.CastSpell(targets, spell, nullptr, TRIGGERED_NONE, item);
    Tick(player, 1500);
    return result;
}

void CheckKneeDeep()
{
    auto gennRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=38539 AND guid=148016 AND map=654");
    auto krennanRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=38144 AND guid=148251 AND map=654");
    auto ratRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE id=37889 AND guid=148195 AND map=654");
    Require(gennRow && krennanRow && ratRow, "The actual start, vermin and receiver were not found");
    Map map(654, 593, 0, DIFFICULTY_NONE);
    TunnelActor genn(map, gennRow->Fetch()), krennan(map, krennanRow->Fetch()), rat(map, ratRow->Fetch());
    LootPlayer owner(SupplySession(), map, 2467801, genn.GetPosition()), observer(SupplySession(), map, 2467802, genn.GetPosition());
    for (auto player : {&owner, &observer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        MoveAndRephase(*player, genn.GetPosition());
    }
    Require(!owner.InSamePhase(&genn) && !owner.InSamePhase(&rat) && !owner.InSamePhase(&krennan), "The tunnel actors appeared before the route started");
    owner.SetRewardedQuest(VengeanceQuest);
    owner.UpdateZoneDependentAuras(owner.GetZoneId());
    MoveAndRephase(owner, genn.GetPosition());
    Require(owner.HasAura(69486) && owner.HasPhaseId(188) && owner.InSamePhase(&genn) && !owner.InSamePhase(&rat) && !observer.InSamePhase(&genn),
        "The existing Vengeance or Survival aura did not reveal the actual King Genn before the bat quest");
    Require(!owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(KneeDeep::Quest), false), "The torch quest no longer requires the bat quest");
    owner.SetRewardedQuest(BatQuest);
    AcceptKneeDeep(owner, genn);
    Require(owner.GetItemCount(Torch) == 1 && owner.GetQuestStatus(KneeDeep::Quest) == QUEST_STATUS_COMPLETE, "Genn did not provide the Torch for the native ready-to-travel quest");
    MoveAndRephase(owner, rat.GetPosition());
#ifdef KNEE_DEEP_BASELINE
    Require(!owner.InSamePhase(&rat), "The original route no longer reproduces its missing vermin phase");
    owner.SetPhaseMask(VerminLayer, false);
    Require(SwingTorch(owner) == SPELL_CAST_OK && !rat.HasUnitState(UNIT_STATE_FLEEING), "The original torch no longer reproduces a cast without repelling the rat");
    std::cout << "EXPECTED_BROKEN the original Genn aura worked, but the vermin phase was missing and the Torch did not repel a placed rat\n";
#else
    auto first = SwingTorch(owner);
    std::cout << "Torch first cast=" << uint32(first) << " phase=" << owner.GetPhaseMask() << " zone=" << owner.GetZoneId()
        << " area=" << owner.GetAreaId() << " rat fleeing=" << rat.HasUnitState(UNIT_STATE_FLEEING) << '\n';
    Require(owner.HasPhaseId(190) && owner.InSamePhase(&rat) && !observer.InSamePhase(&rat) && first == SPELL_CAST_OK && rat.HasUnitState(UNIT_STATE_FLEEING) &&
        owner.GetItemCount(Torch) == 1, "The native Torch did not repel the real city-side rat");
    rat.GetMotionMaster()->UpdateMotion(3500);
    Require(!rat.HasUnitState(UNIT_STATE_FLEEING), "The native fear movement did not expire");

    for (uint32 entry : {37891u, 37892u})
    {
        auto rows = WorldDatabase.PQuery("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
            "WHERE id=%u AND map=654 AND zoneId=4714 AND areaId=4790 ORDER BY guid LIMIT 1", entry);
        Require(bool(rows), "The actual second-side vermin was not found");
        TunnelActor vermin(map, rows->Fetch());
        MoveAndRephase(owner, vermin.GetPosition());
        Require(owner.GetCurrentZoneID() == 4714 && owner.GetAreaId() == 4790 && (owner.GetPhaseMask() & VerminLayer) &&
            SwingTorch(owner) == SPELL_CAST_OK && vermin.HasUnitState(UNIT_STATE_FLEEING), "The true middle-tunnel vermin did not receive the Torch movement");
    }
    GiveSupply(observer, Torch, 1);
    MoveAndRephase(observer, rat.GetPosition());
    Require(SwingTorch(observer) != SPELL_CAST_OK && observer.GetItemCount(Torch) == 1, "An inactive stale Torch was accepted");

    MoveAndRephase(owner, genn.GetPosition());
    Require(SwingTorch(owner) == SPELL_CAST_OK && !genn.HasUnitState(UNIT_STATE_FLEEING), "The Torch affected a nearby unrelated actor");
    auto attempt = owner.GetQuestCancellationReceipt(KneeDeep::Quest);
    AbandonSupply(owner, KneeDeep::Quest);
    MoveAndRephase(owner, rat.GetPosition());
    Require(attempt && attempt->load() && !owner.GetItemCount(Torch) && !owner.InSamePhase(&rat), "Abandonment retained the Torch or the tunnel layer");
    MoveAndRephase(owner, genn.GetPosition());
    AcceptKneeDeep(owner, genn);
    Require(owner.GetQuestStatus(KneeDeep::Quest) == QUEST_STATUS_COMPLETE && owner.GetItemCount(Torch) == 1 &&
        owner.GetQuestCancellationReceipt(KneeDeep::Quest) != attempt, "Genn did not reopen a fresh accepted crossing");

    MoveAndRephase(owner, krennan.GetPosition());
    Require(owner.GetCurrentZoneID() == 4714 && owner.GetAreaId() == 4727 && owner.HasPhaseId(189) && owner.InSamePhase(&krennan), "The actual exit receiver did not phase in at Aderic's Repose");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = KneeDeep::Quest;
    reward.QuestGiverGUID = krennan.GetGUID();
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    MoveAndRephase(owner, krennan.GetPosition());
    Require(owner.GetQuestRewardStatus(KneeDeep::Quest) && !owner.GetItemCount(Torch) && !(owner.GetPhaseMask() & VerminLayer) &&
        !owner.HasPhaseId(190) && owner.HasPhaseId(189) && owner.InSamePhase(&krennan) && krennan.hasQuest(FollowUp),
        "Reward did not remove the Torch and tunnel layer or retain the real follow-up giver");
    std::cout << "PASS original 60 placements, actual city and exit phasing, three vermin types, timed clearance, unrelated NPC, abandonment, retry and Krennan reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{654, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{654, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Knee-Deep probes require isolated characters");
        LoadTunnelData();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckKneeDeep();
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
