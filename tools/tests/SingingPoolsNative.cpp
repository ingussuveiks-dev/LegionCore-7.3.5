#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "AreaTriggerData.h"
#include "ConditionMgr.h"
#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"
#include "MiscPackets.h"
#include "MoveSpline.h"
#include "ScriptedEscortAI.h"
#include "ScriptSystem.h"
#include "ScriptsData.h"
#include "SpellPackets.h"
#include "Vehicle.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Introduction = 29521;
constexpr uint32 DryFur = 29661;
constexpr uint32 Reeds = 29662;
constexpr uint32 ReedItem = 73178;
constexpr uint32 ReedGiverSpawn = 244247;
constexpr uint32 BalancedRock = 29663;
constexpr uint32 TushuiMonk = 55019;
constexpr uint32 MonkSpawns[] = {244325, 244326, 244327, 244436};
constexpr uint32 NextQuest = 29676;
constexpr uint32 SunPearl = 29677;
constexpr uint32 PearlItem = 73184;
constexpr uint32 LiangSpawn = 244441;
constexpr uint32 Sting = 29666;
constexpr uint32 WaterPincer = 60411;
constexpr uint32 PincerSpawns[] = {244421, 244422, 244443, 244444, 244449, 244450};
constexpr uint32 Shu = 29678;
constexpr uint32 LiangPearl = 73791;
constexpr uint32 ReflectionCredit = 57476;
constexpr uint32 CoaxCredit = 55205;
constexpr uint32 NewFriend = 29679;
constexpr uint32 SpoutCredit = 55212;
constexpr uint32 ShoreAysaSpawn = 244519;
constexpr uint32 ShuSpawn = 244520;
constexpr uint32 Livelihood = 29680;
constexpr uint32 CartTender = 57710;
constexpr uint32 CartTenderSpawn = 244485;
constexpr uint32 JiSpawn = 244755;
constexpr uint32 FarmCartSpawn = 244624;
constexpr uint32 WreckCartSpawn = 245583;
constexpr uint32 Rascals = 29769;
constexpr uint32 MissingMallet = 29768;
constexpr uint32 Virmen = 55483;
constexpr uint32 VirmenSpawns[] = {244586,244589,244590,244591,244592,244595,244596,244617,244633,244654};
constexpr uint32 StillGood = 29770;
constexpr uint32 StrongWood = 29771;
constexpr uint32 MinSpawn = 244611;
constexpr uint32 DeloraSpawn = 244620;
constexpr uint32 BellCredit = 55077;
constexpr uint32 BellEntry = 209608;
constexpr uint32 BellSpawn = 102318;
constexpr uint32 AysaSpawn = 244270;
constexpr uint32 ShangTempleSpawn = 243552;
constexpr uint32 BellSpell = 102843;

LONG WINAPI PoolCrashTrace(EXCEPTION_POINTERS* exception)
{
    auto const executable = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    auto const address = reinterpret_cast<uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
    std::cerr << "Native check exception RVA 0x" << std::hex << address - executable << std::dec << std::endl;
    return CrashTrace(exception);
}

void LoadPools()
{
    auto rows = WorldDatabase.Query(
#include "NativeSingingPools.inc"
    );
    Require(rows && rows->GetRowCount() == 15, "Missing the Singing Pools and farm quests");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());

    auto rowsAddon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID IN (29423,29521,29661,29662,29663,29666,29676,29677,29678,29679,29680,29768,29769,29770,29771)");
    Require(rowsAddon && rowsAddon->GetRowCount() == 15, "Missing the original Pools and farm prerequisites");
    do
    {
        auto row = rowsAddon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
        if (quest->NextQuestID && quests[std::abs(quest->NextQuestID)])
            quests[std::abs(quest->NextQuestID)]->prevQuests.push_back(quest->NextQuestID < 0 ? -int32(quest->Id) : int32(quest->Id));
    } while (rowsAddon->NextRow());

    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29661");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == BellCredit,
        "The original Training Bell objective is missing");
    quests[DryFur]->LoadQuestObjective(objective->Fetch());
    quests[DryFur]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto reedsObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29662");
    Require(reedsObjective && reedsObjective->Fetch()[4].GetUInt32() == ReedItem && reedsObjective->Fetch()[5].GetUInt32() == 8,
        "Missing the original reed objective");
    quests[Reeds]->LoadQuestObjective(reedsObjective->Fetch());
    quests[Reeds]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto rockObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29663");
    Require(rockObjective && rockObjective->Fetch()[4].GetUInt32() == TushuiMonk && rockObjective->Fetch()[5].GetUInt32() == 6,
        "Missing the original six-monk objective");
    quests[BalancedRock]->LoadQuestObjective(rockObjective->Fetch());
    quests[BalancedRock]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto pearlObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29677");
    Require(pearlObjective && pearlObjective->Fetch()[4].GetUInt32() == PearlItem && pearlObjective->Fetch()[5].GetUInt32() == 1,
        "Missing the original Sun Pearl objective");
    quests[SunPearl]->LoadQuestObjective(pearlObjective->Fetch());
    quests[SunPearl]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    auto stingObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29666");
    Require(stingObjective && stingObjective->Fetch()[4].GetUInt32() == WaterPincer && stingObjective->Fetch()[5].GetUInt32() == 6,
        "Missing the original Water Pincer objective");
    quests[Sting]->LoadQuestObjective(stingObjective->Fetch());
    quests[Sting]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto shuObjectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29678 ORDER BY StorageIndex");
    Require(shuObjectives && shuObjectives->GetRowCount() == 2, "Missing the two Shu objectives");
    do
        quests[Shu]->LoadQuestObjective(shuObjectives->Fetch());
    while (shuObjectives->NextRow());
    quests[Shu]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto friendObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29679");
    Require(friendObjective && friendObjective->Fetch()[4].GetUInt32() == SpoutCredit && friendObjective->Fetch()[5].GetUInt32() == 5,
        "Missing the five Water Spout interactions");
    quests[NewFriend]->LoadQuestObjective(friendObjective->Fetch());
    quests[NewFriend]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto cartObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29680");
    Require(cartObjective && cartObjective->Fetch()[4].GetUInt32() == CartTender && cartObjective->Fetch()[5].GetUInt32() == 1 &&
        (cartObjective->Fetch()[6].GetUInt32() & QUEST_OBJECTIVE_FLAG_OPTIONAL), "Missing the optional cart objective");
    quests[Livelihood]->LoadQuestObjective(cartObjective->Fetch());
    auto farmObjectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (29768,29769,29770,29771) ORDER BY QuestID,StorageIndex");
    Require(farmObjectives && farmObjectives->GetRowCount() == 6, "Missing the six original farm objectives");
    do
    {
        auto fields = farmObjectives->Fetch();
        quests[fields[1].GetUInt32()]->LoadQuestObjective(fields);
    } while (farmObjectives->NextRow());
    quests[Rascals]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    quests[MissingMallet]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quests[StillGood]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quests[StrongWood]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN (243552,244137,244247,244270,244325,244326,244327,244421,244422,244436,244441,244443,244444,244449,244450,"
        "244485,244519,244520,244586,244589,244590,244591,244592,244595,244596,244611,244617,244620,244624,244633,244654,244755,245583)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (29521,29661,29662,29663,29666,29676,29677,29678,29679,29680,29768,29769,29770,29771)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (29521,29661,29662,29663,29666,29676,29677,29678,29679,29680,29768,29769,29770,29771)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE SourceTypeOrReferenceId=19 AND SourceEntry=29678");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names "
        "WHERE spell_id IN (103069,103070,103077,103116,108590,115904)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE areatrigger_scripts AS SELECT * FROM legion_world.areatrigger_scripts "
        "WHERE entry IN (6986,6987,6988,6990,6991,7012)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells "
        "WHERE npc_entry IN (54993,57710,57741,59497)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE script_waypoint AS SELECT * FROM legion_world.script_waypoint "
        "WHERE entry IN (57207,57742,59499)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_proc_event AS SELECT * FROM legion_world.spell_proc_event "
        "WHERE entry IN (46598,102938,102939,102940,102941,102942,103069,103070,103071,103072,103077,103078,108590,115904,116695,116696,117054,117057)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto const original = table;
    table += " WHERE Entry IN (209507,209584,209639,209640,209641,209642,209643,209644,209645,209646,214406)";
    auto const loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 11 && LootTemplates_Gameobject.GetLootFor(209507) && LootTemplates_Gameobject.GetLootFor(209584),
        "Missing the original reed, pearl or farm loot rows");
    auto& creatureLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto const originalCreatureLoot = creatureLoot;
    creatureLoot += " WHERE Entry IN (55483,60411)";
    (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    creatureLoot = originalCreatureLoot;
    Require(LootTemplates_Creature.GetLootFor(WaterPincer) && LootTemplates_Creature.GetLootFor(Virmen), "Missing the original Water Pincer or Virmen loot");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sScriptSystemMgr->LoadScriptWaypoints();
    sSpellMgr->LoadSpellProcEvents();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    if (WorldDatabase.Query("SELECT 1 FROM conditions LIMIT 1"))
        sConditionMgr->LoadConditions();
    sAreaTriggerDataStore->LoadAreaTriggerScripts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_WanderingIsland();
    AddSC_go_scripts();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
}

