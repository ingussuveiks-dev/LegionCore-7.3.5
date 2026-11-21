#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"

struct DarkmoonSpellConditionTag
{
    using Type = ConditionContainer ConditionMgr::*;
    friend Type Access(DarkmoonSpellConditionTag);
};
template struct FixtureAccess<DarkmoonSpellConditionTag, &ConditionMgr::ConditionStore>;

struct DarkmoonEventQuestTag
{
    using Type = std::unordered_map<uint32, uint16> GameEventMgr::*;
    friend Type Access(DarkmoonEventQuestTag);
};
template struct FixtureAccess<DarkmoonEventQuestTag, &GameEventMgr::_questToEventLinks>;

namespace
{
constexpr uint32 Strength = 29433;
constexpr uint32 Guide = 71634;
constexpr uint32 Trophy = 71096;
constexpr uint16 Faire = 75;

Quest const* LoadStrength()
{
    auto row = WorldDatabase.Query(
#include "NativeDarkmoonStrength.inc"
    );
    Require(bool(row), "Missing Test Your Strength");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Strength] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Strength, quest);
    auto objective = WorldDatabase.Query(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged FROM quest_objectives WHERE QuestID=29433");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == Trophy &&
        objective->Fetch()[5].GetUInt32() == 250, "Missing real 250-trophy objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER | QUEST_SPECIAL_FLAGS_REPEATABLE);
    Require(quest->IsSeasonal() && quest->ItemDrop[0] == Trophy && quest->ItemDropQuantity[0] == 250 &&
        !quest->SourceItemId, "Quest season or trophy cleanup differs from the repaired route");

    auto relations = WorldDatabase.Query(
        "SELECT eventEntry FROM game_event_seasonal_questrelation WHERE questId=29433");
    Require(relations && relations->GetRowCount() == 1 && relations->Fetch()[0].GetUInt16() == Faire, "Wrong Faire repeat bucket");
    (sGameEventMgr->*Access(DarkmoonEventQuestTag{}))[Strength] = relations->Fetch()[0].GetUInt16();
    auto timing = WorldDatabase.Query("SELECT occurence, length FROM game_event WHERE eventEntry=75 AND world_event=0");
    Require(bool(timing), "Missing configured Faire schedule");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(Faire + 1);
    events[Faire].occurence = timing->Fetch()[0].GetUInt64();
    events[Faire].length = timing->Fetch()[1].GetUInt64();

    auto conditions = WorldDatabase.Query(
        "SELECT SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1 FROM conditions "
        "WHERE SourceGroup=0 AND ElseGroup=0 AND ((SourceTypeOrReferenceId=17 AND SourceEntry=100776) "
        "OR (SourceTypeOrReferenceId IN (19,20) AND SourceEntry=29433))");
    Require(conditions && conditions->GetRowCount() == 5, "Wrong native spell, offer or marker conditions");
    do
    {
        auto fields = conditions->Fetch();
        auto condition = new Condition;
        condition->SourceType = ConditionSourceType(fields[0].GetUInt32());
        condition->SourceEntry = fields[1].GetUInt32();
        condition->ConditionType = ConditionTypes(fields[2].GetUInt32());
        condition->ConditionTarget = fields[3].GetUInt8();
        condition->ConditionValue1 = fields[4].GetUInt32();
        Require(condition->ConditionTarget == 0, "A self or player condition was attached to a discarded target");
        Require((sConditionMgr->*Access(ValidateSourceTag{}))(condition) && sConditionMgr->isConditionTypeValid(condition),
            "A native Darkmoon condition is invalid");
        (sConditionMgr->*Access(DarkmoonSpellConditionTag{}))[condition->SourceType][condition->SourceEntry].push_back(condition);
    } while (conditions->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    auto loadLoot = [](LootStore& store, char const* where)
    {
        auto& table = store.*Access(LootTableNameTag{});
        auto original = table;
        table += where;
        auto loaded = (store.*Access(LoadLootTableTag{}))();
        table = original;
        return loaded;
    };
    Require(loadLoot(LootTemplates_Spell, " WHERE Entry=100776") == 1, "Native Darkmoon spell loot row missing");
    Require(loadLoot(LootTemplates_Creature, " WHERE Entry=299") > 0, "Native test creature loot missing");
    auto aura = sSpellMgr->GetSpellInfo(100775);
    auto loot = sSpellMgr->GetSpellInfo(100776);
    Require(aura && aura->GetEffect(EFFECT_0)->TriggerSpell == 100776 && loot &&
        loot->GetEffect(EFFECT_0)->Effect == SPELL_EFFECT_LOOT_BONUS &&
        loot->GetEffect(EFFECT_0)->MiscValue == 74219, "The Legion guide/spell chain differs");
    return quest;
}

void GrantGuide(LootPlayer& player)
{
    Require(player.CastSpell(&player, 103413, true) == SPELL_CAST_OK, "Native guide delivery failed");
    Tick(player, 250);
    Require(player.GetItemCount(Guide) == 1 && player.GetAuraEffect(100775, EFFECT_0), "Guide did not apply its native carried aura");
}

void KillForStrength(Map& map, LootPlayer& owner, Unit& killer, bool tapped = true, uint8 level = 110)
{
    static uint32 guid = 49000;
    Actor victim(&map, 299, ++guid, owner.GetPosition());
    victim.SetLevel(level);
    Require(map.AddToMap(static_cast<Creature*>(&victim)), "Cannot publish creature for the kill route");
    if (tapped)
    {
        victim.SetLootRecipient(&owner);
        victim.LowerPlayerDamageReq(victim.GetMaxHealth());
    }
    killer.Kill(&victim, false);
    Require(victim.isDead(false), "The real kill did not complete");
    killer.Kill(&victim, false);
    Tick(owner, 100);
}

void CheckWorldKills(Quest const* quest)
{
    Map map(1, 300, 0, DIFFICULTY_NONE);
    Position point{1985.0f, -4793.0f, 56.0f, 0.0f};
    LootPlayer owner(SupplySession(), map, 29433, point);
    LootPlayer member(SupplySession(), map, 29434, point);
    GrantGuide(owner);
    KillForStrength(map, owner, owner);
    Require(!owner.GetItemCount(Trophy), "A guide without an accepted quest gave a trophy");

    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.CastSpell(&owner, 100776, false) != SPELL_CAST_OK, "Bonus loot accepted a normal cast");
    KillForStrength(map, owner, owner, false);
    KillForStrength(map, owner, owner, true, 1);
    Require(!owner.GetItemCount(Trophy), "An uncredited or gray kill gave a trophy");

    LootPet pet(owner, map);
    KillForStrength(map, owner, owner);
    owner.RemoveAurasDueToSpell(100775);
    KillForStrength(map, owner, pet);
    Require(owner.GetItemCount(Trophy) == 2 && owner.GetAuraEffect(100775, EFFECT_0), "Player/pet kills did not each award one trophy and retain the guide aura");

    Actor guardian(&map, 299, 49550, point);
    guardian.SetOwnerGUID(owner.GetGUID());
    Require(map.AddToMap(static_cast<Creature*>(&guardian)), "Cannot publish the owned companion");
    KillForStrength(map, owner, guardian);
    Require(owner.GetItemCount(Trophy) == 3, "An owned companion kill did not reach its player");

    member.AddQuestAndCheckCompletion(quest, nullptr);
    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(29433, 0);
    for (auto player : {&owner, &member})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        player->SetGroup(&group, 0);
    }
    sGroupMgr->AddGroup(&group);
    KillForStrength(map, owner, pet);
    Require(owner.GetItemCount(Trophy) == 4 && !member.GetItemCount(Trophy), "No-guide party eligibility failed");
    GrantGuide(member);
    KillForStrength(map, owner, pet);
    Tick(member, 100);
    Require(owner.GetItemCount(Trophy) == 5 && member.GetItemCount(Trophy) == 1, "Nearby guide-carrying party member lost credit");
    SupplyMove(member, {point.GetPositionX() + 150.0f, point.GetPositionY(), point.GetPositionZ(), 0.0f});
    KillForStrength(map, owner, pet);
    Require(owner.GetItemCount(Trophy) == 6 && member.GetItemCount(Trophy) == 1, "Distant party member received credit");
    sGroupMgr->RemoveGroup(&group);
    owner.SetGroup(nullptr);
    member.SetGroup(nullptr);

    auto guide = owner.GetItemByEntry(Guide);
    ItemPosCountVec bank;
    Require(guide && owner.CanBankItem(INVENTORY_SLOT_BAG_0, BANK_SLOT_ITEM_START, bank, guide, false) == EQUIP_ERR_OK,
        "Could not put the real guide in a bank slot");
    owner.RemoveItem(guide, true);
    owner.BankItem(bank, guide, true);
    KillForStrength(map, owner, pet);
    Require(owner.HasItemCount(Guide, 1, true) && !owner.HasItemCount(Guide, 1, false) && owner.GetItemCount(Trophy) == 6,
        "A guide in the bank gave progress");

    AbandonSupply(owner, Strength);
    Require(!owner.GetItemCount(Trophy) && owner.HasItemCount(Guide, 1, true), "Abandon did not remove only trophies");
    AbandonSupply(member, Strength);
    member.AddQuestAndCheckCompletion(quest, nullptr);
    KillForStrength(map, member, member);
    Require(member.GetItemCount(Trophy) == 1, "A fresh acceptance did not restore normal credit");
    std::cout << "PASS native player, pet, owned companion, party distance, XP eligibility, guide bank and abandon/retry\n";
}

