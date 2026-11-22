#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "QuestItemObjectUse.h"

namespace
{
using namespace BloodcupBraid;
constexpr uint32 Gurtar = 34242;
constexpr uint32 Draaka = 33726;
constexpr uint32 Braid = 46314;
constexpr uint32 Prerequisite = 13873;

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

::Quest const* LoadBraidQuest()
{
    auto result = WorldDatabase.Query(
#include "NativeBloodcupBraid.inc"
    );
    Require(result && result->GetRowCount() == 2, "Gurtar's actual quest or prerequisite is missing");
    auto& templates = sQuestDataStore->*Access(QuestVectorTag{});
    templates.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(templates.size());
    do
    {
        auto quest = new ::Quest(result->Fetch());
        templates[quest->GetQuestId()] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->GetQuestId(), quest);
    } while (result->NextRow());
    auto quest = templates[BloodcupBraid::Quest];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13875");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Braid && objective->Fetch()[5].GetUInt32() == 1, "Expected one woven Bloodcup Braid for Draaka");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=13875");
    Require(addon && addon->Fetch()[4].GetUInt32() == Prerequisite && quest->SourceItemId == HairBraid &&
        quest->ItemDrop[1] == Flower && quest->ItemDropQuantity[1] == 8 && quest->ItemDrop[2] == HairBraid,
        "The original Gurtar handoff or reagent roles changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(quest->PrevQuestID);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = lootTable;
    lootTable += " WHERE Entry IN (194997,27108)";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = original;
    Require(loaded == 1 && !LootTemplates_Gameobject.HaveLootFor(Plant) && LootTemplates_Gameobject.HaveLootFor(27108),
        "The flower availability must use the actual loot link rather than an object-ID alias");

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE braid_starter AS SELECT * FROM creature_queststarter WHERE quest=13875");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM braid_starter");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE braid_receiver AS SELECT * FROM creature_questender WHERE quest=13875");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM braid_receiver");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