struct ReflectionRock : GameObject
{
    ReflectionRock(Map& map, uint32 spawn, uint32 trap)
    {
        auto row = WorldDatabase.PQuery("SELECT id,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
            "FROM gameobject WHERE guid=%u AND map=860", spawn);
        Require(bool(row), "Missing an original Reflection rock");
        auto data = row->Fetch();
        Position point(data[1].GetFloat(), data[2].GetFloat(), data[3].GetFloat(), data[4].GetFloat());
        G3D::Quat rotation{data[5].GetFloat(), data[6].GetFloat(), data[7].GetFloat(), data[8].GetFloat()};
        m_DBTableGuid = spawn;
        Require(Create(spawn, data[0].GetUInt32(), &map, data[9].GetUInt32(), point, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && GetGoType() == GAMEOBJECT_TYPE_TRAP && GetGOInfo()->trap.spell == trap,
            "Could not load the original Reflection trap");
    }

    ~ReflectionRock()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

struct Bell : GameObject
{
    explicit Bell(Map& map)
    {
        auto row = WorldDatabase.Query("SELECT position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,phaseMask "
            "FROM gameobject WHERE guid=102318 AND id=209608 AND map=860");
        Require(bool(row), "Missing the placed Training Bell");
        auto data = row->Fetch();
        Position point(data[0].GetFloat(), data[1].GetFloat(), data[2].GetFloat(), data[3].GetFloat());
        G3D::Quat rotation{data[4].GetFloat(), data[5].GetFloat(), data[6].GetFloat(), data[7].GetFloat()};
        m_DBTableGuid = BellSpawn;
        map.LoadGrid(point.GetPositionX(), point.GetPositionY());
        Require(Create(BellSpawn, BellEntry, &map, data[8].GetUInt32(), point, rotation, 255, GO_STATE_READY) &&
            map.AddToMap(static_cast<GameObject*>(this)) && GetGOInfo()->spellCaster.spell == BellSpell, "Could not load the native Bell");
    }

    ~Bell()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

struct Teacher : Creature
{
    explicit Teacher(Map& map, uint32 spawn = AysaSpawn, uint32 entry = 54975) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data && data->id == entry, "An original Singing Pools placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the Singing Pools actor");
    }

    ~Teacher()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void AcceptQuest(Player& player, Creature& teacher, uint32 quest)
{
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = teacher.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
}

void RewardQuest(Player& player, Creature& teacher, uint32 quest, uint32 choice = 0)
{
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = teacher.GetGUID();
    request.QuestID = quest;
    request.ItemChoiceID = choice;
    player.GetSession()->HandleQuestGiverChooseReward(request);
}

uint32 PoolRewardChoice(Player& player, uint32 quest)
{
    auto package = sQuestDataStore->GetQuestTemplate(quest)->PackageID;
    if (auto items = sDB2Manager.GetQuestPackageItems(package))
        for (auto item : *items)
            if (player.CanSelectQuestPackageItem(item))
                return item->ItemID;
    throw std::runtime_error("The pool quest has no suitable native reward choice");
}

void PreparePoolHunter(LootPlayer& player)
{
    player.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    player.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    player.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    player.InitDisplayIds();
    player.GetReputationMgr().LoadFromDB({});
    player.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    player.SetLevel(10);
    player.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        player.SetFloatValue(field, 1.0f);
}

void Ring(LootPlayer& player, Bell& bell, bool report)
{
    if (report)
    {
        WorldPacket wire(CMSG_GAME_OBJ_REPORT_USE);
        wire << bell.GetGUID();
        WorldPackets::GameObject::GameObjReportUse request(std::move(wire));
        request.Read();
        player.GetSession()->HandleGameobjectReportUse(request);
    }
    else
    {
        WorldPacket wire(CMSG_GAME_OBJ_USE);
        wire << bell.GetGUID();
        WorldPackets::GameObject::GameObjectUse request(std::move(wire));
        request.Read();
        player.GetSession()->HandleGameObjectUse(request);
    }
    Tick(player, 3000);
}

#ifndef SINGING_POOLS_BASELINE
void CheckPoolWater(LootPlayer& player, uint32 triggerId, uint32 spell)
{
    Map& map = *player.GetMap();
    auto trigger = sAreaTriggerStore.LookupEntry(triggerId);
    Require(trigger && trigger->ContinentID == 860, "Missing an original pool trigger");
    Position water;
    Position above;
    bool found = false;
    for (int xOffset = -36; xOffset <= 36 && !found; xOffset += 6)
    {
        for (int yOffset = -36; yOffset <= 36 && !found; yOffset += 6)
        {
            float const x = trigger->Pos.X + xOffset;
            float const y = trigger->Pos.Y + yOffset;
            map.LoadGrid(x, y);
            float const surface = map.GetWaterLevel(x, y);
            if (surface <= trigger->Pos.Z - trigger->BoxHeight / 2)
                continue;
            water.Relocate(x, y, std::min(surface - 0.25f, trigger->Pos.Z));
            above.Relocate(x, y, surface + 0.25f);
            if (!(map.getLiquidStatus(x, y, water.GetPositionZ(), MAP_ALL_LIQUIDS) &
                (LIQUID_MAP_IN_WATER | LIQUID_MAP_UNDER_WATER)))
                continue;
            MoveGilneasPlayer(player, water);
            found = player.IsInAreaTriggerRadius(trigger);
            if (found && spell == 102938)
            {
                MoveGilneasPlayer(player, above);
                found = player.IsInAreaTriggerRadius(trigger);
            }
        }
    }
    if (!found)
        std::cout << "CASE could not sample pool " << triggerId << std::endl;
    Require(found, "Could not locate actual pool liquid inside the native trigger");
    auto relocate = [&player, &map](Position const& position) {
        MoveGilneasPlayer(player, position);
        player.UpdateUnderwaterState(&map, position.GetPositionX(), position.GetPositionY(), position.GetPositionZ());
    };
    relocate(above);
    Require(!player.Unit::IsInWater(), "The fixture must first enter above the wet surface");
    WorldPackets::Misc::AreaTrigger entering{WorldPacket(CMSG_AREA_TRIGGER)};
    entering.AreaTriggerID = triggerId;
    entering.Entered = true;
    entering.FromClient = true;
    player.GetSession()->HandleAreaTrigger(entering);
    Tick(player, 1500);
    Require(!player.HasAura(spell), "Dry entry applied the pool curse");

    relocate(water);
    Require(player.Unit::IsInWater() && player.IsInAreaTriggerRadius(trigger), "The fall left the original trigger or missed its water");
    sScriptMgr->OnUpdate(&player, 100);
    Tick(player, 1500);
    auto const transformed = player.HasAura(spell);
    std::cout << "CASE pool " << triggerId << ": wet=" << player.Unit::IsInWater() << " transformed=" << transformed << std::endl;
#ifdef SINGING_POOLS_WATER_BASELINE
    Require(!transformed, "The old one-shot pool script unexpectedly detected the fall");
    std::cout << "EXPECTED_BROKEN falling inside the same trigger never reapplied the curse" << std::endl;
#else
    Require(transformed, "Actual water contact inside the pool did not transform the pupil");
    relocate(above);
    sScriptMgr->OnUpdate(&player, 100);
    Require(!player.HasAura(spell), "A pool curse remained after leaving the liquid");
    std::cout << "PASS pool " << triggerId << " transforms on liquid entry and restores the pupil when dry" << std::endl;
    if (triggerId == 6986)
    {
        Teacher pole(map, 244137, 54993);
        Require(pole.GetVehicleKit(), "An original balance pole has no vehicle");
        auto tryBoard = [&player, &pole] {
            pole.HandleSpellClick(&player);
            for (uint32 elapsed = 0; elapsed < 1000; elapsed += 50)
            {
                Tick(player, 50);
                pole.Unit::Update(50);
            }
        };
        relocate(water);
        sScriptMgr->OnUpdate(&player, 100);
        Require(player.HasAura(spell), "The pupil was not cursed before the boarding attempt");
        MoveGilneasPlayer(player, pole.GetPosition());
        std::cout << "CASE trying the placed pole while cursed" << std::endl;
        tryBoard();
        Require(!player.GetVehicle(), "A frog remained seated on the original pole");
        relocate(above);
        sScriptMgr->OnUpdate(&player, 100);
        MoveGilneasPlayer(player, pole.GetPosition());
        std::cout << "CASE trying the placed pole when dry" << std::endl;
        tryBoard();
        bool const boarded = player.GetVehicleBase() == &pole;
        if (player.GetVehicle())
            player.ExitVehicle();
        Require(boarded && !player.GetVehicle(), "The dry pupil could not board and leave the native balance pole");
        relocate(water);
        sScriptMgr->OnUpdate(&player, 100);
        Require(player.HasAura(spell), "Water after leaving the pole did not restore the curse");
        relocate(above);
        sScriptMgr->OnUpdate(&player, 100);
        Require(!player.HasAura(spell), "The repeated fall did not recover on dry land");
        std::cout << "PASS frog boarding rejection, native dry seat, dismount and actual-water retry" << std::endl;
    }
#endif
}
#endif

#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE) && !defined(SINGING_POOLS_REEDS_BASELINE)
struct PoolPet : LootPet
{
    PoolPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        setFaction(owner.getFaction());
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }
};

void CheckBalancedRock()
{
    Map map(860, 665, 0, DIFFICULTY_NONE);
    Teacher aysa(map);
    LootPlayer owner(SupplySession(), map, 2966301, aysa.GetPosition());
    PreparePoolHunter(owner);
    owner.SetRewardedQuest(Introduction);
    AcceptQuest(owner, aysa, BalancedRock);
    Require(owner.GetQuestStatus(BalancedRock) == QUEST_STATUS_INCOMPLETE, "Aysa did not admit the six-monk lesson");
    PoolPet pet(owner, map);

#ifdef SINGING_POOLS_ROCK_BASELINE
    constexpr unsigned battles = 1;
#else
    constexpr unsigned battles = 6;
#endif
    auto quest = sQuestDataStore->GetQuestTemplate(BalancedRock);
    for (unsigned index = 0; index < battles; ++index)
    {
        uint32 const spawn = MonkSpawns[index % std::size(MonkSpawns)];
        if (index >= std::size(MonkSpawns))
            map.RemoveCreatureRespawnTime(spawn);
        Teacher monk(map, spawn, TushuiMonk);
        monk.AI()->UpdateAI(1100);
#ifndef SINGING_POOLS_ROCK_BASELINE
        if (!index)
        {
            std::list<Creature*> poles;
            monk.GetCreatureListWithEntryInGrid(poles, 54993, 5.0f);
            auto failed = std::find_if(poles.begin(), poles.end(), [](Creature* pole) { return pole->ToTempSummon(); });
            Require(failed != poles.end() && !monk.GetVehicleBase(), "No pending native pole join to interrupt");
            ObjectGuid const failedGuid = (*failed)->GetGUID();
            (*failed)->RemoveAurasByType(SPELL_AURA_CONTROL_VEHICLE);
            monk.Unit::Update(50);
            Require(!monk.GetVehicleBase(), "Interrupting the ride aura did not cancel the native join");
            monk.AI()->UpdateAI(1100);
            map.RemoveAllObjectsInRemoveList();
            Require(!map.GetCreature(failedGuid), "An interrupted join left its unused pole");
            std::cout << "PASS canceled native join, unused-pole cleanup and retry" << std::endl;
        }
#endif
        for (uint32 elapsed = 0; elapsed < 2500 && !monk.GetVehicleBase(); elapsed += 50)
            monk.Unit::Update(50);
        Creature* pole = monk.GetVehicleBase() ? monk.GetVehicleBase()->ToCreature() : nullptr;
        Require(pole && pole->GetEntry() == 54993 && pole->ToTempSummon(), "The genuine Tushui Monk did not take its native pole seat");
        auto const poleGuid = pole->GetGUID();

        monk.AI()->Reset();
        monk.AI()->UpdateAI(1100);
        Require(monk.GetVehicleBase() == pole, "Reset changed the Tushui Monk's occupied pole");
        MoveGilneasPlayer(owner, monk.GetPosition());
        Unit& killer = index % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(owner);
        if (&killer == &pet)
        {
            map.CreatureRelocation(&pet, monk.GetPositionX(), monk.GetPositionY(), monk.GetPositionZ(), monk.GetOrientation());
            map.MoveAllCreaturesInMoveList();
        }
        if (!killer.IsValidAttackTarget(&monk))
            std::cout << "Monk attack admission: visible=" << killer.canSeeOrDetect(&monk, false)
                << " flags=" << monk.GetUInt32Value(UNIT_FIELD_FLAGS) << " attackerFlags=" << killer.GetUInt32Value(UNIT_FIELD_FLAGS)
                << " factions=" << killer.getFaction() << '/' << monk.getFaction() << " reactions="
                << killer.GetReactionTo(&monk) << '/' << monk.GetReactionTo(&killer)
                << " alive=" << monk.IsAlive() << " unattackable=" << monk.HasUnitState(UNIT_STATE_UNATTACKABLE) << std::endl;
        Require(killer.IsValidAttackTarget(&monk), "A seated monk was not a native attack target");
        killer.DealDamage(&monk, uint32(monk.GetMaxHealth()) * 2);
        Tick(owner, 100);
        if (Creature* pending = map.GetCreature(poleGuid))
            pending->Update(1500);
        map.RemoveAllObjectsInRemoveList();
        bool const leftBehind = map.GetCreature(poleGuid);
        if (Creature* retained = map.GetCreature(poleGuid))
        {
            retained->DespawnOrUnsummon();
            map.RemoveAllObjectsInRemoveList();
        }
        Require(!monk.IsAlive() && owner.GetQuestObjectiveData(BalancedRock, TushuiMonk) == int32(index + 1),
            "The placed monk did not award one actual defeat to the player or pet owner");
#ifdef SINGING_POOLS_ROCK_BASELINE
        Require(leftBehind, "The old reset unexpectedly retained ownership of the temporary pole");
        std::cout << "EXPECTED_BROKEN Monk reset left its temporary pole after the actual defeat" << std::endl;
#else
        Require(!leftBehind && owner.CanRewardQuest(quest, false) == (index + 1 == battles),
            "Defeat left a temporary pole or changed the six-monk reward gate");
#endif
    }
#ifndef SINGING_POOLS_ROCK_BASELINE
    MoveGilneasPlayer(owner, aysa.GetPosition());
    RewardQuest(owner, aysa, BalancedRock);
    Require(owner.IsQuestRewarded(BalancedRock), "Aysa did not accept the six native defeats");
    std::cout << "PASS four placed Tushui Monks, six real defeats, owner and pet credit, reset cleanup and Aysa's turn-in" << std::endl;
#endif
}
#endif

void CheckBell()
{
    for (uint32 spawn : {ShangTempleSpawn, AysaSpawn, ReedGiverSpawn, LiangSpawn, 244137u})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    for (uint32 spawn : MonkSpawns)
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    for (uint32 spawn : PincerSpawns)
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    for (uint32 spawn : {CartTenderSpawn, ShoreAysaSpawn, ShuSpawn, FarmCartSpawn, JiSpawn, WreckCartSpawn, MinSpawn, DeloraSpawn})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    for (uint32 spawn : VirmenSpawns)
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    Map map(860, 663, 0, DIFFICULTY_NONE);
    Teacher aysa(map);
    Teacher shang(map, ShangTempleSpawn, 54786);
    Bell bell(map);
    LootPlayer pupil(SupplySession(), map, 2966101, shang.GetPosition());
    PreparePoolHunter(pupil);
    pupil.SetRewardedQuest(29423);
    AcceptQuest(pupil, shang, Introduction);
    Require(pupil.GetQuestStatus(Introduction) == QUEST_STATUS_COMPLETE, "Shang Xi did not admit the trip to the Singing Pools");
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    RewardQuest(pupil, aysa, Introduction);
    Require(pupil.IsQuestRewarded(Introduction), "Aysa did not accept the Singing Pools introduction");
    if (pupil.GetQuestStatus(DryFur) == QUEST_STATUS_NONE)
        AcceptQuest(pupil, aysa, DryFur);
    Require(pupil.GetQuestStatus(DryFur) == QUEST_STATUS_INCOMPLETE, "Aysa did not admit the training lesson");

#ifndef SINGING_POOLS_BASELINE
    Ring(pupil, bell, false);
    Require(!pupil.GetQuestObjectiveData(DryFur, BellCredit), "Remote ordinary use completed the lesson");
    MoveGilneasPlayer(pupil, bell.GetPosition());
    Ring(pupil, bell, false);
    Require(pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1, "Ordinary use lost the native bell spell");
    AbandonSupply(pupil, DryFur);
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    AcceptQuest(pupil, aysa, DryFur);
    Require(pupil.GetQuestStatus(DryFur) == QUEST_STATUS_INCOMPLETE && !pupil.GetQuestObjectiveData(DryFur, BellCredit),
        "The next attempt kept the previous bell credit");
#endif
    MoveGilneasPlayer(pupil, bell.GetPosition());
    Ring(pupil, bell, true);
    auto report = pupil.GetQuestObjectiveData(DryFur, BellCredit);
    std::cout << "CASE Training Bell report credit=" << report << std::endl;
#ifdef SINGING_POOLS_BASELINE
    Ring(pupil, bell, false);
    Require(!report && pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1,
        "Expected only ordinary use to ring the original bell");
    std::cout << "EXPECTED_BROKEN report-use skipped the normal bell spell" << std::endl;
#else
    Require(report == 1, "Report-use did not ring the bell");
    Ring(pupil, bell, false);
    Require(pupil.GetQuestObjectiveData(DryFur, BellCredit) == 1 && pupil.GetQuestStatus(DryFur) == QUEST_STATUS_COMPLETE,
        "Duplicate use changed the completed objective");
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    uint32 const choice = PoolRewardChoice(pupil, DryFur);
    RewardQuest(pupil, aysa, DryFur, choice);
    Require(pupil.IsQuestRewarded(DryFur) && pupil.GetItemCount(choice) == 1, "Aysa did not issue the selected belt");
    if (pupil.GetQuestStatus(NextQuest) == QUEST_STATUS_NONE)
        AcceptQuest(pupil, aysa, NextQuest);
    Require(pupil.GetQuestStatus(NextQuest) != QUEST_STATUS_NONE, "Aysa did not admit Finding an Old Friend");
    std::cout << "PASS native Aysa arrival, Training Bell and next quest" << std::endl;
    CheckPoolWater(pupil, 6986, 102938);
#ifndef SINGING_POOLS_WATER_BASELINE
    CheckPoolWater(pupil, 6987, 102938);
    CheckPoolWater(pupil, 6988, 102939);
    CheckPoolWater(pupil, 7012, 102940);
    CheckPoolWater(pupil, 6991, 102941);
    CheckPoolWater(pupil, 6990, 102942);
#endif
#endif
}

#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE)
void CheckReeds()
{
    Map map(860, 664, 0, DIFFICULTY_NONE);
    Teacher gee(map, ReedGiverSpawn, 55021);
    LootPlayer owner(SupplySession(), map, 2966201, gee.GetPosition());
    LootPlayer visitor(SupplySession(), map, 2966202, gee.GetPosition());
    PreparePoolHunter(owner);
    PreparePoolHunter(visitor);
    owner.SetRewardedQuest(29423);
    AcceptQuest(owner, gee, Reeds);
    Require(owner.GetQuestStatus(Reeds) == QUEST_STATUS_INCOMPLETE, "The placed Jojo did not admit Stronger Than Reeds");

    auto original = WorldDatabase.Query("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=209507 AND map=860 AND zoneId=5736 ORDER BY guid");
    Require(original && original->GetRowCount() == 30, "Missing original Hard Tearwood Reed placements");
    std::vector<Placement> reeds;
    do
    {
        auto row = original->Fetch();
        reeds.push_back({row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    } while (original->NextRow());

    {
        auto const& placement = reeds.front();
        Supply first(map, placement);
        MoveGilneasPlayer(visitor, placement.position);
        OpenSupply(visitor, first);
#ifdef SINGING_POOLS_REEDS_BASELINE
        Require(visitor.GetLootGUID() == first.GetGUID() && first.getLootState() == GO_ACTIVATED && first.loot.isLooted(),
            "The original reeds no longer open empty shared loot without the quest");
        ReleaseSupply(visitor, first);
        first.Rearm();
#else
        Require(!visitor.GetLootGUID() && first.getLootState() == GO_READY, "A visitor without the quest consumed the reed");
#endif
        MoveGilneasPlayer(owner, placement.position);
        OpenSupply(owner, first);
        Require(TakeSupply(owner, first, ReedItem), "The original reed did not yield its item");
        ReleaseSupply(owner, first);
    }
#ifndef SINGING_POOLS_REEDS_BASELINE
    GiveSupply(owner, ReedItem, 2); // An older attempt can still have extras in its bags.
#endif
    AbandonSupply(owner, Reeds);
#ifdef SINGING_POOLS_REEDS_BASELINE
    Require(owner.GetItemCount(ReedItem) == 1, "The old abandoned attempt did not retain the reed as expected");
    std::cout << "EXPECTED_BROKEN unopened quest admitted empty reed loot, and abandonment retained the item" << std::endl;
#else
    Require(!owner.GetItemCount(ReedItem), "Abandonment kept a collected reed");
    MoveGilneasPlayer(owner, gee.GetPosition());
    AcceptQuest(owner, gee, Reeds);
    Require(owner.GetQuestStatus(Reeds) == QUEST_STATUS_INCOMPLETE, "Could not retake the reed quest");
    for (auto const& placement : reeds)
    {
        Supply stock(map, placement);
        Require(stock.ActivateToQuest(&owner) && !stock.ActivateToQuest(&visitor), "A placed Reed has the wrong quest marker");
    }
    for (size_t index = 1; index <= 8; ++index)
    {
        auto const& placement = reeds[index];
        Supply stock(map, placement);
        MoveGilneasPlayer(owner, placement.position);
        OpenSupply(owner, stock);
        Require(TakeSupply(owner, stock, ReedItem), "A real reed did not yield its item");
        ReleaseSupply(owner, stock);
        Require(owner.GetItemCount(ReedItem) == index &&
            owner.CanRewardQuest(sQuestDataStore->GetQuestTemplate(Reeds), false) == (index == 8),
            "A reed gave duplicate or missing loot or changed the eight-item reward gate");
    }
    MoveGilneasPlayer(owner, gee.GetPosition());
    RewardQuest(owner, gee, Reeds);
    Require(owner.IsQuestRewarded(Reeds) && !owner.GetItemCount(ReedItem), "The placed reed giver did not consume eight reeds");
    std::cout << "PASS 30 original reed placements, eight actual items, refusal, retry and turn-in" << std::endl;
#endif
}

void CheckSunPearl()
{
    Map map(860, 666, 0, DIFFICULTY_NONE);
    Teacher aysa(map);
    Teacher liang(map, LiangSpawn, 55020);
    LootPlayer owner(SupplySession(), map, 2967701, aysa.GetPosition());
    LootPlayer visitor(SupplySession(), map, 2967702, aysa.GetPosition());
    PreparePoolHunter(owner);
    PreparePoolHunter(visitor);
    owner.SetRewardedQuest(DryFur);
    AcceptQuest(owner, aysa, NextQuest);
    Require(owner.GetQuestStatus(NextQuest) == QUEST_STATUS_COMPLETE, "Aysa did not admit the trip to Liang");
    MoveGilneasPlayer(owner, liang.GetPosition());
    RewardQuest(owner, liang, NextQuest);
    Require(owner.IsQuestRewarded(NextQuest), "The hilltop Liang did not accept Finding an Old Friend");
    if (owner.GetQuestStatus(SunPearl) == QUEST_STATUS_NONE)
        AcceptQuest(owner, liang, SunPearl);
    Require(owner.GetQuestStatus(SunPearl) == QUEST_STATUS_INCOMPLETE, "The hilltop Liang did not offer The Sun Pearl");

    auto rowset = WorldDatabase.Query("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,"
        "rotation0,rotation1,rotation2,rotation3 FROM gameobject WHERE id=209584 AND guid=102472 AND map=860");
    Require(bool(rowset), "Missing the only placed Ancient Clam");
    auto row = rowset->Fetch();
    Placement const placement{row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
        {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
        G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())};
    Supply clam(map, placement);
    MoveGilneasPlayer(visitor, placement.position);
    OpenSupply(visitor, clam);
#ifdef SINGING_POOLS_PEARL_BASELINE
    Require(visitor.GetLootGUID() == clam.GetGUID() && clam.getLootState() == GO_ACTIVATED && clam.loot.isLooted(),
        "The original clam no longer opens empty loot for a visitor without the quest");
    ReleaseSupply(visitor, clam);
    clam.Rearm();
#else
    Require(!visitor.GetLootGUID() && clam.getLootState() == GO_READY && !clam.ActivateToQuest(&visitor) && clam.ActivateToQuest(&owner),
        "A visitor could consume the clam or the active pupil lost the quest marker");
#endif
    MoveGilneasPlayer(owner, placement.position);
    OpenSupply(owner, clam);
    Require(TakeSupply(owner, clam, PearlItem), "The original clam did not yield a real Sun Pearl");
    ReleaseSupply(owner, clam);
    AbandonSupply(owner, SunPearl);
#ifdef SINGING_POOLS_PEARL_BASELINE
    Require(owner.GetItemCount(PearlItem) == 1, "The original abandoned quest no longer retains the pearl");
    std::cout << "EXPECTED_BROKEN a visitor could empty the only shared clam and abandonment retained the pearl" << std::endl;
#else
    Require(!owner.GetItemCount(PearlItem), "Abandoning The Sun Pearl kept the item from the previous attempt");
    MoveGilneasPlayer(owner, liang.GetPosition());
    AcceptQuest(owner, liang, SunPearl);
    Require(owner.GetQuestStatus(SunPearl) == QUEST_STATUS_INCOMPLETE, "An abandoned attempt could not be restarted");
    clam.Rearm();
    MoveGilneasPlayer(owner, placement.position);
    OpenSupply(owner, clam);
    Require(TakeSupply(owner, clam, PearlItem), "A fresh attempt could not collect the pearl");
    ReleaseSupply(owner, clam);
    MoveGilneasPlayer(owner, liang.GetPosition());
    uint32 const choice = PoolRewardChoice(owner, SunPearl);
    RewardQuest(owner, liang, SunPearl, choice);
    Require(owner.IsQuestRewarded(SunPearl) && !owner.GetItemCount(PearlItem) && owner.GetItemCount(choice) == 1,
        "Liang did not consume the real Sun Pearl and grant the selected reward");
    std::cout << "PASS Aysa to hilltop Liang, original Ancient Clam guard, pearl retry and actual turn-in" << std::endl;
#endif
}

void CheckShuAdmission()
{
    Map map(860, 667, 0, DIFFICULTY_NONE);
    Teacher liang(map, LiangSpawn, 55020);
    LootPlayer owner(SupplySession(), map, 2967801, liang.GetPosition());
    LootPlayer pearlOnly(SupplySession(), map, 2967802, liang.GetPosition());
    PreparePoolHunter(owner);
    PreparePoolHunter(pearlOnly);
    PoolPet pet(owner, map);
    owner.SetRewardedQuest(NextQuest);
    pearlOnly.SetRewardedQuest(NextQuest);

    AcceptQuest(owner, liang, Shu);
    Require(owner.GetQuestStatus(Shu) == QUEST_STATUS_NONE && !owner.GetItemCount(LiangPearl),
        "Liang issued the supplied pearl before either prerequisite");
    AcceptQuest(owner, liang, Sting);
    Require(owner.GetQuestStatus(Sting) == QUEST_STATUS_INCOMPLETE, "Liang did not offer The Sting of Learning");
    for (size_t index = 0; index < std::size(PincerSpawns); ++index)
    {
        Teacher pincer(map, PincerSpawns[index], WaterPincer);
        MoveGilneasPlayer(owner, pincer.GetPosition());
        Unit& killer = index % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(owner);
        if (&killer == &pet)
        {
            map.CreatureRelocation(&pet, pincer.GetPositionX(), pincer.GetPositionY(), pincer.GetPositionZ(), pincer.GetOrientation());
            map.MoveAllCreaturesInMoveList();
        }
        Require(killer.IsValidAttackTarget(&pincer), "An original Water Pincer was not a native attack target");
        killer.DealDamage(&pincer, uint32(pincer.GetMaxHealth()) * 2);
        Tick(owner, 100);
        Require(!pincer.IsAlive() && owner.GetQuestObjectiveData(Sting, WaterPincer) == int32(index + 1),
            "Water Pincer defeat did not grant exactly one credit to the owner or pet owner");
    }
    MoveGilneasPlayer(owner, liang.GetPosition());
    RewardQuest(owner, liang, Sting);
    Require(owner.IsQuestRewarded(Sting), "Liang did not accept six Water Pincer defeats");

    pearlOnly.SetRewardedQuest(SunPearl);
    AcceptQuest(owner, liang, Shu);
    AcceptQuest(pearlOnly, liang, Shu);
#ifdef SINGING_POOLS_SHU_ADMISSION_BASELINE
    Require(owner.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE && pearlOnly.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE
        && owner.GetItemCount(LiangPearl) == 1 && pearlOnly.GetItemCount(LiangPearl) == 1,
        "The original either-quest prerequisite no longer issues Liang's pearl");
    std::cout << "EXPECTED_BROKEN Liang offered Shu and its item after only one of the two prerequisite tasks" << std::endl;
#else
    Require(owner.GetQuestStatus(Shu) == QUEST_STATUS_NONE && pearlOnly.GetQuestStatus(Shu) == QUEST_STATUS_NONE
        && !owner.GetItemCount(LiangPearl) && !pearlOnly.GetItemCount(LiangPearl),
        "Liang offered Shu after only one of the two prerequisite tasks");
    owner.SetRewardedQuest(SunPearl);
    AcceptQuest(owner, liang, Shu);
    Require(owner.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(LiangPearl) == 1,
        "Liang did not give exactly one pearl after both prerequisites");
    AbandonSupply(owner, Shu);
    Require(!owner.GetItemCount(LiangPearl), "Abandoning Shu kept the provided item");
    AcceptQuest(owner, liang, Shu);
    Require(owner.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE && owner.GetItemCount(LiangPearl) == 1,
        "Liang did not provide exactly one pearl again on retry");
    std::cout << "PASS six native Water Pincer defeats, owner and pet credit, Liang's turn-in, both prerequisites and Shu item retry" << std::endl;
#endif
}

void UseReflectionPearl(LootPlayer& pupil)
{
    Item* pearl = pupil.GetItemByEntry(LiangPearl);
    Require(pearl, "Liang's quest-supplied pearl is missing");
    WorldPackets::Spells::ItemUse use{WorldPacket(CMSG_USE_ITEM)};
    use.itemGUID = pearl->GetGUID();
    use.bagIndex = pearl->GetBagSlot();
    use.slot = pearl->GetSlot();
    use.Cast.SpellID = 103116;
    use.Cast.Target.Flags = TARGET_FLAG_UNIT;
    use.Cast.Target.Unit = pupil.GetGUID();
    pupil.GetSession()->HandleUseItemOpcode(use);
    Tick(pupil, 4000);
}

Creature* FindOwnedCartActor(LootPlayer& player, uint32 entry)
{
    std::list<Creature*> actors;
    player.GetCreatureListWithEntryInGrid(actors, entry, 20.0f);
    Creature* result = nullptr;
    for (Creature* actor : actors)
        if (TempSummon* summon = actor->ToTempSummon(); summon && summon->GetSummonerGUID() == player.GetGUID())
        {
            Require(!result, "The cart interaction spawned more than one actor of an owned type");
            result = actor;
        }
    return result;
}

void CheckLivelihoodCart(Map& map, LootPlayer& pupil, Teacher& aysa)
{
    if (pupil.GetQuestStatus(Livelihood) == QUEST_STATUS_NONE)
        AcceptQuest(pupil, aysa, Livelihood);
    Require(pupil.GetQuestStatus(Livelihood) == QUEST_STATUS_COMPLETE && !pupil.GetQuestObjectiveData(Livelihood, CartTender),
        "Travel should be optional when Aysa sends the pupil to Ji");
    Teacher tender(map, CartTenderSpawn, CartTender);
    Teacher ji(map, JiSpawn, 55477);
    MoveGilneasPlayer(pupil, tender.GetPosition());
    WorldPackets::Spells::SpellClick use{WorldPacket(CMSG_SPELL_CLICK)};
    use.SpellClickUnitGuid = tender.GetGUID();
    pupil.GetSession()->HandleSpellClick(use);
#ifndef SINGING_POOLS_CART_BASELINE
    pupil.GetSession()->HandleSpellClick(use);
#endif
    Creature* cart = FindOwnedCartActor(pupil, 57208);
    Creature* yak = FindOwnedCartActor(pupil, 57207);
    Require(cart && yak && cart->GetVehicleKit(), "The original cart tender did not make the native cart and yak");
    Tick(pupil, 2000);
    Require(cart->GetVehicleKit()->GetPassenger(0) == &pupil, "The real carriage did not board the clicker");
    auto* escort = dynamic_cast<npc_escortAI*>(yak->AI());
    Require(escort && sScriptSystemMgr->GetPointMoveList(57207).size() == 17, "The cart yak did not load its original route");
    escort->UpdateAI(2600);
    Require(escort->HasEscortState(STATE_ESCORT_ESCORTING), "The mounted rider did not admit the original yak route");
    auto const cartGuid = cart->GetGUID();
    auto const yakGuid = yak->GetGUID();
    pupil.ExitVehicle();
    cart->Unit::Update(1100);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(cartGuid), "The carriage did not leave after an early exit");
    if (Creature* activeYak = map.GetCreature(yakGuid))
        activeYak->AI()->UpdateAI(300);
    map.RemoveAllObjectsInRemoveList();
#ifdef SINGING_POOLS_CART_BASELINE
    Creature* orphan = map.GetCreature(yakGuid);
    Require(orphan && !orphan->IsDespawn(), "The original early exit no longer leaves the yak");
    orphan->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN an early exit removed the native carriage but left its owned yak" << std::endl;
#else
    Require(!map.GetCreature(yakGuid), "The original yak remained after the owner abandoned the ride");
    MoveGilneasPlayer(pupil, tender.GetPosition());
    pupil.GetSession()->HandleSpellClick(use);
    Creature* rejectedCart = FindOwnedCartActor(pupil, 57208);
    Creature* rejectedYak = FindOwnedCartActor(pupil, 57207);
    Require(rejectedCart && rejectedYak && !pupil.GetVehicleBase(), "The retry did not have a pending boarding event");
    auto const rejectedCartGuid = rejectedCart->GetGUID();
    auto const rejectedYakGuid = rejectedYak->GetGUID();
    rejectedCart->RemoveAurasByType(SPELL_AURA_CONTROL_VEHICLE);
    Tick(pupil, 100);
    Require(!pupil.GetVehicleBase(), "The canceled ride still boarded the player");
    rejectedYak->AI()->UpdateAI(2600);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(rejectedCartGuid) && !map.GetCreature(rejectedYakGuid), "A failed native join left the cart or its yak behind");
    std::cout << "PASS duplicate click suppression, failed native boarding cleanup and an independent retry" << std::endl;
#endif
    MoveGilneasPlayer(pupil, ji.GetPosition());
    RewardQuest(pupil, ji, Livelihood);
    Require(pupil.IsQuestRewarded(Livelihood), "Ji rejected the optional route after leaving the carriage");
    std::cout << "PASS original cart tender boarding, early exit and actual Ji turn-in with optional travel" << std::endl;
#ifndef SINGING_POOLS_CART_BASELINE
    struct OtherCart
    {
        uint32 Spawn;
        uint32 Tender;
        uint32 Yak;
        size_t Waypoints;
    };
    for (OtherCart const& route : {OtherCart{FarmCartSpawn, 59497, 59499, 14}, OtherCart{WreckCartSpawn, 57741, 57742, 24}})
    {
        Teacher other(map, route.Spawn, route.Tender);
        MoveGilneasPlayer(pupil, other.GetPosition());
        use.SpellClickUnitGuid = other.GetGUID();
        pupil.GetSession()->HandleSpellClick(use);
        Creature* sharedCart = FindOwnedCartActor(pupil, 57208);
        Creature* sharedYak = FindOwnedCartActor(pupil, route.Yak);
        if (!sharedCart || !sharedYak)
            std::cout << "Cart " << route.Tender << " area=" << pupil.GetAreaId() << " pending=" << pupil.waitOnSeat
                << " vehicle=" << bool(pupil.GetVehicleBase()) << " flags=" << other.GetUInt32Value(UNIT_FIELD_NPC_FLAGS)
                << " cart=" << bool(sharedCart) << " yak=" << bool(sharedYak) << std::endl;
        Require(sharedCart && sharedYak && sharedCart->GetVehicleKit(), "A shared original cart route lost its actors");
        Tick(pupil, 2000);
        Require(sharedCart->GetVehicleKit()->GetPassenger(0) == &pupil, "A shared original cart route did not seat the rider");
        auto* sharedEscort = dynamic_cast<npc_escortAI*>(sharedYak->AI());
        Require(sharedEscort && sScriptSystemMgr->GetPointMoveList(route.Yak).size() == route.Waypoints, "A shared cart lost its native route");
        sharedEscort->UpdateAI(2600);
        Require(sharedEscort->HasEscortState(STATE_ESCORT_ESCORTING), "A shared cart failed to start with its real rider");
        auto const sharedCartGuid = sharedCart->GetGUID();
        auto const sharedYakGuid = sharedYak->GetGUID();
        pupil.ExitVehicle();
        sharedCart->Unit::Update(1100);
        if (Creature* activeYak = map.GetCreature(sharedYakGuid))
            activeYak->AI()->UpdateAI(300);
        map.RemoveAllObjectsInRemoveList();
        Require(!map.GetCreature(sharedCartGuid) && !map.GetCreature(sharedYakGuid), "Leaving a shared route left its actors");
    }
    std::cout << "PASS native boarding, route start and ownership cleanup for the other two cart tenders" << std::endl;
#endif
}

void CheckFarmSupply(Map& map, LootPlayer& pupil, LootPlayer& visitor, Teacher& giver, uint32 questId,
    uint32 firstEntry, uint32 lastEntry, size_t placementCount, bool expectLegacyFault)
{
    struct Stock
    {
        Placement Place;
        uint32 Item;
    };
    auto rows = WorldDatabase.PQuery("SELECT g.id,g.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,"
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3,l.Item FROM gameobject g JOIN gameobject_loot_template l ON l.Entry=g.id "
        "WHERE g.map=860 AND g.id BETWEEN %u AND %u ORDER BY l.Chance DESC,g.guid", firstEntry, lastEntry);
    Require(rows && rows->GetRowCount() == placementCount, "The original farm stock placements did not match");
    std::vector<Stock> stock;
    do
    {
        auto row = rows->Fetch();
        stock.push_back({{row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())}, row[11].GetUInt32()});
    } while (rows->NextRow());

