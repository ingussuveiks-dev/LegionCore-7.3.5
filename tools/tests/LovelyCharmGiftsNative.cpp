#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "ItemEnchantmentMgr.h"
#include "NPCPackets.h"
#include "SmartAI.h"
#include "SmartScriptMgr.h"

namespace
{
constexpr uint32 Charm = 49655;
constexpr uint32 Kit = 49661;
constexpr uint32 Bracelet = 49916;
constexpr uint32 Token = 49927;
constexpr uint32 KitAura = 69510;
constexpr uint32 CharmProc = 69511;
constexpr uint32 Recipe = 69531;
constexpr uint16 Holiday = 8;
constexpr uint32 ProphetGift = 24611;
constexpr uint32 GossipMenuId = 10948;

void SelectFixtureRows(char const* sql)
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute(sql);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
}

void LoadGifts()
{
    auto rows = WorldDatabase.Query(
#include "NativeLovelyCharmGifts.inc"
    );
    Require(rows && rows->GetRowCount() == 8, "The eight native holiday gifts are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        auto id = quest->GetQuestId();
        quests[id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(id, quest);
        Require(quest->SourceItemId == Kit && quest->IsDaily() && quest->RewardItemId[0], "Gift lost its daily, kit or card metadata");
#ifdef LOVELY_CHARM_GIFTS_BASELINE
        Require(!quest->RewardItemId[1], "The old daily now has the second reward");
#else
        Require(quest->RewardItemId[1] == Token && quest->RewardItemCount[1] == 5, "Gift did not restore five Love Tokens");
#endif
    } while (rows->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (24597,24609,24610,24611,24612,24613,24614,24615)");
    Require(objectives && objectives->GetRowCount() == 8, "Missing the original eight bracelet objectives");
    do
    {
        auto fields = objectives->Fetch();
        Require(fields[2].GetUInt8() == QUEST_OBJECTIVE_ITEM && fields[4].GetUInt32() == Bracelet && fields[5].GetUInt32() == 1,
            "The actual gift objective must take one bracelet");
        quests[fields[1].GetUInt32()]->LoadQuestObjective(fields);
    } while (objectives->NextRow());

    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID IN (24597,24609,24610,24611,24612,24613,24614,24615)");
    Require(addons && addons->GetRowCount() == 8, "Gift addon metadata is incomplete");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    } while (addons->NextRow());

    auto event = WorldDatabase.Query("SELECT holiday,occurence,length,YEAR(end_time) FROM game_event WHERE eventEntry=8 AND world_event=0");
    Require(event && event->Fetch()[0].GetUInt32() == HOLIDAY_LOVE_IS_IN_THE_AIR, "The configured seasonal event is missing");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(Holiday + 1);
    events[Holiday].holiday_id = HOLIDAY_LOVE_IS_IN_THE_AIR;
    events[Holiday].occurence = event->Fetch()[1].GetUInt32();
    events[Holiday].length = event->Fetch()[2].GetUInt32();
    Require(events[Holiday].occurence == 525600 && events[Holiday].length == 20160, "The configured yearly recurrence or two-week window changed");
#ifdef LOVELY_CHARM_GIFTS_BASELINE
    Require(event->Fetch()[3].GetUInt32() == 2021, "The original calendar is no longer expired");
#else
    Require(event->Fetch()[3].GetUInt32() == 2035, "The seasonal calendar remains expired");
#endif
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    LoadRandomEnchantmentsTable();
    auto& creatureLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto originalLoot = creatureLoot;
    creatureLoot += " WHERE Entry IN(299,17183)";
    auto lootRows = (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    creatureLoot = originalLoot;
    Require(lootRows > 0 && LootTemplates_Creature.HaveLootFor(17183) && LootTemplates_Creature.HaveLootFor(299),
        "The original creature loot groups did not load");
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadNPCText();
    sQuestDataStore->LoadPointsOfInterest();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    SelectFixtureRows("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (24597,24609,24610,24611,24612,24613,24614,24615)");
    SelectFixtureRows("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (24597,24609,24610,24611,24612,24613,24614,24615)");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    SelectFixtureRows("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "(SourceTypeOrReferenceId=15 AND SourceGroup=10948) OR "
        "(SourceTypeOrReferenceId IN(13,17,19,20) AND SourceEntry IN(69511,70648,24597,24609,24610,24611,24612,24613,24614,24615))");
    sConditionMgr->LoadConditions();
    SelectFixtureRows("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE source_type=0 AND entryorguid IN (37887,38039,38040,38041,38042,38043,38044,38045)");
    sSmartScriptMgr->LoadSmartAIFromDB();
}

struct CapitalActor : Actor
{
    using Actor::Actor;
    void AddToWorld() override
    {
        sObjectAccessor->AddObject(static_cast<Creature*>(this));
        Unit::AddToWorld();
        Require(AIM_Initialize(GetCreatureTemplate()->AIName == "SmartAI" ? static_cast<CreatureAI*>(new SmartAI(this)) :
            new NullCreatureAI(this)), "The actual capital actor could not initialize");
    }
};

void SelectKit(LootPlayer& player, CapitalActor& giver)
{
    WorldPacket data(CMSG_GOSSIP_SELECT_OPTION);
    data << giver.GetGUID() << GossipMenuId << uint32(0);
    data.WriteBits(0, 8);
    data.FlushBits();
    WorldPackets::NPC::GossipSelectOption packet(std::move(data));
    packet.Read();
    player.GetSession()->HandleGossipSelectOption(packet);
    giver.Unit::Update(500);
    Tick(player, 100);
}

void AcceptGift(LootPlayer& player, Creature const& giver, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    Tick(player, 100);
}

void OfferKit(LootPlayer& player, Creature& giver)
{
    WorldPacket data(CMSG_TALK_TO_GOSSIP);
    data << giver.GetGUID();
    WorldPackets::NPC::Hello packet(std::move(data));
    packet.Read();
    player.GetSession()->HandleGossipHelloOpcode(packet);
}

void ConfigureGiftPlayer(LootPlayer& player, uint8 race)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    player.setFactionForRace(race);
    player.InitDisplayIds();
    player.SetLevel(20);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
}

void CheckCharmKills(Map& map, LootPlayer& player, Unit& killer, uint8 level, bool expectCharm)
{
    uint32 const initial = player.GetItemCount(Charm);
    uint32 const entry = level == 1 ? 299 : 17183;
    static uint32 guid = 250000;
    for (uint32 i = 0; i < 48; ++i)
    {
        Actor victim(&map, entry, ++guid, player.GetPosition());
        victim.SetLevel(level);
        victim.SetLootRecipient(&player);
        victim.LowerPlayerDamageReq(victim.GetMaxHealth());
        Require(map.AddToMap(static_cast<Creature*>(&victim)), "The original creature could not enter the test map");
        Require(player.isHonorOrXPTarget(&victim) == (level != 1), "The native scaling invalidated the kill test's eligibility input");
        GameTime::UpdateGameTimers();
        killer.Kill(&victim, false);
        Require(victim.isDead(false), "A real creature kill did not execute");
        killer.Kill(&victim, false);
        Tick(player, 200);
        if (player.GetItemCount(Charm) != initial)
            break;
    }
    auto check = "Real kill and native charm proc disagreed, level=" + std::to_string(level) +
        " pet=" + std::to_string(killer.isPet()) + " expected increase=" + std::to_string(expectCharm) +
        " before=" + std::to_string(initial) + " after=" + std::to_string(player.GetItemCount(Charm));
    Require(player.GetItemCount(Charm) == initial + (expectCharm ? 1 : 0), check.c_str());
}

void CraftBracelet(LootPlayer& player)
{
    Item* charm = player.GetItemByEntry(Charm);
    Require(charm, "The actual charm cast item is missing");
    SpellCastTargets targets;
    targets.SetUnitTarget(&player);
    int32 misc[2] = {};
    player.CastItemUseSpell(charm, targets, misc, ObjectGuid::Empty);
    Tick(player, 5500);
}

void CheckNearbyGroup(Map& map, CapitalActor& giver, LootPlayer& owner, LootPlayer& withoutKit)
{
    LootPlayer equipped(SupplySession(), map, 2461103, owner.GetPosition());
    ConfigureGiftPlayer(equipped, RACE_DRAENEI);
    OfferKit(equipped, giver);
    SelectKit(equipped, giver);
    Require(equipped.GetItemCount(Kit) == 1, "The extra party member did not receive their own kit");

    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(2461101, 0);
    for (LootPlayer* member : {&owner, &withoutKit, &equipped})
    {
        Group::MemberSlot slot;
        slot.Guid = member->GetGUID();
        slot.Class = member->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        member->SetGroup(&group, 0);
    }
    CheckCharmKills(map, owner, owner, 20, true);
    bool const filtered = equipped.GetItemCount(Charm) == 1 && !withoutKit.GetItemCount(Charm);
    for (LootPlayer* member : {&owner, &withoutKit, &equipped})
        member->SetGroup(nullptr);
    Require(filtered, "The native nearby raid target did not enforce each member's carried kit");
}

void CheckProphet()
{
    auto placed = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation FROM creature WHERE map=530 "
        "AND zoneId=3557 AND guid IN (189170,36574) ORDER BY guid DESC");
    Require(placed && placed->GetRowCount() == 2, "Missing Peddlefeet or Velen in the actual Exodar");
    auto position = [](Field* row) { return Position{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()}; };
    Map map(530, 565, 0, DIFFICULTY_NONE);
    auto row = placed->Fetch();
    auto giverPos = position(row);
    map.LoadGrid(giverPos.GetPositionX(), giverPos.GetPositionY());
    CapitalActor giver(&map, row[1].GetUInt32(), row[0].GetUInt32(), giverPos);
    Require(placed->NextRow(), "Cannot read the actual Velen placement");
    row = placed->Fetch();
    auto receiverPos = position(row);
    map.LoadGrid(receiverPos.GetPositionX(), receiverPos.GetPositionY());
    CapitalActor receiver(&map, row[1].GetUInt32(), row[0].GetUInt32(), receiverPos);
    for (auto actor : {&giver, &receiver})
        Require(map.AddToMap(static_cast<Creature*>(actor)), "Failed to publish a real quest actor");

    LootPlayer owner(SupplySession(), map, 2461101, giverPos), other(SupplySession(), map, 2461102, giverPos);
    ConfigureGiftPlayer(owner, RACE_DRAENEI);
    ConfigureGiftPlayer(other, RACE_DRAENEI);
    auto quest = sQuestDataStore->GetQuestTemplate(ProphetGift);
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
#ifdef LOVELY_CHARM_GIFTS_BASELINE
    Require(owner.CanTakeQuest(quest, false), "The original quest no longer bypasses the inactive event in direct admission");
#else
    Require(!owner.CanTakeQuest(quest, false), "The gift bypassed inactive event admission");
    OfferKit(owner, giver);
    Require(!owner.PlayerTalkClass->GetGossipMenu().GetItem(0), "The kit option was visible outside the holiday");
    SelectKit(owner, giver);
    Require(!owner.GetItemCount(Kit), "A stale kit option created an item outside the holiday");
#endif
    active.insert(Holiday);
    OfferKit(owner, giver);
    Require(owner.PlayerTalkClass->GetGossipMenu().GetMenuId() == GossipMenuId && owner.PlayerTalkClass->GetGossipMenu().GetItem(0),
        "The original quest-independent option is missing");
    SelectKit(owner, giver);
#ifdef LOVELY_CHARM_GIFTS_BASELINE
    Require(!owner.GetItemCount(Kit), "The original missing kit action no longer reproduces");
    AcceptGift(owner, giver, ProphetGift);
    Require(owner.GetItemCount(Kit) == 1 && owner.GetQuestStatus(ProphetGift) == QUEST_STATUS_INCOMPLETE,
        "Quest acceptance must still provide the original unique kit");
    AbandonSupply(owner, ProphetGift);
    Require(!owner.GetItemCount(Kit), "The original abandonment no longer destroys the independent kit");
    std::cout << "EXPECTED_BROKEN gift admitted off season, gossip granted nothing and abandonment removed the kit\n";
#else
    Require(owner.GetItemCount(Kit) == 1 && owner.HasAura(KitAura) && !other.GetItemCount(Kit) &&
        owner.GetQuestStatus(ProphetGift) == QUEST_STATUS_NONE, "The real gossip did not apply the kit and its carried aura to only the selecting player");
    OfferKit(owner, giver);
    Require(!owner.PlayerTalkClass->GetGossipMenu().GetItem(0), "An owned kit did not hide its unique option");
    SelectKit(owner, giver);
    Require(owner.GetItemCount(Kit) == 1, "A repeated selection created a duplicate kit");

    Require(other.CastSpell(&other, CharmProc, true) != SPELL_CAST_OK, "A direct charm spell skipped the required carried kit");
    CheckCharmKills(map, owner, owner, 1, false);
    CheckCharmKills(map, owner, owner, 20, true);
    LootPet pet(owner, map);
    CheckCharmKills(map, owner, pet, 20, true);
    CheckCharmKills(map, other, other, 20, false);
    active.erase(Holiday);
    CheckCharmKills(map, owner, owner, 20, false);
    active.insert(Holiday);
    CheckNearbyGroup(map, giver, owner, other);

    GiveSupply(owner, Charm, 6);
    Require(owner.GetItemCount(Charm) == 9, "Expected nine actual materials before the insufficient recipe test");
    CraftBracelet(owner);
    Require(!owner.GetItemCount(Bracelet) && owner.GetItemCount(Charm) == 9, "The native recipe consumed nine materials");
    GiveSupply(owner, Charm, 1);
    CraftBracelet(owner);
    Require(owner.GetItemCount(Bracelet) == 1 && !owner.GetItemCount(Charm), "The native recipe did not consume exactly ten and create a bracelet outside a quest");

    AcceptGift(owner, giver, ProphetGift);
    Require(owner.GetQuestStatus(ProphetGift) == QUEST_STATUS_COMPLETE && owner.GetItemCount(Kit) == 1,
        "The placed Peddlefeet did not admit the gift with an already crafted bracelet");
    AbandonSupply(owner, ProphetGift);
    Require(owner.GetItemCount(Bracelet) == 1 && owner.GetItemCount(Kit) == 1 && owner.HasAura(KitAura),
        "Abandonment took portable holiday materials");
    AcceptGift(owner, giver, ProphetGift);
    GiveSupply(owner, Bracelet, 1);
    SupplyMove(owner, receiverPos);
    active.erase(Holiday);
    Require(!owner.CanRewardQuest(quest, false) && owner.GetItemCount(Bracelet) == 2,
        "A saved complete gift could turn in after the event ended");
    active.insert(Holiday);
    Require(owner.CanInteractWithQuestGiver(&receiver) && owner.CanRewardQuest(quest, false), "The actual Velen could not take the gift");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = receiver.GetGUID();
    reward.QuestID = ProphetGift;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.IsQuestDailyRewarded(ProphetGift) && owner.GetItemCount(Bracelet) == 1 && owner.GetItemCount(Token) == 5 &&
        owner.GetItemCount(quest->RewardItemId[0]) == 1 && owner.GetItemCount(Kit) == 1 && owner.HasAura(KitAura),
        "Native turn-in did not take exactly one bracelet, award the correct card and tokens, and retain the reusable kit");
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetItemCount(Bracelet) == 1 && owner.GetItemCount(Token) == 5, "A repeated reward packet transferred items twice");
    std::cout << "PASS actual gossip, carried aura, XP/pet kill proc, ten-charm recipe, expiration, abandonment and Velen reward\n";
