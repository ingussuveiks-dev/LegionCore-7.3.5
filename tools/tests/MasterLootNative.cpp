// Actual packet decoding, WorldSession dispatcher, native condition admission
// and Player inventory receipt. Actors/group/loot flags are private fixtures.
#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"

using NativeLootRecord = decltype(LootItem::item);
static_assert(offsetof(LootItem, item) == 0 && offsetof(NativeLootRecord, CurrencyID) == 4 &&
    offsetof(LootItem, type) == 0x38 && offsetof(LootItem, count) == 0x60,
    "Update canonical Release call-argument layout checks for the changed native LootItem");

struct GroupRollsTag { using Type = std::vector<Roll*> Group::*; friend Type Access(GroupRollsTag); };
template struct FixtureAccess<GroupRollsTag, &Group::RollId>;
struct ProbeParty : Group
{
    std::vector<LootPlayer*> members;
    ProbeParty(LootPlayer& master, LootPlayer& target) : members{&master, &target}
    {
        this->*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(1001, 0);
        SetLootMethod(MASTER_LOOT); SetLooterGuid(master.GetGUID()); SetLootThreshold(ITEM_QUALITY_POOR);
        for (auto p : members)
        {
            MemberSlot slot; slot.Guid = p->GetGUID(); slot.Class = p->getClass();
            const_cast<MemberSlotList&>(GetMemberSlots()).push_back(slot); p->SetGroup(this, 0);
        }
        sGroupMgr->AddGroup(this);
    }
    void ClearRolls()
    {
        auto& rolls = this->*Access(GroupRollsTag{});
        for (auto roll : rolls) delete roll;
        rolls.clear();
    }
    ~ProbeParty()
    {
        ClearRolls();
        for (auto p : members) p->SetGroup(nullptr);
        sGroupMgr->RemoveGroup(this);
    }
};

static void Assign(WorldSession& master, ObjectGuid target, ObjectGuid source, uint8 slot = 1)
{
    WorldPacket wire(CMSG_MASTER_LOOT_ITEM);
    wire << uint32(1) << target << source << slot;
    WorldPackets::Loot::MasterLootItem packet(std::move(wire));
    packet.Read();
    Require(packet.Loot.size() == 1 && packet.Target == target && packet.Loot[0].Object == source && packet.Loot[0].LootListID == slot,
        "Native master-loot request decoder changed");
    master.HandleMasterLootItem(packet);
}

static void CheckAssignment(Map& map, LootPlayer& master, LootPlayer& target, uint32 itemId, bool needsQuest,
    ConditionList const& conditions, bool followRules, bool expected, bool oldExpected, char const* label)
{
    Actor source(&map, 116180, ++victimGuid, master.GetPosition());
    Require(map.AddToMap(static_cast<Creature*>(&source)), "Master-loot source lookup setup failed");
    auto& loot = source.loot; loot.SetLootOwner(&master); loot.isClear = false;
    LootStoreItem row(itemId, 0, 0, 100.0f, needsQuest, 0, 0, 1, 1); row.conditions = conditions;
    loot.AddItem(row);
    auto& item = needsQuest ? loot.quest_items.front() : loot.items.front();
    item.follow_loot_rules = followRules;
    // Build actual initial cached lists for the eligible master; a target may
    // be ineligible. Distribution must test the target rather than the master.
    (loot.*Access(FillCachesTag{}))(&master, true);
    auto before = target.GetItemCount(itemId, true); auto masterBefore = master.GetItemCount(itemId, true);
    auto unlooted = loot.unlootedCount; auto count = item.count;
    Assign(*master.GetSession(), target.GetGUID(), source.GetGUID());
    bool received = target.GetItemCount(itemId, true) == before + count;
    std::cout << "ASSIGN " << label << " quest=" << needsQuest << " follow=" << followRules
        << " inventory=" << before << "->" << target.GetItemCount(itemId, true) << " looted=" << item.is_looted << std::endl;
#ifdef MASTER_LOOT_NATIVE_BASELINE
    expected = oldExpected;
#endif
    Require(received == expected, "Master-loot recipient admission mismatch");
    Require(master.GetItemCount(itemId, true) == masterBefore, "Master received another recipient's item");
    if (expected)
        Require(item.is_looted && item.count == 0 && loot.unlootedCount == unlooted - 1, "Successful native distribution did not consume once");
    else
        Require(!item.is_looted && item.count == count && loot.unlootedCount == unlooted && target.GetItemCount(itemId, true) == before,
            "Rejected assignment mutated loot or inventory");
}