    MoveGilneasPlayer(pupil, giver.GetPosition());
    AcceptQuest(pupil, giver, questId);
    Require(pupil.GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE, "The placed farm supplier did not offer the gathering quest");
    auto const* quest = sQuestDataStore->GetQuestTemplate(questId);
    auto const collect = [&](Supply& chest, uint32 item)
    {
        bool received = false;
        for (unsigned attempt = 0; attempt < 3 && !received; ++attempt)
        {
            OpenSupply(pupil, chest);
            received = TakeSupply(pupil, chest, item);
            ReleaseSupply(pupil, chest);
            if (!received)
                chest.Rearm();
        }
        Require(received, "Original farm stock did not yield its quest item after a retry");
    };
    auto const& first = stock.front();
    {
        Supply chest(map, first.Place);
        MoveGilneasPlayer(visitor, first.Place.position);
        OpenSupply(visitor, chest);
        if (expectLegacyFault)
        {
            Require(visitor.GetLootGUID() == chest.GetGUID() && chest.getLootState() == GO_ACTIVATED && chest.loot.isLooted(),
                "Old shared farm stock no longer opened an empty chest without its quest");
            ReleaseSupply(visitor, chest);
            chest.Rearm();
        }
        else
            Require(!visitor.GetLootGUID() && chest.getLootState() == GO_READY, "A visitor consumed original farm stock");
        MoveGilneasPlayer(pupil, first.Place.position);
        collect(chest, first.Item);
    }
    if (!expectLegacyFault)
        for (QuestObjective const& objective : quest->GetObjectives())
            GiveSupply(pupil, uint32(objective.ObjectID), uint32(objective.Amount + 1));
    AbandonSupply(pupil, questId);
    if (expectLegacyFault)
    {
        Require(pupil.GetItemCount(first.Item) != 0, "The old abandoned farm quest no longer retains its stock");
        std::cout << "EXPECTED_BROKEN quest " << questId << " allowed empty shared loot and retained abandoned stock" << std::endl;
        return;
    }
    for (QuestObjective const& objective : quest->GetObjectives())
        Require(!pupil.GetItemCount(objective.ObjectID), "The abandoned farm quest retained a gathered item");
    MoveGilneasPlayer(pupil, giver.GetPosition());
    AcceptQuest(pupil, giver, questId);
    for (Stock const& source : stock)
    {
        Supply chest(map, source.Place);
        Require(chest.ActivateToQuest(&pupil) && !chest.ActivateToQuest(&visitor), "A placed farm marker has wrong eligibility");
    }
    for (QuestObjective const& objective : quest->GetObjectives())
    {
        for (Stock const& source : stock)
        {
            if (source.Item != uint32(objective.ObjectID) || pupil.GetItemCount(source.Item) >= uint32(objective.Amount))
                continue;
            Supply chest(map, source.Place);
            MoveGilneasPlayer(pupil, source.Place.position);
            collect(chest, source.Item);
        }
        Require(pupil.GetItemCount(objective.ObjectID) == uint32(objective.Amount), "Original farm deposits did not reach the exact target amount");
    }
    MoveGilneasPlayer(pupil, giver.GetPosition());
    uint32 const choice = quest->PackageID ? PoolRewardChoice(pupil, questId) : 0;
    RewardQuest(pupil, giver, questId, choice);
    Require(pupil.IsQuestRewarded(questId) && (!choice || pupil.GetItemCount(choice) == 1), "The real farm giver did not reward the collected stock");
    for (QuestObjective const& objective : quest->GetObjectives())
        Require(!pupil.GetItemCount(objective.ObjectID), "Farm reward did not consume its required items");
    std::cout << "PASS " << placementCount << " original farm placements, retry and reward for quest " << questId << std::endl;
}

