#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GameObjectAI.h"
#include "QuestItemObjectUse.h"

void AddSC_azuremyst_isle();

struct RavagerSpawnTag { using Type = uint64 Creature::*; friend Type Access(RavagerSpawnTag); };
template struct FixtureAccess<RavagerSpawnTag, &Creature::m_DBTableGuid>;

namespace
{
constexpr uint32 Ruada = 17480;

struct RavagerParty : Group
{
    ~RavagerParty()
    {
        if (Registered)
            sGroupMgr->RemoveGroup(this);
    }

    bool Registered = false;
};

struct CageRavager : Actor
{
    CageRavager(Map& map, Field* fields) :
        Actor(&map, RavagerCage::Ravager, fields[0].GetUInt32(), {fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat(), fields[4].GetFloat()})
    {
        this->*Access(RavagerSpawnTag{}) = fields[0].GetUInt64();
    }

    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(), "The registered ravager AI could not load");
    }
};

struct PlacedCage : GameObject
{
    PlacedCage(Map& map, Field* row)
    {
        m_DBTableGuid = row[0].GetUInt32();
        Position point{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()};
        G3D::Quat rotation(row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat());
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Require(Create(m_DBTableGuid, row[1].GetUInt32(), &map, 1, point, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "An actual Azuremyst object could not load");
        if (!GetGOInfo()->GetDespawnPossibility() && !GetGOInfo()->IsDespawnAtAction())
            SetRespawnDelayTime(0); // The native spawn loader keeps these doors present through deactivation.
        Update(5000);
        if (GetGoType() == GAMEOBJECT_TYPE_DOOR)
            Require(getLootState() == GO_READY, "The actual object update did not initialize the door's ready state");
    }

    ~PlacedCage()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

Quest const* LoadRavagerQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeRavagerCage.inc"
    );
    Require(bool(row), "Missing Strength of One");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[RavagerCage::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(RavagerCage::Quest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9582 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 2 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objectives->Fetch()[4].GetUInt32() == RavagerCage::Ravager && objectives->Fetch()[5].GetUInt32() == 1, "Expected the actual ravager death");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objectives->Fetch()[4].GetUInt32() == RavagerCage::Key &&
        objectives->Fetch()[5].GetUInt32() == 1, "The original persistent key must be returned");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=9582");
    Require(addon && addon->Fetch()[5].GetUInt32() == 10350, "The original follow-up quest changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == RavagerCage::Key && quest->SourceItemIdCount == 1 && !quest->PackageID && !quest->m_rewChoiceItemsCount,
        "Expected one original key with no selectable reward");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    auto& lootTable = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry=17556";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded && LootTemplates_Creature.HaveLootFor(RavagerCage::Ravager), "The actual ravager loot table did not load");

    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_azuremyst_isle();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE ravager_questender AS SELECT * FROM creature_questender WHERE quest=9582");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM ravager_questender");
#ifndef RAVAGER_CAGE_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE ravager_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=30767");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM ravager_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifndef RAVAGER_CAGE_BASELINE
    sConditionMgr->LoadConditions();
#endif
    return quest;
}

void ClickCage(Player& player, GameObject const& cage)
{
    Require(player.CanContact() && player.GetUnitBeingMoved() == &player, "The cage use packet requires the actual mover");
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << cage.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void CheckCage(Quest const* quest)
{
    auto receiverData = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=9582 JOIN creature_questender e ON e.id=c.id AND e.quest=9582 "
        "WHERE c.id=17480 AND c.guid=16791 AND c.map=530 AND c.zoneId=3524");
    auto creatureData = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=17556 AND guid=35799 AND map=530 AND zoneId=3524");
    auto cageData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=7903 AND id=181849 AND map=530 AND zoneId=3524");
    auto nearbyData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=560 AND id=1617 AND map=530 AND zoneId=3524");
    Require(receiverData && creatureData && cageData && nearbyData, "Missing Ruada, original cage, actual ravager or nearby world object");
    Map map(530, 500, 0, DIFFICULTY_NONE);
    RavagerParty party;
    party.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(9582, 0);
    auto receiverRow = receiverData->Fetch();
    Position receiverPoint{receiverRow[1].GetFloat(), receiverRow[2].GetFloat(), receiverRow[3].GetFloat(), receiverRow[4].GetFloat()};
    map.LoadGrid(receiverPoint.GetPositionX(), receiverPoint.GetPositionY());
    Actor receiver(&map, Ruada, receiverRow[0].GetUInt32(), receiverPoint);
    PlacedCage cage(map, cageData->Fetch()), nearby(map, nearbyData->Fetch());
    CageRavager ravager(map, creatureData->Fetch());
    Require(map.AddToMap(static_cast<Creature*>(&receiver)) && map.AddToMap(static_cast<Creature*>(&ravager)), "Could not publish the actual Ruada and ravager");
    auto lock = sLockStore.LookupEntry(cage.GetGOInfo()->GetLockId());
    Require(lock && lock->ID == 1668 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == RavagerCage::Key &&
        sObjectMgr->GetScriptName(cage.GetScriptId()) == "go_ravager_cage" && sObjectMgr->GetScriptName(ravager.GetScriptId()) == "npc_death_ravager",
        "The actual script bindings or key lock are missing");

    LootPlayer owner(SupplySession(), map, 958201, cage.GetPosition()), other(SupplySession(), map, 958202, cage.GetPosition());
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_DRAENEI);
        player->setFactionForRace(RACE_DRAENEI);
        player->InitDisplayIds();
        player->SetLevel(15);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(RavagerCage::Key) == 1 && cage.GetGoState() == GO_STATE_READY &&
        ravager.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE), "The actual quest key, closed cage, or idle ravager did not initialize");
    owner.DestroyItemCount(RavagerCage::Key, 1, true);
    ClickCage(owner, cage);
