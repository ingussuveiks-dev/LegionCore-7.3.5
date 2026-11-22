#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "QuestItemObjectUse.h"

namespace
{
constexpr uint32 Tonga = 3448;
constexpr uint32 Jerrik = 34626;

Quest const* LoadOasisQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeStagnantOasis.inc"
    );
    Require(bool(row), "Missing The Stagnant Oasis");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[StagnantOasis::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(StagnantOasis::Quest, quest);

    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=877");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_GAMEOBJECT &&
        objective->Fetch()[4].GetUInt32() == StagnantOasis::Fissure && objective->Fetch()[5].GetUInt32() == 1,
        "Expected one actual Bubbling Fissure activation");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepFaction,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=877");
    Require(addon && addon->Fetch()[4].GetInt32() == 13988 && addon->Fetch()[5].GetUInt32() == 880,
        "The native Jerrik and Tonga quest order has changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(quest->SourceItemId == StagnantOasis::Seeds && quest->SourceItemIdCount == 1 && !quest->PackageID && !quest->m_rewChoiceItemsCount,
        "Expected one original dose of Dried Seeds and no selectable reward");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE oasis_questender AS SELECT * FROM creature_questender WHERE quest=877");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM oasis_questender");
#ifndef STAGNANT_OASIS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE oasis_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=5206");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM oasis_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifndef STAGNANT_OASIS_BASELINE
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, StagnantOasis::PlantSeeds).size() == 3,
        "The three seed-cast conditions did not load");
#endif
    return quest;
}

struct PlacedObject : GameObject
{
    PlacedObject(Map& map, Field* row, uint32 alternateGuid = 0)
    {
        m_DBTableGuid = alternateGuid ? alternateGuid : row[0].GetUInt32();
        Position position{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()};
        G3D::Quat rotation(row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat());
        Require(Create(m_DBTableGuid, row[1].GetUInt32(), &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "The actual quest or nearby object could not load");
    }

    ~PlacedObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

Position ActorPosition(Field* row)
{
    return {row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
}

void ClickFissure(Player& player, GameObject const& fissure)
{
    Require(player.CanContact() && player.GetUnitBeingMoved() == &player, "A real game object use packet requires the player's mover");
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << fissure.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

SpellCastResult Plant(LootPlayer& player, GameObject& object)
{
    Item* seeds = player.GetItemByEntry(StagnantOasis::Seeds);
    Require(seeds, "The actual dried seeds are missing");
    return player.CastSpell(&object, StagnantOasis::PlantSeeds, false, seeds);
}

void CheckOasis(Quest const* quest)
{
    auto receiverData = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=877 JOIN creature_questender e ON e.id=c.id AND e.quest=877 "
        "WHERE c.id=3448 AND c.guid=87126 AND c.map=1 AND c.zoneId=17");
    auto giverData = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=877 WHERE c.id=34626 AND c.guid=87500 AND c.map=1 AND c.zoneId=17");
    auto fissureData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=32492 AND id=3737 AND map=1 AND zoneId=17 AND areaId=388");
    auto unrelatedData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=32491 AND id=180658 AND map=1 AND zoneId=17 AND areaId=388");
    Require(receiverData && giverData && fissureData && unrelatedData, "Missing Tonga, Jerrik, the real submerged fissure or the nearby world object");
    Map map(1, 500, 0, DIFFICULTY_NONE);
    auto receiverRow = receiverData->Fetch();
    auto giverRow = giverData->Fetch();
    Actor receiver(&map, Tonga, receiverRow[0].GetUInt32(), ActorPosition(receiverRow)),
        giver(&map, Jerrik, giverRow[0].GetUInt32(), ActorPosition(giverRow));
    Require(map.AddToMap(static_cast<Creature*>(&receiver)) && map.AddToMap(static_cast<Creature*>(&giver)), "The actual quest NPCs did not load");
    PlacedObject fissure(map, fissureData->Fetch()), unrelated(map, unrelatedData->Fetch()), wrongPlacement(map, fissureData->Fetch(), 877001);
    auto const* lock = sLockStore.LookupEntry(fissure.GetGOInfo()->GetLockId());
    Require(lock && lock->ID == 73 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == StagnantOasis::Seeds &&
        fissure.GetGOInfo()->goober.questID == StagnantOasis::Quest && fissure.GetGOInfo()->goober.eventID == 525,
        "The original fissure lock, quest gate or event has changed");

    LootPlayer owner(SupplySession(), map, 87701, fissure.GetPosition()), other(SupplySession(), map, 87702, fissure.GetPosition());
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_TAUREN);
        player->setFactionForRace(RACE_TAUREN);
        player->InitDisplayIds();
        player->SetLevel(17);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    owner.AddQuestAndCheckCompletion(quest, &giver);
    Item* provided = owner.GetItemByEntry(StagnantOasis::Seeds);
    bool singleDose = false;
    if (provided)
        for (ItemEffectEntry const* effect : provided->GetTemplate()->Effects)
            if (effect && effect->SpellID == StagnantOasis::PlantSeeds)
                singleDose = effect->Charges == -1 && provided->GetSpellCharges(effect->LegacySlotIndex) == -1;
    std::cout << "Oasis start item=" << owner.GetItemCount(StagnantOasis::Seeds) << " nativeDose=" << singleDose << '\n';
    Require(owner.GetItemCount(StagnantOasis::Seeds) == 1 && singleDose,
        "Jerrik must give the original single-use seed dose");
#ifdef STAGNANT_OASIS_BASELINE
    ClickFissure(owner, fissure);
    Require(owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure) == 1 && owner.GetItemCount(StagnantOasis::Seeds) == 1,
        "The old direct click did not expose the skipped seed cast and item consumption");
    std::cout << "EXPECTED_BROKEN ordinary fissure click advanced immediately and left the seeds in bags\n";
#else
    Require(fissure.ActivateToQuest(&owner) && !fissure.ActivateToQuest(&other) && !wrongPlacement.ActivateToQuest(&owner),
        "Fissure availability did not match the actual accepted quest and placement");
    GiveSupply(other, StagnantOasis::Seeds, 1);
    ClickFissure(other, fissure);
    Require(Plant(other, fissure) != SPELL_CAST_OK && !fissure.ActivateToQuest(&other) && other.GetItemCount(StagnantOasis::Seeds) == 1,
        "An old seed or ordinary click activated the fissure without its quest");
    other.DestroyItemCount(StagnantOasis::Seeds, 1, true);
    other.AddQuestAndCheckCompletion(quest, &receiver);
    SupplyMove(owner, unrelated.GetPosition());
    Require(Plant(owner, unrelated) != SPELL_CAST_OK && owner.GetItemCount(StagnantOasis::Seeds) == 1,
        "Dried Seeds targeted the unrelated actual Barrens object");
    SupplyMove(owner, fissure.GetPosition());
    Require(Plant(owner, wrongPlacement) != SPELL_CAST_OK && owner.CastSpell(&fissure, StagnantOasis::PlantSeeds, false) != SPELL_CAST_OK,
        "The cast admitted the wrong placement or lacked its actual item");

    Position outOfRange = fissure.GetPosition();
    outOfRange.m_positionX += 35.0f;
    SupplyMove(owner, outOfRange);
    ClickFissure(owner, fissure);
    Tick(owner, 7000);
    Require(owner.GetItemCount(StagnantOasis::Seeds) == 1 && !owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure),
        "A remote right-click planted the seeds");
    SupplyMove(owner, fissure.GetPosition());

