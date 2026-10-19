// Real quest packet/handler, inventory, reward, item-use and collection paths.
// Initial players and defeated questgiver pose are fixtures; persistence uses
// schema-only disposable character/auth databases, never personal characters.
#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "QuestPackets.h"
#include "CollectionMgr.h"
#include "Bag.h"
#include <thread>

struct EndersTag { using Type = QuestRelations QuestDataStoreMgr::*; friend Type Access(EndersTag); };
template struct FixtureAccess<EndersTag, &QuestDataStoreMgr::_creatureQuestInvolvedRelations>;
struct InventoryLoadTag { using Type = void (Player::*)(PreparedQueryResult, PreparedQueryResult, uint32); friend Type Access(InventoryLoadTag); };
template struct FixtureAccess<InventoryLoadTag, &Player::_LoadInventory>;
struct RewardedLoadTag { using Type = void (Player::*)(PreparedQueryResult); friend Type Access(RewardedLoadTag); };
template struct FixtureAccess<RewardedLoadTag, &Player::_LoadQuestStatusRewarded>;

static void FlushWrites()
{
    bool done = false;
    auto callback = CharacterDatabase.AsyncQuery("SELECT 1").WithCallback([&](QueryResult r) { Require(bool(r), "Scratch barrier failed"); done = true; });
    for (unsigned i = 0; !done && i < 10000; ++i) { callback.InvokeIfReady(); if (!done) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    Require(done, "Scratch writes timed out");
}
static PreparedQueryResult ReadPrepared(CharacterDatabasePreparedStatement* stmt)
{
    bool done = false; PreparedQueryResult result;
    auto callback = CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback([&](PreparedQueryResult r) { result = r; done = true; });
    for (unsigned i = 0; !done && i < 10000; ++i) { callback.InvokeIfReady(); if (!done) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    Require(done, "Native prepared load timed out"); return result;
}
static void OpenScratch()
{
    auto name = sConfigMgr->GetStringDefault("NativeRollScratchDatabase", "");
    auto connection = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
    Require(name.size() == 37 && name.find("codex_roll_lifecycle_") == 0 && connection.substr(connection.find_last_of(';') + 1) == name, "Refusing non-scratch character DB");
    CharacterDatabase.SetConnectionInfo(connection, 1, 1);
    Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Scratch character pool failed");
    auto rows = CharacterDatabase.Query("SELECT DATABASE(),(SELECT COUNT(*) FROM characters)");
    Require(rows && rows->Fetch()[0].GetString() == name && !rows->Fetch()[1].GetUInt32(), "Scratch identity/empty check failed");
    name = sConfigMgr->GetStringDefault("NativeRollScratchAuth", "");
    connection = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
    Require(name.size() == 32 && name.find("codex_roll_auth_") == 0 && connection.substr(connection.find_last_of(';') + 1) == name, "Refusing non-scratch auth DB");
    LoginDatabase.SetConnectionInfo(connection, 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Scratch auth pool failed");
}
static WorldSession* Session()
{ return new WorldSession(0, "Turnin", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {}); }
static uint32 playerId = 500;
static uint32 Rewards[] = {143518, 143514, 143510, 143506};
static uint32 Sets[] = {183, 179, 175, 171};
static Quest const* quest;
struct TurnInPlayer : LootPlayer
{
    TurnInPlayer(Map& map, uint8 cls, uint8 race, uint32 guid = 0) : LootPlayer(Session(), map, guid ? guid : ++playerId, {5478,470,150,0})
    {
        SetName("Turn" + std::to_string(GetGUIDLow()));
        SetQuestStatus(44746, QUEST_STATUS_NONE);
        SetByteValue(UNIT_FIELD_BYTES_0, 1, cls); SetByteValue(UNIT_FIELD_BYTES_0, 0, race); setFactionForRace(race);
        // Native appearance admission needs the character's trained armor skill.
        SetSkill(415, 1, 1, 1); SetSkill(414, 1, 1, 1); SetSkill(413, 1, 1, 1); SetSkill(293, 1, 1, 1);
    }
    void Begin() { AddQuestAndCheckCompletion(quest, nullptr); }
    void Put(uint32 item, uint32 count)
    {
        ItemPosCountVec dest;
        Require(CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item, count) == EQUIP_ERR_OK, "Native inventory placement rejected");
        Require(StoreNewItem(dest, item, true), "Native item storage failed");
    }
};
static uint32 Expected(TurnInPlayer& p)
{
    auto entries = sDB2Manager.GetQuestPackageItems(18559); Require(entries, "Missing native package18559");
    uint32 item = 0;
    for (auto entry : *entries) if (p.CanSelectQuestPackageItem(entry)) { Require(!item, "Ambiguous class package"); item = entry->ItemID; }
    Require(item, "No eligible class reward"); return item;
}
static void Choose(TurnInPlayer& p, Actor& giver, uint32 item)
{
    WorldPacket wire(CMSG_QUEST_GIVER_CHOOSE_REWARD);
    wire << giver.GetGUID() << int32(45088) << int32(item);
    WorldPackets::Quest::QuestGiverChooseReward packet(std::move(wire)); packet.Read();
    p.GetSession()->HandleQuestGiverChooseReward(packet);
    p.Unit::Update(200); FlushWrites();
}
static void CheckEmpty(Map& map, Actor& giver)
{
    TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); p.Put(142487, 1000); p.Begin();
    Require(p.GetQuestStatus(45088) == QUEST_STATUS_COMPLETE, "Native item objective did not complete");
    Choose(p, giver, 0);
#ifdef LOST_ARMY_TURNIN_BASELINE
    Require(p.GetQuestRewardStatus(45088) && !p.GetItemCount(142487) && !p.GetItemCount(Expected(p)), "Baseline empty choice no longer loses reward");
#else
    Require(!p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 1000 && !p.GetItemCount(Expected(p)), "Empty reward choice consumed quest/fragments");
#endif
    std::cout << "PASS: native empty-choice packet checked; baseline requires completed quest with no ensemble, current preserves retry.\n";
}
static void CheckValid(Map& map, Actor& giver, uint8 cls, uint8 race)
{
    TurnInPlayer p(map, cls, race); auto reward = Expected(p);
    p.Put(142487, 1007); p.Begin();
    Require(p.CanRewardQuest(quest, reward, false), "Native valid turn-in admission failed");
    Choose(p, giver, reward);
    Require(p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 7 && p.GetItemCount(reward) == 1, "Native reward lost fragment excess or class ensemble");
    Choose(p, giver, reward);
    Require(p.GetItemCount(reward) == 1 && p.GetItemCount(142487) == 7, "Repeated reward duplicated ensemble/consumption");
    std::cout << "PASS: native class=" << uint32(cls) << " race=" << uint32(race) << " giver=" << giver.GetEntry() << " reward=" << reward << " excess=7 repeat=denied\n";
}
static void CheckStorage(Map& map, Actor& giver)
{
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); p.Put(142487, 999); p.Begin();
        Choose(p, giver, Expected(p));
        Require(!p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 999, "Under-quota turn-in consumed items");
    }
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN);
        auto fragments = Item::CreateItem(142487, 1000, &p); Require(fragments, "Bank fragment creation failed");
        ItemPosCountVec bank; bank.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | BANK_SLOT_ITEM_START, 1000);
        Require(p.BankItem(bank, fragments, true), "Native bank storage failed"); p.Begin();
        Require(p.GetItemCount(142487, true) == 1000 && !p.GetItemCount(142487), "Bank-only native counters changed");
        Choose(p, giver, Expected(p));
        Require(!p.GetQuestRewardStatus(45088) && p.GetItemCount(142487, true) == 1000, "Bank-only packet consumed quest/fragments");
    }
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN);
        auto bag = Item::CreateItem(21841, 1, &p); Require(bag && bag->IsBag(), "Native bag missing");
        ItemPosCountVec bagPosition; bagPosition.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | INVENTORY_SLOT_BAG_START, 1);
        Require(p.StoreItem(bagPosition, bag, true), "Native bag placement failed");
        ItemPosCountVec dest; Require(p.CanStoreNewItem(INVENTORY_SLOT_BAG_START, 0, dest, 142487, 1000) == EQUIP_ERR_OK, "Bag fragment admission failed");
        Require(p.StoreNewItem(dest, 142487, true), "Bag fragment placement failed"); p.Begin();
        Choose(p, giver, Expected(p));
        Require(p.GetQuestRewardStatus(45088) && !p.GetItemCount(142487, true) && p.GetItemCount(Expected(p)) == 1, "Native bag-contained objective/reward failed");
    }
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); p.Put(142487, 1007); p.Begin();
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot) if (!p.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        { ItemPosCountVec dest; dest.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | slot, sObjectMgr->GetItemTemplate(2589)->GetMaxStackSize()); Require(p.StoreNewItem(dest, 2589, true), "Full inventory fixture failed"); }
        Require(!p.CanRewardQuest(quest, Expected(p), false), "Native full inventory was admitted");
        Choose(p, giver, Expected(p));
        Require(!p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 1007, "Full inventory consumed fragments");
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot) if (auto item = p.GetItemByPos(INVENTORY_SLOT_BAG_0, slot)) if (item->GetEntry() == 2589)
        { p.DestroyItem(INVENTORY_SLOT_BAG_0, slot, true); break; }
        Choose(p, giver, Expected(p));
        Require(p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 7 && p.GetItemCount(Expected(p)) == 1, "Freed-slot retry failed");
    }
    std::cout << "PASS: native under-quota, bank-only refusal, bag-contained turn-in and full-inventory retry preserve items.\n";
}
static void CheckChoiceVariants(Map& map, Actor& giver)
{
    // Private reward metadata variants, never written to World/Hotfix. Keep
    // the actual packet, interaction, admission and reward handlers intact.
    auto mutableQuest = const_cast<Quest*>(quest); auto package = mutableQuest->PackageID;
    mutableQuest->PackageID = 0;
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); p.Put(142487, 1000); p.Begin();
        Choose(p, giver, 0);
        Require(p.GetQuestRewardStatus(45088) && !p.GetItemCount(142487), "Zero choice blocked a quest with no selectable items");
    }
    mutableQuest->m_rewChoiceItemsCount = 1; mutableQuest->RewardChoiceItemId[0] = 143518; mutableQuest->RewardChoiceItemCount[0] = 1;
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); p.Put(142487, 1000); p.Begin(); Choose(p, giver, 0);
#ifdef LOST_ARMY_TURNIN_BASELINE
        Require(p.GetQuestRewardStatus(45088) && !p.GetItemCount(143518), "Baseline explicit empty choice changed");
