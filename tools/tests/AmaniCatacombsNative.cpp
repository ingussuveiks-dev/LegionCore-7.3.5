#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "AreaTriggerData.h"
#include "GameObjectPackets.h"
#include "MiscPackets.h"
#include "QuestItemObjectUse.h"

namespace
{
constexpr uint32 Valwyn = 16289;

Quest const* LoadCatacombsQuest()
{
    auto row = WorldDatabase.Query(
#include "NativeAmaniCatacombs.inc"
    );
    Require(bool(row), "Missing Investigate the Amani Catacombs");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[AmaniCatacombs::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(AmaniCatacombs::Quest, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9193 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 3 && objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_GAMEOBJECT &&
        objectives->Fetch()[4].GetUInt32() == AmaniCatacombs::Remains && objectives->Fetch()[5].GetUInt32() == 10,
        "Expected ten real remains, the returned torch and exploration");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && objectives->Fetch()[4].GetUInt32() == AmaniCatacombs::Torch &&
        objectives->Fetch()[5].GetUInt32() == 1, "The spent torch must be returned to Valwyn");
    quest->LoadQuestObjective(objectives->Fetch());
    objectives->NextRow();
    Require(objectives->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_AREATRIGGER, "The original exploration objective is missing");
    quest->LoadQuestObjective(objectives->Fetch());
    auto addon = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=9193");
    Require(bool(addon), "Missing the original torch provision");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_DELIVER);
    Require(quest->SourceItemId == AmaniCatacombs::Torch && quest->SourceItemIdCount == 1 && !quest->PackageID && !quest->m_rewChoiceItemsCount &&
        quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT), "Expected one original torch and an exploration requirement");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE amani_receiver AS SELECT * FROM creature_questender WHERE quest=9193");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM amani_receiver");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE amani_trigger AS SELECT * FROM areatrigger_questender WHERE id=4071 AND quest=9193");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE areatrigger_questender AS SELECT * FROM amani_trigger");
#ifndef AMANI_CATACOMBS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE amani_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=28451");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM amani_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sAreaTriggerDataStore->LoadQuestAreaTriggers();
    auto triggers = sAreaTriggerDataStore->GetQuestsForAreaTrigger(AmaniCatacombs::Entrance);
    Require(triggers && triggers->count(AmaniCatacombs::Quest), "The actual quest entrance did not load");
#ifndef AMANI_CATACOMBS_BASELINE
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, AmaniCatacombs::BurnRemains).size() == 3,
        "The three native torch checks did not load");
#endif
    return quest;
}