void AcceptBraid(Player& player, Creature const& giver)
{
    SupplyMove(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = BloodcupBraid::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void CheckBraid(::Quest const* quest)
{
    auto giverRow = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=112170 AND id=34242 AND map=1 AND zoneId=331");
    auto receiverRow = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE guid=111938 AND id=33726 AND map=1 AND zoneId=331");
    auto placements = WorldDatabase.Query("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,areaId "
        "FROM gameobject WHERE id=194997 AND map=1 AND zoneId=331 ORDER BY guid");
    Require(giverRow && receiverRow && placements && placements->GetRowCount() == 59, "Expected Gurtar, Draaka and all 59 original flowers");
    Map map(1, 503, 0, DIFFICULTY_NONE);
    Position giverPos = ReadPosition(giverRow->Fetch() + 1);
    Position receiverPos = ReadPosition(receiverRow->Fetch() + 1);
    map.LoadGrid(giverPos.GetPositionX(), giverPos.GetPositionY());
    map.LoadGrid(receiverPos.GetPositionX(), receiverPos.GetPositionY());
    Actor giver(&map, Gurtar, giverRow->Fetch()[0].GetUInt32(), giverPos);
    Actor receiver(&map, Draaka, receiverRow->Fetch()[0].GetUInt32(), receiverPos);
    Require(map.AddToMap(static_cast<Creature*>(&giver)) && map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Gurtar and Draaka");
    LootPlayer owner(SupplySession(), map, 1387501, giverPos), other(SupplySession(), map, 1387502, giverPos);
    for (LootPlayer* player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_ORC);
        player->setFactionForRace(RACE_ORC);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    }
    AcceptBraid(owner, giver);
    Require(!owner.GetItemCount(HairBraid), "Gurtar offered his braid before the prior quest");
    owner.SetRewardedQuest(Prerequisite);
    AcceptBraid(owner, giver);
    Require(owner.GetQuestStatus(BloodcupBraid::Quest) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(HairBraid) == 1 && owner.HasQuestForItem(Flower),
        "Gurtar did not give the original Orc-Hair Braid and start the flower collection");

    GiveSupply(other, HairBraid, 1);
    GiveSupply(other, Flower, 8);
    uint32 placementCount = 0;
    do
    {
        Field* row = placements->Fetch();
        Placement placement{row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(), ReadPosition(row + 3),
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())};
        SupplyMove(owner, placement.position);
        SupplyMove(other, placement.position);
        Supply plant(map, placement);
        Require(plant.GetAreaId() == row[11].GetUInt32() && plant.GetZoneId() == 331 && plant.GetGOInfo()->GetLootId() == 27108,
            "A real flower did not resolve its Ashenvale placement and loot link");
#ifdef BLOODCUP_BRAID_BASELINE
        Require(plant.ActivateToQuest(&owner) && !plant.ActivateToQuest(&other), "The existing explicit flower marker changed");
        OpenSupply(other, plant);
        Require(other.GetLootGUID() == plant.GetGUID() && plant.getLootState() == GO_ACTIVATED && plant.loot.isLooted(),
            "The old keyless flower did not reproduce an empty opening by an inactive player");
        Require(other.CastSpell(&other, Weave, false, other.GetItemByEntry(HairBraid)) == SPELL_CAST_OK, "The old out-of-quest weave no longer starts");
        Tick(other, 3000);
        Require(other.GetItemCount(Braid) == 1, "The old out-of-quest recipe did not produce the turn-in braid");
        std::cout << "EXPECTED_BROKEN an inactive player opened the flower and wove a braid without the quest\n";
        return;
#else
        bool needsFlowers = owner.GetItemCount(Flower) < 8;
        Require(plant.ActivateToQuest(&owner) == needsFlowers && !plant.ActivateToQuest(&other), "Flower marker does not follow the actual quest and reagent cap");
        if (!placementCount)
        {
            Require(other.CastSpell(&plant, 3365, false) != SPELL_CAST_OK, "An inactive player started opening the flower");
            other.SendLoot(plant.GetGUID(), LOOT_SKINNING);
            Require(!other.GetLootGUID() && plant.getLootState() == GO_READY, "An inactive player changed the flower stock through the loot path");
            Require(other.CastSpell(&other, Weave, false, other.GetItemByEntry(HairBraid)) != SPELL_CAST_OK &&
                owner.CastSpell(&owner, Weave, false, owner.GetItemByEntry(HairBraid)) != SPELL_CAST_OK, "The recipe accepted missing reagents or an inactive quest");
        }
        if (needsFlowers)
        {
            OpenSupply(owner, plant);
            TakeSupply(owner, plant, Flower);
            ReleaseSupply(owner, plant);
            Sleep(1600);
            GameTime::UpdateGameTimers();
        }
        ++placementCount;
#endif
    } while (placements->NextRow());
    Require(placementCount == 59 && owner.GetItemCount(Flower) == 8 && !owner.HasQuestForItem(Flower), "Eight genuine flower pickups did not cap the collection");
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < owner.GetInventoryEndSlot(); ++slot)
        if (!owner.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ItemPosCountVec slotPosition;
            slotPosition.emplace_back(uint16(INVENTORY_SLOT_BAG_0 << 8) | slot, 1);
            Require(owner.StoreNewItem(slotPosition, 2589, true), "Could not fill the actual backpack for the native recipe check");
        }
    Require(owner.CastSpell(&owner, Weave, false, owner.GetItemByEntry(HairBraid)) != SPELL_CAST_OK &&
        owner.GetItemCount(Flower) == 8 && owner.GetItemCount(HairBraid) == 1 && !owner.GetItemCount(Braid), "A full backpack consumed the authentic recipe components");
    owner.DestroyItemCount(2589, owner.GetItemCount(2589), true);
    Require(owner.CastSpell(&owner, Weave, false) != SPELL_CAST_OK, "A direct spell without the original item wove a braid");
    Require(owner.CastSpell(&owner, Weave, false, owner.GetItemByEntry(HairBraid)) == SPELL_CAST_OK, "The real braid and eight flowers did not start weaving");
    Tick(owner, 1000);
    Require(owner.GetItemCount(Flower) == 8 && owner.GetItemCount(HairBraid) == 1 && !owner.GetItemCount(Braid), "The native recipe consumed components before its cast completed");
    Tick(owner, 2000);
    Require(!owner.GetItemCount(Flower) && !owner.GetItemCount(HairBraid) && owner.GetItemCount(Braid) == 1 &&
        !other.GetItemCount(Braid) && owner.GetQuestStatus(BloodcupBraid::Quest) == QUEST_STATUS_COMPLETE, "The native self-use did not make one braid from exactly the original materials");

    GiveSupply(owner, Flower, 1);
    GiveSupply(owner, HairBraid, 1);
    SupplyMove(owner, receiverPos);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (QuestPackageItemEntry const* item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); fallback && !fallback->empty())
            choice = fallback->front()->ItemID;
    Require(quest->PackageID == 1643 && choice && owner.CanInteractWithQuestGiver(&receiver) && owner.CanRewardQuest(quest, choice, false),
        "Expected Draaka's genuine class reward");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = receiver.GetGUID();
    reward.QuestID = BloodcupBraid::Quest;
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(BloodcupBraid::Quest) && !owner.GetItemCount(Braid) && !owner.GetItemCount(Flower) && !owner.GetItemCount(HairBraid),
        "Draaka did not take the woven braid and clean up spare materials");

    other.DestroyItemCount(HairBraid, 1, true);
    other.DestroyItemCount(Flower, 8, true);
    other.SetRewardedQuest(Prerequisite);
    AcceptBraid(other, giver);
    GiveSupply(other, Flower, 8);
    Require(other.CastSpell(&other, Weave, false, other.GetItemByEntry(HairBraid)) == SPELL_CAST_OK, "The second quest attempt could not start weaving");
    Tick(other, 1000);
    AbandonSupply(other, BloodcupBraid::Quest);
    Require(!other.GetItemCount(Flower) && !other.GetItemCount(HairBraid), "The abandoned attempt retained reagents");
    AcceptBraid(other, giver);
    GiveSupply(other, Flower, 8);
    Tick(other, 2200);
    Require(!other.GetItemCount(Braid) && other.GetItemCount(Flower) == 8 && other.GetItemCount(HairBraid) == 1,
        "A cancelled cast wove or consumed materials from a new acceptance");
    Sleep(1800);
    GameTime::UpdateGameTimers();
    Require(other.CastSpell(&other, Weave, false, other.GetItemByEntry(HairBraid)) == SPELL_CAST_OK, "The new quest could not weave its own braid");
    Tick(other, 3000);
    Require(other.GetItemCount(Braid) == 1 && other.GetQuestStatus(BloodcupBraid::Quest) == QUEST_STATUS_COMPLETE,
        "The new quest did not actually craft a braid before abandonment");
    AbandonSupply(other, BloodcupBraid::Quest);
    Require(!other.GetItemCount(Braid), "Abandoning Gurtar's completed quest kept the woven output");
    std::cout << "PASS actual flower loot markers and 59 placements, eight pickups, scoped material use, cancelled repeat and Draaka reward\n";
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
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The reward requires an isolated character schema");
        auto quest = LoadBraidQuest();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckBraid(quest);
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