static void CheckRoll(Map& map, ProbeParty& group, LootPlayer& master, LootPlayer& target,
    uint32 itemId, bool needsQuest, ConditionList const& conditions, bool manual, char const* label)
{
    Actor source(&map, 116180, ++victimGuid, master.GetPosition());
    Require(map.AddToMap(static_cast<Creature*>(&source)), "Native roll source setup failed");
    auto& loot = source.loot; loot.SetLootOwner(&master); loot.isClear = false; loot.objGuid = source.GetGUID();
    LootStoreItem row(itemId, 0, 0, 100.0f, needsQuest, 0, 0, 1, 1); row.conditions = conditions;
    loot.AddItem(row);
    auto& item = needsQuest ? loot.quest_items.front() : loot.items.front(); item.follow_loot_rules = true;
    // Fragment metadata is FFA in live data. This private branch fixture must
    // be one-player loot so GroupLoot enters its ordinary roll admission loop.
    item.freeforall = false;
    std::cout << "ROLL_INPUT item=" << itemId << " ffa=" << item.freeforall << " quality=" << unsigned(item.quality)
        << " contact=" << master.CanContact() << '/' << target.CanContact() << " quest=" << master.HasQuestForItem(itemId) << '/' << target.HasQuestForItem(itemId)
        << " range=" << master.IsWithinDistInMap(&source, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false)
        << '/' << target.IsWithinDistInMap(&source, sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE), false) << std::endl;
    sLootMgr->AddLoot(&loot); // Native Roll::FillPacket resolves this registry.
    auto masterBefore = master.GetItemCount(itemId, true); auto before = target.GetItemCount(itemId, true);
    group.SetLootMethod(manual ? MASTER_LOOT : GROUP_LOOT);
    if (manual)
        group.DoRollForAllMembers(source.GetGUID(), 0, map.GetId(), &loot, item, &master);
    else
        group.GroupLoot(&loot, &source);
    auto& rolls = group.*Access(GroupRollsTag{});
    unsigned expected = conditions.empty() ? 2 : 1; // Target has10/10; master0/10.
#ifdef MASTER_LOOT_NATIVE_BASELINE
    expected = !needsQuest && !conditions.empty() ? 2 : 0;
#endif
    std::cout << "ROLL " << label << " manual=" << manual << " quest=" << needsQuest << " expected=" << expected
        << " actual=" << (rolls.empty() ? 0 : rolls.front()->totalPlayersRolling) << std::endl;
    Require(rolls.size() == (expected ? 1 : 0), "Native roll admission/list count mismatch");
    if (expected)
    {
        auto roll = rolls.front();
        Require(roll->totalPlayersRolling == expected && roll->playerVote.size() == expected && roll->playerVote.count(master.GetGUID()),
            "Native roll lost eligible master or has incorrect votes");
        Require(bool(roll->playerVote.count(target.GetGUID())) == (expected == 2), "Native roll admitted wrong target");
    }
    Require(master.GetItemCount(itemId, true) == masterBefore && target.GetItemCount(itemId, true) == before && !item.is_looted && item.count == 1,
        "Roll creation consumed loot before voting");
    group.ClearRolls(); group.SetLootMethod(MASTER_LOOT);
}

