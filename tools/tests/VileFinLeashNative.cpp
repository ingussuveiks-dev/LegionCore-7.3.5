#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "SmartAI.h"
#include "SmartScriptMgr.h"
#include "TemporarySummon.h"

void AddSC_SmartScripts();

namespace
{
constexpr uint32 LeashQuest = 24974;
constexpr uint32 Leash = 52059;
constexpr uint32 Capture = 73108;
constexpr uint32 Puddlejumper = 1543;
constexpr uint32 Oracle = 1544;
constexpr uint32 CapturedPuddlejumper = 38923;
constexpr uint32 CapturedOracle = 39078;
constexpr uint32 Returned = 38887;
constexpr uint32 Calston = 38925;

void ShadowLeashRows(char const* name, char const* filter)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE vile_fin_%s AS SELECT * FROM %s WHERE %s",
        name, name, filter).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM vile_fin_%s", name, name).c_str());
}

Quest const* LoadLeashQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeVileFinLeash.inc"
    );
    Require(bool(row), "Missing Ever So Lonely");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[LeashQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(LeashQuest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=24974 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2, "Expected the two actual leash objectives");
    do
    {
        Field* fields = objectives->Fetch();
        Require(fields[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER && fields[5].GetUInt32() == 1,
            "Expected one real capture and return");
        quest->LoadQuestObjective(fields);
    } while (objectives->NextRow());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=24974");
    Require(bool(addon), "Missing Ever So Lonely quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Leash && quest->SourceItemIdCount == 1, "Expected one quest-provided Murloc Leash");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowLeashRows("smart_scripts", "source_type=0 AND entryorguid IN(1543,1544,38923,38925,39078)");
    ShadowLeashRows("creature_questender", "quest=24974");
    ShadowLeashRows("conditions", "(SourceTypeOrReferenceId=17 AND SourceEntry=73108) OR "
        "(SourceTypeOrReferenceId=22 AND SourceEntry IN(1543,1544,38923,38925,39078)) OR "
        "(SourceTypeOrReferenceId=17 AND SourceEntry=69228 AND ConditionTypeOrReference=2)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    return quest;
}

struct LeashActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(new SmartAI(this)), "The original murloc or receiver AI could not initialize");
    }
};

std::pair<uint32, Position> LeashPlacement(uint32 entry, uint32 area)
{
    auto rows = WorldDatabase.PQuery("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=%u AND map=0 AND zoneId=85 AND areaId=%u ORDER BY guid", entry, area);
    Require(bool(rows), "The prescribed NPC is missing from its original map or area");
    Field* fields = rows->Fetch();
    return {fields[0].GetUInt32(), {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()}};
}

SpellCastResult ThrowLeash(LootPlayer& player, Unit& target)
{
    auto item = player.GetItemByEntry(Leash);
    Require(item, "Expected the original carried leash");
    return player.CastSpell(&target, Capture, false, item);
}

TempSummon* Captive(LootPlayer& owner, uint32 entry)
{
    for (auto guid : *owner.GetSummonList(entry))
        if (Creature* creature = ObjectAccessor::GetCreature(owner, guid))
            if (!creature->IsDespawn())
                return creature->ToTempSummon();
    return nullptr;
}

