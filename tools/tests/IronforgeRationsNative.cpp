#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "PoolMgr.h"
#include <thread>

namespace
{
constexpr uint32 RationsQuest = 29351;
constexpr uint32 Rations = 69981;
constexpr uint32 Feed = 99479;
constexpr uint32 Fed = 99478;
constexpr uint32 Credit = 53564;
constexpr uint32 Guard = 5595;

void LoadRationsQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeIronforgeRations.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "The original Daryl daily is missing");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[RationsQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(RationsQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29351");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Credit && objective->Fetch()[5].GetUInt32() == 6, "Wrong actual cooking objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=29351");
    Require(addon && addon->Fetch()[9].GetUInt32() == 185 && addon->Fetch()[10].GetUInt32() == 1 && quest->SourceItemId == Rations,
        "Wrong original Cooking requirement or Rations");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    auto rotation = WorldDatabase.Query("SELECT p.pool_entry,t.max_limit FROM pool_quest p JOIN pool_template t ON t.entry=p.pool_entry WHERE p.entry=29351");
    Require(rotation && rotation->Fetch()[0].GetUInt32() == 5688 && rotation->Fetch()[1].GetUInt32() == 1, "Wrong original cooking daily pool");
    Require(sSpellMgr->GetSpellInfo(Feed)->GetEffect(EFFECT_0)->TriggerSpell == Fed &&
        sSpellMgr->GetSpellInfo(Feed)->GetEffect(EFFECT_1)->Effect == SPELL_EFFECT_KILL_CREDIT, "Original trigger or personal credit changed");
    for (uint32 text = 52137; text <= 52142; ++text)
        Require(sBroadcastTextStore.LookupEntry(text), "One localized thank-you line is missing");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=29351");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=29351");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    auto inactive = sQuestDataStore->GetCreatureQuestRelationBounds(5159);
    Require(inactive.first == inactive.second, "The daily was visible before selection");
    PoolGroup<Quest> daily;
    daily.SetPoolId(5688);
    PoolObject selection(RationsQuest, 0.0f);
    daily.AddEntry(selection, 1);
    ActivePoolData active;
    daily.SpawnObject(active, 1, 1);
    Require(active.IsActiveObject<Quest>(RationsQuest), "The native pool did not publish the selected cooking daily");
}

struct PlacedGuard : Actor
{
    PlacedGuard(Map& map, Field* row) : Actor(&map, Guard, row[0].GetUInt32(),
        {row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()})
    {
        m_DBTableGuid = row[0].GetUInt32();
        map.LoadGrid(GetPositionX(), GetPositionY());
        Require(map.AddToMap(static_cast<Creature*>(this)), "Could not publish an actual guard");
    }
};