void CheckDaiLoFarm(Map& map, LootPlayer& pupil)
{
    Teacher ji(map, JiSpawn, 55477);
    Teacher delora(map, DeloraSpawn, 55479);
    Teacher min(map, MinSpawn, 55478);
    LootPlayer visitor(SupplySession(), map, 2977002, delora.GetPosition());
    PreparePoolHunter(visitor);
    PoolPet pet(pupil, map);
    MoveGilneasPlayer(pupil, ji.GetPosition());
    AcceptQuest(pupil, ji, Rascals);
    Require(pupil.GetQuestStatus(Rascals) == QUEST_STATUS_INCOMPLETE, "Ji did not open the actual Rascals quest");
    for (size_t index = 0; index < std::size(VirmenSpawns); ++index)
    {
        Teacher virmen(map, VirmenSpawns[index], Virmen);
        MoveGilneasPlayer(pupil, virmen.GetPosition());
        Unit& killer = index % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(pupil);
        if (&killer == &pet)
        {
            map.CreatureRelocation(&pet, virmen.GetPositionX(), virmen.GetPositionY(), virmen.GetPositionZ(), virmen.GetOrientation());
            map.MoveAllCreaturesInMoveList();
        }
        Require(killer.IsValidAttackTarget(&virmen), "An original Virmen was not a native attack target");
        killer.DealDamage(&virmen, uint32(virmen.GetMaxHealth()) * 2);
        Tick(pupil, 100);
        Require(!virmen.IsAlive() && pupil.GetQuestObjectiveData(Rascals, Virmen) == int32(index + 1), "A Virmen defeat lost or duplicated quest credit");
    }
    MoveGilneasPlayer(pupil, ji.GetPosition());
    RewardQuest(pupil, ji, Rascals);
    Require(pupil.IsQuestRewarded(Rascals), "Ji did not accept the ten actual Virmen defeats");
    std::cout << "PASS ten original Virmen defeats with player and pet credit and Ji's turn-in" << std::endl;
#ifdef SINGING_POOLS_FARM_BASELINE
    CheckFarmSupply(map, pupil, visitor, delora, StillGood, 209639, 209645, 38, true);
    CheckFarmSupply(map, pupil, visitor, min, StrongWood, 209646, 209646, 14, true);
#else
    CheckFarmSupply(map, pupil, visitor, delora, StillGood, 209639, 209645, 38, false);
    CheckFarmSupply(map, pupil, visitor, min, StrongWood, 209646, 209646, 14, false);
#ifdef SINGING_POOLS_MALLET_BASELINE
    CheckFarmSupply(map, pupil, visitor, ji, MissingMallet, 214406, 214406, 1, true);
#else
    CheckFarmSupply(map, pupil, visitor, ji, MissingMallet, 214406, 214406, 1, false);
#endif
#endif
}

