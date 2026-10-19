// Native Unit::Kill -> personal/group loot -> delayed quest credit -> packet
// and inventory execution. Only initial actor/task/rotation state is seeded.
#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "LootMgr.h"
#include "LootPackets.h"
#include "Item.h"
#include "Pet.h"
#include "GroupMgr.h"

struct LootTableNameTag { using Type = std::string LootStore::*; friend Type Access(LootTableNameTag); };
template struct FixtureAccess<LootTableNameTag, &LootStore::m_name>;
struct LoadLootTableTag { using Type = uint32 (LootStore::*)(); friend Type Access(LoadLootTableTag); };
template struct FixtureAccess<LoadLootTableTag, &LootStore::LoadLootTable>;
struct AttachLootConditionTag { using Type = bool (ConditionMgr::*)(Condition*, LootTemplate*); friend Type Access(AttachLootConditionTag); };
template struct FixtureAccess<AttachLootConditionTag, &ConditionMgr::addToLootTemplate>;
struct GroupGuidTag { using Type = ObjectGuid Group::*; friend Type Access(GroupGuidTag); };
template struct FixtureAccess<GroupGuidTag, &Group::m_guid>;
struct GroupFlagsTag { using Type = GroupFlags Group::*; friend Type Access(GroupFlagsTag); };
template struct FixtureAccess<GroupFlagsTag, &Group::m_groupFlags>;
struct FillCachesTag { using Type = void (Loot::*)(Player*, bool); friend Type Access(FillCachesTag); };
template struct FixtureAccess<FillCachesTag, &Loot::FillNotNormalLootFor>;

