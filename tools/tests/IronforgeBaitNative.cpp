#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "PoolMgr.h"
#include "QuestItemObjectUse.h"

namespace
{
using namespace IronforgeBait;

constexpr uint32 Lure = 69978;
constexpr uint32 Bauble = 6529;
constexpr uint32 HairTrigger = 39684;

void LoadBaitInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeIronforgeBait.inc"
    );
    Require(rows && rows->GetRowCount() == 1, "The original Grimnur daily is missing");
    auto quest = new ::Quest(rows->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[IronforgeBait::Quest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(IronforgeBait::Quest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29350");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_ITEM &&
        objective->Fetch()[4].GetUInt32() == Lure && objective->Fetch()[5].GetUInt32() == 1, "Wrong exact lure objective");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=29350");
    Require(addon && addon->Fetch()[9].GetUInt32() == 356 && quest->SourceItemId == Blueprint, "Wrong original fishing line or Blueprint");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

    auto vendors = WorldDatabase.Query("SELECT COUNT(DISTINCT v.item) FROM npc_vendor v JOIN creature c ON c.id=v.entry "
        "WHERE c.map=0 AND c.zoneId=1537 AND ((c.id=5162 AND v.item=6529) OR (c.id=5175 AND v.item=39684))");
    Require(vendors && vendors->Fetch()[0].GetUInt32() == 2, "The two required vendor materials are missing in Ironforge");
    auto tram = WorldDatabase.Query("SELECT COUNT(*) FROM areatrigger_teleport WHERE id IN(2173,2175) AND target_map=369");
    Require(tram && tram->Fetch()[0].GetUInt32() == 2, "The original entrances to the Tram are missing");
    auto rotation = WorldDatabase.Query("SELECT p.pool_entry,t.max_limit FROM pool_quest p JOIN pool_template t ON t.entry=p.pool_entry WHERE p.entry=29350");
    Require(rotation && rotation->Fetch()[0].GetUInt32() == 5682 && rotation->Fetch()[1].GetUInt32() == 1, "Wrong original fishing daily pool");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto original = table;
    table += " WHERE Entry=208864";
    auto loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1, "Expected the matching Rat Trap loot group");
    sQuestDataStore->LoadGameObjectForQuests();

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=29350");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=29350");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    Require(sQuestDataStore->GetCreatureQuestRelationBounds(5161).first == sQuestDataStore->GetCreatureQuestRelationBounds(5161).second,
        "The fishing daily was advertised before the pool selected it");
    PoolGroup<::Quest> daily;
    daily.SetPoolId(5682);
    PoolObject selection(IronforgeBait::Quest, 0.0f);
    daily.AddEntry(selection, 1);
    ActivePoolData active;
    daily.SpawnObject(active, 1, 1);
    Require(active.IsActiveObject<::Quest>(IronforgeBait::Quest), "The native pool did not publish the selected fishing daily");
}

Position ReadPosition(Field* row)
{
    return {row[0].GetFloat(), row[1].GetFloat(), row[2].GetFloat(), row[3].GetFloat()};
}

void AcceptBait(LootPlayer& player, Creature& trainer)
{
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = trainer.GetGUID();
    packet.QuestID = IronforgeBait::Quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
    if (player.GetQuestStatus(IronforgeBait::Quest) != QUEST_STATUS_INCOMPLETE || player.GetItemCount(Blueprint) != 1)
    {
        auto quest = sQuestDataStore->GetQuestTemplate(IronforgeBait::Quest);
        std::cout << "Grimnur: starter=" << trainer.hasQuest(IronforgeBait::Quest)
            << " interaction=" << player.CanInteractWithQuestGiver(&trainer) << " can take=" << player.CanTakeQuest(quest, false)
            << " can add=" << player.CanAddQuest(quest, false) << " status=" << uint32(player.GetQuestStatus(IronforgeBait::Quest))
            << " blueprint=" << player.GetItemCount(Blueprint) << " questgiver flags=" << trainer.GetUInt32Value(UNIT_FIELD_NPC_FLAGS)
            << " npc faction=" << trainer.getFaction() << " player faction=" << player.getFaction() << '\n';
    }
    Require(player.GetQuestStatus(IronforgeBait::Quest) == QUEST_STATUS_INCOMPLETE && player.GetItemCount(Blueprint) == 1,
        "The actual Grimnur did not start the daily and provide the Blueprint");
}