Creature* FindWaterSpout(Teacher& shu)
{
    std::list<Creature*> spouts;
    shu.GetCreatureListWithEntryInGrid(spouts, 60488, 20.0f);
    if (spouts.empty())
        return nullptr;
    Require(spouts.size() == 1 && spouts.front()->ToTempSummon() && spouts.front()->ToTempSummon()->GetSummonerGUID() == shu.GetGUID(),
        "Shu did not publish exactly one owned water spout");
    return spouts.front();
}

Creature* WaitForWaterSpout(Teacher& shu)
{
    shu.AI()->UpdateAI(6000);
    for (uint32 elapsed = 0; elapsed < 4000; elapsed += 20)
        shu.Unit::Update(20);
    shu.GetMap()->MoveAllCreaturesInMoveList();
    Require(shu.movespline->Finalized() && shu.movespline->Initialized() && shu.movespline->ComputeProgress() == 1.0f,
        "Shu did not finish the native rock jump");
    shu.AI()->UpdateAI(2000);
    return FindWaterSpout(shu);
}

void CheckNewFriend(Map& map, LootPlayer& pupil)
{
    Require(pupil.GetPopupQuestId() == NewFriend, "Remote completion did not publish A New Friend");
    WorldPackets::Quest::QuestGiverAcceptQuest follow{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    follow.QuestGiverGUID = pupil.GetGUID();
    follow.QuestID = NewFriend;
    pupil.GetSession()->HandleQuestGiverAcceptQuest(follow);
    Require(pupil.GetQuestStatus(NewFriend) == QUEST_STATUS_INCOMPLETE, "The native follow-up offer was not accepted");
    Teacher shu(map, ShuSpawn, 65493);
    Teacher aysa(map, ShoreAysaSpawn, 54975);

#ifdef SINGING_POOLS_SPOUT_BASELINE
    Require(!WaitForWaterSpout(shu) && shu.SelectNearestPlayerNotGM(50.0f) == &pupil,
        "The old jump unexpectedly produced a spout or there was no eligible nearby pupil");
    std::cout << "EXPECTED_BROKEN the original movement ID left the spout sequence idle after Shu landed" << std::endl;
    // Isolate the secondary cleanup fault by delivering the expected callback.
    shu.AI()->MovementInform(EFFECT_MOTION_TYPE, 1);
    shu.AI()->UpdateAI(2000);
    Creature* pending = FindWaterSpout(shu);
    Require(pending, "The original spout failed to spawn even with its expected movement callback");
    auto const canceledGuid = pending->GetGUID();
    shu.AI()->Reset();
    map.RemoveAllObjectsInRemoveList();
    Creature* leaked = map.GetCreature(canceledGuid);
    Require(leaked && !leaked->IsDespawn(), "The original Shu reset no longer leaves a spout behind");
    leaked->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN resetting Shu left a manual-despawn spout in the pool" << std::endl;
#else
    Creature* pending = WaitForWaterSpout(shu);
    Require(pending, "Shu's native landing did not start the water spout cycle");
    auto const canceledGuid = pending->GetGUID();
    shu.AI()->Reset();
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(canceledGuid), "Resetting Shu left its old water spout");
    for (int32 count = 0; count <= 5; ++count)
    {
        Creature* spout = WaitForWaterSpout(shu);
        Require(spout, "A new water spout cycle did not start after the last one ended");
        auto const spoutGuid = spout->GetGUID();
        MoveGilneasPlayer(pupil, count ? spout->GetPosition() : aysa.GetPosition());
        shu.AI()->UpdateAI(500);
        Require(pupil.GetQuestObjectiveData(NewFriend, SpoutCredit) == std::max(0, count - 1),
            "The warning phase awarded spout credit");
        shu.AI()->UpdateAI(7100);
        Tick(pupil, 3500);
        Require(pupil.GetQuestObjectiveData(NewFriend, SpoutCredit) == count,
            "The genuine burst awarded missing, remote or duplicate water spout credit");
        shu.AI()->UpdateAI(3100);
        map.RemoveAllObjectsInRemoveList();
        Require(!map.GetCreature(spoutGuid), "A completed water spout stayed in the pool");
    }
    MoveGilneasPlayer(pupil, aysa.GetPosition());
    uint32 const choice = PoolRewardChoice(pupil, NewFriend);
    RewardQuest(pupil, aysa, NewFriend, choice);
    Require(pupil.IsQuestRewarded(NewFriend) && pupil.GetItemCount(choice) == 1, "Aysa did not accept the five genuine bursts and issue the selected reward");
    std::cout << "PASS actual Shu follow-up acceptance, water spout reset cleanup, a missed burst, five contacts and the shore Aysa turn-in" << std::endl;
    CheckLivelihoodCart(map, pupil, aysa);
#ifndef SINGING_POOLS_CART_BASELINE
    CheckDaiLoFarm(map, pupil);
#endif
#endif
}

void CheckReflectionCrossing()
{
    Map map(860, 668, 0, DIFFICULTY_NONE);
    Teacher liang(map, LiangSpawn, 55020);
    LootPlayer pupil(SupplySession(), map, 2967831, liang.GetPosition());
    PreparePoolHunter(pupil);
    pupil.GetMotionMaster()->Initialize();
    pupil.SetRewardedQuest(NextQuest);
    pupil.SetRewardedQuest(Sting);
    pupil.SetRewardedQuest(SunPearl);
    AcceptQuest(pupil, liang, Shu);
    Require(pupil.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE, "Liang did not admit the Reflection crossing");
#ifndef SINGING_POOLS_REFLECTION_BASELINE
    UseReflectionPearl(pupil);
#ifdef SINGING_POOLS_COAX_BASELINE
    Require(pupil.GetQuestObjectiveData(Shu, CoaxCredit) == 1 && !pupil.GetQuestObjectiveData(Shu, ReflectionCredit),
        "The original pearl no longer awarded the second objective in front of Liang");
    std::cout << "EXPECTED_BROKEN the provided pearl called Shu from Liang's house before the crossing" << std::endl;
    return;
#else
    Require(!pupil.GetQuestObjectiveData(Shu, CoaxCredit) && pupil.GetItemCount(LiangPearl) == 1,
        "Using the pearl before crossing awarded credit or consumed it");
#endif
#endif

    {
        ReflectionRock last(map, 102478, 103071);
        MoveGilneasPlayer(pupil, last.GetPosition());
        last.Use(&pupil);
        Require(!pupil.movespline->Finalized(), "The genuine final rock did not start a movement spline");
#ifdef SINGING_POOLS_REFLECTION_BASELINE
        Require(pupil.GetQuestObjectiveData(Shu, ReflectionCredit) == 1, "The old route no longer credited the pupil on launch");
        Tick(pupil, 4000);
        std::cout << "EXPECTED_BROKEN crossing credit was awarded at takeoff while the movement spline was still active" << std::endl;
#else
        Require(!pupil.GetQuestObjectiveData(Shu, ReflectionCredit), "Crossing credit was awarded before landing");
        pupil.StopMoving();
        Tick(pupil, 100);
        Require(!pupil.GetQuestObjectiveData(Shu, ReflectionCredit), "Interrupted movement granted Reflection crossing credit");
#endif
    }
#ifndef SINGING_POOLS_REFLECTION_BASELINE
    ReflectionRock first(map, 102474, 103071);
    ReflectionRock second(map, 102476, 103072);
    ReflectionRock third(map, 102477, 103078);
    ReflectionRock last(map, 102478, 103071);
    MoveGilneasPlayer(pupil, first.GetPosition());
    for (ReflectionRock* rock : {&first, &second, &third, &last})
    {
        Require(pupil.IsWithinDistInMap(rock, 1.0f), "The previous jump did not land next to the next placed rock");
        rock->Use(&pupil);
        Require(!pupil.movespline->Finalized() && !pupil.GetQuestObjectiveData(Shu, ReflectionCredit),
            "The next Rock Jump did not start or awarded credit early");
        Tick(pupil, 4000);
        Require(pupil.movespline->Finalized() && pupil.GetQuestObjectiveData(Shu, ReflectionCredit) == (rock == &last ? 1 : 0),
            "A Rock Jump failed to land or changed the crossing objective at the wrong point");
    }
    Require(pupil.GetQuestStatus(Shu) == QUEST_STATUS_INCOMPLETE, "Crossing alone finished Shu without the pearl action");
    std::cout << "PASS all four original rock jumps, canceled motion and credit on the actual shore landing" << std::endl;
    MoveGilneasPlayer(pupil, liang.GetPosition());
    UseReflectionPearl(pupil);
    Require(!pupil.GetQuestObjectiveData(Shu, CoaxCredit) && pupil.GetItemCount(LiangPearl) == 1,
        "The pearl was usable from outside the Pool of Reflection after crossing");
    MoveGilneasPlayer(pupil, Position{1112.0f, 2865.0f, 92.2f});
    Require(pupil.GetAreaId() == 5862, "The native pool point was outside the Pool of Reflection");
    UseReflectionPearl(pupil);
    Require(pupil.GetQuestObjectiveData(Shu, CoaxCredit) == 1 && pupil.GetQuestStatus(Shu) == QUEST_STATUS_COMPLETE,
        "The genuine pearl use did not finish Shu at the pool");
    WorldPackets::Quest::QuestGiverChooseReward turnIn{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    turnIn.QuestGiverGUID = pupil.GetGUID();
    turnIn.QuestID = Shu;
    pupil.GetSession()->HandleQuestGiverChooseReward(turnIn);
    Require(pupil.IsQuestRewarded(Shu), "The native remote turn-in did not reward Shu's quest");
    std::cout << "PASS no remote pearl credit, one real item use at Reflection and native remote turn-in" << std::endl;
    CheckNewFriend(map, pupil);
#endif
}
#endif
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(PoolCrashTrace);
    try
    {
        InitializeNative(argc, argv);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Singing Pools check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadPools();
        CheckBell();
#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE)
        CheckReeds();
#endif
#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE) && !defined(SINGING_POOLS_REEDS_BASELINE)
        CheckBalancedRock();
#endif
#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE) && !defined(SINGING_POOLS_REEDS_BASELINE) && !defined(SINGING_POOLS_ROCK_BASELINE)
        CheckSunPearl();
#endif
#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE) && !defined(SINGING_POOLS_REEDS_BASELINE) && !defined(SINGING_POOLS_ROCK_BASELINE) && !defined(SINGING_POOLS_PEARL_BASELINE)
        CheckShuAdmission();
#endif
#if !defined(SINGING_POOLS_BASELINE) && !defined(SINGING_POOLS_WATER_BASELINE) && !defined(SINGING_POOLS_REEDS_BASELINE) && !defined(SINGING_POOLS_ROCK_BASELINE) && !defined(SINGING_POOLS_PEARL_BASELINE) && !defined(SINGING_POOLS_SHU_ADMISSION_BASELINE)
        CheckReflectionCrossing();
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