    fissure.Use(&owner);
    Require(!owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure), "Direct trigger bypassed the actual planting");
    ClickFissure(owner, fissure);
    Tick(owner, 300);
    Spell* planting = owner.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    Require(planting && planting->GetSpellInfo()->Id == StagnantOasis::PlantSeeds, "The real right-click did not start the native planting cast");
    Require(owner.GetItemCount(StagnantOasis::Seeds) == 1 && !owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure),
        "The clicked cast spent the seeds before completion");
    AbandonSupply(owner, StagnantOasis::Quest);
    Require(!owner.GetItemCount(StagnantOasis::Seeds) && !fissure.ActivateToQuest(&owner), "Abandonment left an active fissure or its seeds");
    owner.AddQuestAndCheckCompletion(quest, &giver);
    Tick(owner, 8000);
    Require(owner.GetItemCount(StagnantOasis::Seeds) == 1 && !owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure),
        "The cancelled attempt altered the newly accepted quest");

    // Unit ticks exercise the cast timer but do not advance the global real-time cooldown.
    Sleep(1800);
    GameTime::UpdateGameTimers();
    ClickFissure(owner, fissure);
    Tick(owner, 8000);
    std::cout << "Oasis after planting status=" << unsigned(owner.GetQuestStatus(StagnantOasis::Quest))
        << " ownerProgress=" << owner.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure)
        << " ownerSeeds=" << owner.GetItemCount(StagnantOasis::Seeds)
        << " otherProgress=" << other.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure)
        << " otherSeeds=" << other.GetItemCount(StagnantOasis::Seeds) << " objectState=" << unsigned(fissure.GetGoState()) << '\n';
    Require(owner.GetQuestStatus(StagnantOasis::Quest) == QUEST_STATUS_COMPLETE && !owner.GetItemCount(StagnantOasis::Seeds) &&
        !other.GetQuestObjectiveData(StagnantOasis::Quest, StagnantOasis::Fissure) && other.GetItemCount(StagnantOasis::Seeds) == 1 &&
        !fissure.ActivateToQuest(&owner), "Planting must consume exactly one dose, complete the actual owner and deactivate their interaction");

    GiveSupply(owner, StagnantOasis::Seeds, 1);
    Require(Plant(owner, fissure) != SPELL_CAST_OK, "The completed quest still accepted spare seeds");
    SupplyMove(owner, receiver.GetPosition());
    Require(owner.CanInteractWithQuestGiver(&receiver), "The actual Tonga receiver is unreachable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = receiver.GetGUID();
    reward.QuestID = StagnantOasis::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(StagnantOasis::Quest) && !owner.GetItemCount(StagnantOasis::Seeds) && !fissure.ActivateToQuest(&owner),
        "Tonga's actual reward left a spare item or enabled the completed fissure");
    AbandonSupply(other, StagnantOasis::Quest);
    Require(!other.GetItemCount(StagnantOasis::Seeds), "The other player's unused native dose survived abandonment");
    std::cout << "PASS real right-click seed cast, exact fissure, one charge, cancelled planting, one owner and Tonga reward\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Oasis reward requires the disposable character schema");
        auto quest = LoadOasisQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckOasis(quest);
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
