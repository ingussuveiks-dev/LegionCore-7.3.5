#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameEventMgr.h"
#include "QuestItemObjectUse.h"
#include <thread>

namespace
{
using namespace DarkmoonBanner;

void LoadSelectedRows(char const* sql)
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute(sql);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
}

void LoadBannerInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeDarkmoonBanner.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "The native Tailoring quest is missing");
    auto quest = new ::Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[DarkmoonBanner::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(DarkmoonBanner::Quest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29520");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Credit && objective->Fetch()[5].GetUInt32() == 1, "Wrong original banner objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=29520");
    Require(addon && addon->Fetch()[9].GetUInt32() == 197 && addon->Fetch()[10].GetUInt32() == 75 && quest->SourceItemId == Kit,
        "Wrong original Tailoring requirement or kit");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);

    auto row = WorldDatabase.Query("SELECT occurence,length FROM game_event WHERE eventEntry=75 AND world_event=0");
    Require(bool(row), "The original Faire event is missing");
    auto& events = const_cast<GameEventMgr::GameEventDataMap&>(sGameEventMgr->GetEventMap());
    events.resize(76);
    events[75].occurence = row->Fetch()[0].GetUInt32();
    events[75].length = row->Fetch()[1].GetUInt32();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sQuestDataStore->LoadGameObjectForQuests();
    auto templateInfo = sObjectMgr->GetGameObjectTemplate(RaisedBanner);
    auto source = WorldDatabase.Query("SELECT COUNT(*) FROM gameobject WHERE id=179965 AND map=974 AND zoneId=5861");
    Require(templateInfo && templateInfo->displayId == 6091 && source && source->Fetch()[0].GetUInt32() == 3,
        "The existing Darkmoon Island banner asset is missing");
    Require(sSpellMgr->GetSpellInfo(Plant)->CastingReq.RequiresSpellFocus == 1728 &&
        sSpellMgr->GetSpellInfo(Plant)->GetEffect(EFFECT_1)->Effect == SPELL_EFFECT_ACTIVATE_OBJECT &&
        sSpellMgr->GetSpellInfo(Plant)->GetEffect(EFFECT_2)->Effect == SPELL_EFFECT_KILL_CREDIT,
        "The native three-effect Banner spell or its focus differs");

    LoadSelectedRows("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=29520");
    LoadSelectedRows("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=29520");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
#ifdef DARKMOON_BANNER_BASELINE
    LoadSelectedRows("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "SourceTypeOrReferenceId=19 AND SourceEntry=29520 AND ConditionTypeOrReference=12");
#else
    LoadSelectedRows("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE "
        "(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(101992,101993)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=29520)");
#endif
    sConditionMgr->LoadConditions();
}

struct LooseStone : GameObject
{
    LooseStone(Map& map, Placement const& placement)
    {
        m_DBTableGuid = placement.guid;
        Require(Create(placement.guid, placement.entry, &map, 1, placement.position, placement.rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)), "The real stones could not be published on the Island");
        Update(100);
        Require(GetGoType() == GAMEOBJECT_TYPE_SPELL_FOCUS && GetGOInfo()->spellFocus.spellFocusType == 1728,
            "The original stones are not the expected native spell focus");
    }

    ~LooseStone()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

Position SpotPosition(Field* fields)
{
    return {fields[0].GetFloat(), fields[1].GetFloat(), fields[2].GetFloat(), fields[3].GetFloat()};
}

void AcceptBanner(LootPlayer& player, Creature& trainer)
{
    SupplyMove(player, trainer.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = trainer.GetGUID();
    packet.QuestID = DarkmoonBanner::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    Require(player.GetQuestStatus(DarkmoonBanner::Quest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Kit) == 1,
        "The placed Selina did not start the quest and provide the kit");
}

SpellCastResult UseBannerItem(LootPlayer& player, uint32 spell, uint32 entry)
{
    auto item = player.GetItemByEntry(entry);
    Require(item, "The native Banner item was not carried");
    auto spellInfo = sSpellMgr->GetSpellInfo(spell);
    GameTime::UpdateGameTimers();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (player.GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo) && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        GameTime::UpdateGameTimers();
    }
    Require(!player.GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo), "Actual item use still had a global cooldown after three seconds");
    SpellCastTargets targets;
    targets.SetCaster(&player);
    if (spell == Sew)
        targets.SetUnitTarget(&player);
    auto result = player.CastSpell(targets, spellInfo, nullptr, TRIGGERED_NONE, item);
    Tick(player, 6500);
    std::cout << "Item use " << spell << " result=" << uint32(result) << " cooldown=" << player.GetSpellCooldownDelay(spell) << '\n';
    return result;
}