struct CatacombsObject : GameObject
{
    CatacombsObject(Map& map, Field* row)
    {
        m_DBTableGuid = row[0].GetUInt32();
        Position position{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()};
        G3D::Quat rotation(row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat());
        map.LoadGrid(position.GetPositionX(), position.GetPositionY());
        Require(Create(m_DBTableGuid, row[1].GetUInt32(), &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "The actual catacombs object could not load");
    }

    ~CatacombsObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void UseRemains(Player& player, GameObject const& object)
{
    Require(player.CanContact() && player.GetUnitBeingMoved() == &player, "The native use handler requires the actual mover");
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << object.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

void CrossEntrance(Player& player, bool entered)
{
    WorldPackets::Misc::AreaTrigger packet{WorldPacket(CMSG_AREA_TRIGGER)};
    packet.AreaTriggerID = AmaniCatacombs::Entrance;
    packet.Entered = entered;
    player.GetSession()->HandleAreaTrigger(packet);
}

uint32 RemainingCharges(Player& player)
{
    Item* torch = player.GetItemByEntry(AmaniCatacombs::Torch);
    Require(torch, "The actual torch was removed before hand-in");
    for (ItemEffectEntry const* effect : torch->GetTemplate()->Effects)
        if (effect && effect->SpellID == AmaniCatacombs::BurnRemains)
        {
            Require(effect->Charges == 10, "The active torch effect has the wrong native maximum");
            return torch->GetSpellCharges(effect->LegacySlotIndex);
        }
    Require(false, "The actual torch burn effect is missing");
    return 0;
}

void CheckCatacombs(Quest const* quest)
{
    auto receiverData = WorldDatabase.Query("SELECT c.guid,c.position_x,c.position_y,c.position_z,c.orientation FROM creature c "
        "JOIN creature_queststarter s ON s.id=c.id AND s.quest=9193 JOIN creature_questender e ON e.id=c.id AND e.quest=9193 "
        "WHERE c.id=16289 AND c.guid=33091 AND c.map=530 AND c.zoneId=3433");
    auto remainsData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=181148 AND map=530 AND zoneId=3433 AND areaId=3510 ORDER BY guid");
    auto unrelatedData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE guid=11816 AND id=181665 AND map=530 AND zoneId=3433 AND areaId=3510");
    Require(receiverData && remainsData && remainsData->GetRowCount() == 25 && unrelatedData, "Missing Valwyn, the 25 real remains or the nearby catacombs object");

    Map map(530, 500, 0, DIFFICULTY_NONE);
    auto receiverRow = receiverData->Fetch();
    Position receiverPoint{receiverRow[1].GetFloat(), receiverRow[2].GetFloat(), receiverRow[3].GetFloat(), receiverRow[4].GetFloat()};
    map.LoadGrid(receiverPoint.GetPositionX(), receiverPoint.GetPositionY());
    Actor receiver(&map, Valwyn, receiverRow[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "The actual Valwyn did not load");
    CatacombsObject unrelated(map, unrelatedData->Fetch());
    std::vector<std::unique_ptr<CatacombsObject>> remains;
    do
    {
        remains.push_back(std::make_unique<CatacombsObject>(map, remainsData->Fetch()));
    } while (remainsData->NextRow());
    auto lock = sLockStore.LookupEntry(remains.front()->GetGOInfo()->GetLockId());
    Require(lock && lock->ID == 1638 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == AmaniCatacombs::Torch,
        "The original remains lock must require the actual torch");

    auto entrance = sAreaTriggerStore.LookupEntry(AmaniCatacombs::Entrance);
    Require(entrance && entrance->ContinentID == 530 && entrance->Radius == 20.0f, "Expected the original catacombs entrance geometry");
    Position entrancePoint{entrance->Pos.X, entrance->Pos.Y, entrance->Pos.Z, 0.0f};
    LootPlayer owner(SupplySession(), map, 919301, receiverPoint), other(SupplySession(), map, 919302, receiverPoint);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
        player->InitDisplayIds();
        player->SetLevel(17);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(RemainingCharges(owner) == 10, "Valwyn did not provide the actual ten-charge torch");
    auto explored = [&](Player& player) { return player.GetQuestObjectiveData(quest, quest->GetObjectives()[2].StorageIndex); };
    CrossEntrance(owner, true);
    Require(!explored(owner), "A remote entry was accepted by the native area radius");
#ifdef AMANI_CATACOMBS_BASELINE
    SupplyMove(owner, remains.front()->GetPosition());
    UseRemains(owner, *remains.front());
    Require(owner.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains) == 1 && RemainingCharges(owner) == 10,
        "The old direct click did not reproduce the skipped torch charge");
    SupplyMove(owner, receiverPoint);
    CrossEntrance(owner, false);
    Require(explored(owner) && owner.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false),
        "The old exit notification did not expose premature full completion");
    std::cout << "EXPECTED_BROKEN ordinary remains click spent no charge and the old area exit enabled rewards with one of ten burns\n";
#else
    CrossEntrance(owner, false);
    Require(!explored(owner), "An exit could complete the exploration without entering the real catacombs");
    auto& first = *remains.front();
    for (auto const& object : remains)
        Require(object->GetAreaId() == 3510, "An original remains placement is outside the native catacombs area");
    Require(first.ActivateToQuest(&owner) && !first.ActivateToQuest(&other), "An active torch and quest must control remains availability");
    other.AddQuestAndCheckCompletion(quest, &receiver);
    other.SetQuestObjectiveData(quest, &quest->GetObjectives()[0], 1);
    other.SetQuestObjectiveData(quest, &quest->GetObjectives()[2], 1);
    other.CompleteQuest(AmaniCatacombs::Quest);
    Require(!other.CanRewardQuest(quest, false), "A previously stored false completion could get the quest reward");
    AmaniCatacombs::RestoreIncompleteQuest(other);
    Require(other.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_INCOMPLETE && !explored(other) &&
        other.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains) == 1,
        "Login recovery kept the invalid completion or accepted the old unproven exploration");
    SupplyMove(other, entrancePoint);
    CrossEntrance(other, true);
    Require(explored(other) && other.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_INCOMPLETE,
        "The actual entrance must set only the exploration objective until the ten burns are complete");
    SupplyMove(other, first.GetPosition());

    SupplyMove(owner, unrelated.GetPosition());
    Item* torch = owner.GetItemByEntry(AmaniCatacombs::Torch);
    Require(owner.CastSpell(&unrelated, AmaniCatacombs::BurnRemains, false, torch) != SPELL_CAST_OK && RemainingCharges(owner) == 10,
        "The actual torch targeted a different nearby object");
    SupplyMove(owner, first.GetPosition());
    first.Use(&owner);
    Require(!owner.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains), "An ordinary internal use bypassed the actual torch");

    auto burn = [&](LootPlayer& player, uint32 firstPlacement, uint32 count, uint32 previouslyBurned)
    {
        for (uint32 offset = 0; offset < count; ++offset)
        {
            CatacombsObject& object = *remains[firstPlacement + offset];
            SupplyMove(player, object.GetPosition());
            UseRemains(player, object);
            Tick(player, 250);
            uint32 expected = previouslyBurned + offset + 1;
            std::cout << "Torch result=" << expected << " progress=" << player.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains)
                << " charges=" << RemainingCharges(player) << '\n';
            Require(player.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains) == expected && RemainingCharges(player) == 9 - offset,
                "Each distinct native burn must award once and spend one torch charge");
            UseRemains(player, object);
            Tick(player, 250);
            Require(player.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains) == expected && RemainingCharges(player) == 9 - offset,
                "The burning object could consume another charge or give duplicate progress");
        }
    };
    burn(owner, 0, 10, 0);
    Require(owner.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_INCOMPLETE && !explored(owner) && owner.GetItemCount(AmaniCatacombs::Torch) == 1,
        "Burning alone completed exploration or destroyed the spent return item");
    SupplyMove(owner, remains[10]->GetPosition());
    UseRemains(owner, *remains[10]);
    Require(!remains[10]->ActivateToQuest(&owner) && RemainingCharges(owner) == 0 &&
        other.GetQuestObjectiveData(AmaniCatacombs::Quest, AmaniCatacombs::Remains) == 1 && RemainingCharges(other) == 10,
        "A spent torch remained usable or another player received credit or lost a charge");

    SupplyMove(owner, entrancePoint);
    CrossEntrance(owner, true);
    Require(explored(owner) && owner.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_COMPLETE,
        "The real native entrance did not finish the last missing objective");
    SupplyMove(owner, receiverPoint);
    Require(owner.CanInteractWithQuestGiver(&receiver), "Valwyn cannot receive the visible completed quest");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = receiver.GetGUID();
    reward.QuestID = AmaniCatacombs::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(AmaniCatacombs::Quest) && !owner.GetItemCount(AmaniCatacombs::Torch) && !first.ActivateToQuest(&owner),
        "Valwyn's native turn-in kept the spent torch or enabled an old remains interaction");
    burn(other, 10, 9, 1);
    Require(other.GetQuestStatus(AmaniCatacombs::Quest) == QUEST_STATUS_COMPLETE && explored(other) && RemainingCharges(other) == 1,
        "The alternative entrance-first order did not finish from the final burn or changed old legitimate progress");
    AbandonSupply(other, AmaniCatacombs::Quest);
    Require(!other.GetItemCount(AmaniCatacombs::Torch) && !first.ActivateToQuest(&other), "Abandonment retained a partially used torch or an active object");
    std::cout << "PASS 25 actual placements, ten individual burns, actual exploration, spent torch and native Valwyn turn-in\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Amani reward requires the disposable character schema");
        auto quest = LoadCatacombsQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckCatacombs(quest);
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