#endif
    active.erase(Holiday);
}

#ifndef LOVELY_CHARM_GIFTS_BASELINE
void CheckCapitalTurnIns()
{
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(Holiday);
    struct CapitalGift {uint32 quest; uint32 mascot; uint32 receiver; uint8 race;};
    for (auto const& gift : std::array<CapitalGift, 8>{{
        {24597, 11774505, 265840, RACE_HUMAN}, {24609, 189164, 82260, RACE_DWARF},
        {24610, 189161, 78609, RACE_NIGHTELF}, {24611, 189170, 36574, RACE_DRAENEI},
        {24612, 11774492, 264165, RACE_ORC}, {24613, 189148, 129343, RACE_UNDEAD_PLAYER},
        {24614, 189138, 94350, RACE_TAUREN}, {24615, 189153, 11672, RACE_BLOODELF}}})
    {
        auto rows = WorldDatabase.PQuery(
            "SELECT m.map,m.id,m.position_x,m.position_y,m.position_z,m.orientation,"
            "r.id,r.position_x,r.position_y,r.position_z,r.orientation "
            "FROM creature m JOIN game_event_creature e ON e.guid=m.guid AND e.eventEntry=8 "
            "JOIN creature r ON r.map=m.map AND r.zoneId=m.zoneId "
            "JOIN creature_queststarter o ON o.id=m.id AND o.quest=%u "
            "JOIN creature_questender f ON f.id=r.id AND f.quest=o.quest WHERE m.guid=%u AND r.guid=%u",
            gift.quest, gift.mascot, gift.receiver);
        Require(rows && rows->GetRowCount() == 1, "Missing the actual linked holiday giver and same-city receiver");
        auto row = rows->Fetch();
        Map map(row[0].GetUInt16(), 600 + gift.quest, 0, DIFFICULTY_NONE);
        Position start{row[2].GetFloat(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat()};
        Position finish{row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat()};
        map.LoadGrid(start.GetPositionX(), start.GetPositionY());
        map.LoadGrid(finish.GetPositionX(), finish.GetPositionY());
        CapitalActor giver(&map, row[1].GetUInt32(), gift.mascot, start), receiver(&map, row[6].GetUInt32(), gift.receiver, finish);
        Require(map.AddToMap(static_cast<Creature*>(&giver)) && map.AddToMap(static_cast<Creature*>(&receiver)), "Capital actors could not enter their actual map");
        LootPlayer player(SupplySession(), map, 2500000 + gift.quest, start);
        ConfigureGiftPlayer(player, gift.race);
        OfferKit(player, giver);
        SelectKit(player, giver);
        Require(player.GetItemCount(Kit) == 1 && player.HasAura(KitAura), "The capital's kit action failed");
        GiveSupply(player, Bracelet, 1);
        AcceptGift(player, giver, gift.quest);
        SupplyMove(player, finish);
        WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
        reward.QuestGiverGUID = receiver.GetGUID();
        reward.QuestID = gift.quest;
        reward.ItemChoiceID = 0;
        player.GetSession()->HandleQuestGiverChooseReward(reward);
        Require(player.IsQuestDailyRewarded(gift.quest) && !player.GetItemCount(Bracelet) && player.GetItemCount(Token) == 5 &&
            player.GetItemCount(sQuestDataStore->GetQuestTemplate(gift.quest)->RewardItemId[0]) == 1 &&
            player.GetItemCount(Kit) == 1 && player.HasAura(KitAura), "The capital's native reward path failed");
    }
    active.erase(Holiday);
    std::cout << "PASS all eight actual capital starter, gossip and receiver packet paths\n";
}
#endif
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}, {530, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}, {530, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Gift turn-in requires an isolated character schema");
        LoadGifts();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckProphet();
#ifndef LOVELY_CHARM_GIFTS_BASELINE
        CheckCapitalTurnIns();
#endif
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
