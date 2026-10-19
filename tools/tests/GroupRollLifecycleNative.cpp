// Actual group removal and full native session logout use a schema-only scratch
// character/auth DBs. World/Hotfix remain read-only; no client is connected.
#define ROLL_COMPLETION_NATIVE_SUPPORT_ONLY
#include "RollCompletionNative.cpp"
#include "SocialMgr.h"
#include <thread>

struct GroupDbTag { using Type = uint32 Group::*; friend Type Access(GroupDbTag); };
template struct FixtureAccess<GroupDbTag, &Group::m_dbStoreId>;
struct SocialTag { using Type = PlayerSocial* Player::*; friend Type Access(SocialTag); };
template struct FixtureAccess<SocialTag, &Player::m_social>;
static void FlushCharacterWrites()
{
    bool done = false;
    auto callback = CharacterDatabase.AsyncQuery("SELECT 1").WithCallback([&](QueryResult result) { Require(bool(result), "Scratch async barrier failed"); done = true; });
    for (unsigned i = 0; !done && i < 10000; ++i)
    { callback.InvokeIfReady(); if (!done) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    Require(done, "Scratch writes did not finish");
}
static void FlushAuthWrites()
{
    bool done = false;
    auto callback = LoginDatabase.AsyncQuery("SELECT 1").WithCallback([&](QueryResult result) { Require(bool(result), "Scratch auth barrier failed"); done = true; });
    for (unsigned i = 0; !done && i < 10000; ++i)
    { callback.InvokeIfReady(); if (!done) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    Require(done, "Scratch auth writes did not finish");
}
static uint32 partyId = 2000;
struct PartyOwner
{
    Group* group = new Group(); ObjectGuid guid;
    explicit PartyOwner(std::initializer_list<LootPlayer*> players)
    {
        uint32 id = ++partyId; guid = ObjectGuid::Create<HighGuid::Party>(id, 0);
        group->*Access(GroupGuidTag{}) = guid; group->*Access(GroupDbTag{}) = id;
        group->SetLootMethod(GROUP_LOOT); group->SetLootThreshold(ITEM_QUALITY_POOR); group->SetLooterGuid((*players.begin())->GetGUID());
        sGroupMgr->AddGroup(group); sGroupMgr->RegisterGroupDbStoreId(id, group);
        for (auto player : players)
        {
            player->Accept(0); Group::MemberSlot slot; slot.Guid = player->GetGUID(); slot.Class = player->getClass(); slot.Name = player->GetName();
            const_cast<Group::MemberSlotList&>(group->GetMemberSlots()).push_back(slot); player->SetGroup(group, 0);
            CharacterDatabase.DirectExecute(Trinity::StringFormat("REPLACE INTO group_member(guid,memberGuid) VALUES (%u,%llu)", id, player->GetGUIDLow()).c_str());
        }
    }
    ~PartyOwner()
    {
        if (sGroupMgr->GetGroupByGUID(guid) != group) return; // Native Disband deletes the real Group.
        for (auto const& slot : group->GetMemberSlots()) if (auto p = ObjectAccessor::FindPlayer(slot.Guid))
        { if (p->GetGroup() == group) p->SetGroup(nullptr); if (p->GetOriginalGroup() == group) p->SetOriginalGroup(nullptr); }
        sGroupMgr->FreeGroupDbStoreId(group); sGroupMgr->RemoveGroup(group); delete group;
    }
};
struct LifecycleSource : Actor
{
    uint8 aoeSlot;
    LifecycleSource(Map& map, Group& group, LootPlayer& master, uint32 itemId = 2589, bool quest = false, ConditionList const& conditions = {})
        : Actor(&map, 116180, ++victimGuid, master.GetPosition())
    {
        Require(map.AddToMap(static_cast<Creature*>(this)), "Lifecycle source map admission failed");
        loot.SetLootOwner(&master); loot.isClear = false; loot.objGuid = GetGUID();
        LootStoreItem row(itemId, 0, 0, 100.0f, quest, 0, 0, 1, 1); row.conditions = conditions; loot.AddItem(row);
        auto item = const_cast<LootItem*>(loot.GetItemInSlot(0)); item->follow_loot_rules = true; item->freeforall = false;
        sLootMgr->AddLoot(&loot); group.GroupLoot(&loot, this);
        auto& rolls = group.*Access(GroupRollsTag{}); Require(!rolls.empty(), "Lifecycle roll not constructed");
        aoeSlot = rolls.back()->aoeSlot;
    }
    void CastVote(LootPlayer& player, uint8 choice) { Vote(player, loot.GetGUID(), aoeSlot, choice); }
};
static void CheckLeave(Map& map, LootPlayer& master, LootPlayer& target, LootPlayer& third, LootPlayer& fourth)
{
    for (uint8 choice : {uint8(ROLL_PASS), uint8(ROLL_NEED), uint8(ROLL_GREED), uint8(MAX_ROLL_TYPE)})
    {
        PartyOwner owner{&master, &target, &third, &fourth}; auto& group = *owner.group;
        LifecycleSource first(map, group, master), second(map, group, master);
        for (auto source : {&first, &second})
        { source->CastVote(master, ROLL_GREED); source->CastVote(third, ROLL_PASS); if (choice != MAX_ROLL_TYPE) source->CastVote(target, choice); }
        Require(group.RemoveMember(target.GetGUID()), "Native first removal unexpectedly disbanded");
        target.Unit::Update(200); // Execute the native delayed quest-object callback while this player is alive.
        Require(!target.GetGroup() && group.GetMembersCount() == 3, "Native reference/member removal failed");
        auto& rolls = group.*Access(GroupRollsTag{});
        Require(rolls.size() == 2, "Roll resolved before the other pending member left");
        for (auto roll : rolls) Require(roll->totalPlayersRolling == 3 && roll->totalNeed == 0 && roll->totalGreed == 1 && roll->totalPass == 1 && !roll->playerVote.count(target.GetGUID()), "Removal did not update native roll counters");
        auto before = master.GetItemCount(2589, true);
        Require(group.RemoveMember(fourth.GetGUID()), "Native second removal unexpectedly disbanded");
        fourth.Unit::Update(200);
        FlushCharacterWrites();
        auto rows = CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM group_member WHERE guid=%u", group.GetDbStoreId()).c_str());
        Require(rows && rows->Fetch()[0].GetUInt32() == 2, "Actual scratch member DELETE did not persist");
#ifdef GROUP_ROLL_LIFECYCLE_BASELINE
        Require(rolls.size() == 2 && master.GetItemCount(2589, true) == before, "Baseline pending-vote removal failure changed");
        group.EndRoll(&first.loot); group.EndRoll(&second.loot);
#else
        Require(rolls.empty() && master.GetItemCount(2589, true) == before + 2 && first.loot.GetItemInSlot(0)->is_looted && second.loot.GetItemInSlot(0)->is_looted, "Native removal failed immediate multi-roll inventory delivery");
        group.EndRoll(&first.loot); group.EndRoll(&second.loot);
        Require(master.GetItemCount(2589, true) == before + 2, "Repeated completion granted twice");
#endif
    }
    std::cout << "PASS: actual RemoveMember handles PASS/NEED/GREED/pending leavers, references, scratch member persistence and two simultaneous rolls; baseline asserts delayed completion.\n";
}
static void CheckDisband(Map& map, LootPlayer& master, LootPlayer& target)
{
    PartyOwner owner{&master, &target}; auto guid = owner.guid;
    LifecycleSource source(map, *owner.group, master); source.CastVote(master, ROLL_NEED);
    Require(!owner.group->RemoveMember(target.GetGUID()), "Two-member native group did not disband");
    Require(!sGroupMgr->GetGroupByGUID(guid) && !master.GetGroup() && !target.GetGroup(), "Native disband did not unlink/delete group");
    auto item = source.loot.GetItemInSlot(0);
#ifdef GROUP_ROLL_LIFECYCLE_BASELINE
    Require(item->is_blocked && !item->is_looted, "Baseline disband blocked loot changed");
#else
    Require(!item->is_blocked && !item->is_looted, "Disband left unresolved loot blocked or granted it");
    auto before = master.GetItemCount(2589, true); master.StoreLootItem(0, &source.loot);
    Require(master.GetItemCount(2589, true) == before + 1, "Native post-disband loot retry failed");
#endif
    FlushCharacterWrites();
    std::cout << "PASS: actual two-member RemoveMember/Disband deletes the group and preserves loot recovery; baseline asserts blocked item.\n";
}
static void CheckLastVoter(Map& map, LootPlayer& master, LootPlayer& target, LootPlayer& third)
{
    PartyOwner owner{&master, &target, &third}; auto& group = *owner.group;
    Loot seed; seed.SetLootOwner(&master); seed.isClear = false;
    for (unsigned i = 0; seed.quest_items.empty() && i < 40; ++i)
    { seed.clear(); Require(seed.FillLoot(116180, LootTemplates_Creature, &master, true), "Last-voter condition seed failed"); }
    Require(!seed.quest_items.empty(), "No last-voter condition seed rolled");
    auto conditions = seed.quest_items.front().conditions;
    master.SetQuestStatus(44923, QUEST_STATUS_NONE); third.SetQuestStatus(44923, QUEST_STATUS_NONE);
    LifecycleSource source(map, group, master, 142487, true, conditions);
    auto& rolls = group.*Access(GroupRollsTag{});
    Require(rolls.size() == 1 && rolls.front()->totalPlayersRolling == 1, "Last-voter native admission changed");
    Require(group.RemoveMember(target.GetGUID()), "Last-voter removal disbanded a three-member party");
    target.Unit::Update(200);
#ifdef GROUP_ROLL_LIFECYCLE_BASELINE
    Require(rolls.size() == 1 && rolls.front()->totalPlayersRolling == 0 && source.loot.GetItemInSlot(0)->is_blocked, "Baseline zero-voter stuck roll changed");
    group.EndRoll(&source.loot);
#else
    Require(rolls.empty() && !source.loot.GetItemInSlot(0)->is_blocked && !source.loot.GetItemInSlot(0)->is_looted, "Last eligible voter left a stuck/consumed quest item");
#endif
    master.Accept(0); third.Accept(0); target.Accept(0);
    FlushCharacterWrites();
    std::cout << "PASS: actual quest-only last eligible voter removal releases loot without granting it; baseline asserts zero-voter wait.\n";
}
static WorldSession* Session(char const* name)
{ return new WorldSession(0, std::string(name), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {}); }
static uint32 logoutGuid = 104;
static void CheckLogout(Map& map, LootPlayer& master, LootPlayer& third, uint8 race, bool originalOnly)
{
    auto session = Session("Lifecycle logout");
    auto target = new LootPlayer(session, map, logoutGuid++, master.GetPosition()); target->SetName("Native" + std::string(1, char('a' + target->GetGUIDLow() % 26)));
    target->SetByteValue(UNIT_FIELD_BYTES_0, 0, race); target->setFactionForRace(race);
    target->*Access(SocialTag{}) = sSocialMgr->LoadFromDB({}, target->GetGUID());
    auto guid = target->GetGUID();
    PartyOwner owner{&master, target, &third}; auto& group = *owner.group;
    LifecycleSource source(map, group, master);
    source.CastVote(master, ROLL_GREED); source.CastVote(third, ROLL_PASS);
    auto before = master.GetItemCount(2589, true);
    if (originalOnly) { target->SetGroup(nullptr); target->SetOriginalGroup(&group, 0); }
    target->SaveToDB(true); FlushCharacterWrites();
    // Native disconnect/logout dispatcher after the map-removal gate. No
    // socket, authenticated login or world scheduler is replaced or claimed.
    session->SetCanLogout(); session->Update(1);
    Require(!session->GetPlayer() && !ObjectAccessor::FindPlayer(guid), "Actual logout did not clear session/player lookup");
    auto& rolls = group.*Access(GroupRollsTag{});
#ifdef GROUP_ROLL_LIFECYCLE_BASELINE
    Require(rolls.size() == 1 && master.GetItemCount(2589, true) == before, "Baseline logout retained-vote behavior changed");
    group.EndRoll(&source.loot);
#else
    Require(rolls.empty() && master.GetItemCount(2589, true) == before + 1, "Actual logout did not resolve remaining votes");
#endif
    Require(group.GetMembersCount() == 3, "Logout removed persistent party membership");
    FlushCharacterWrites();
    FlushAuthWrites();
    auto saved = CharacterDatabase.Query(Trinity::StringFormat("SELECT online FROM characters WHERE guid=%llu", guid.GetCounter()).c_str());
    Require(saved && !saved->Fetch()[0].GetBool(), "Native logout/save did not persist offline character state");
    auto membership = CharacterDatabase.Query(Trinity::StringFormat("SELECT COUNT(*) FROM group_member WHERE memberGuid=%llu AND guid=%u", guid.GetCounter(), group.GetDbStoreId()).c_str());
    Require(membership && membership->Fetch()[0].GetUInt32() == 1, "Actual logout unexpectedly deleted saved party membership");
    std::cout << "LOGOUT original=" << originalOnly << " guid=" << guid.GetCounter() << " offline=1 saved-member=1\n";
    std::cout << "PASS: actual WorldSession Update/LogoutPlayer(true), save, lookup/map cleanup and retained party membership execute in scratch databases.\n";
}
static int LifecycleRun(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs();
    auto scratch = sConfigMgr->GetStringDefault("NativeRollScratchDatabase", "");
    auto connection = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
    Require(scratch.size() == 37 && scratch.find("codex_roll_lifecycle_") == 0 && connection.substr(connection.find_last_of(';') + 1) == scratch, "Refusing non-scratch character connection");
    CharacterDatabase.SetConnectionInfo(connection, 1, 1);
    Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Scratch character pool failed");
    auto database = CharacterDatabase.Query("SELECT DATABASE(),(SELECT COUNT(*) FROM characters)");
    Require(database && database->Fetch()[0].GetString() == scratch && !database->Fetch()[1].GetUInt32(), "Character scratch identity/empty data check failed");
    auto auth = sConfigMgr->GetStringDefault("NativeRollScratchAuth", "");
    auto authConnection = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
    Require(auth.find("codex_roll_auth_") == 0 && auth.size() == 32 && authConnection.substr(authConnection.find_last_of(';') + 1) == auth, "Refusing non-scratch auth connection");
    LoginDatabase.SetConnectionInfo(authConnection, 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Scratch login pool failed");
    auto authDatabase = LoginDatabase.Query("SELECT DATABASE(),(SELECT COUNT(*) FROM account)");
    Require(authDatabase && authDatabase->Fetch()[0].GetString() == auth && !authDatabase->Fetch()[1].GetUInt32(), "Auth scratch identity/empty data check failed");
    HashMapHolder<Player>::SetSize(32);
    {
        Map map(1220, 300, 9999993, DIFFICULTY_NONE);
        LootPlayer master(Session("Lifecycle master"), map, 100, {5478.0f,470.0f,150.0f,0.0f}); master.SetName("NativeMaster");
        LootPlayer target(Session("Lifecycle target"), map, 101, master.GetPosition()); target.SetName("NativeTarget");
        LootPlayer third(Session("Lifecycle third"), map, 102, master.GetPosition()); third.SetName("NativeThird");
        LootPlayer fourth(Session("Lifecycle fourth"), map, 103, master.GetPosition()); fourth.SetName("NativeFourth");
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)})
        {
            target.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); target.setFactionForRace(race);
            CheckLeave(map, master, target, third, fourth); CheckDisband(map, master, target); CheckLastVoter(map, master, target, third);
            CheckLogout(map, master, third, race, false); CheckLogout(map, master, third, race, true);
        }
    }
    FlushCharacterWrites(); FlushAuthWrites(); CharacterDatabase.Close(); LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close(); return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return LifecycleRun(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
