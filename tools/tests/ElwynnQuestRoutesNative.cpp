#define LOST_ARMY_NATIVE_SUPPORT_ONLY
#include "LostArmyLootNative.cpp"
#include "GameObject.h"
#include "GameObjectModel.h"
#include "QuestPackets.h"

void AddSC_elwynn_forest();

struct ElwynnNpcEnders
{
    using Type = QuestRelations QuestDataStoreMgr::*;
    friend Type Access(ElwynnNpcEnders);
};
template struct FixtureAccess<ElwynnNpcEnders, &QuestDataStoreMgr::_creatureQuestInvolvedRelations>;

struct ElwynnObjectEnders
{
    using Type = QuestRelations QuestDataStoreMgr::*;
    friend Type Access(ElwynnObjectEnders);
};
template struct FixtureAccess<ElwynnObjectEnders, &QuestDataStoreMgr::_goQuestInvolvedRelations>;

namespace
{
constexpr char QuestIds[] = "(11,35,37,40,45,46,52,71,239,5545)";
constexpr uint32 Wood = 13872;
constexpr uint32 Bundle = 176793;
constexpr uint32 Opening = 6478;

void LoadRelations(char const* table, QuestRelations& relations, uint64 expected)
{
    auto rows = WorldDatabase.PQuery("SELECT id,quest FROM %s WHERE quest IN %s", table, QuestIds);
    Require(rows && rows->GetRowCount() == expected, "Wrong number of questgiver relations");
    do
    {
        auto fields = rows->Fetch();
        relations.emplace(fields[0].GetUInt32(), fields[1].GetUInt32());
    } while (rows->NextRow());
}

void LoadInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeElwynnQuests.inc"
    );
    Require(rows && rows->GetRowCount() == 10, "Missing one of the ten Elwynn quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addons = WorldDatabase.PQuery(
        "SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,RewardMailTemplateID,RewardMailDelay,"
        "RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMinRepValue,RequiredMaxRepFaction,RequiredMaxRepValue,"
        "ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN %s", QuestIds);
    Require(addons && addons->GetRowCount() == 10, "Missing live prerequisites or provided items");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.PQuery(
        "SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN %s ORDER BY QuestID,StorageIndex", QuestIds);
    Require(objectives && objectives->GetRowCount() == 6, "Wrong number of item and kill objectives");
    do
    {
        auto fields = objectives->Fetch();
        auto quest = quests[fields[1].GetUInt32()];
        quest->LoadQuestObjective(fields);
        quest->SetSpecialFlag(fields[2].GetUInt8() == QUEST_OBJECTIVE_ITEM ? QUEST_SPECIAL_FLAGS_DELIVER :
            QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());
    auto conditions = WorldDatabase.PQuery(
        "SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN (19,20) AND SourceEntry IN %s", QuestIds);
    Require(conditions && !conditions->Fetch()[0].GetUInt32(), "New questgiver conditions need native admission");
    LoadRelations("creature_queststarter", *sQuestDataStore->GetCreatureQuestRelationMap(), 7);
    LoadRelations("creature_questender", sQuestDataStore->*Access(ElwynnNpcEnders{}), 8);
    LoadRelations("gameobject_queststarter", *sQuestDataStore->GetGOQuestRelationMap(), 3);
    LoadRelations("gameobject_questender", sQuestDataStore->*Access(ElwynnObjectEnders{}), 2);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto loadLoot = [](LootStore& store, char const* filter, uint32 expected)
    {
        auto& table = store.*Access(LootTableNameTag{});
        auto original = table;
        table += filter;
        auto loaded = (store.*Access(LoadLootTableTag{}))();
        table = original;
        Require(loaded == expected, "The scoped native loot loader found different quest sources");
    };
    loadLoot(LootTemplates_Gameobject, " WHERE Entry=176793", 1);
    loadLoot(LootTemplates_Creature,
        " WHERE Entry IN(118,1922,822) OR (Entry IN(46,732) AND Item=780) OR (Entry IN(97,478) AND Item=782)", 132);
}

struct Placement
{
    uint32 guid;
    uint32 respawn;
    Position position;
    G3D::Quat rotation;
};

std::vector<Placement> ObjectLocations(uint32 entry, uint64 expected)
{
    auto rows = WorldDatabase.PQuery(
        "SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=%u AND map=0 AND zoneId=12 AND phaseMask=1 ORDER BY guid", entry);
    Require(rows && rows->GetRowCount() == expected, "Missing Elwynn quest object placement");
    std::vector<Placement> result;
    do
    {
        auto row = rows->Fetch();
        result.push_back({row[0].GetUInt32(),row[1].GetUInt32(),{row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat(),row[5].GetFloat()},
            G3D::Quat(row[6].GetFloat(),row[7].GetFloat(),row[8].GetFloat(),row[9].GetFloat())});
    } while (rows->NextRow());
    return result;
}

Position CreatureLocation(uint32 id, uint32 offset = 0)
{
    auto row = WorldDatabase.PQuery(
        "SELECT position_x,position_y,position_z,orientation FROM creature WHERE id=%u AND map=0 AND zoneId=12 AND phaseMask=1 "
        "ORDER BY guid LIMIT %u,1", id, offset);
    Require(bool(row), "Expected Elwynn quest actor or target has no spawn");
    auto f = row->Fetch();
    return {f[0].GetFloat(),f[1].GetFloat(),f[2].GetFloat(),f[3].GetFloat()};
}

struct FixtureObject : GameObject
{
    FixtureObject(Map& map, uint32 entry, Placement const& placement) : respawn(placement.respawn)
    {
        m_DBTableGuid = placement.guid;
        Require(Create(placement.guid, entry, &map, 1, placement.position, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "Could not publish the original quest object");
        SetRespawnDelayTime(respawn);
        Update(10000);
        Require(getLootState() == GO_READY, "Native quest object did not reach its ready update");
    }

    ~FixtureObject()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }

    void Rearm()
    {
        Require(getLootState() == GO_JUST_DEACTIVATED, "Legitimate loot did not consume the used bundle");
        Update(10000);
        Require(!isSpawned() && m_respawnTime == GameTime::GetGameTime() + respawn, "Expected the native shared respawn interval");
        m_respawnTime = GameTime::GetGameTime();
        Update(10000);
        Require(isSpawned() && getLootState() == GO_READY, "The bundle did not respawn");
    }

    uint32 const respawn;
};

