#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameTime.h"
#include <thread>

void AddSC_ghostlands();

namespace
{
constexpr uint32 SeasoningQuest = 9275;
constexpr uint32 PoisonItem = 22796;
constexpr uint32 PoisonSpell = 29517;

Quest const* LoadQuest()
{
    auto rows = WorldDatabase.Query(
#include "NativeSeasoningQuest.inc"
    );
    Require(bool(rows) && rows->GetRowCount() == 1, "Missing native Seasoning quest");
    auto quest = new Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    quests[SeasoningQuest] = quest;
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size() - 1);
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->Id] = quest;
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=9275 ORDER BY StorageIndex");
    Require(bool(objectives) && objectives->GetRowCount() == 3, "Missing native rack objectives");
    do
        quest->LoadQuestObjective(objectives->Fetch());
    while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_CAST);
    quest->SourceItemIdCount = 1;
    return quest;
}

Position Placement(uint32 entry)
{
    auto rows = WorldDatabase.PQuery("SELECT position_x,position_y,position_z,orientation FROM gameobject WHERE map=530 AND id=%u", entry);
    Require(bool(rows) && rows->GetRowCount() == 1, "Expected one native Ghostlands object placement");
    auto row = rows->Fetch();
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

struct Rack : GameObject
{
    ~Rack()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        if (FindMap())
            ResetMap();
    }

    void Spawn(Map& map, uint32 entry, Position const& pos)
    {
        auto guid = sObjectMgr->GetGenerator<HighGuid::GameObject>()->Generate();
        Require(Create(guid, entry, &map, 1, pos, G3D::Quat(0, 0, 0, 1), 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && map.GetGameObject(GetGUID()) == this, "Rack failed to enter the world");
        // Preserve the real animation timer without waiting through the shared DB respawn delay in the probe.
        SetRespawnDelayTime(0);
        Update(10000);
    }
};

void Accept(LootPlayer& player)
{
    player.SetQuestStatus(SeasoningQuest, QUEST_STATUS_INCOMPLETE);
    player.SetQuestSlot(0, SeasoningQuest);
}

Item* AddPoison(LootPlayer& player)
{
    ItemPosCountVec dest;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, PoisonItem, 1) == EQUIP_ERR_OK, "Cannot store original poison");
    auto item = player.StoreNewItem(dest, PoisonItem, true);
    Require(item && item->GetSpellCharges(0) == -3, "Poison does not have its original three doses");
    return item;
}

void Cast(LootPlayer& player, GameObject& rack, Item* item)
{
    player.CastSpell(&rack, PoisonSpell, false, item);
    Tick(player, 5000);
}

void WaitForRack(Rack& rack)
{
    std::this_thread::sleep_for(std::chrono::seconds(4));
    GameTime::UpdateGameTimers();
    rack.Update(10000);
    rack.Update(10000);
    rack.Update(10000);
    Require(rack.isSpawned() && !rack.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE) && rack.GetGoState() == GO_STATE_READY,
        "Native shared animation did not reset");
}

void CheckRacks(Quest const* quest, WorldSession* first, WorldSession* other)
{
    GameTime::UpdateGameTimers();
    Position rawPosition = Placement(181250);
    Map map(530, 300, 9999978, DIFFICULTY_NONE);
    LootPlayer owner(first, map, 100, rawPosition), second(other, map, 101, rawPosition);
    for (LootPlayer* player : {&owner, &second})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_BLOODELF);
        player->setFactionForRace(RACE_BLOODELF);
    }
    Rack raw, smoke, fish, wrong;
    raw.Spawn(map, 181250, rawPosition);
    smoke.Spawn(map, 181251, Placement(181251));
    fish.Spawn(map, 181252, Placement(181252));
    // Bring the genuine Aquantion altar template to the user to test target rejection.
    wrong.Spawn(map, 181157, rawPosition);
    auto poison = AddPoison(owner);

#ifdef SEASONING_UNGUARDED_BASELINE
    Require(raw.GetGOInfo()->goober.questID == 0 && raw.GetGOInfo()->GetLockId() == 1655, "Counterfactual requires the original rack data");
    Accept(owner);
    Cast(owner, raw, poison);
    Require(poison->GetSpellCharges(0) == -2 && owner.GetQuestObjectiveData(SeasoningQuest, 181250) == 1, "Baseline first dose did not poison the rack");
    WaitForRack(raw);
    Cast(owner, raw, poison);
    Require(poison->GetSpellCharges(0) == -1 && owner.GetQuestObjectiveData(SeasoningQuest, 181250) == 1, "Baseline did not reproduce a wasted duplicate dose");
    Accept(second);
    auto const& smokePosition = smoke.GetPosition();
    map.PlayerRelocation(&second, smokePosition.GetPositionX(), smokePosition.GetPositionY(), smokePosition.GetPositionZ(), 0.0f);
    smoke.Use(&second);
    Require(second.GetQuestObjectiveData(SeasoningQuest, 181251) == 1 && !second.GetItemCount(PoisonItem), "Baseline did not reproduce direct-use credit without poison");
    std::cout << "EXPECTED_BROKEN duplicate native cast wasted a dose and direct Use credited without poison\n";
#else
    Require(raw.GetGOInfo()->goober.questID == SeasoningQuest && raw.GetGOInfo()->GetLockId() == 1655, "Repaired fixture did not load the real lock and quest gate");
    Cast(owner, raw, poison);
    Require(poison->GetSpellCharges(0) == -3, "A player without the quest spent a dose");
    Accept(owner);
    Accept(second);
    auto otherPoison = AddPoison(second);
    raw.Use(&owner);
    raw.AI()->GossipHello(&owner, false);
    Cast(owner, raw, nullptr);
    Cast(owner, wrong, poison);
    Require(!owner.GetQuestObjectiveData(SeasoningQuest, 181250) && poison->GetSpellCharges(0) == -3 &&
        !raw.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Direct, itemless or wrong target use changed the rack or charges");
    Require(owner.CastSpell(&raw, PoisonSpell, false, poison) == SPELL_CAST_OK &&
        second.CastSpell(&raw, PoisonSpell, false, otherPoison) == SPELL_CAST_OK, "Two players could not begin the native cast on a ready rack");
    Tick(owner, 5000);
    Tick(second, 5000);
    Require(poison->GetSpellCharges(0) == -2 && owner.GetQuestObjectiveData(SeasoningQuest, 181250) == 1 &&
        raw.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Native opening did not advance the first objective and animate the rack");
    Require(!second.GetQuestObjectiveData(SeasoningQuest, 181250) && otherPoison->GetSpellCharges(0) == -3,
        "The second pending cast charged poison or gave credit on the now-busy rack");
    Cast(second, raw, otherPoison);
    Require(otherPoison->GetSpellCharges(0) == -3, "Busy rack consumed another player's dose");
    WaitForRack(raw);
    Cast(owner, raw, poison);
    Require(poison->GetSpellCharges(0) == -2 && !raw.HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_IN_USE), "Repeated rack consumed the owner's dose after the native reset");
    Cast(second, raw, otherPoison);
    Require(otherPoison->GetSpellCharges(0) == -2 && second.GetQuestObjectiveData(SeasoningQuest, 181250) == 1,
        "The reset rack was not available to the second player");
    Require(second.TakeQuestSourceItem(SeasoningQuest, false) && !second.GetItemCount(PoisonItem), "Abandon did not remove remaining poison");
    second.RemoveActiveQuest(SeasoningQuest);
    second.SetQuestSlot(0, 0);
    Accept(second);
    Require(second.GiveQuestSourceItem(quest) && second.GetItemCount(PoisonItem) == 1 && !second.GetQuestObjectiveData(SeasoningQuest, 181250),
        "Clean reaccept did not restart the poison and objective state");

    for (Rack* rack : {&smoke, &fish})
    {
        auto const& pos = rack->GetPosition();
        map.PlayerRelocation(&owner, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), 0.0f);
        Cast(owner, *rack, poison);
        Require(owner.GetQuestObjectiveData(SeasoningQuest, rack->GetEntry()) == 1, "A distinct rack failed to give native credit");
    }
    Require(owner.GetQuestStatus(SeasoningQuest) == QUEST_STATUS_COMPLETE && owner.CanRewardQuest(quest, false) && !owner.GetItemCount(PoisonItem),
        "Three distinct racks did not finish the quest and consume precisely three doses");
    std::cout << "PASS native poison charges, original lock and three objective updates, direct-use rejection, shared reset, abandon and reaccept\n";
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
        Quest const* quest = LoadQuest();
        sObjectMgr->LoadItemTemplates();
        sObjectMgr->LoadItemTemplateAddon();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_ghostlands();
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        auto first = new WorldSession(0, std::string("Seasoning owner"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        auto other = new WorldSession(0, std::string("Seasoning other"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
        CheckRacks(quest, first, other);
        LoginDatabase.Close();
        HotfixDatabase.Close();
        WorldDatabase.Close();
        return 0;
    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
