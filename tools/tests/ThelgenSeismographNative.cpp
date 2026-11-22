#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "QuestItemObjectUse.h"

namespace
{
using namespace ThelgenSeismograph;
constexpr uint32 Prospector = 41129;
constexpr uint32 Prerequisite = 25726;
constexpr uint32 OpenTinkering = 6477;
constexpr uint32 LinkedLoot = 29575;

Position ThelgenPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

::Quest const* LoadSeismographQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeThelgenSeismograph.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "The original Wetlands quests are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new ::Quest(rows->Fetch());
        quests[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (rows->NextRow());
    auto quest = quests[ThelgenSeismograph::Quest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=25734");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Record && objective->Fetch()[5].GetUInt32() == 1, "Expected one actual seismic record");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=25734");
    Require(addon && addon->Fetch()[4].GetUInt32() == Prerequisite && !quest->SourceItemId && !quest->PackageID && !quest->m_rewChoiceItemsCount,
        "The original Wetlands sequence or reward changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quest->prevQuests.push_back(quest->PrevQuestID);
#ifdef THELGEN_SEISMOGRAPH_BASELINE
    Require(!quest->ItemDrop[0], "The original quest lacks abandonment cleanup metadata");
#else
    Require(quest->ItemDrop[0] == Record && quest->ItemDropQuantity[0] == 1, "The original record was not registered for native cleanup");
#endif

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry IN (203184,29575)";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded == 1 && LootTemplates_Gameobject.HaveLootFor(LinkedLoot) && !LootTemplates_Gameobject.HaveLootFor(Seismograph),
        "Expected only the actual independent seismic loot group");

    auto info = const_cast<GameObjectTemplate*>(sObjectMgr->GetGameObjectTemplate(Seismograph));
    Require(info && info->chest.questID == ThelgenSeismograph::Quest && std::none_of(std::begin(info->QuestItems),
        std::end(info->QuestItems), [](uint32 item) { return item != 0; }), "The actual template must exercise the chest loot fallback");
    uint32 savedQuest = info->chest.questID;
    info->chest.questID = 0;
    sQuestDataStore->LoadGameObjectForQuests();
    bool registeredWithoutHint = sQuestDataStore->IsGameObjectForQuests(Seismograph);
    info->chest.questID = savedQuest;
    sQuestDataStore->LoadGameObjectForQuests();
    Require(sQuestDataStore->IsGameObjectForQuests(Seismograph), "The original template could not register its native quest binding");
#ifdef THELGEN_SEISMOGRAPH_BASELINE
    Require(!registeredWithoutHint, "The old loader no longer misses a linked chest without an explicit hint");
#else
    Require(registeredWithoutHint, "The linked loot row alone did not register its quest chest");
#endif

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE seismograph_starter AS SELECT * FROM creature_queststarter WHERE quest=25734");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM seismograph_starter");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE seismograph_receiver AS SELECT * FROM creature_questender WHERE quest=25734");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM seismograph_receiver");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

void AcceptRecord(Player& player, Creature const& giver)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = ThelgenSeismograph::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void ReadRecord(Map& map, LootPlayer& owner, Placement const& placement)
{
    SupplyMove(owner, placement.position);
    for (uint32 roll = 0; roll < 4; ++roll)
    {
        Supply stock(map, placement);
        Require(owner.CastSpell(&stock, OpenTinkering, false) == SPELL_CAST_OK, "The real native Open Tinkering spell refused the placed seismograph");
        Tick(owner, 5500);
        bool received = TakeSupply(owner, stock, Record);
        if (received)
            Require(!stock.ActivateToQuest(&owner), "The seismograph was marked usable after its record was taken");
        ReleaseSupply(owner, stock);
        if (received)
            return;
        Sleep(1600);
        GameTime::UpdateGameTimers();
    }
    Require(false, "The native record did not drop in four authentic attempts");
}

void CheckSeismograph(::Quest const* quest)
{
    auto giverRow = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=41129 AND guid=119142 AND map=0 AND zoneId=11");
    auto objectRow = WorldDatabase.Query("SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=203184 AND guid=37153 AND map=0 AND zoneId=11 AND areaId=1021");
    Require(giverRow && objectRow, "Missing the actual Wetlands giver and seismograph");
    Map map(0, 506, 0, DIFFICULTY_NONE);
    Position giverPos = ThelgenPosition(giverRow->Fetch() + 1);
    map.LoadGrid(giverPos.GetPositionX(), giverPos.GetPositionY());
    Actor giver(&map, Prospector, giverRow->Fetch()[0].GetUInt32(), giverPos);
    Require(map.AddToMap(static_cast<Creature*>(&giver)), "Could not publish the original Wetlands quest NPC");
    LootPlayer owner(SupplySession(), map, 2573401, giverPos), other(SupplySession(), map, 2573402, giverPos);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DWARF);
        player->setFactionForRace(RACE_DWARF);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptRecord(owner, giver);
    Require(owner.GetQuestStatus(ThelgenSeismograph::Quest) == QUEST_STATUS_NONE, "The active quest skipped the original prerequisite");
    owner.SetRewardedQuest(Prerequisite);
    AcceptRecord(owner, giver);
    Require(owner.GetQuestStatus(ThelgenSeismograph::Quest) == QUEST_STATUS_INCOMPLETE && !owner.GetItemCount(Record),
        "The true Wetlands NPC did not start the quest for the record");

    auto row = objectRow->Fetch();
    Placement placement{Seismograph, row[0].GetUInt32(), row[1].GetUInt32(), ThelgenPosition(row + 2),
        G3D::Quat(row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat())};
    SupplyMove(owner, placement.position);
    SupplyMove(other, placement.position);
    {
        Supply stock(map, placement);
        auto lock = sLockStore.LookupEntry(stock.GetGOInfo()->GetLockId());
        bool opensWithTinkering = false;
        if (lock)
            for (uint8 slot = 0; slot < MAX_LOCK_CASE; ++slot)
                opensWithTinkering |= lock->Type[slot] == LOCK_KEY_SKILL && lock->Index[slot] == LOCKTYPE_OPEN_TINKERING;
        Require(stock.GetAreaId() == 1021 && stock.GetGOInfo()->GetLootId() == LinkedLoot && lock && lock->ID == 93 &&
            opensWithTinkering, "The actual position or native Open Tinkering lock changed");
#ifdef THELGEN_SEISMOGRAPH_BASELINE
        Require(!stock.ActivateToQuest(&owner), "The old real seismograph no longer reproduced its missing quest marker");
        Require(other.CastSpell(&stock, OpenTinkering, false) == SPELL_CAST_OK, "The old out-of-quest opening did not start");
        Tick(other, 5500);
        Require(other.GetLootGUID() == stock.GetGUID() && stock.getLootState() == GO_ACTIVATED && stock.loot.isLooted(),
            "The original outsider never opened empty seismograph loot");
        ReleaseSupply(other, stock);
#else
        Require(stock.ActivateToQuest(&owner) && !stock.ActivateToQuest(&other), "The actual marker did not use its linked quest loot");
        Require(other.CastSpell(&stock, OpenTinkering, false) != SPELL_CAST_OK, "An inactive player started Open Tinkering on the seismograph");
        other.SendLoot(stock.GetGUID(), LOOT_SKINNING);
        Require(!other.GetLootGUID() && stock.getLootState() == GO_READY, "An inactive player changed the actual seismograph loot state");
        Placement wrong = placement;
        wrong.guid = 450003;
        Supply decoy(map, wrong);
        Require(!decoy.ActivateToQuest(&owner) && owner.CastSpell(&decoy, OpenTinkering, false) != SPELL_CAST_OK,
            "A copy of the seismograph at the same position was usable");
#endif
    }
    ReadRecord(map, owner, placement);
    Require(owner.GetItemCount(Record) == 1 && owner.GetQuestStatus(ThelgenSeismograph::Quest) == QUEST_STATUS_COMPLETE, "The actual record did not complete the quest");
#ifdef THELGEN_SEISMOGRAPH_BASELINE
    AbandonSupply(owner, ThelgenSeismograph::Quest);
    Require(owner.GetItemCount(Record) == 1, "The original abandonment no longer retained the record");
    std::cout << "EXPECTED_BROKEN the placed seismograph lost its marker, opened empty for an outsider and kept the abandoned record\n";
#else
    SupplyMove(owner, giverPos);
    Require(owner.CanInteractWithQuestGiver(&giver) && owner.CanRewardQuest(quest, false), "The true Wetlands NPC could not take its actual record");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = giver.GetGUID();
    reward.QuestID = ThelgenSeismograph::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(ThelgenSeismograph::Quest) && !owner.GetItemCount(Record), "The real reward retained the seismic record");

    other.SetRewardedQuest(Prerequisite);
    AcceptRecord(other, giver);
    ReadRecord(map, other, placement);
    AbandonSupply(other, ThelgenSeismograph::Quest);
    Require(!other.GetItemCount(Record), "The actual abandoned quest retained its seismic record");
    std::cout << "PASS native linked loot marker, actual Open Tinkering, genuine record, reward and abandonment cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        auto quest = LoadSeismographQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckSeismograph(quest);
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
