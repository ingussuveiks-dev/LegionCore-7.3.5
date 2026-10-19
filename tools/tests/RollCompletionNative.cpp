// Full native group roll votes, winner delivery, corpse timeout and inventory.
// Socketless sessions and initial actor/group/loot state remain private fixtures.
#define MASTER_LOOT_NATIVE_SUPPORT_ONLY
#include "MasterLootNative.cpp"

static void Vote(LootPlayer& player, ObjectGuid loot, uint8 slot, uint8 choice)
{
    WorldPacket wire(CMSG_LOOT_ROLL); wire << loot << slot << choice;
    WorldPackets::Loot::LootRoll packet(std::move(wire)); packet.Read();
    Require(packet.LootObj == loot && packet.LootListID == slot && packet.RollType == choice, "Native vote packet decode failed");
    player.GetSession()->HandleLootRoll(packet);
}
struct RollSource : Actor
{
    ProbeParty& group;
    RollSource(Map& map, ProbeParty& party, LootPlayer& master, LootPlayer& target,
        uint32 itemId = 2589, bool quest = false, ConditionList const& conditions = {})
        : Actor(&map, 116180, ++victimGuid, master.GetPosition()), group(party)
    {
        Require(map.AddToMap(static_cast<Creature*>(this)), "Roll source map admission failed");
        loot.SetLootOwner(&master); loot.isClear = false; loot.objGuid = GetGUID();
        LootStoreItem row(itemId, 0, 0, 100.0f, quest, 0, 0, 1, 1); row.conditions = conditions; loot.AddItem(row);
        auto item = const_cast<LootItem*>(loot.GetItemInSlot(0)); Require(item, "Native roll item missing");
        item->freeforall = false; item->follow_loot_rules = true; // Explicit private generic roll variants.
        group.SetLootMethod(GROUP_LOOT);
        (loot.*Access(FillCachesTag{}))(&master, true); (loot.*Access(FillCachesTag{}))(&target, true);
        sLootMgr->AddLoot(&loot); group.GroupLoot(&loot, this);
        Require(Rolls().size() == 1 && Rolls().front()->totalPlayersRolling == 2, "Expected two admitted native roll participants");
    }
    std::vector<Roll*>& Rolls() { return group.*Access(GroupRollsTag{}); }
    Roll* Current() { Require(Rolls().size() == 1, "Expected one active native roll"); return Rolls().front(); }
    void VoteFor(LootPlayer& player, uint8 choice) { Vote(player, loot.GetGUID(), Current()->aoeSlot, choice); }
    void Timeout()
    {
        // Seed only corpse state. Execute Creature::Update's real60-second
        // countdown/GroupMgr lookup/EndRoll rather than a modeled timer.
        Unit::setDeathState(CORPSE);
        Require(m_groupLootTimer == NORMAL_ROLL_TIMER, "Native group timer was not armed");
        Update(NORMAL_ROLL_TIMER - 1); Require(!Rolls().empty() && m_groupLootTimer == 1, "Native timer resolved too early");
        // Creature::Update batches idle actors; allow its native update cadence
        // to flush the final millisecond instead of replacing that throttle.
        for (unsigned i = 0; m_groupLootTimer && i < 5000; ++i) Update(1);
        Require(Rolls().empty() && !m_groupLootTimer && !lootingGroupLowGUID, "Native corpse timeout did not finish/clear");
    }
    ~RollSource() { group.ClearRolls(); }
};
static void ClearBackpack(LootPlayer& player)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot)) player.DestroyItem(INVENTORY_SLOT_BAG_0, slot, false);
}
static void FillBackpack(LootPlayer& player)
{
    ClearBackpack(player);
    auto proto = sObjectMgr->GetItemTemplate(2589); Require(proto, "Missing ordinary control item");
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        ItemPosCountVec dest; dest.emplace_back((INVENTORY_SLOT_BAG_0 << 8) | slot, proto->GetMaxStackSize());
        Require(player.StoreNewItem(dest, 2589, false), "Native full-backpack setup failed");
    }
    ItemPosCountVec dest;
    Require(player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 2589, 1) == EQUIP_ERR_INV_FULL, "Native inventory is not full");
}
static void CheckVotes(Map& map, ProbeParty& group, LootPlayer& master, LootPlayer& target)
{
    master.Accept(0); target.Accept(0);
    for (bool quest : {false, true}) for (uint8 choice : {uint8(ROLL_NEED), uint8(ROLL_GREED)})
    {
        uint32 itemId = quest ? 142487 : 2589; auto before = target.GetItemCount(itemId, true);
        RollSource source(map, group, master, target, itemId, quest);
        source.VoteFor(target, choice); source.VoteFor(master, ROLL_PASS);
#ifdef ROLL_COMPLETION_NATIVE_BASELINE
        if (quest)
        {
            Require(source.Current()->TotalEmited() == 0 && target.GetItemCount(itemId, true) == before, "Baseline quest-only vote refusal changed");
            source.Timeout(); Require(!source.loot.GetItemInSlot(0)->is_looted, "Baseline quest timeout unexpectedly delivered");
            continue;
        }
#endif
        Require(source.Rolls().empty() && target.GetItemCount(itemId, true) == before + 1 && source.loot.GetItemInSlot(0)->is_looted,
            "Native Need/Greed winner did not receive ordinary/quest item");
    }
    {
        auto before = target.GetItemCount(2589, true); RollSource source(map, group, master, target);
        source.VoteFor(target, ROLL_NEED); auto slot = source.Current()->aoeSlot;
        Vote(target, source.loot.GetGUID(), slot, ROLL_NEED); // Same vote cannot stand in for the other member.
#ifdef ROLL_COMPLETION_NATIVE_BASELINE
        Require(source.Rolls().empty() && target.GetItemCount(2589, true) == before + 1, "Baseline duplicate premature completion changed");
#else
        Require(source.Current()->totalNeed == 1 && source.Current()->TotalEmited() == 1 && target.GetItemCount(2589, true) == before,
            "Duplicate vote completed or incremented a native roll");
        source.VoteFor(master, ROLL_PASS);
        Require(source.Rolls().empty() && target.GetItemCount(2589, true) == before + 1, "Valid final vote did not finish once");
#endif
    }
    for (uint8 choice : {uint8(ROLL_NEED), uint8(ROLL_DISENCHANT), uint8(MAX_ROLL_TYPE)})
    {
        RollSource source(map, group, master, target); auto roll = source.Current();
        roll->rollVoteMask = ROLL_FLAG_TYPE_PASS | ROLL_FLAG_TYPE_GREED;
        source.VoteFor(target, choice);
#ifdef ROLL_COMPLETION_NATIVE_BASELINE
        Require(roll->TotalEmited() == (choice == MAX_ROLL_TYPE ? 0 : 1), "Baseline disabled vote behavior changed");
#else
        Require(roll->TotalEmited() == 0 && roll->playerVote[target.GetGUID()] == NOT_EMITED_YET, "Disabled/invalid vote consumed participation");
#endif
    }
    {
        RollSource source(map, group, master, target); auto roll = source.Current(); roll->itemSlot = 99;
        source.VoteFor(target, ROLL_PASS);
#ifdef ROLL_COMPLETION_NATIVE_BASELINE
        Require(roll->totalPass == 1, "Baseline invalid-slot vote behavior changed");
#else
        Require(!roll->TotalEmited(), "Missing native roll slot consumed a vote");
#endif
    }
    std::cout << "PASS: native packet votes and Need/Greed delivery cover ordinary/quest-only loot, duplicate/disabled/invalid votes and missing slots; baseline asserts old defects.\n";
}
static void CheckTimeoutInventory(Map& map, ProbeParty& group, LootPlayer& master, LootPlayer& target)
{
    for (bool voted : {false, true})
    {
        auto before = target.GetItemCount(2589, true); RollSource source(map, group, master, target);
        if (voted) source.VoteFor(target, ROLL_GREED);
        source.Timeout(); auto item = source.loot.GetItemInSlot(0);
        Require(target.GetItemCount(2589, true) == before + (voted ? 1 : 0) && item->is_looted == voted && (voted || !item->is_blocked),
            "Native timeout failed winner delivery or all-pass release");
        group.EndRoll(&source.loot); Require(target.GetItemCount(2589, true) == before + (voted ? 1 : 0), "Repeated timeout granted twice");
    }
    for (uint8 choice : {uint8(ROLL_NEED), uint8(ROLL_GREED)})
    {
        FillBackpack(target); auto before = target.GetItemCount(2589, true);
        RollSource source(map, group, master, target); auto unlooted = source.loot.unlootedCount;
        source.VoteFor(target, choice); source.VoteFor(master, ROLL_PASS);
        auto item = source.loot.GetItemInSlot(0);
        Require(source.Rolls().empty() && !item->is_looted && !item->is_blocked && source.loot.unlootedCount == unlooted && target.GetItemCount(2589, true) == before,
            "Full inventory lost/consumed/blocked the winner item");
        target.DestroyItem(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, false); before = target.GetItemCount(2589, true);
        target.StoreLootItem(0, &source.loot);
        Require(target.GetItemCount(2589, true) == before + 1 && source.loot.GetItemInSlot(0)->is_looted, "Native inventory-full retry did not deliver");
        ClearBackpack(target);
    }
    std::cout << "PASS: actual Creature corpse countdown resolves timeout, nonvoters/all-pass and repeat callbacks; native full inventory preserves loot and later StoreLootItem retry delivers.\n";
}
static int CompletionRun(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Native login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = LoginDatabase.Query("SELECT @@session.transaction_read_only"); Require(readOnly && readOnly->Fetch()[0].GetBool(), "Login is not read-only");
    auto first = new WorldSession(0, std::string("Roll completion master"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto second = new WorldSession(0, std::string("Roll completion target"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    HashMapHolder<Player>::SetSize(32);
    {
        Map map(1220, 300, 9999994, DIFFICULTY_NONE);
        LootPlayer master(first, map, 100, {5478.0f, 470.0f, 150.0f, 0.0f}); LootPlayer target(second, map, 101, master.GetPosition());
        ProbeParty group(master, target);
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            target.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); target.setFactionForRace(race);
            CheckVotes(map, group, master, target); CheckTimeoutInventory(map, group, master, target);
        }
    }
    LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close(); return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return CompletionRun(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