void AcceptRations(LootPlayer& player, Actor& trainer)
{
    SupplyMove(player, trainer.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestID = RationsQuest;
    accept.QuestGiverGUID = trainer.GetGUID();
    player.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(player.GetQuestStatus(RationsQuest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Rations) == 1,
        "The actual Daryl did not provide the Rations on the selected day");
}

SpellCastResult UseRations(LootPlayer& player, Unit& target, bool advance = true)
{
    constexpr float halfTurn = 3.14159265359f;
    for (float offset : {0.0f, halfTurn / 2.0f, -halfTurn / 2.0f, halfTurn})
    {
        float const facing = target.GetOrientation() + offset;
        Position const approach{target.GetPositionX() - 1.5f * std::cos(facing), target.GetPositionY() - 1.5f * std::sin(facing),
            target.GetPositionZ(), Position::NormalizeOrientation(facing)};
        SupplyMove(player, approach);
        if (player.IsWithinLOSInMap(&target))
            break;
    }
    auto item = player.GetItemByEntry(Rations);
    Require(item, "Rations were not carried");
    auto spell = sSpellMgr->GetSpellInfo(Feed);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    GameTime::UpdateGameTimers();
    while (player.GetGlobalCooldownMgr().HasGlobalCooldown(spell) && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        GameTime::UpdateGameTimers();
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(spell), "Rations global cooldown exceeded three seconds");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    targets.SetUnitTarget(&target);
    auto result = player.CastSpell(targets, spell, nullptr, TRIGGERED_NONE, item);
    if (advance)
        Tick(player, 2500);
    return result;
}

void CheckRations()
{
    auto starter = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=5159 AND guid=82192 AND map=0 AND zoneId=1537");
    auto guards = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=5595 AND map=0 AND zoneId=1537 ORDER BY guid");
    auto wrongZone = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=5595 AND map=0 AND zoneId=1 ORDER BY guid LIMIT 1");
    Require(starter && guards && guards->GetRowCount() == 98 && wrongZone, "Matching trainer, 98 urban guards or outside guard missing");
    auto point = starter->Fetch();
    Position const origin{point[0].GetFloat(), point[1].GetFloat(), point[2].GetFloat(), point[3].GetFloat()};
    Map map(0, 585, 0, DIFFICULTY_NONE);
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Actor daryl(&map, 5159, 82192, origin);
    Require(map.AddToMap(static_cast<Creature*>(&daryl)), "Could not publish the actual Daryl");
    LootPlayer owner(SupplySession(), map, 2935101, origin), other(SupplySession(), map, 2935102, origin);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetSkill(185, 1, 15, 75);
    }
    AcceptRations(owner, daryl);
    auto wrong = UseRations(owner, daryl);
    auto progress = [&]() { return owner.GetQuestObjectiveData(RationsQuest, Credit); };
#ifdef IRONFORGE_RATIONS_BASELINE
    Require(wrong == SPELL_CAST_OK && progress() == 1, "The original library no longer advances on a non-guard");
    std::cout << "EXPECTED_BROKEN native Rations on Daryl advanced one objective, aura=" << daryl.HasAura(Fed, owner.GetGUID()) << '\n';
#else
    Require(wrong != SPELL_CAST_OK && !progress() && !daryl.HasAura(Fed), "The trainer was treated as a guard");
    {
        PlacedGuard outside(map, wrongZone->Fetch());
        Require(UseRations(owner, outside) != SPELL_CAST_OK && !progress() && !outside.HasAura(Fed), "A guard outside Ironforge was fed");
    }

    PlacedGuard first(map, guards->Fetch());
    AcceptRations(other, daryl);
    auto firstCast = UseRations(owner, first);
    std::cout << "First actual guard: cast=" << uint32(firstCast) << " area=" << first.GetAreaId() << " zone=" << first.GetZoneId()
        << " spawn=" << first.GetDBTableGUIDLow() << " alive=" << first.IsAlive() << " despawn=" << first.IsDespawn()
        << " aura=" << first.HasAura(Fed, owner.GetGUID()) << " any aura=" << first.HasAura(Fed) << " objective=" << progress() << '\n';
    Require(firstCast == SPELL_CAST_OK && progress() == 1 && first.HasAura(Fed, owner.GetGUID()),
        "The actual guard did not receive its native aura and one player credit");
    auto aura = first.GetAura(Fed, owner.GetGUID());
    Require(aura && aura->GetMaxDuration() == 120000, "The native Well Fed duration changed");
    Require(UseRations(owner, first) != SPELL_CAST_OK && UseRations(other, first) != SPELL_CAST_OK && progress() == 1 &&
        !other.GetQuestObjectiveData(RationsQuest, Credit), "Repeated feeding advanced a player while the guard was still fed");
    aura->SetDuration(1);
    first.Unit::Update(10);
    first.Unit::Update(10);
    Require(!first.HasAura(Fed) && UseRations(other, first) == SPELL_CAST_OK && first.HasAura(Fed, other.GetGUID()) &&
        other.GetQuestObjectiveData(RationsQuest, Credit) == 1 && progress() == 1, "Native aura expiry did not make the guard available again");

    Require(guards->NextRow(), "Missing guard for cancellation");
    {
        PlacedGuard canceled(map, guards->Fetch());
        SupplyMove(other, canceled.GetPosition());
        Require(other.CastSpell(&canceled, Fed, true) != SPELL_CAST_OK && !canceled.HasAura(Fed), "An unbound direct trigger fed a guard");
        Require(UseRations(other, canceled, false) == SPELL_CAST_OK, "Valid pre-cancellation cast did not start");
        AbandonSupply(other, RationsQuest);
        Tick(other, 2500);
        Require(!other.GetItemCount(Rations) && !canceled.HasAura(Fed), "An abandoned cast still fed the guard or retained the item");
        GiveSupply(other, Rations, 1);
        Require(UseRations(other, canceled) != SPELL_CAST_OK && !canceled.HasAura(Fed), "A leftover Rations item fed a guard without an active quest");
    }

    for (uint32 count = 2; count <= 6; ++count)
    {
        Require(guards->NextRow(), "Not enough actual guard placements");
        PlacedGuard guard(map, guards->Fetch());
        auto result = UseRations(owner, guard);
        if (result != SPELL_CAST_OK || !guard.HasAura(Fed, owner.GetGUID()) || progress() != count)
            std::cout << "Guard " << guard.GetDBTableGUIDLow() << ": cast=" << uint32(result) << " zone=" << guard.GetZoneId()
                << " aura=" << guard.HasAura(Fed, owner.GetGUID()) << " objective=" << progress() << '\n';
        Require(result == SPELL_CAST_OK && guard.HasAura(Fed, owner.GetGUID()) && progress() == count,
            "A real feeding did not give exactly one objective");
    }
    Require(owner.GetItemCount(Rations) == 1 && owner.GetQuestStatus(RationsQuest) == QUEST_STATUS_COMPLETE,
        "The reusable quest item or final objective changed");
    SupplyMove(owner, origin);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = daryl.GetGUID();
    reward.QuestID = RationsQuest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.FindQuestSlot(RationsQuest) == MAX_QUEST_LOG_SIZE && !owner.GetItemCount(Rations), "Daryl did not remove the Rations on reward");
    std::cout << "PASS 98 placed guards, six actual Well Fed applications, cross-player duplicate rejection, cancellation and real reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Rations checks require an isolated character schema");
        LoadRationsQuest();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRations();
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