#ifdef RAVAGER_CAGE_BASELINE
    Require(ravager.getVictim() == &owner && !ravager.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && cage.GetGoState() == GO_STATE_READY,
        "The old click did not reproduce the keyless attack through the closed cage");
    std::cout << "EXPECTED_BROKEN no-key direct click released the shared ravager while leaving the cage closed\n";
#else
    Require(!ravager.getVictim() && ravager.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && !cage.ActivateToQuest(&owner),
        "The no-key click released the ravager or showed an available cage");
    GiveSupply(other, RavagerCage::Key, 1);
    ClickCage(other, cage);
    Require(other.CastSpell(&cage, RavagerCage::OpenCage, false, other.GetItemByEntry(RavagerCage::Key)) != SPELL_CAST_OK &&
        !cage.ActivateToQuest(&other), "An inactive owner used a retained key");
    other.DestroyItemCount(RavagerCage::Key, 1, true);

    AbandonSupply(owner, RavagerCage::Quest);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    other.AddQuestAndCheckCompletion(quest, &receiver);
    SupplyMove(owner, nearby.GetPosition());
    Require(owner.CastSpell(&nearby, RavagerCage::OpenCage, false, owner.GetItemByEntry(RavagerCage::Key)) != SPELL_CAST_OK,
        "The actual key opened a different world object");
    SupplyMove(owner, cage.GetPosition());
    ClickCage(owner, cage);
    Tick(owner, 1000);
    auto cast = owner.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    Require(cast && cast->GetSpellInfo()->Id == RavagerCage::OpenCage && cage.GetGoState() == GO_STATE_READY && !ravager.getVictim(),
        "The official five-second lock did not start or released its target before completion");
    AbandonSupply(owner, RavagerCage::Quest);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Tick(owner, 6500);
    Require(cage.GetGoState() == GO_STATE_READY && !ravager.getVictim() && owner.GetItemCount(RavagerCage::Key) == 1,
        "A cast from a cancelled quest attempt opened the real cage");
    Sleep(1800);
    GameTime::UpdateGameTimers();

    ClickCage(owner, cage);
    auto secondCast = owner.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    std::cout << "Ravager retry start=" << (secondCast ? secondCast->GetSpellInfo()->Id : 0) << " lootState=" << unsigned(cage.getLootState())
        << " cageAI=" << typeid(*cage.AI()).name() << " ravagerAI=" << typeid(*ravager.AI()).name() << '\n';
    Tick(owner, 5500);
    std::cout << "Ravager retry state=" << unsigned(cage.GetGoState()) << " lootState=" << unsigned(cage.getLootState())
        << " victim=" << (ravager.getVictim() ? ravager.getVictim()->GetGUIDLow() : 0) << " key=" << owner.GetItemCount(RavagerCage::Key)
        << " flags=" << ravager.GetUInt32Value(UNIT_FIELD_FLAGS) << " quest=" << unsigned(owner.GetQuestStatus(RavagerCage::Quest)) << '\n';
    Require(cage.GetGoState() == GO_STATE_ACTIVE && ravager.getVictim() == &owner && !ravager.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) &&
        owner.GetItemCount(RavagerCage::Key) == 1 && !owner.GetQuestObjectiveData(RavagerCage::Quest, RavagerCage::Ravager),
        "The authorized cast did not animate the cage, start its actual fight or preserve the reusable key");
    ClickCage(other, cage);
    Tick(other, 5500);
    uint32 damage = 100;
    ravager.AI()->DamageTaken(&other, damage, DIRECT_DAMAGE);
    Require(!damage && ravager.getVictim() == &owner, "The other quest owner changed or damaged the shared fight");
    AbandonSupply(owner, RavagerCage::Quest);
    ravager.AI()->UpdateAI(100);
    Require(ravager.HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) && !ravager.getVictim() && !owner.GetItemCount(RavagerCage::Key),
        "Abandonment retained the stale fight or old key");

    Sleep(6200);
    GameTime::UpdateGameTimers();
    for (uint32 elapsed = 0; elapsed < 3000; elapsed += 50)
    {
        ravager.Update(50);
        cage.Update(50);
        map.MoveAllCreaturesInMoveList();
    }
    cage.Update(5000);
    cage.Update(5000);
    std::cout << "Ravager reset state=" << unsigned(cage.GetGoState()) << " lootState=" << unsigned(cage.getLootState())
        << " actorDistance=" << cage.GetDistance(&ravager) << " evade=" << ravager.IsInEvadeMode() << '\n';
    Require(cage.GetGoState() == GO_STATE_READY && cage.getLootState() == GO_READY && !ravager.IsInEvadeMode() && cage.ActivateToQuest(&other),
        "The actual cage or ravager did not become ready for a different quest owner");

    owner.AddQuestAndCheckCompletion(quest, &receiver);
    for (LootPlayer* player : {&owner, &other})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(party.GetMemberSlots()).push_back(slot);
        player->SetGroup(&party, 0);
    }
    sGroupMgr->AddGroup(&party);
    party.Registered = true;
    ClickCage(other, cage);
    auto nextCast = other.GetCurrentSpell(CURRENT_GENERIC_SPELL);
    std::cout << "Ravager next owner cast=" << (nextCast ? nextCast->GetSpellInfo()->Id : 0) << " ready=" << unsigned(cage.getLootState()) << '\n';
    Tick(other, 5500);
    std::cout << "Ravager next owner state=" << unsigned(cage.GetGoState()) << " victim=" << (ravager.getVictim() ? ravager.getVictim()->GetGUIDLow() : 0) << '\n';
    Require(cage.GetGoState() == GO_STATE_ACTIVE && ravager.getVictim() == &other, "The second genuine cast could not start a new cage fight");
    damage = 100;
    ravager.AI()->DamageTaken(&owner, damage, DIRECT_DAMAGE);
    Require(damage == 100, "The actual challenger could not receive legitimate party assistance");
    {
        LootPet pet(other, map);
        pet.SetLevel(other.getLevel());
        pet.SetEffectiveLevel(other.GetEffectiveLevel());
        pet.SetOwnerGUID(ObjectGuid::Empty);
        other.SetMinion(&pet, true);
        Require(pet.IsControlledByPlayer() && pet.DealDamage(&ravager, 1000000, nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
            ravager.isDead(false), "A genuine controlled pet hit did not kill the actual released ravager");
    }
    Tick(other, 200);
    Tick(owner, 200);
    Require(other.GetQuestStatus(RavagerCage::Quest) == QUEST_STATUS_COMPLETE && other.GetItemCount(RavagerCage::Key) == 1 &&
        !cage.ActivateToQuest(&other) && owner.GetQuestStatus(RavagerCage::Quest) == QUEST_STATUS_COMPLETE && owner.GetItemCount(RavagerCage::Key) == 1,
        "The actual pet defeat did not complete its eligible party or consumed a return key");
    SupplyMove(other, receiverPoint);
    Require(other.CanInteractWithQuestGiver(&receiver), "Ruada is not an actual reachable receiver");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = receiver.GetGUID();
    reward.QuestID = RavagerCage::Quest;
    reward.ItemChoiceID = 0;
    other.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(other.GetQuestRewardStatus(RavagerCage::Quest) && !other.GetItemCount(RavagerCage::Key) && !cage.ActivateToQuest(&other),
        "Ruada did not remove the returned key or the completed cage remained available");
    AbandonSupply(owner, RavagerCage::Quest);
    Require(!owner.GetItemCount(RavagerCage::Key), "The legitimate group recipient retained a key after abandonment");
    std::cout << "PASS native five-second lock, actual cage and fight, interrupted retry, pet/group credit, abandonment and Ruada return\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{530, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Ravager reward requires the disposable character schema");
        auto quest = LoadRavagerQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckCage(quest);
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