void EnterMap(LootPlayer& player, Map& map, Position const& position)
{
    player.Unit::RemoveFromWorld();
    player.RemoveFromGrid();
    player.ResetMap();
    player.Relocate(position);
    player.SetMap(&map);
    map.LoadGrid(position.GetPositionX(), position.GetPositionY());
    map.AddToGrid(static_cast<Player*>(&player), Cell(Trinity::ComputeCellCoord(position.GetPositionX(), position.GetPositionY())));
    player.Unit::AddToWorld();
}

SpellCastResult UseBlueprint(LootPlayer& player)
{
    auto blueprint = player.GetItemByEntry(Blueprint);
    Require(blueprint, "The Blueprint was not carried");
    GameTime::UpdateGameTimers();
    SpellCastTargets targets;
    targets.SetCaster(&player);
    targets.SetUnitTarget(&player);
    auto result = player.CastSpell(targets, sSpellMgr->GetSpellInfo(Assemble), nullptr, TRIGGERED_NONE, blueprint);
    Tick(player, 6000);
    return result;
}

void CheckBait()
{
    auto npc = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation FROM creature WHERE guid=82226 AND id=5161 AND map=0 AND zoneId=1537");
    Require(bool(npc), "Original Grimnur Stonebrand placement is missing");
    auto rows = WorldDatabase.Query("SELECT guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=208864 AND map=369 AND zoneId=2257 ORDER BY guid");
    Require(rows && rows->GetRowCount() == 18, "Expected 18 original traps in the tram");
    std::vector<Placement> placements;
    do
    {
        auto row = rows->Fetch();
        placements.push_back({Trap, row[0].GetUInt32(), row[1].GetUInt32(), ReadPosition(row + 2),
            {row[6].GetFloat(), row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat()}});
    } while (rows->NextRow());

    Map ironforge(0, 581, 0, DIFFICULTY_NONE), tram(369, 582, 0, DIFFICULTY_NONE);
    Position const origin = ReadPosition(npc->Fetch());
    Position const start = placements.front().position;
    ironforge.LoadGrid(origin.GetPositionX(), origin.GetPositionY());
    Actor trainer(&ironforge, 5161, 82226, origin);
    Require(ironforge.AddToMap(static_cast<Creature*>(&trainer)), "Could not publish the real receiver");
    LootPlayer owner(SupplySession(), ironforge, 2935001, origin), outsider(SupplySession(), tram, 2935002, start);
    for (auto player : {&owner, &outsider})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_HUMAN);
        player->setFactionForRace(RACE_HUMAN);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        player->SetSkill(356, 1, 15, 75);
    }
    AcceptBait(owner, trainer);
    EnterMap(owner, tram, start);

    GiveSupply(outsider, Blueprint, 1);
    GiveSupply(outsider, Bauble, 5);
    GiveSupply(outsider, HairTrigger, 1);
    GiveSupply(outsider, TrapItem, 3);
    auto stale = UseBlueprint(outsider);
#ifndef IRONFORGE_BAIT_BASELINE
    Require(stale != SPELL_CAST_OK && outsider.GetItemCount(Blueprint) == 1 && !outsider.GetItemCount(Lure),
        "An inactive Blueprint consumed materials or made a lure");
#endif

    auto collect = [&]()
    {
#ifndef IRONFORGE_BAIT_BASELINE
        for (auto const& placement : placements)
        {
            SupplyMove(owner, placement.position);
            Supply trap(tram, placement);
            Require(trap.ActivateToQuest(&owner) && !trap.ActivateToQuest(&outsider), "One original trap lost its active-quest marker");
        }
#endif
        unsigned marked = 0;
        for (auto const& placement : placements)
        {
            SupplyMove(owner, placement.position);
            Supply trap(tram, placement);
            Require(!trap.ActivateToQuest(&outsider), "A trap was marked without its active quest");
            if (owner.GetItemCount(TrapItem) == 3)
            {
                Require(!trap.ActivateToQuest(&owner), "A trap stayed marked after gathering three");
                continue;
            }
            Require(trap.ActivateToQuest(&owner), "An original trap was missing its eligible loot marker");
            ++marked;
            OpenSupply(owner, trap);
            TakeSupply(owner, trap, TrapItem);
            ReleaseSupply(owner, trap);
        }
        Require(marked >= 3 && owner.GetItemCount(TrapItem) == 3, "The real Tram loot did not supply three Rat Traps");
    };

    {
        Supply first(tram, placements.front());
        GameTime::UpdateGameTimers();
        auto invalid = outsider.CastSpell(&first, 3365, false);
        Tick(outsider, 6000);
        if (outsider.GetLootGUID())
            ReleaseSupply(outsider, first);
        outsider.SendLoot(first.GetGUID(), LOOT_SKINNING);
#ifdef IRONFORGE_BAIT_BASELINE
        std::cout << "Original unaccepted item result=" << uint32(stale) << " lure=" << outsider.GetItemCount(Lure)
            << " open spell=" << uint32(invalid) << " raw loot=" << bool(outsider.GetLootGUID()) << '\n';
        if (outsider.GetLootGUID())
            ReleaseSupply(outsider, first);
#else
        Require(invalid != SPELL_CAST_OK && !outsider.GetLootGUID() && first.getLootState() == GO_READY,
            "The native Open Lock or raw loot path let an outsider open a working trap");
#endif
    }

    auto craft = [&]()
    {
        GiveSupply(owner, Bauble, 4);
        GiveSupply(owner, HairTrigger, 1);
        Require(UseBlueprint(owner) != SPELL_CAST_OK && owner.GetItemCount(Blueprint) == 1 &&
            owner.GetItemCount(Bauble) == 4 && owner.GetItemCount(HairTrigger) == 1 && owner.GetItemCount(TrapItem) == 3,
            "An incomplete recipe consumed the Blueprint or materials");
        GiveSupply(owner, Bauble, 1);
        Require(UseBlueprint(owner) == SPELL_CAST_OK && owner.GetItemCount(Lure) == 1 &&
            owner.GetQuestStatus(IronforgeBait::Quest) == QUEST_STATUS_COMPLETE, "The actual native recipe did not make a ready lure");
        Require(!owner.GetItemCount(Blueprint) && !owner.GetItemCount(Bauble) && !owner.GetItemCount(HairTrigger) && !owner.GetItemCount(TrapItem),
            "The recipe left the consumed Blueprint or reagents behind");
    };
    collect();
    craft();
    AbandonSupply(owner, IronforgeBait::Quest);
#ifdef IRONFORGE_BAIT_BASELINE
    Require(owner.GetItemCount(Lure) == 1, "The original quest no longer reproduces the leftover crafted lure");
    std::cout << "EXPECTED_BROKEN the original completed-then-abandoned quest retained the actual crafted lure\n";
#else
    Require(!owner.GetItemCount(Lure), "Abandonment retained the crafted lure");
    EnterMap(owner, ironforge, origin);
    AcceptBait(owner, trainer);
    EnterMap(owner, tram, start);
    collect();
    craft();
    EnterMap(owner, ironforge, origin);
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = trainer.GetGUID();
    reward.QuestID = IronforgeBait::Quest;
    reward.ItemChoiceID = 0;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.FindQuestSlot(IronforgeBait::Quest) == MAX_QUEST_LOG_SIZE && !owner.GetItemCount(Lure) && !owner.GetItemCount(Blueprint),
        "The real Grimnur reward did not consume the crafted lure");
    std::cout << "PASS original 18 Tram traps, actual three-item loot, native recipe with the accepted quest, abandonment, retry and Grimnur reward\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {369, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {369, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Bait checks require an isolated character schema");
        LoadBaitInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckBait();
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