void CheckLeash(Quest const* quest)
{
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=24974 AND id=38925")), "Missing Calston's quest offer");
    auto keeper = LeashPlacement(Calston, 4916);
    auto first = LeashPlacement(Puddlejumper, 169);
    auto second = LeashPlacement(Oracle, 169);
    auto wrong = LeashPlacement(1545, 168);
    Map map(0, 500, 0, DIFFICULTY_NONE);
    LeashActor calston(&map, Calston, keeper.first, keeper.second);
    LeashActor puddlejumper(&map, Puddlejumper, first.first, first.second);
    LeashActor oracle(&map, Oracle, second.first, second.second);
    LeashActor outsider(&map, 1545, wrong.first, wrong.second);
    for (auto actor : {&calston, &puddlejumper, &oracle, &outsider})
        Require(map.AddToMap(static_cast<Creature*>(actor)), "The actual placed NPC could not enter the native map");
    LootPlayer player(SupplySession(), map, 24974, first.second), visitor(SupplySession(), map, 24975, second.second);
    for (auto owner : {&player, &visitor})
    {
        owner->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        owner->setFactionForRace(RACE_UNDEAD_PLAYER);
        owner->InitDisplayIds();
        owner->SetLevel(9);
        owner->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        owner->GetZoneAndAreaId(owner->m_zoneId, owner->m_areaId);
        Require(owner->GetCurrentZoneID() == 85 && owner->GetCurrentAreaID() == 169,
            "The original spawn must resolve to Whispering Shore in Tirisfal");
    }
    for (auto target : {&puddlejumper, &oracle, &outsider})
    {
        target->SetMaxHealth(120);
        target->SetHealth(120);
    }

    GiveSupply(player, Leash, 1);
#ifdef VILE_FIN_LEASH_BASELINE
    Require(ThrowLeash(player, puddlejumper) == SPELL_CAST_OK, "The old implementation did not expose the full-health, inactive cast");
    Tick(player, 1500);
    std::cout << "CASE old inactive cast aura=" << puddlejumper.HasAura(Capture, player.GetGUID())
        << " follower=" << bool(Captive(player, CapturedPuddlejumper)) << std::endl;
    visitor.AddQuestAndCheckCompletion(quest, &calston);
    oracle.SetHealth(20);
    Require(ThrowLeash(visitor, oracle) == SPELL_CAST_OK, "The old oracle cast did not start");
    Tick(visitor, 1500);
    Require(!Captive(visitor, CapturedOracle) && !visitor.GetQuestObjectiveData(LeashQuest, CapturedPuddlejumper),
        "The old native Oracle route was unexpectedly present");
    std::cout << "EXPECTED_BROKEN old leash began casting on a full-health target without a quest, and gave no capture for an injured Oracle\n";
#else
    Require(ThrowLeash(player, puddlejumper) != SPELL_CAST_OK, "A retained leash was usable without the quest");
    player.DestroyItemCount(Leash, 1, true);
    player.AddQuestAndCheckCompletion(quest, &calston);
    visitor.AddQuestAndCheckCompletion(quest, &calston);
    Require(ThrowLeash(player, puddlejumper) != SPELL_CAST_OK, "The leash accepted a full-health murloc");
    SupplyMove(player, wrong.second);
    outsider.SetHealth(1);
    Require(ThrowLeash(player, outsider) != SPELL_CAST_OK, "The leash accepted an unrelated Vile Fin subtype");
    SupplyMove(player, first.second);
    puddlejumper.SetHealth(30);
    Require(player.CastSpell(&puddlejumper, Capture, false) != SPELL_CAST_OK, "A free spell bypassed the carried leash");
    Require(ThrowLeash(player, puddlejumper) == SPELL_CAST_OK, "A wounded Puddlejumper could not be leashed");
    Tick(player, 350);
    puddlejumper.SetHealth(45);
    Tick(player, 1200);
    Require(!Captive(player, CapturedPuddlejumper), "A target healed during the cast was replaced");
    puddlejumper.SetHealth(30);
    Require(ThrowLeash(player, puddlejumper) == SPELL_CAST_OK, "Retry after the target recovered could not start");
    Tick(player, 350);
    AbandonSupply(player, LeashQuest);
    player.AddQuestAndCheckCompletion(quest, &calston);
    Tick(player, 1200);
    Require(!Captive(player, CapturedPuddlejumper) && !player.GetQuestObjectiveData(LeashQuest, CapturedPuddlejumper),
        "A pending leash from the abandoned attempt advanced a later quest");

    puddlejumper.SetLootRecipient(&visitor);
    Require(ThrowLeash(player, puddlejumper) != SPELL_CAST_OK, "An unrelated player's tap could be captured");
    puddlejumper.SetLootRecipient(nullptr);
    LootPet ownerPet(player, map);
    puddlejumper.SetHealth(31);
    ownerPet.DealDamage(&puddlejumper, 1);
    Require(puddlejumper.IsAlive() && puddlejumper.GetHealthPct() <= 25.0f, "A real pet hit did not weaken the intended murloc");
    oracle.SetHealth(20);
    Require(ThrowLeash(player, puddlejumper) == SPELL_CAST_OK && ThrowLeash(visitor, oracle) == SPELL_CAST_OK,
        "The two authored species could not start independent captures");
    Tick(player, 1500);
    Tick(visitor, 1500);
    auto owned = Captive(player, CapturedPuddlejumper);
    auto neighboring = Captive(visitor, CapturedOracle);
    Require(owned && neighboring && owned->GetSummoner() == &player && neighboring->GetSummoner() == &visitor &&
        owned->HasQuestObjectScope() && neighboring->HasQuestObjectScope() && !owned->CanTargetQuestObject(&visitor) &&
        !neighboring->CanTargetQuestObject(&player), "Both actual captives must stay bound to their leash users");
    Require(owned->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE &&
        neighboring->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE,
        "Neither captive should follow a nearby unrelated player");
    Require(!player.CanRewardQuest(quest, false) && !visitor.CanRewardQuest(quest, false) &&
        player.GetQuestObjectiveData(LeashQuest, CapturedPuddlejumper) == 1 &&
        visitor.GetQuestObjectiveData(LeashQuest, CapturedPuddlejumper) == 1,
        "The actual capture advances its first stage alone");

    // The owners are still on the shore. Load Calston's actual grid before
    // moving the captives across grid boundaries for the unattended-arrival case.
    map.LoadGrid(keeper.second.GetPositionX(), keeper.second.GetPositionY());
    owned->NearTeleportTo(keeper.second);
    neighboring->NearTeleportTo(keeper.second);
    map.MoveAllCreaturesInMoveList();
    Require(owned->FindNearestCreature(Calston, INTERACTION_DISTANCE, true) == &calston &&
        neighboring->FindNearestCreature(Calston, INTERACTION_DISTANCE, true) == &calston,
        "Both captives must reach the actual receiver for the arrival regression");
    owned->AI()->UpdateAI(3500);
    Require(!player.CanRewardQuest(quest, false), "An unattended captive completed the owner's return");
    SupplyMove(player, keeper.second);
    SupplyMove(visitor, keeper.second);
    for (auto captive : {owned, neighboring})
    {
        auto keeperNear = captive->FindNearestCreature(Calston, INTERACTION_DISTANCE, true);
        Unit* lead = captive->GetSummoner();
        std::cout << "CASE Calston arrival " << captive->GetEntry() << " distance=" << captive->GetDistance(&calston)
            << " owner=" << bool(lead) << " nearest=" << (keeperNear == &calston) << " line=" << calston.IsWithinLOSInMap(captive)
            << " ownerline=" << (lead && calston.IsWithinLOSInMap(lead)) << " combat=" << captive->isInCombat() << std::endl;
    }
    owned->AI()->UpdateAI(3500);
    neighboring->AI()->UpdateAI(3500);
    std::cout << "CASE returned credits " << player.GetQuestObjectiveData(LeashQuest, Returned) << ' '
        << visitor.GetQuestObjectiveData(LeashQuest, Returned) << std::endl;
    Require(player.CanRewardQuest(quest, false) && visitor.CanRewardQuest(quest, false) &&
        player.GetQuestObjectiveData(LeashQuest, Returned) == 1 && visitor.GetQuestObjectiveData(LeashQuest, Returned) == 1,
        "Each actual return at Calston must advance only the correct quest");

    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = calston.GetGUID();
    packet.QuestID = LeashQuest;
    packet.ItemChoiceID = 0;
    player.GetSession()->HandleQuestGiverChooseReward(packet);
    AbandonSupply(visitor, LeashQuest);
    map.RemoveAllObjectsInRemoveList();
    Require(player.GetQuestRewardStatus(LeashQuest) && !player.GetItemCount(Leash) && !visitor.GetItemCount(Leash) &&
        !Captive(player, CapturedPuddlejumper) && !Captive(visitor, CapturedOracle) && !player.HasAura(73109) && !visitor.HasAura(73433),
        "The native reward and abandonment must clear their quest item and companion");
    std::cout << "PASS actual leash, both intended creatures, private companions, real Calston return, quest item cleanup and retry\n";
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
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The leash test requires the isolated character schema");
        auto quest = LoadLeashQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckLeash(quest);
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