#ifndef MASTER_LOOT_NATIVE_SUPPORT_ONLY
static int MasterRun(int argc, char** argv)
{
    LootItem layout; layout.needs_quest = true;
    Require((reinterpret_cast<uint8 const*>(&layout)[0x64] & 0x40) != 0, "Release quest-bit layout changed");
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Native login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readOnly = LoginDatabase.Query("SELECT @@session.transaction_read_only");
    Require(readOnly && readOnly->Fetch()[0].GetBool(), "Login session is not read-only");
    // Bounded socketless account0 sessions deliberately survive to process exit
    // to avoid their normal account UPDATE destructor. No CharacterDB is opened.
    auto first = new WorldSession(0, std::string("Master-loot native probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto second = new WorldSession(0, std::string("Master-loot target probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    HashMapHolder<Player>::SetSize(32);
    {
        Map map(1220, 300, 9999995, DIFFICULTY_NONE);
        LootPlayer master(first, map, 100, {5478.0f, 470.0f, 150.0f, 0.0f});
        LootPlayer target(second, map, 101, master.GetPosition());
        ProbeParty group(master, target);
        // Copy real condition pointers from the native live loader without
        // replacing their evaluator. Only per-case flags/state are fixtures.
        master.Accept(0);
        Loot seed; seed.SetLootOwner(&master); seed.isClear = false;
        Require(seed.FillLoot(116180, LootTemplates_Creature, &master, false), "Live condition seed failed");
        // Native drop chance is preserved. Retry independent seed rolls only.
        for (unsigned i = 0; seed.quest_items.empty() && i < 40; ++i)
        { seed.clear(); seed.FillLoot(116180, LootTemplates_Creature, &master, false); }
        Require(!seed.quest_items.empty(), "No live fragment condition seed rolled");
        auto conditions = seed.quest_items.front().conditions; Require(conditions.size() == 4, "Changed live condition list");
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            target.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); target.setFactionForRace(race);
            master.Accept(0); target.Accept(0);
            CheckAssignment(map, master, target, 2589, false, {}, true, true, false, "ordinary-follow-rules");
            CheckAssignment(map, master, target, 142487, true, conditions, true, true, false, "eligible-quest");
            CheckAssignment(map, master, target, 142487, false, conditions, true, true, true, "eligible-conditional");
            target.SetQuestObjectiveData(landing, &landing->GetObjectives()[0], 10);
            CheckAssignment(map, master, target, 142487, false, conditions, true, false, true, "target-quota");
            target.Accept(0); target.SetQuestStatus(44923, QUEST_STATUS_NONE); target.SetQuestSlot(0, 0);
            CheckAssignment(map, master, target, 142487, false, conditions, true, false, true, "target-landing-absent");
            target.Accept(0); target.SetQuestStatus(45088, QUEST_STATUS_NONE); target.SetQuestSlot(1, 0);
            CheckAssignment(map, master, target, 142487, true, conditions, true, false, false, "target-item-quest-absent");
            // Ordinary loot must not acquire a HasQuestForItem requirement.
            CheckAssignment(map, master, target, 2589, false, {}, true, true, false, "ordinary-no-task");
            CheckAssignment(map, master, target, 2589, false, {}, false, true, true, "ordinary-existing-no-follow-rule");
            master.Accept(0); target.Accept(10);
            for (bool manual : {false, true})
            {
                CheckRoll(map, group, master, target, 2589, false, {}, manual, "ordinary-over-threshold");
                CheckRoll(map, group, master, target, 142487, false, conditions, manual, "conditional-target-quota");
                CheckRoll(map, group, master, target, 142487, true, conditions, manual, "quest-target-quota");
            }
        }
    }
    LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close();
#ifdef MASTER_LOOT_NATIVE_BASELINE
    std::cout << "PASS: pinned native handler reproduces ordinary/quest false rejection and conditional recipient bypass.\n";
#else
    std::cout << "PASS: native packet/handler/inventory accepts ordinary and eligible quest items, rejects target quota/missing-task conditions without consuming loot, and preserves the non-follow-rule branch for both factions.\n";
#endif
    std::cout << "PASS: native GroupLoot ordinary/quest branches and DoRollForAllMembers execute recipient admission and preserve loot until voting; baseline mode asserts the old rejection/bypass.\n";
    return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return MasterRun(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif
