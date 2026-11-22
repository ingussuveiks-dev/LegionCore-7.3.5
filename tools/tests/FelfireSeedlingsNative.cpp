#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"

namespace
{
constexpr uint32 PlantingQuest = 26446;
constexpr uint32 LegacyPlantingQuest = 9526;
constexpr uint32 Seedlings = 23788;
constexpr uint32 PlantSpell = 29773;
constexpr uint32 Dirt = 181690;
constexpr uint32 Gnarl = 17310;

void LoadPlantingQuests()
{
    auto templates = WorldDatabase.Query(
#include "NativeFelfireSeedlings.inc"
    );
    Require(templates && templates->GetRowCount() == 2, "Missing the current and legacy planting quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(9526,26446)");
    Require(objectives && objectives->GetRowCount() == 2, "Expected the planting objectives for both quest versions");
    do
    {
        auto row = objectives->Fetch();
        Require(row[2].GetUInt8() == QUEST_OBJECTIVE_GAMEOBJECT && row[4].GetUInt32() == Dirt && row[5].GetUInt32() == 8,
            "Both quests must require eight real mound plantings");
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
    } while (objectives->NextRow());
    auto addons = WorldDatabase.Query(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN(9526,26446)");
    Require(addons && addons->GetRowCount() == 2, "Missing the original seedling provisioning");
    do
    {
        auto quest = quests[addons->Fetch()[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(addons->Fetch());
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
        Require(quest->SourceItemId == Seedlings && quest->SourceItemIdCount == 1 && !quest->PackageID,
            "Expected one eight-use bundle and no reward choice");
    } while (addons->NextRow());
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE felfire_seed_end AS SELECT * FROM creature_questender WHERE quest IN(9526,26446)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM felfire_seed_end");
#ifndef FELFIRE_SEEDLINGS_BASELINE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE felfire_seed_conditions AS SELECT * FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=29773");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM felfire_seed_conditions");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifndef FELFIRE_SEEDLINGS_BASELINE
    sConditionMgr->LoadConditions();
    Require(sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_SPELL, PlantSpell).size() == 6,
        "The current and legacy lock rules did not load");
#endif
}

struct PlantingObject : GameObject
{
    PlantingObject(Map& map, Field* row, uint32 entry)
    {
        m_DBTableGuid = row[0].GetUInt32();
        Position position{row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat()};
        G3D::Quat rotation(row[5].GetFloat(), row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat());
        Require(Create(m_DBTableGuid, entry, &map, 1, position, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the original Felfire object");
    }
    ~PlantingObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void ClickMound(LootPlayer& player, GameObject& mound)
{
    Require(player.CanContact() && player.GetUnitBeingMoved() == &player, "The real use packet requires the player's own mover");
    WorldPacket wire(CMSG_GAME_OBJ_USE);
    wire << mound.GetGUID();
    WorldPackets::GameObject::GameObjectUse packet(std::move(wire));
    packet.Read();
    player.GetSession()->HandleGameObjectUse(packet);
}

SpellCastResult Plant(LootPlayer& player, GameObject& mound)
{
    Item* item = player.GetItemByEntry(Seedlings);
    Require(item, "The test must use the real carried seedlings");
    return player.CastSpell(&mound, PlantSpell, false, item);
}

void CheckPlanting()
{
    auto gnarl = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=17310 AND map=1 AND zoneId=331 AND guid=111469");
    auto soil = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=181690 AND map=1 AND zoneId=331 AND areaId=434 ORDER BY guid LIMIT 9");
    auto unrelated = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=194458 AND guid=36173 AND map=1 AND zoneId=331");
    Require(gnarl && soil && soil->GetRowCount() == 9 && unrelated &&
        WorldDatabase.Query("SELECT 1 FROM creature_queststarter WHERE quest=26446 AND id=17310"),
        "Missing the actual Gnarl offer, Felfire mounds or unrelated world object");
    Map map(1, 520, 0, DIFFICULTY_NONE);
    auto row = gnarl->Fetch();
    Position receiverPoint{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    Actor receiver(&map, Gnarl, row[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Gnarl");
    LootPlayer owner(SupplySession(), map, 26446, receiverPoint), other(SupplySession(), map, 9526, receiverPoint);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->setFactionForRace(RACE_NIGHTELF);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    Quest const* quest = sQuestDataStore->GetQuestTemplate(PlantingQuest);
    Quest const* legacy = sQuestDataStore->GetQuestTemplate(LegacyPlantingQuest);
    owner.AddQuestAndCheckCompletion(quest, &receiver);
    Require(owner.GetItemCount(Seedlings) == 1 && owner.GetItemByEntry(Seedlings)->GetSpellCharges(0) == -8,
        "Acceptance should provide the original eight-use bundle");
    for (uint32 index = 0; index < 9; ++index, soil->NextRow())
    {
        PlantingObject mound(map, soil->Fetch(), Dirt);
        SupplyMove(owner, mound.GetPosition());
        SupplyMove(other, mound.GetPosition());
        if (index == 0)
        {
            auto lock = sLockStore.LookupEntry(mound.GetGOInfo()->GetLockId());
            Require(lock && lock->ID == 1661 && lock->Type[0] == LOCK_KEY_ITEM && lock->Index[0] == Seedlings,
                "The original terrain lock no longer requires the actual item");
            ClickMound(owner, mound);
#ifdef FELFIRE_SEEDLINGS_BASELINE
            std::cout << "CASE direct mound credit=" << owner.GetQuestObjectiveData(PlantingQuest, Dirt)
                << " charges=" << owner.GetItemByEntry(Seedlings)->GetSpellCharges(0)
                << " status=" << owner.GetQuestStatus(PlantingQuest) << " spawned=" << mound.isSpawned()
                << " flags=" << mound.GetUInt32Value(GAMEOBJECT_FIELD_FLAGS) << " state=" << mound.GetGoState()
                << " mapFound=" << (map.GetGameObject(mound.GetGUID()) == &mound) << " spectator=" << owner.IsSpectator() << '\n';
            Require(owner.GetQuestObjectiveData(PlantingQuest, Dirt) == 1 &&
                owner.GetItemByEntry(Seedlings)->GetSpellCharges(0) == -8, "Expected ordinary clicking to bypass the seed dose");
            std::cout << "EXPECTED_BROKEN direct mound use granted planting credit and spent no dose\n";
            return;
#endif
            Require(!owner.GetQuestObjectiveData(PlantingQuest, Dirt) && mound.ActivateToQuest(&owner) &&
                !mound.ActivateToQuest(&other), "Ordinary clicking changed the mound or quest activation leaked");
            GiveSupply(other, Seedlings, 1);
            Require(Plant(other, mound) != SPELL_CAST_OK, "A leftover bundle worked without the quest");
            other.DestroyItemCount(Seedlings, 1, true);
            {
                PlantingObject fire(map, unrelated->Fetch(), 194458);
                SupplyMove(owner, fire.GetPosition());
                Require(Plant(owner, fire) != SPELL_CAST_OK && owner.GetItemByEntry(Seedlings)->GetSpellCharges(0) == -8,
                    "A different actual Felfire object accepted the planting");
            }
            SupplyMove(owner, mound.GetPosition());
            Require(Plant(owner, mound) == SPELL_CAST_OK, "Could not start the native five-second planting");
            Tick(owner, 2400);
            AbandonSupply(owner, PlantingQuest);
            Require(!owner.GetItemCount(Seedlings) && !mound.ActivateToQuest(&owner), "Abandon left the item or mound interaction");
            owner.AddQuestAndCheckCompletion(quest, &receiver);
            Tick(owner, 5500);
            Require(!owner.GetQuestObjectiveData(PlantingQuest, Dirt) && owner.GetItemByEntry(Seedlings)->GetSpellCharges(0) == -8 &&
                mound.GetGoState() == GO_STATE_READY, "The old planting crossed into the new acceptance");
        }
        if (index == 8)
        {
            GiveSupply(owner, Seedlings, 1);
            ClickMound(owner, mound);
            Require(!mound.ActivateToQuest(&owner) && Plant(owner, mound) != SPELL_CAST_OK && mound.GetGoState() == GO_STATE_READY,
                "The completed quest could still use a retained bundle");
            other.AddQuestAndCheckCompletion(legacy, nullptr);
            Require(mound.ActivateToQuest(&other) && Plant(other, mound) == SPELL_CAST_OK, "A legacy accepted quest could not use the real lock");
            Tick(other, 5500);
            Require(other.GetQuestObjectiveData(LegacyPlantingQuest, Dirt) == 1, "The existing legacy quest did not receive its native credit");
            AbandonSupply(other, LegacyPlantingQuest);
            Require(!other.GetItemCount(Seedlings) && !mound.ActivateToQuest(&other), "Legacy abandonment left a bundle or object activation");
            break;
        }

        Require(Plant(owner, mound) == SPELL_CAST_OK, "The native planting cast rejected a live unspent mound");
        Tick(owner, 5500);
        Require(owner.GetQuestObjectiveData(PlantingQuest, Dirt) == index + 1, "The real plant did not advance the native object objective");
        if (index < 7)
        {
            Require(owner.GetItemByEntry(Seedlings) && owner.GetItemByEntry(Seedlings)->GetSpellCharges(0) == int32(index) - 7,
                "Expected exactly one dose to be consumed");
            Require(Plant(owner, mound) != SPELL_CAST_OK, "A shared mound in use accepted a second dose");
        }
        else
            Require(!owner.GetItemCount(Seedlings), "The eighth use did not consume the bundle");
    }

    SupplyMove(owner, receiverPoint);
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = PlantingQuest;
    packet.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(PlantingQuest) && !owner.GetItemCount(Seedlings), "The native handoff left the retained bundle");
    std::cout << "PASS eight actual planting casts, ordinary click refused, native dose use, legacy scope, reward and abandon cleanup\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The planting test requires the isolated character schema");
        LoadPlantingQuests();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPlanting();
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