void SewBanner(LootPlayer& player)
{
    for (auto id : {2320, 2604, 6260})
        GiveSupply(player, id, 1);
    Require(UseBannerItem(player, Sew, Kit) == SPELL_CAST_OK, "The original three-reagent recipe did not start");
    Require(player.GetItemCount(FoldedBanner) == 1 && !player.GetItemCount(Kit) && !player.GetItemCount(2320) &&
        !player.GetItemCount(2604) && !player.GetItemCount(6260), "Native sewing did not produce and consume the matching items");
}

void CheckBanner()
{
    auto npc = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=2100 AND id=10445 AND map=974");
    Require(bool(npc), "Original Selina Dourman world placement missing");
    Map map(974, 589, 0, DIFFICULTY_NONE);
    Position origin = SpotPosition(npc->Fetch());
    map.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Actor trainer(&map, 10445, 2100, origin);
    Require(map.AddToMap(static_cast<Creature*>(&trainer)), "Could not publish the real quest receiver");
    LootPlayer owner(SupplySession(), map, 2952001, origin), other(SupplySession(), map, 2952002, origin);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetSkill(197, 1, 75, 75);
    }
    auto& active = const_cast<GameEventMgr::ActiveEvents&>(sGameEventMgr->GetActiveEventList());
    active.insert(75);
    AcceptBanner(owner, trainer);
    SewBanner(owner);

    auto spots = WorldDatabase.Query("SELECT g.id,g.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,"
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3,c.guid,c.position_x,c.position_y,c.position_z,c.orientation "
        "FROM gameobject g JOIN creature c ON c.map=g.map AND c.zoneId=g.zoneId AND c.id=54545 AND "
        "POW(g.position_x-c.position_x,2)+POW(g.position_y-c.position_y,2)+POW(g.position_z-c.position_z,2)<0.09 "
        "WHERE g.id=209288 AND g.map=974 AND g.zoneId=5861 ORDER BY g.guid");
    Require(spots && spots->GetRowCount() == 18, "Expected all 18 matching Loose Stones and creature-marker pairs");

    auto row = spots->Fetch();
    auto first = SpotPosition(row + 3);
    map.LoadGrid(first.GetPositionX(), first.GetPositionY());
    Actor marker(&map, Credit, row[11].GetUInt32(), SpotPosition(row + 12));
    Require(map.AddToMap(static_cast<Creature*>(&marker)), "The actual invisible creature marker could not enter the map");
    SupplyMove(owner, first);
    SupplyMove(other, first);
    auto legacy = Placement{209285, 984952, 300, first, {row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat()}};
#ifdef DARKMOON_BANNER_BASELINE
    auto location = Placement{LooseStones, row[1].GetUInt32(), row[2].GetUInt32(), first, legacy.rotation};
    LooseStone actual(map, location);
    auto original = UseBannerItem(owner, Plant, FoldedBanner);
    Require(!owner.GetGameObject(Plant), "The original binary no longer reproduces the missing placed banner at the actual stones");
    std::cout << "EXPECTED_BROKEN the actual stones did not publish the banner; original cast result=" << uint32(original)
        << " objective=" << owner.GetQuestObjectiveData(DarkmoonBanner::Quest, Credit) << " folded=" << owner.GetItemCount(FoldedBanner) << '\n';