WorldSession* Session()
{
    return new WorldSession(0, "Elwynn quest route probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {});
}

void MoveTo(Player& player, Position const& point)
{
    player.GetMap()->PlayerRelocation(&player, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
}

void Accept(LootPlayer& player, WorldObject& giver, uint32 quest)
{
    MoveTo(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void Reward(LootPlayer& player, WorldObject& receiver, uint32 quest)
{
    MoveTo(player, receiver.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = quest;
    auto info = sQuestDataStore->GetQuestTemplate(quest);
    packet.ItemChoiceID = info->RewardChoiceItemId[0];
    if (!packet.ItemChoiceID && info->PackageID)
    {
        if (auto rewards = sDB2Manager.GetQuestPackageItems(info->PackageID))
            for (auto reward : *rewards)
                if (player.CanSelectQuestPackageItem(reward))
                {
                    packet.ItemChoiceID = reward->ItemID;
                    break;
                }
        if (!packet.ItemChoiceID)
            if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(info->PackageID); fallback && !fallback->empty())
                packet.ItemChoiceID = fallback->front()->ItemID;
    }
    player.GetSession()->HandleQuestGiverChooseReward(packet);
}

void Abandon(LootPlayer& player, uint32 quest)
{
    WorldPackets::Quest::QuestLogRemoveQuest packet{WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST)};
    packet.Entry = player.FindQuestSlot(quest);
    Require(packet.Entry != MAX_QUEST_LOG_SIZE, "Missing accepted quest to abandon");
    player.GetSession()->HandleQuestLogRemoveQuest(packet);
    Require(player.GetQuestStatus(quest) == QUEST_STATUS_NONE, "Quest was not abandoned");
}

void Open(LootPlayer& player, FixtureObject& chest)
{
    auto result = player.CastSpell(&chest, Opening, false);
    Require(result == SPELL_CAST_OK, "Opening did not match the original Bundle of Wood lock");
    auto castTime = sSpellMgr->GetSpellInfo(Opening)->CalcCastTime();
    Tick(player, std::max(6000u, castTime + 2000));
    std::cout << "CASE bundle cast_ms=" << castTime << " state=" << unsigned(chest.getLootState()) <<
        " own_window=" << (player.GetLootGUID() == chest.GetGUID()) << " empty=" << chest.loot.isLooted() << '\n';
}

void Release(LootPlayer& player, FixtureObject& chest)
{
    WorldPackets::Loot::LootRelease packet{WorldPacket(CMSG_LOOT_RELEASE)};
    packet.Unit = chest.GetGUID();
    player.GetSession()->HandleLootRelease(packet);
}

bool Take(LootPlayer& player, Loot& loot, uint32 item)
{
    WorldPackets::Loot::LootResponse response;
    loot.BuildLootResponse(response, &player, ALL_PERMISSION);
    for (auto const& row : response.Items)
        if (row.Loot.ItemID == item)
        {
            auto initial = player.GetItemCount(item);
            WorldPackets::Loot::AutoStoreLootItem packet{WorldPacket(CMSG_LOOT_ITEM)};
            packet.Loot.push_back({loot.GetGUID(), row.LootListID});
            player.GetSession()->HandleAutostoreLootItemOpcode(packet);
            Require(row.Quantity == 1 && player.GetItemCount(item) == initial + 1, "Native loot packet did not put one item in the bag");
            return true;
        }
    return false;
}

uint32 VictimGuid = 30000;
bool KillForItem(Map& map, LootPlayer& owner, Unit& killer, uint32 entry, uint32 item, uint32 offset)
{
    auto pos = CreatureLocation(entry, offset);
    MoveTo(owner, pos);
    Actor victim(&map, entry, ++VictimGuid, pos);
    Require(map.AddToMap(static_cast<Creature*>(&victim)), "Could not publish the original creature target");
    victim.LowerPlayerDamageReq(victim.GetMaxHealth());
    victim.SetLootRecipient(&owner);
    killer.Kill(&victim, false);
    Require(victim.isDead(false), "A native kill did not kill the original creature");
    Tick(owner, 100);
    return item && Take(owner, victim.loot, item);
}

void CheckWood(Map& map, LootPlayer& owner, LootPlayer& other, Actor& raelen)
{
    auto placements = ObjectLocations(Bundle, 15);
    auto const& first = placements.front();
    {
        FixtureObject chest(map, Bundle, first);
        Require(chest.GetGOInfo()->GetLockId() == 43 && chest.GetGOInfo()->GetLootId() == Bundle, "Original collection lock or loot changed");
        MoveTo(other, first.position);
        Require(!chest.ActivateToQuest(&other), "Wood glows before acceptance");
        Open(other, chest);
#ifdef ELWYNN_QUESTS_BASELINE
        Require(chest.getLootState() == GO_ACTIVATED && chest.loot.isLooted() && other.GetLootGUID() == chest.GetGUID(),
            "Old data did not open empty wood loot without the quest");
        Release(other, chest);
        chest.Rearm();
#else
        Require(chest.getLootState() == GO_READY && !other.GetLootGUID(), "Ineligible opener changed the shared bundle");
#endif
    }
    Accept(owner, raelen, 5545);
    Require(owner.GetQuestStatus(5545) == QUEST_STATUS_INCOMPLETE, "Raelen did not start A Bundle of Trouble");
    FixtureObject firstChest(map, Bundle, first);
    MoveTo(owner, first.position);
    Open(owner, firstChest);
    Require(Take(owner, firstChest.loot, Wood), "Expected guaranteed Bundle of Wood loot");
    Release(owner, firstChest);
    firstChest.Rearm();
    Abandon(owner, 5545);
#ifdef ELWYNN_QUESTS_BASELINE
    Require(owner.GetItemCount(Wood) == 1, "Old data no longer keeps its abandoned quest wood");
    std::cout << "EXPECTED_BROKEN an ineligible player consumed a bundle and abandonment kept the collected wood\n";
#else
    Require(!owner.GetItemCount(Wood) && !firstChest.ActivateToQuest(&owner), "Abandon kept quest wood or its marker");
    Accept(owner, raelen, 5545);
    Accept(other, raelen, 5545);
    for (auto const& placement : placements)
    {
        std::unique_ptr<FixtureObject> next;
        if (placement.guid != first.guid)
            next = std::make_unique<FixtureObject>(map, Bundle, placement);
        FixtureObject& chest = next ? *next : firstChest;
        MoveTo(owner, placement.position);
        if (owner.GetItemCount(Wood) == 8)
        {
            Open(owner, chest);
            Require(!owner.GetLootGUID() && !chest.ActivateToQuest(&owner) && chest.getLootState() == GO_READY,
                "The completed quest still activates or opens unused bundles");
            continue;
        }
        Require(chest.ActivateToQuest(&owner), "A needed wood bundle did not activate");
        Open(owner, chest);
        if (placement.guid == first.guid)
        {
            MoveTo(other, placement.position);
            Open(other, chest);
            Release(other, chest);
            Require(!other.GetLootGUID() && chest.getLootState() == GO_ACTIVATED && owner.GetLootGUID() == chest.GetGUID(),
                "A concurrent eligible opener discarded the first player's loot");
        }
        Require(Take(owner, chest.loot, Wood), "One of the eight actual bundles gave no quest wood");
        Release(owner, chest);
        chest.Rearm();
    }
    Reward(owner, raelen, 5545);
    Require(owner.GetQuestRewardStatus(5545) && !owner.GetItemCount(Wood), "Raelen's turn-in left wood behind");
    MoveTo(owner, first.position);
    Open(owner, firstChest);
    Require(!owner.GetLootGUID() && firstChest.getLootState() == GO_READY, "A rewarded quest could still open a bundle");
    MoveTo(other, first.position);
    Open(other, firstChest);
    Require(Take(other, firstChest.loot, Wood), "Second eligible player could not use the restored bundle");
    Release(other, firstChest);
    std::cout << "PASS 5545 fifteen placements, exact lock, concurrent use, eight carried items, reward, abandon and shared rearm\n";
#endif
}

void CheckDelivery(Map& map, LootPlayer& owner, LootPlayer& other, WorldObject& giver, Actor& receiver,
    uint32 quest, uint32 item, uint32 first, uint32 second)
{
    Require(!KillForItem(map, other, other, first, item, 0), "A character without the quest got its creature loot");
    Accept(owner, giver, quest);
    Require(owner.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "The correct questgiver did not offer the bounty");
    LootPet pet(owner, map);
    for (uint32 trial = 0; trial < 36 && owner.GetItemCount(item) < 8; ++trial)
    {
        if (trial == 0)
            Require(!owner.CanRewardQuest(sQuestDataStore->GetQuestTemplate(quest), false), "Incomplete delivery admitted reward");
        KillForItem(map, owner, trial % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(owner),
            trial % 2 ? second : first, item, trial / 2);
    }
    Require(owner.GetItemCount(item) == 8 && owner.GetQuestObjectiveData(quest, item) == 8, "Native creatures did not provide eight bounty items");
    Require(!KillForItem(map, owner, pet, second, item, 0), "Completed bounty still provided quest loot");
    Abandon(owner, quest);
#ifdef ELWYNN_QUESTS_BASELINE
    Require(owner.GetItemCount(item) == 8, "Original data no longer retains an abandoned creature delivery");
    std::cout << "EXPECTED_BROKEN " << quest << " retained eight collected items after abandonment\n";
#else
    Require(!owner.GetItemCount(item), "Abandon did not clear the original creature quest item's stack");
    Accept(owner, giver, quest);
    for (uint32 trial = 0; trial < 36 && owner.GetItemCount(item) < 8; ++trial)
        KillForItem(map, owner, trial % 2 ? static_cast<Unit&>(owner) : static_cast<Unit&>(pet),
            trial % 2 ? second : first, item, trial / 2);
    Require(owner.GetItemCount(item) == 8, "Reaccepted delivery did not regain eight bounty items");
    Reward(owner, receiver, quest);
    Require(owner.GetQuestRewardStatus(quest) && !owner.GetItemCount(item), "Bounty reward retained quest items");
    Require(!KillForItem(map, owner, owner, first, item, 0), "A rewarded quest still drops its creature item");
    std::cout << "PASS " << quest << " real player/pet kills, eight loot receipts, completion quota, abandon, retry and item consumption\n";
#endif
}

void CheckFrontier(Map& map, LootPlayer& owner, LootPlayer& member, Actor& thomas)
{
    Accept(owner, thomas, 52);
    Accept(member, thomas, 52);
    Require(owner.GetQuestStatus(52) == QUEST_STATUS_INCOMPLETE && member.GetQuestStatus(52) == QUEST_STATUS_INCOMPLETE,
        "Thomas did not admit the frontier quest");
    LootPet pet(owner, map);
    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(1252, 0);
    for (auto player : {&owner,&member})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        player->SetGroup(&group, 0);
    }
    sGroupMgr->AddGroup(&group);
    for (uint32 i = 0; i < 14; ++i)
    {
        uint32 entry = i < 8 ? (i % 2 ? 1922 : 118) : 822;
        MoveTo(member, CreatureLocation(entry, i));
        KillForItem(map, owner, i % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(owner), entry, 0, i);
        Tick(member, 100);
        Require(owner.GetQuestObjectiveData(52, 118) == int32(std::min(i + 1, 8u)) &&
            owner.GetQuestObjectiveData(52, 822) == int32(i < 8 ? 0 : std::min(i - 7, 5u)) &&
            owner.GetQuestObjectiveData(52, 118) == member.GetQuestObjectiveData(52, 118) &&
            owner.GetQuestObjectiveData(52, 822) == member.GetQuestObjectiveData(52, 822),
            "A wolf alias, player/pet death or nearby party recipient miscounted");
    }
    owner.SetGroup(nullptr);
    member.SetGroup(nullptr);
    sGroupMgr->RemoveGroup(&group);
    Reward(owner, thomas, 52);
    Require(owner.GetQuestRewardStatus(52), "Exact frontier totals did not permit reward");
    std::cout << "PASS 52 Prowler and Gray Forest Wolf share 8 credit, bears cap at 5, player/pet and party recipients\n";
}

void CheckRoutes()
{
    Map map(0, 300, 0, DIFFICULTY_NONE);
    auto start = CreatureLocation(241);
    LootPlayer owner(Session(), map, 340, start), other(Session(), map, 341, start);
    for (auto player : {&owner,&other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->SetLevel(7);
    }
    Actor remy(&map, 241, 241, start);
    Actor dughan(&map, 240, 240, CreatureLocation(240));
    Actor thomas(&map, 261, 261, CreatureLocation(261));
    Actor rainer(&map, 963, 963, CreatureLocation(963));
    Actor raelen(&map, 10616, 10616, CreatureLocation(10616));
    for (auto npc : {&remy, &dughan, &thomas, &rainer, &raelen})
        Require(map.AddToMap(static_cast<Creature*>(npc)), "Original questgiver could not be published");
    FixtureObject malakai(map, 55, ObjectLocations(55,1).front());
    FixtureObject rolf(map, 56, ObjectLocations(56,1).front());
    FixtureObject board(map, 203733, ObjectLocations(203733,1).front());
    Accept(other, dughan, 35);
    Accept(other, thomas, 37);
    Accept(other, malakai, 45);
    Accept(other, rolf, 71);
    Accept(other, rainer, 11);
    for (uint32 quest : {35,37,45,71,11})
        Require(other.GetQuestStatus(quest) == QUEST_STATUS_NONE, "A follow-up bypassed the previous reward");
    for (auto [quest,giver,receiver] : std::array<std::tuple<uint32,WorldObject*,WorldObject*>,5>{{
        {40,&remy,&dughan}, {35,&dughan,&thomas}, {37,&thomas,&malakai}, {45,&malakai,&rolf}, {239,&dughan,&rainer}}})
    {
        Accept(owner, *giver, quest);
        Require(owner.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE && !owner.GetQuestRewardStatus(quest),
            "Native travel quest did not require its actual receiver turn-in");
        Reward(owner, *receiver, quest);
        Require(owner.GetQuestRewardStatus(quest), "The native travel reward packet was refused");
        std::cout << "PASS " << quest << " actual Elwynn offer, prerequisite and receiver packet\n";
    }
    Accept(owner, rolf, 71);
    Require(owner.GetItemCount(735) == 1 && owner.GetQuestStatus(71) == QUEST_STATUS_COMPLETE, "Rolf did not provide the single medallion");
    Abandon(owner, 71);
    Require(!owner.GetItemCount(735), "Abandoning Report to Thomas kept the medallion");
    Accept(owner, rolf, 71);
    Require(owner.GetItemCount(735) == 1, "Reacceptance did not restore a single medallion");
    Reward(owner, thomas, 71);
    Require(owner.GetQuestRewardStatus(71) && !owner.GetItemCount(735), "Thomas did not receive the medallion");
    std::cout << "PASS 71 corpse acceptance, one medallion in bags, abandon, reacceptance and Thomas turn-in\n";
    CheckWood(map, owner, other, raelen);
    CheckDelivery(map, owner, other, rainer, rainer, 11, 782, 97, 478);
    CheckDelivery(map, owner, other, board, thomas, 46, 780, 46, 732);
    CheckFrontier(map, owner, other, thomas);
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoadGameObjectModelList(argv[2]);
        LoadInputs();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
#ifndef ELWYNN_QUESTS_BASELINE
        AddSC_elwynn_forest();
#endif
        sScriptMgr->SwapScriptContext(true);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "Rewards require a disposable character schema");
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto fixture = CharacterDatabase.Query("SELECT DATABASE(), (SELECT COUNT(*) FROM characters)");
        Require(fixture && fixture->Fetch()[0].GetString() == "fixture_char" && !fixture->Fetch()[1].GetUInt32(),
            "Rewards require an empty character fixture");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckRoutes();
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
