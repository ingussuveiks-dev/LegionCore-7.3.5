#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameEventMgr.h"
#include "GameObject.h"

namespace
{
constexpr uint32 Herb = 71035;
constexpr uint32 LootId = 39847;

struct HerbObject : GameObject
{
    ~HerbObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

void LoadPrecautions()
{
    auto rows = WorldDatabase.Query(
#include "NativeTakingPrecautionsQuest.inc"
    );
    Require(bool(rows), "Missing Taking Precautions rows");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID, QuestID, Type, StorageIndex, ObjectID, Amount, Flags, Flags2, TaskStep, Description, Bugged "
        "FROM quest_objectives WHERE QuestID IN (29402,29426) ORDER BY QuestID,StorageIndex");
    Require(bool(objectives), "Missing item objectives");
    do
    {
        auto fields = objectives->Fetch();
        quests[fields[1].GetUInt32()]->LoadQuestObjective(fields);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=39847";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1, "Native gameobject loot loader did not read Blood Nettle");

    auto row = WorldDatabase.Query("SELECT ConditionTypeOrReference,ConditionValue1 FROM conditions "
        "WHERE SourceTypeOrReferenceId=4 AND SourceGroup=39847 AND SourceEntry=71035");
    Require(bool(row) && row->GetRowCount() == 1, "Missing seasonal loot condition");
    auto condition = new Condition;
    condition->SourceType = CONDITION_SOURCE_TYPE_GAMEOBJECT_LOOT_TEMPLATE;
    condition->SourceGroup = LootId;
    condition->SourceEntry = Herb;
    condition->ConditionType = ConditionTypes(row->Fetch()[0].GetUInt32());
    condition->ConditionValue1 = row->Fetch()[1].GetUInt32();
    Require((sConditionMgr->*Access(AttachLootConditionTag{}))(condition, LootTemplates_Gameobject.GetLootForConditionFill(LootId)),
        "Native gameobject condition attachment failed");
}

void Give(LootPlayer& player, uint32 item, uint32 count)
{
    ItemPosCountVec slots;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, slots, item, count) == EQUIP_ERR_OK && player.StoreNewItem(slots, item, true),
        "Native inventory fixture could not receive an item");
}

bool VisibleLoot(LootPlayer& player, GameObject& plant)
{
    plant.loot.clear();
    plant.loot.FillLoot(LootId, LootTemplates_Gameobject, &player, true, true, &plant);
    WorldPackets::Loot::LootResponse response;
    plant.loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    return response.Items.size() == 1 && response.Items.front().Loot.ItemID == Herb && response.Items.front().Quantity == 1;
}

void CheckFaction(WorldSession* first, WorldSession* second, uint32 mapId, uint32 questId, uint8 race, Position const& pos)
{
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    Map map(mapId, 300, 9999995 + mapId, DIFFICULTY_NONE);
    LootPlayer owner(first, map, 100, pos);
    LootPlayer bystander(second, map, 101, pos);
    for (LootPlayer* player : {&owner, &bystander})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
        player->setFactionForRace(race);
    }
    HerbObject plant;
    Require(plant.Create(202611210 + mapId, 209059, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY), "Could not create native plant");
    Require(map.AddToMap(static_cast<GameObject*>(&plant)), "Native plant was not added to the map");
    active.insert(12);
    Require(plant.IsNeverVisible(&owner) && !VisibleLoot(owner, plant), "Unaccepted quest exposed the herb");
    owner.SetQuestStatus(questId, QUEST_STATUS_INCOMPLETE);
    owner.SetQuestSlot(0, questId);
    Require(!plant.IsNeverVisible(&owner) && plant.ActivateToQuest(&owner), "Accepted quest could not see the herb");
    Tick(owner, 150);
    Require(owner.HaveAtClient(&plant), "Accepting while stationary did not publish the plant");
    owner.RemoveActiveQuest(questId);
    owner.SetQuestSlot(0, 0);
    Tick(owner, 150);
    Require(!owner.HaveAtClient(&plant), "Abandoning without herbs while stationary did not hide the plant");
    owner.SetQuestStatus(questId, QUEST_STATUS_INCOMPLETE);
    owner.SetQuestSlot(0, questId);
    Require(plant.IsNeverVisible(&bystander) && !VisibleLoot(bystander, plant), "Bystander shared herb admission");
    active.erase(12);
    Require(plant.IsNeverVisible(&owner) && !VisibleLoot(owner, plant), "Stopped holiday still allowed loot");
    active.insert(12);

    for (uint32 count = 1; count <= 5; ++count)
    {
        Require(VisibleLoot(owner, plant), "Missing native loot while objective outstanding");
        owner.StoreLootItem(0, &plant.loot);
        Require(owner.GetItemCount(Herb) == count && owner.GetQuestObjectiveData(questId, Herb) == int32(count),
            "Loot did not advance the native item objective exactly once");
    }
    auto quest = sQuestDataStore->GetQuestTemplate(questId);
    Require(plant.IsNeverVisible(&owner) && !VisibleLoot(owner, plant), "Fifth herb did not close gathering");
    Tick(owner, 150);
    Require(!owner.HaveAtClient(&plant), "The fifth herb did not remove the stationary plant from the client update set");
    owner.DestroyItemCount(Herb, 1, true);
    Tick(owner, 150);
    Require(owner.HaveAtClient(&plant) && VisibleLoot(owner, plant), "Losing one herb did not make the plant visible again");
    owner.StoreLootItem(0, &plant.loot);
    Require(!owner.CanCompleteQuest(questId), "Missing reagents allowed completion");
    Give(owner, 10940, 2);
    Give(owner, 3371, 5);
    Require(owner.GetQuestStatus(questId) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false), "Native turn-in refused all three reagents");
    Require(owner.TakeQuestSourceItem(questId, false), "Native abandon item cleanup refused");
    owner.RemoveActiveQuest(questId);
    owner.SetQuestSlot(0, 0);
    Require(owner.GetItemCount(Herb) == 0 && owner.GetItemCount(10940) == 2 && owner.GetItemCount(3371) == 5,
        "Abandon did not consume just the herbs");
    Require(plant.IsNeverVisible(&owner) && !VisibleLoot(owner, plant), "Abandoned quest retained gathering");
    owner.SetQuestStatus(questId, QUEST_STATUS_INCOMPLETE);
    owner.SetQuestSlot(0, questId);
    Require(!plant.IsNeverVisible(&owner) && VisibleLoot(owner, plant), "Reaccepted quest did not regain gathering");
    active.erase(12);
    plant.loot.clear();
    std::cout << "PASS native visibility, holiday gate, five loot receipts, turn-in admission, abandon and retry for " << questId << '\n';
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}});
        LoadPrecautions();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Native login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto readOnly = LoginDatabase.Query("SELECT @@session.transaction_read_only");
        Require(readOnly && readOnly->Fetch()[0].GetBool(), "Native login pool is not read-only");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        // Socketless account-zero sessions survive until process exit to avoid account writes.
        auto first = new WorldSession(0, std::string("Native holiday owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto second = new WorldSession(0, std::string("Native holiday bystander"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckFaction(first, second, 0, 29402, RACE_HUMAN, {-8248.32f, 716.128f, 69.6695f, 0.382229f});
        CheckFaction(first, second, 1, 29426, RACE_ORC, {1891.91f, -4322.7f, 24.1831f, 0.470429f});
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