#else
    {
        LooseStone decoy(map, legacy);
        Require(UseBannerItem(owner, Plant, FoldedBanner) != SPELL_CAST_OK && owner.GetItemCount(FoldedBanner) == 1 &&
            !owner.GetQuestObjectiveData(DarkmoonBanner::Quest, Credit), "The same-focus decoy consumed the item or gave credit");
    }
    auto location = Placement{LooseStones, row[1].GetUInt32(), row[2].GetUInt32(), first, legacy.rotation};
    LooseStone actual(map, location);
    uint32 inspected = 1;
    Require(actual.ActivateToQuest(&owner) && !actual.ActivateToQuest(&other), "The active quest did not show its own real stone marker");
    while (spots->NextRow())
    {
        auto next = spots->Fetch();
        Placement location{LooseStones, next[1].GetUInt32(), next[2].GetUInt32(), SpotPosition(next + 3),
            {next[7].GetFloat(), next[8].GetFloat(), next[9].GetFloat(), next[10].GetFloat()}};
        LooseStone spot(map, location);
        Require(spot.ActivateToQuest(&owner) && !spot.ActivateToQuest(&other), "One actual Loose Stones placement lost its marker guard");
        ++inspected;
    }
    Require(inspected == 18, "Expected every actual Loose Stones placement");
    active.erase(75);
    Require(!actual.ActivateToQuest(&owner) && UseBannerItem(owner, Plant, FoldedBanner) != SPELL_CAST_OK &&
        owner.GetItemCount(FoldedBanner) == 1, "An expired Faire still admitted planting");
    active.insert(75);
    Require(UseBannerItem(owner, Plant, FoldedBanner) == SPELL_CAST_OK, "The actual Loose Stones refused the real Banner use");
    auto firstBanner = owner.GetGameObject(Plant);
    std::cout << "Actual stone result: object=" << bool(firstBanner) << " folded=" << owner.GetItemCount(FoldedBanner)
        << " objective=" << owner.GetQuestObjectiveData(DarkmoonBanner::Quest, Credit) << " marker distance=" << owner.GetExactDist(&marker)
        << " spell range=" << sSpellMgr->GetSpellInfo(Plant)->GetMaxRange(true) << '\n';
    if (firstBanner)
        std::cout << "Placed object: id=" << firstBanner->GetEntry() << " distance=" << firstBanner->GetExactDist(&actual)
            << " owner visible=" << owner.canSeeOrDetect(firstBanner) << " foreign visible=" << other.canSeeOrDetect(firstBanner)
            << " owner collision=" << firstBanner->CanCollideWith(&owner) << " foreign collision=" << firstBanner->CanCollideWith(&other) << '\n';
    Require(firstBanner && firstBanner->GetEntry() == RaisedBanner && firstBanner->GetExactDist(&actual) < 0.1f &&
        owner.canSeeOrDetect(firstBanner) && !other.canSeeOrDetect(firstBanner) &&
        firstBanner->CanCollideWith(&owner) && !firstBanner->CanCollideWith(&other), "The placed banner lost the actual position or accepted owner scope");
    Require(owner.GetQuestStatus(DarkmoonBanner::Quest) == QUEST_STATUS_COMPLETE && !owner.GetItemCount(FoldedBanner) &&
        !actual.ActivateToQuest(&owner) && !other.GetQuestObjectiveData(DarkmoonBanner::Quest, Credit), "The real planting did not consume and credit exactly one attempt");

    AcceptBanner(other, trainer);
    SewBanner(other);
    SupplyMove(other, first);
    Require(UseBannerItem(other, Plant, FoldedBanner) == SPELL_CAST_OK && other.GetGameObject(Plant) &&
        other.GetGameObject(Plant) != firstBanner && !owner.canSeeOrDetect(other.GetGameObject(Plant)), "A second player could not independently plant at the same real stones");
    auto otherGuid = other.GetGameObject(Plant)->GetGUID();
    auto firstGuid = firstBanner->GetGUID();
    AbandonSupply(other, DarkmoonBanner::Quest);
    map.RemoveAllObjectsInRemoveList();
    auto abandoned = map.GetGameObject(otherGuid);
    Require((!abandoned || abandoned->IsQuestObjectExpired()) && map.GetGameObject(firstGuid) == firstBanner &&
        !firstBanner->IsQuestObjectExpired(), "Abandonment did not restrict cleanup to its own player and object");
    SupplyMove(owner, trainer.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = trainer.GetGUID();
    reward.QuestID = DarkmoonBanner::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    map.RemoveAllObjectsInRemoveList();
    auto rewarded = map.GetGameObject(firstGuid);
    Require(owner.FindQuestSlot(DarkmoonBanner::Quest) == MAX_QUEST_LOG_SIZE && !owner.GetItemCount(Kit) && !owner.GetItemCount(FoldedBanner) &&
        (!rewarded || rewarded->IsQuestObjectExpired()), "Selina's real reward did not remove the Banner and its scope");
    std::cout << "PASS 18 actual pairs, decoy denial, two independent plants, correct item, owner collision and both cleanup paths\n";
#endif
    active.erase(75);
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{974, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{974, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Banner checks require an isolated character schema");
        LoadBannerInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckBanner();
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