static Quest* landing;
static void LoadLootInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeLostArmyQuest.inc"
    );
    Require(bool(rows), "Missing native quest rows");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{}); quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = 45088;
    do
    {
        auto quest = new Quest(rows->Fetch());
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates())[quest->GetQuestId()] = quest;
        quests[quest->GetQuestId()] = quest;
        if (quest->GetQuestId() == 44923) landing = quest;
    } while (rows->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID, QuestID, Type, StorageIndex, ObjectID, Amount, Flags, Flags2, TaskStep, Description, Bugged FROM quest_objectives WHERE QuestID IN(44923,45088) ORDER BY StorageIndex ASC");
    Require(bool(objectives), "Missing actual objectives");
    do
    {
        auto fields = objectives->Fetch(); auto quest = quests[fields[1].GetUInt32()];
        Require(quest, "Unknown objective quest"); quest->LoadQuestObjective(fields);
        if (fields[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER) quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
        if (fields[2].GetUInt8() == QUEST_OBJECTIVE_ITEM) quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (objectives->NextRow());
    Require(landing && landing->GetObjectives().size() == 2 && landing->GetObjectives()[0].ObjectID == 115845 &&
        landing->GetObjectives()[0].Amount == 10 && landing->GetObjectives()[1].ObjectID == 115846, "Native landing objectives changed");
    static WorldQuestTemplate policy;
    auto& rotation = const_cast<WorldQuestMap&>(*sQuestDataStore->GetWorldQuestMap())[landing->QuestSortID][44923];
    rotation.ResetTime = GameTime::GetGameTime() + 3600; rotation.worldQuest = &policy;
    sObjectMgr->LoadItemTemplates(); sObjectMgr->LoadItemTemplateAddon();
    auto itemScripts = WorldDatabase.Query("SELECT COUNT(*) FROM item_script_names WHERE Id IN(142487,2589)");
    Require(itemScripts && !itemScripts->Fetch()[0].GetUInt32(), "Probe must register newly added item scripts before proceeding");
    // Run the native loader with a read-only scoped SELECT, then restore its
    // exact table identity before normal FillLoot/world-loot dispatch.
    auto& tableName = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalName = tableName;
    tableName += " WHERE Entry IN(116180,116600,116601) AND Item=142487 ORDER BY Entry";
    auto loaded = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    tableName = originalName;
    Require(loaded == 3, "Native scoped loot loader did not admit three rows");
    auto loot = WorldDatabase.Query("SELECT Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,shared FROM creature_loot_template WHERE Entry IN(116180,116600,116601) AND Item=142487 ORDER BY Entry");
    Require(bool(loot), "Missing actual fragment loot"); unsigned lootRows = 0;
    do
    {
        auto f = loot->Fetch(); auto entry = f[0].GetUInt32();
        LootStoreItem item(f[1].GetUInt32(), f[2].GetUInt32(), f[3].GetUInt32(), f[4].GetFloat(), f[5].GetBool(), f[6].GetUInt16(), f[7].GetUInt8(), f[8].GetInt32(), f[9].GetUInt32());
        item.shared = f[10].GetBool(); Require(item.IsValid(LootTemplates_Creature, entry), "Native loot row validation failed");
        Require(!item.shared && LootTemplates_Creature.HaveLootFor(entry), "Scoped loader requires zero shared flags"); ++lootRows;
        std::cout << "LOOT entry=" << entry << " chance=" << item.chance << " count=" << item.mincount << ".." << item.maxcount << '\n';
    } while (loot->NextRow());
    Require(lootRows == 3, "Changed fragment template count");
    auto conditions = WorldDatabase.Query("SELECT SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference, ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorTextId, ScriptName FROM conditions WHERE SourceTypeOrReferenceId=1 AND SourceGroup IN(116180,116600,116601) AND SourceEntry=142487");
    Require(bool(conditions), "Missing actual fragment conditions"); unsigned count = 0;
    do
    {
        auto f = conditions->Fetch(); auto condition = new Condition;
        Require(f[0].GetInt32() == 1 && f[5].GetInt32() >= 0 && f[12].GetString().empty(), "Unsupported new native condition");
        condition->SourceType = ConditionSourceType(f[0].GetInt32()); condition->SourceGroup = f[1].GetUInt32();
        condition->SourceEntry = f[2].GetUInt32(); condition->SourceId = f[3].GetInt32(); condition->ElseGroup = f[4].GetUInt32();
        condition->ConditionType = ConditionTypes(f[5].GetInt32()); condition->ConditionTarget = f[6].GetUInt8();
        condition->ConditionValue1 = f[7].GetUInt32(); condition->ConditionValue2 = f[8].GetUInt32(); condition->ConditionValue3 = f[9].GetUInt32();
        condition->NegativeCondition = f[10].GetBool(); condition->ErrorTextId = f[11].GetUInt32();
        Require(sConditionMgr->isConditionTypeValid(condition) && (sConditionMgr->*Access(ValidateSourceTag{}))(condition) &&
            (sConditionMgr->*Access(AttachLootConditionTag{}))(condition, LootTemplates_Creature.GetLootForConditionFill(condition->SourceGroup)), "Native loot condition admission failed");
        ++count;
    } while (conditions->NextRow());
    Require(count == 12, "Changed native scoped condition count");
}
struct LootPlayer : NativePlayer
{
    using NativePlayer::NativePlayer;
    void Accept(uint32 kills = 0)
    {
        SetQuestStatus(44923, QUEST_STATUS_INCOMPLETE); SetQuestSlot(0, 44923);
        SetQuestStatus(45088, QUEST_STATUS_INCOMPLETE); SetQuestSlot(1, 45088);
        SetQuestObjectiveData(landing, &landing->GetObjectives()[0], kills);
        SetQuestObjectiveData(landing, &landing->GetObjectives()[1], 0);
    }
    ~LootPlayer()
    {
        SetGroup(nullptr);
        for (uint16 slot = 0; slot < PLAYER_SLOTS_COUNT; ++slot)
            if (auto item = GetItemByPos(INVENTORY_SLOT_BAG_0, slot)) if (item->IsInWorld()) item->RemoveFromWorld();
    }
};
static unsigned FragmentView(Loot& loot, Player& player)
{
    WorldPackets::Loot::LootResponse packet;
    loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
    Require(packet.Write()->size() > 0, "Native loot packet serialization failed");
    unsigned count = 0;
    for (auto const& item : packet.Items) if (item.Loot.ItemID == 142487) count += item.Quantity;
    return count;
}
static void Tick(LootPlayer& player, uint32 duration)
{
    for (uint32 i = 0; i < duration; i += 10) player.Unit::Update(10);
}
struct LootPet : Pet
{
    LootPet(LootPlayer& owner, Map& map) : Pet(&owner, HUNTER_PET)
    {
        Require(Create(200, &map, 1, 299, 1), "Native wolf pet creation failed");
        setPetType(HUNTER_PET); SetOwnerGUID(owner.GetGUID()); SetCreatorGUID(owner.GetGUID()); Relocate(owner.GetPosition());
        SetLevel(110); SetMaxHealth(1000000); SetHealth(1000000);
        Require(map.AddToMap(static_cast<Creature*>(this)), "Native pet map admission failed");
    }
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Pet*>(this)); Unit::AddToWorld();
        Require(AIM_Initialize(new NullCreatureAI(this)), "Native idle pet AI failed");
    }
    ~LootPet()
    {
        CombatStop(true); CastStop(); RemoveAllAuras(); m_Events.KillAllEvents(true);
        if (IsInWorld()) GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};
static uint32 victimGuid = 30000;
template<class Setup, class Verify>
static void WithKillLoot(Map& map, LootPlayer& owner, Unit& killer, uint32 entry, uint32 kills, Setup setup, Verify verify)
{
    bool exercised = false;
    for (unsigned trial = 0; trial < 40 && !exercised; ++trial)
    {
        owner.Accept(kills); setup();
        Actor victim(&map, entry, ++victimGuid, owner.GetPosition());
        Require(map.AddToMap(static_cast<Creature*>(&victim)), "Native victim map admission failed");
        // Seed only the tap/damage-contribution input, then execute the full
        // actual Unit::Kill flow. No modeled recipient/reward/loot callbacks.
        victim.LowerPlayerDamageReq(uint32(victim.GetMaxHealth())); victim.SetLootRecipient(&owner);
        killer.Kill(&victim, false);
        Require(victim.isDead(false) && owner.personalLoot.count(victim.GetGUID()), "Native kill/personal loot split failed");
        auto& loot = victim.loot;
        unsigned before = FragmentView(loot, owner);
        Require(owner.GetQuestObjectiveData(44923, 115845) == kills, "Native delayed credit ran before its update");
        Tick(owner, 100);
        Require(owner.GetQuestObjectiveData(44923, 115845) == std::min(kills + 1, 10u), "Native kill credit did not advance exactly once");
        if (!before) continue; // Preserve the live drop chances/counts; retry independent cases.
        exercised = true; verify(loot, victim, before);
    }
    Require(exercised, "No fragment rolled in forty independent native kills");
}
static void CheckDenied(Loot& loot, LootPlayer& player, unsigned before, char const* reason)
{
    auto amount = player.GetItemCount(142487, true); auto unlooted = loot.unlootedCount;
    auto view = FragmentView(loot, player);
    auto slot = uint8(loot.items.size());
    auto available = loot.LootItemInSlot(slot, &player);
    player.StoreLootItem(slot, &loot);
    std::cout << "DENY reason=" << reason << " before=" << before << " view=" << view << " slot=" << bool(available)
        << " inventory=" << amount << "->" << player.GetItemCount(142487, true) << std::endl;
#ifdef LOST_ARMY_NATIVE_BASELINE
    Require(view == before && available && player.GetItemCount(142487, true) == amount + before, "Baseline did not reproduce stale inventory receipt");
#else
    Require(!view && !available && player.GetItemCount(142487, true) == amount && loot.unlootedCount == unlooted && !loot.hasItemFor(&player), "Native stale loot was shown, consumed or mutated");
#endif
}
static void CheckOtherCaches(LootPlayer& player, ConditionList const& conditions)
{
    for (bool personal : {false, true}) for (unsigned kind = 0; kind < 3; ++kind)
    {
        player.Accept(9); Loot loot; loot.SetLootOwner(&player); loot.isClear = false; loot.personal = personal;
        LootStoreItem entry(kind == 2 ? 0 : 142487, kind == 2 ? 1220 : 0, 0, 100.0f, false, 0, 0, 1, 1);
        entry.conditions = conditions; loot.AddItem(entry);
        Require(loot.items.size() == 1, "Controlled cache item was not constructed natively");
        loot.items[0].freeforall = kind == 0;
        (loot.*Access(FillCachesTag{}))(&player, true);
        auto view = [&]()
        {
            WorldPackets::Loot::LootResponse packet; loot.BuildLootResponse(packet, &player, ALL_PERMISSION);
            Require(packet.Write()->size() > 0, "Native cache packet serialization failed");
            return kind == 2 ? packet.Currencies.size() : packet.Items.size();
        };
        Require(view() == 1 && loot.LootItemInSlot(0, &player), "Eligible cached item was not available");
        auto unlooted = loot.unlootedCount;
        player.SetQuestObjectiveData(landing, &landing->GetObjectives()[0], 10);
#ifdef LOST_ARMY_NATIVE_BASELINE
        Require(view() == 1 && loot.LootItemInSlot(0, &player), "Baseline no longer reproduces cached branch admission");
#else
        Require(view() == 0 && !loot.LootItemInSlot(0, &player) && loot.unlootedCount == unlooted, "A cached FFA/conditional/currency branch bypassed current conditions");
#endif
    }
    std::cout << "PASS: actual native FFA, conditional and currency cache view/slot branches exercise current conditions for shared and personal loot.\n";
}
static void CheckAutoStore(LootPlayer& player, ConditionList const& conditions)
{
    // Read-only branch fixture: load the same actual fragment rows as ordinary
    // conditional loot, making a physical slot exist without an admitted cache.
    // No runtime row, chance, count or live item metadata is changed.
    LootStore store("(SELECT Entry,Item,Currency,Reference,Chance,0 AS QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM creature_loot_template WHERE Entry IN(116180,116600,116601) AND Item=142487) AS native_autostore", "entry", true, "");
    Require((store.*Access(LoadLootTableTag{}))() == 3, "Native AutoStore fixture loader failed");
    for (auto condition : conditions)
        Require((sConditionMgr->*Access(AttachLootConditionTag{}))(condition, store.GetLootForConditionFill(116180)), "Native AutoStore fixture condition attachment failed");
    player.Accept(10); auto before = player.GetItemCount(142487, true); bool rolled = false;
    for (unsigned i = 0; i < 40 && !rolled; ++i) rolled = player.AutoStoreLoot(NULL_BAG, NULL_SLOT, 116180, store, false, 0);
    Require(rolled && player.GetItemCount(142487, true) == before, "Native AutoStore failed to safely skip a rejected physical slot");
    std::cout << "PASS: full native AutoStoreLoot safely handles a physical conditional slot rejected by current admission.\n";
}
#ifndef LOST_ARMY_NATIVE_SUPPORT_ONLY
static int LootRun(int argc, char** argv)
{
    InitializeNative(argc, argv); opcodeTable.Initialize(); LoadLootInputs();
    sSpellMgr->LoadSpellBonusess(); sSpellMgr->LoadSpellThreats();
    LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
    Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Read-only login pool failed");
    LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY"); LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
    auto readonly = LoginDatabase.Query("SELECT @@session.transaction_read_only"); Require(readonly && readonly->Fetch()[0].GetBool(), "Login session not read-only");
    // Same bounded socketless native session lifetime as the offensive probe:
    // skip its destructor's account UPDATE; no character DB is opened.
    auto session = new WorldSession(0, std::string("Lost Army native probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    auto otherSession = new WorldSession(0, std::string("Lost Army group probe"), {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
    HashMapHolder<Player>::SetSize(32); HashMapHolder<Pet>::SetSize(32);
    {
        Map map(1220, 300, 9999996, DIFFICULTY_NONE);
        LootPlayer player(session, map, 100, {5478.0f, 470.0f, 150.0f, 0.0f}); player.Accept(9);
        Require(player.CanContact() && player.HasQuestForItem(142487), "Native task/item admission failed");
        LootPet pet(player, map);
        Require(pet.isPet() && pet.GetCharmerOrOwnerPlayerOrPlayerItself() == &player && pet.GetOwner() == &player, "Actual Pet ownership did not resolve");
        for (uint8 race : {uint8(RACE_HUMAN), uint8(RACE_ORC)}) for (bool petKill : {false, true})
        {
            player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race); player.setFactionForRace(race);
            Unit& killer = petKill ? static_cast<Unit&>(pet) : static_cast<Unit&>(player);
            for (uint32 entry : {116180u, 116600u, 116601u})
                WithKillLoot(map, player, killer, entry, 9, [] {}, [&](Loot& loot, Actor&, unsigned before) { CheckDenied(loot, player, before, "tenth-credit"); });
            for (uint32 questId : {44923u, 45088u}) for (auto status : {QUEST_STATUS_NONE, QUEST_STATUS_COMPLETE})
                WithKillLoot(map, player, killer, 116180, 8, [] {}, [&](Loot& loot, Actor&, unsigned before)
                {
                    player.SetQuestStatus(questId, status);
                    if (status == QUEST_STATUS_NONE) player.SetQuestSlot(questId == 44923 ? 0 : 1, 0);
                    CheckDenied(loot, player, before, questId == 44923 ? "landing-state" : "fragment-quest-state");
                });
            WithKillLoot(map, player, killer, 116180, 8, [] {}, [&](Loot& loot, Actor&, unsigned before)
            {
                auto amount = player.GetItemCount(142487, true);
                Require(FragmentView(loot, player) == before && loot.hasItemFor(&player), "Eligible below-quota recipient lost loot");
                player.StoreLootItem(uint8(loot.items.size()), &loot);
                Require(player.GetItemCount(142487, true) == amount + before, "Eligible native inventory receipt failed");
            });
        }
        std::cout << "PASS: both factions and actual Player/Pet Unit::Kill credit, all three entries, quota, completed/abandoned landing/fragment quest and eligible native inventory receipt.\n";

        LootPlayer member(otherSession, map, 101, player.GetPosition());
        Group group; group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(1000, 0);
        for (auto p : {&player, &member})
        {
            Group::MemberSlot slot; slot.Guid = p->GetGUID(); slot.Class = p->getClass();
            const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot); p->SetGroup(&group, 0);
        }
        sGroupMgr->AddGroup(&group);
        WithKillLoot(map, player, pet, 116600, 8, [&] { Tick(member, 100); member.Accept(9); }, [&](Loot& loot, Actor&, unsigned before)
        {
            Require(FragmentView(loot, member) == before, "Nearby member was not in the native shared quest loot");
            Tick(member, 100); Require(member.GetQuestObjectiveData(44923, 115845) == 10, "Nearby native group credit missing");
            CheckDenied(loot, member, before, "independent-group-quota");
#ifdef LOST_ARMY_NATIVE_BASELINE
            // Old receipt can consume a shared non-FFA item; this is part of
            // the recorded baseline, not a reason to change live item flags.
#else
            Require(FragmentView(loot, player) == before, "Rejecting one member removed another eligible player's loot");
            auto amount = player.GetItemCount(142487, true); player.StoreLootItem(uint8(loot.items.size()), &loot);
            Require(player.GetItemCount(142487, true) == amount + before, "Eligible group member could not receive native loot");
#endif
        });
        WithKillLoot(map, player, pet, 116180, 8, [&] { Tick(member, 100); member.Accept(0); }, [&](Loot& loot, Actor&, unsigned before)
        {
            Tick(member, 100);
            group.*Access(GroupFlagsTag{}) = GROUP_FLAG_RAID;
            CheckDenied(loot, player, before, "party-to-raid");
            group.*Access(GroupFlagsTag{}) = GROUP_FLAG_NONE;
        });
        WithKillLoot(map, player, pet, 116180, 8, [&] { Tick(member, 100); member.Accept(0); }, [&](Loot& loot, Actor&, unsigned before)
        {
            Tick(member, 100);
            auto& item = loot.quest_items.front(); Require(!item.follow_loot_rules, "Live fragment group-rule metadata changed");
            // Controlled branch fixture: preserve the existing display-only
            // group/master-looter exception for unrelated FOLLOW_LOOT_RULES items.
            item.follow_loot_rules = true; group.SetLootMethod(MASTER_LOOT); group.SetLooterGuid(player.GetGUID());
            player.SetQuestStatus(45088, QUEST_STATUS_NONE); player.SetQuestSlot(1, 0);
            Require(FragmentView(loot, player) == before, "Group loot-master display exception regressed");
            group.SetLootMethod(PERSONAL_LOOT); item.follow_loot_rules = false;
        });
        player.SetGroup(nullptr); member.SetGroup(nullptr); sGroupMgr->RemoveGroup(&group);
        std::cout << "PASS: actual Group references/registry and shared corpse caches retain independent member eligibility after native pet kill credit.\n";

        WithKillLoot(map, player, player, 116180, 8, [] {}, [&](Loot& loot, Actor&, unsigned before)
        {
            player.SetQuestStatus(44923, QUEST_STATUS_NONE); player.SetQuestSlot(0, 0);
            CheckDenied(loot, player, before, "abandon-before-reaccept");
#ifndef LOST_ARMY_NATIVE_BASELINE
            player.Accept(9);
            Require(FragmentView(loot, player) == before, "Denied cache entry was destroyed rather than remaining eligible for a valid retry");
            loot.isOnlyQuest = false;
            loot.AddItem(LootStoreItem(2589, 0, 0, 100.0f, false, 0, 0, 1, 1)); // Private ordinary-item control.
            player.SetQuestStatus(44923, QUEST_STATUS_NONE); player.SetQuestSlot(0, 0);
            auto cloth = player.GetItemCount(2589, true); player.StoreLootItem(0, &loot);
            Require(player.GetItemCount(2589, true) == cloth + 1, "Unrelated ordinary loot stopped being receivable");
#endif
        });
#ifndef LOST_ARMY_NATIVE_BASELINE
        std::cout << "PASS: stale rejection does not destroy shared cache state; valid re-entry and ordinary inventory receipt remain available.\n";
#endif
        WithKillLoot(map, player, player, 116180, 8, [] {}, [&](Loot& loot, Actor&, unsigned)
        {
            auto conditions = loot.quest_items.front().conditions;
            CheckOtherCaches(player, conditions); CheckAutoStore(player, conditions);
        });
    }
    LoginDatabase.Close(); HotfixDatabase.Close(); WorldDatabase.Close();
    return 0;
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); SetUnhandledExceptionFilter(CrashTrace);
    try { return LootRun(argc, argv); }
    catch (std::exception const& error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif
