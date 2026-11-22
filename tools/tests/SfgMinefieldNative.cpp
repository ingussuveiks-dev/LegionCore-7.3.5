#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

namespace
{
constexpr uint32 MinefieldQuest = 14472;
constexpr uint32 Sfg = 49700;
constexpr uint32 Shot = 69743;
constexpr uint32 Giant = 36868;
constexpr uint32 Glix = 36744;

Quest const* LoadMinefieldQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeSfgMinefield.inc"
    );
    Require(bool(row), "Missing In The Face");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[MinefieldQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(MinefieldQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14472");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Giant && objective->Fetch()[5].GetUInt32() == 1, "Expected the original giant death objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14472");
    Require(bool(addon), "Missing In The Face quest addon");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == Sfg && quest->SourceItemIdCount == 1, "The quest must provide an SFG");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto& table = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=36868";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded && LootTemplates_Creature.HaveLootFor(Giant), "The actual giant's normal loot stock did not load");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE sfg_conditions AS SELECT * FROM conditions "
        "WHERE (SourceTypeOrReferenceId=17 AND SourceEntry=69743) OR "
        "(SourceTypeOrReferenceId=17 AND SourceEntry=69228 AND ConditionTypeOrReference=2)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM sfg_conditions");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE sfg_questender AS SELECT * FROM creature_questender WHERE quest=14472");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM sfg_questender");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sConditionMgr->LoadConditions();
    return quest;
}

struct PlacedGiant
{
    uint32 Guid;
    Position Point;
};

struct MinefieldParty : Group
{
    ~MinefieldParty()
    {
        if (Registered)
            sGroupMgr->RemoveGroup(this);
    }

    bool Registered = false;
};

PlacedGiant ReadPlacement(Field* fields)
{
    return {fields[0].GetUInt32(), {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()}};
}

SpellCastResult FireSfg(LootPlayer& player, Unit& target)
{
    auto item = player.GetItemByEntry(Sfg);
    Require(item, "Expected the actual carried SFG");
    return player.CastSpell(&target, Shot, false, item);
}

void UseMinefield(Map& map, Actor& giant)
{
    map.LoadGrid(giant.GetPositionX(), giant.GetPositionY());
    giant.SetLevel(18);
    giant.SetMaxHealth(1000);
    giant.SetHealth(1000);
    Require(map.AddToMap(static_cast<Creature*>(&giant)), "The actual giant did not load in the native map");
}

void CheckMinefield(Quest const* quest)
{
    auto placer = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=14472 JOIN creature_questender e ON e.id=c.id AND e.quest=14472 "
        "WHERE c.id=36744 AND c.guid=99024 AND c.map=1 AND c.zoneId=16");
    auto local = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=36868 AND map=1 AND zoneId=16 ORDER BY guid");
    Require(placer && local && local->GetRowCount() == 9, "Missing the original Glix or minefield placements");
    auto giver = ReadPlacement(placer->Fetch());
    auto first = ReadPlacement(local->Fetch());
    local->NextRow();
    auto second = ReadPlacement(local->Fetch());
    Position firingPoint = second.Point;
    firingPoint.m_positionX += 14.0f;
    firingPoint.SetOrientation(firingPoint.GetAngle(&second.Point));
    Map map(1, 500, 0, DIFFICULTY_NONE);
    MinefieldParty party;
    party.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(14472, 0);
    Actor glix(&map, Glix, giver.Guid, giver.Point);
    Require(map.AddToMap(static_cast<Creature*>(&glix)), "The actual Glix did not load");
    LootPlayer shooter(SupplySession(), map, 14472, firingPoint), member(SupplySession(), map, 14473, firingPoint),
        stale(SupplySession(), map, 14474, firingPoint), outsider(SupplySession(), map, 14475, firingPoint);
    for (auto player : {&shooter, &member, &stale, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
        player->setFactionForRace(RACE_GOBLIN);
        player->InitDisplayIds();
        player->SetLevel(18);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    Actor ordinary(&map, Giant, first.Guid, first.Point), intended(&map, Giant, second.Guid, second.Point);
    Actor decoy(&map, 6135, 144720, second.Point);
    UseMinefield(map, ordinary);
    UseMinefield(map, intended);
    Require(map.AddToMap(static_cast<Creature*>(&decoy)), "Unrelated local creature did not load");
    GiveSupply(shooter, Sfg, 1);
#ifdef SFG_MINEFIELD_BASELINE
    Require(FireSfg(shooter, decoy) == SPELL_CAST_OK, "Original SFG did not admit an unrelated target outside the quest");
    shooter.CastStop();
#else
    Require(FireSfg(shooter, intended) != SPELL_CAST_OK, "An old SFG worked without the quest");
#endif
    shooter.DestroyItemCount(Sfg, 1, true);
    shooter.AddQuestAndCheckCompletion(quest, &glix);
    ordinary.SetLootRecipient(&shooter);
    ordinary.LowerPlayerDamageReq(ordinary.GetMaxHealth());
    {
        LootPet pet(shooter, map);
        pet.Kill(&ordinary, false);
    }
    Tick(shooter, 200);
    Require(ordinary.isDead(false), "The native pet killing blow did not occur");
#ifdef SFG_MINEFIELD_BASELINE
    Require(shooter.GetQuestObjectiveData(MinefieldQuest, Giant) == 1,
        "The old ordinary death did not expose missing SFG provenance");
    std::cout << "EXPECTED_BROKEN inactive SFG aimed at the wrong target and ordinary pet death completed the SFG objective\n";
#else
    Require(!shooter.GetQuestObjectiveData(MinefieldQuest, Giant), "An ordinary pet killing blow completed the SFG objective");
    Require(FireSfg(shooter, decoy) != SPELL_CAST_OK, "An active SFG accepted the wrong target");
    Require(shooter.CastSpell(&intended, Shot, false) != SPELL_CAST_OK, "A free cast bypassed the actual item");
    intended.SetLootRecipient(&outsider);
    Require(FireSfg(shooter, intended) != SPELL_CAST_OK, "The item targeted an unrelated player's tap");
    intended.SetLootRecipient(nullptr);
    Require(FireSfg(shooter, intended) == SPELL_CAST_OK, "The actual SFG did not start its three-second cast");
    Tick(shooter, 3040);
    Require(intended.IsAlive() && !shooter.GetItemCount(Sfg), "The SFG must spend its one charge at launch before impact");
    AbandonSupply(shooter, MinefieldQuest);
    shooter.AddQuestAndCheckCompletion(quest, &glix);
    Tick(shooter, 1000);
    Require(intended.IsAlive() && intended.GetHealth() == intended.GetMaxHealth() &&
        !shooter.GetQuestObjectiveData(MinefieldQuest, Giant), "A projectile from a cancelled attempt still damaged the minefield target");

    for (auto player : {&member, &stale, &outsider})
        player->AddQuestAndCheckCompletion(quest, &glix);
    for (auto player : {&shooter, &member, &stale})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(party.GetMemberSlots()).push_back(slot);
        player->SetGroup(&party, 0);
    }
    sGroupMgr->AddGroup(&party);
    party.Registered = true;
    intended.SetLootRecipient(&member);
    intended.LowerPlayerDamageReq(intended.GetMaxHealth());
    Require(FireSfg(shooter, intended) == SPELL_CAST_OK, "An eligible party target rejected the actual SFG");
    Tick(shooter, 6000);
    std::cout << "CASE SFG target health=" << intended.GetHealth() << " max=" << intended.GetMaxHealth()
        << " combat=" << intended.isInCombat() << " credit=" << shooter.GetQuestObjectiveData(MinefieldQuest, Giant) << std::endl;
    Require(intended.isDead(false), "The original direct SFG damage did not kill the real target");
    AbandonSupply(stale, MinefieldQuest);
    stale.AddQuestAndCheckCompletion(quest, &glix);
    Tick(stale, 200);
    Tick(member, 200);
    Tick(outsider, 200);
    for (auto player : {&shooter, &member, &stale})
        player->SetGroup(nullptr);
    sGroupMgr->RemoveGroup(&party);
    party.Registered = false;
    Require(shooter.GetQuestObjectiveData(MinefieldQuest, Giant) == 1 && member.GetQuestObjectiveData(MinefieldQuest, Giant) == 1 &&
        !stale.GetQuestObjectiveData(MinefieldQuest, Giant) && !outsider.GetQuestObjectiveData(MinefieldQuest, Giant),
        "The native delayed death credit must belong to eligible, still-current quest attempts");
    SupplyMove(shooter, giver.Point);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = glix.GetGUID();
    reward.QuestID = MinefieldQuest;
    reward.ItemChoiceID = 0;
    shooter.GetSession()->HandleQuestGiverChooseReward(reward);
    SupplyMove(member, giver.Point);
    Require(member.GetItemCount(Sfg) == 1, "A party member's unused charge must remain until turn-in");
    member.GetSession()->HandleQuestGiverChooseReward(reward);
    AbandonSupply(outsider, MinefieldQuest);
    Require(shooter.GetQuestRewardStatus(MinefieldQuest) && !shooter.GetItemCount(Sfg) &&
        member.GetQuestRewardStatus(MinefieldQuest) && !member.GetItemCount(Sfg) && !outsider.GetItemCount(Sfg),
        "The actual Glix turn-in or abandonment retained the SFG");
    std::cout << "PASS actual SFG target, timed shot, pet rejection, eligible party and stale credit, Glix and item cleanup\n";
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
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The SFG test requires the isolated character schema");
        auto quest = LoadMinefieldQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckMinefield(quest);
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