void CheckFairTurnIn(Quest const* quest)
{
    auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=2170 AND id=14832 AND map=974");
    Require(bool(row), "Real island Kerri Hicks placement missing");
    auto fields = row->Fetch();
    Position point{fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
    Map map(974, 300, 0, DIFFICULTY_NONE);
    LootPlayer owner(SupplySession(), map, 29435, point);
    GrantGuide(owner);
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    Require(!owner.CanTakeQuest(quest, false), "Inactive Faire offered the quest");
    active.insert(Faire);
    Require(owner.CanTakeQuest(quest, false), "The active Faire did not offer the quest");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    GiveSupply(owner, Trophy, 249);
    active.erase(Faire);
    KillForStrength(map, owner, owner);
    Require(owner.GetItemCount(Trophy) == 250 && owner.GetQuestStatus(Strength) == QUEST_STATUS_COMPLETE,
        "The 250th real trophy did not complete the active quest outside the Faire window");
    KillForStrength(map, owner, owner);
    Require(owner.GetItemCount(Trophy) == 250, "Completed quest continued dropping loot");
    Require(!owner.CanRewardQuest(quest, false), "Completed quest allowed an off-event reward");
    active.insert(Faire);
    Require(owner.CanRewardQuest(quest, false), "The reopened Faire denied a legitimate turn-in");
    owner.RewardQuest(quest, 0, nullptr, false);
    Require(owner.IsQuestSeasonalRewarded(Strength) && !owner.GetItemCount(Trophy) && owner.GetItemCount(Guide) == 1,
        "Reward did not clear trophies and retain the guide");
    Require(!owner.CanTakeQuest(quest, false), "The same Faire offered a second turn-in");

    auto& faire = const_cast<GameEventData&>(sGameEventMgr->GetEventMap()[Faire]);
    faire.start = GameTime::GetGameTime() - 120;
    sWorld->ResetEventSeasonalQuests(Faire);
    CharacterDatabase.DirectExecute("REPLACE INTO character_queststatus_seasonal (guid,account,quest,event) VALUES (29435,0,29433,75)");
    sWorld->ResetEventSeasonalQuests(Faire);
    Require(bool(CharacterDatabase.Query("SELECT quest FROM character_queststatus_seasonal WHERE guid=29435 AND quest=29433 AND event=75")),
        "Reapplying the same event reset a recorded reward");
    // Simulate the next active recurrence with its scheduled start a few seconds ago.
    faire.start = GameTime::GetGameTime() - 10;
    sWorld->ResetEventSeasonalQuests(Faire);
    Require(!CharacterDatabase.Query("SELECT quest FROM character_queststatus_seasonal WHERE guid=29435 AND quest=29433 AND event=75"),
        "A new Faire failed to clear the old reward");
    active.erase(Faire);
    std::cout << "PASS native 250-trophy objective, season turn-in, preserved guide and restart-safe repeat bucket\n";
}

void CheckHonorKills(Quest const* quest)
{
    Map map(1, 300, 0, DIFFICULTY_NONE);
    Position point{1985.0f, -4793.0f, 56.0f, 0.0f};
    LootPlayer owner(SupplySession(), map, 29436, point);
    LootPlayer ally(SupplySession(), map, 29437, point);
    LootPlayer enemy(SupplySession(), map, 29438, point);
    for (auto player : {&owner, &ally, &enemy})
    {
        player->SetLevel(19);
        player->setFactionForRace(player == &enemy ? RACE_ORC : RACE_HUMAN);
    }
    GrantGuide(owner);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    owner.Kill(&ally, false);
    Tick(owner, 100);
    Require(!owner.GetItemCount(Trophy), "Same-faction kill granted a trophy without honor");
    LootPet pet(owner, map);
    pet.Kill(&enemy, false);
    Tick(owner, 100);
    Require(owner.GetItemCount(Trophy) == 1, "Honorable kill by the pet did not reward its owner");
    std::cout << "PASS native same-faction rejection and honorable enemy kill by a pet\n";
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
        sSpellMgr->LoadSpellProcEvents();
        sSpellMgr->LoadSpellProcs();
        sSpellMgr->LoadSpellPrcoCheck();
        sSpellMgr->LoadSpellTriggered();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}, {974, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}, {974, {}}});
        LoadGameObjectModelList(argv[2]);
        auto quest = LoadStrength();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Reward writes require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(), "Reward requires an empty fixture");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckWorldKills(quest);
        CheckHonorKills(quest);
        CheckFairTurnIn(quest);
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