#else
        Require(!p.GetQuestRewardStatus(45088) && p.GetItemCount(142487) == 1000, "Explicit empty choice consumed items");
        Choose(p, giver, 143518);
        Require(p.GetQuestRewardStatus(45088) && p.GetItemCount(143518) == 1, "Explicit choice retry failed");
#endif
    }
    mutableQuest->m_rewChoiceItemsCount = 0; mutableQuest->RewardChoiceItemId[0] = 0; mutableQuest->RewardChoiceItemCount[0] = 0; mutableQuest->PackageID = package;
    std::cout << "PASS: native explicit-choice and no-selectable-reward variants retain valid zero-choice behavior and retry.\n";
}
static void CheckSavedStorage(Map& map, Actor& giver, bool bankOnly)
{
    uint32 guid;
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN); guid = p.GetGUIDLow();
        ItemPosCountVec dest;
        if (bankOnly)
        {
            auto item = Item::CreateItem(142487, 1000, &p); Require(item, "Saved bank item creation failed");
            dest.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | BANK_SLOT_ITEM_START, 1000); Require(p.BankItem(dest, item, true), "Saved bank item placement failed");
        }
        else
        {
            auto bag = Item::CreateItem(21841, 1, &p); Require(bag && bag->IsBag(), "Saved bag creation failed");
            dest.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | INVENTORY_SLOT_BAG_START, 1); Require(p.StoreItem(dest, bag, true), "Saved bag placement failed"); dest.clear();
            Require(p.CanStoreNewItem(INVENTORY_SLOT_BAG_START, 0, dest, 142487, 1000) == EQUIP_ERR_OK && p.StoreNewItem(dest, 142487, true), "Saved bag fragment placement failed");
        }
        p.Begin(); p.SaveToDB(true); FlushWrites();
    }
    {
        TurnInPlayer p(map, CLASS_WARRIOR, RACE_HUMAN, guid);
        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY); stmt->setUInt64(0, guid);
        (p.*Access(InventoryLoadTag{}))(ReadPrepared(stmt), {}, 0);
        // Real AddQuest scans the actual loaded inventory. This is a scoped
        // reacceptance fixture, not authenticated login/quest-log loading.
        p.Begin(); Require(p.GetItemCount(142487, true) == 1000, "Saved native bank/bag fragment count lost");
        Choose(p, giver, Expected(p));
        Require(p.GetQuestRewardStatus(45088) == !bankOnly, "Saved native bank/bag turn-in admission changed");
        Require(p.GetItemCount(142487, true) == (bankOnly ? 1000u : 0u), "Saved native bank/bag consumption changed");
        std::cout << "PASS: actual SaveToDB/prepared inventory reload retains " << (bankOnly ? "bank-only fragments and refuses turn-in" : "bag-contained fragments and permits turn-in") << ".\n";
    }
}
static void CheckPersistence(Map& map, Actor& giver, uint8 cls)
{
    uint32 guid, reward, set;
    {
        TurnInPlayer p(map, cls, RACE_HUMAN); guid = p.GetGUIDLow(); reward = Expected(p);
        auto index = std::find(std::begin(Rewards), std::end(Rewards), reward) - std::begin(Rewards); Require(index < 4, "Unknown ensemble"); set = Sets[index];
        p.Put(142487, 1007); p.Begin(); p.SaveToDB(true); FlushWrites();
        Choose(p, giver, reward);
        auto item = p.GetItemByEntry(reward); Require(item && item->GetTemplate()->Effects.size() == 1 && item->GetTemplate()->Effects.front()->Charges < 0, "Missing native consumable ensemble use");
        SpellCastTargets targets; targets.SetUnitTarget(&p); int32 misc[2]{};
        p.CastItemUseSpell(item, targets, misc, ObjectGuid::Empty);
        for (uint32 time = 0; time < 3000; time += 10) p.Unit::Update(10);
        auto appearances = sDB2Manager.GetTransmogSetItems(set); Require(appearances && appearances->size() == 9, "Native appearance set changed");
        for (auto appearance : *appearances) Require(p.GetCollectionMgr()->HasTransmog(appearance->ItemModifiedAppearanceID), "Native ensemble use failed to learn an appearance");
        Require(!p.GetItemCount(reward), "Native ensemble use failed to consume the item");
        p.SaveToDB(); FlushWrites();
        std::cout << "PASS: native ensemble use learned nine appearances and saved quest/inventory/collection; class=" << uint32(cls) << '\n';
    }
    {
        TurnInPlayer p(map, cls, RACE_HUMAN, guid);
        auto stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_INVENTORY); stmt->setUInt64(0, guid);
        (p.*Access(InventoryLoadTag{}))(ReadPrepared(stmt), {}, 0);
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_QUESTSTATUSREW); stmt->setUInt32(0, 0); stmt->setUInt64(1, guid);
        (p.*Access(RewardedLoadTag{}))(ReadPrepared(stmt));
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_TRANSMOGS); stmt->setUInt32(0, 0);
        Require(p.GetCollectionMgr()->LoadFromDB({}, {}, ReadPrepared(stmt), {}, {}), "Native collection reload failed");
        Require(p.GetQuestRewardStatus(45088) && p.GetItemCount(142487, true) == 7 && !p.GetItemCount(reward), "Native saved quest/inventory reload lost state");
        auto appearances = sDB2Manager.GetTransmogSetItems(set);
        for (auto appearance : *appearances) Require(p.GetCollectionMgr()->HasTransmog(appearance->ItemModifiedAppearanceID), "Saved ensemble appearance missing on native reload");
        auto before = p.GetItemCount(reward); Choose(p, giver, reward);
        Require(p.GetItemCount(reward) == before && p.GetItemCount(142487, true) == 7, "Reload allowed duplicate reward");
        std::cout << "PASS: native prepared inventory/rewarded-quest/collection reload retains state and denies repeat; class=" << uint32(cls) << '\n';
    }
}
static int Run(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs(); OpenScratch();
    quest = sQuestDataStore->GetQuestTemplate(45088);
    Require(quest && quest->PackageID == 18559 && !quest->IsWorld() && !quest->m_rewChoiceItemsCount, "Live quest reward structure changed");
    auto rows = WorldDatabase.Query("SELECT id,quest FROM creature_questender WHERE quest=45088 ORDER BY id");
    Require(bool(rows), "Missing live quest enders"); unsigned count = 0;
    do { (sQuestDataStore->*Access(EndersTag{})).emplace(rows->Fetch()[0].GetUInt32(), 45088); ++count; } while (rows->NextRow());
    Require(count == 2, "Reinspect quest enders"); HashMapHolder<Player>::SetSize(256);
    {
        Map map(1220, 300, 9999992, DIFFICULTY_NONE);
        for (uint32 entry : {114263u, 116760u})
        {
            Actor giver(&map, entry, 8000 + entry, {5479,470,150,0});
            Require(map.AddToMap(static_cast<Creature*>(&giver)), "Native Odyn admission failed");
            Require(giver.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER), "Odyn template lacks questgiver flag");
            CheckEmpty(map, giver);
            CheckStorage(map, giver);
            CheckChoiceVariants(map, giver);
            CheckSavedStorage(map, giver, false); CheckSavedStorage(map, giver, true);
            for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
                for (uint8 cls = CLASS_WARRIOR; cls <= CLASS_DEMON_HUNTER; ++cls) CheckValid(map, giver, cls, race);
            if (entry == 116760) for (uint8 cls : {uint8(CLASS_WARRIOR), uint8(CLASS_HUNTER), uint8(CLASS_ROGUE), uint8(CLASS_MAGE)}) CheckPersistence(map, giver, cls);
        }
    }
    FlushWrites(); CharacterDatabase.Close(); LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close(); return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return Run(argc, argv); } catch (std::exception const& e) { std::cerr << e.what() << '\n'; return 1; }
}
