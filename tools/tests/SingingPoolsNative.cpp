#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "AreaTriggerData.h"
#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"
#include "MiscPackets.h"
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
    Require(rows && rows->GetRowCount() == 6, "Missing the Singing Pools quests");
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
        "WHERE ID IN (29423,29521,29661,29662,29663,29676)");
    Require(rowsAddon && rowsAddon->GetRowCount() == 6, "Missing the original Pools prerequisites");
    do
    {
        auto row = rowsAddon->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
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

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN (243552,244137,244247,244270,244325,244326,244327,244436)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (29521,29661,29662,29663,29676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (29521,29661,29662,29663,29676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE areatrigger_scripts AS SELECT * FROM legion_world.areatrigger_scripts "
        "WHERE entry IN (6986,6987,6988,6990,6991,7012)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells "
        "WHERE npc_entry=54993");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_proc_event AS SELECT * FROM legion_world.spell_proc_event "
        "WHERE entry IN (46598,102938,102939,102940,102941,102942)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& table = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto const original = table;
    table += " WHERE Entry=209507";
    auto const loaded = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    table = original;
    Require(loaded == 1 && LootTemplates_Gameobject.GetLootFor(209507), "Missing the original reed loot row");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSpellMgr->LoadSpellProcEvents();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sAreaTriggerDataStore->LoadAreaTriggerScripts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_WanderingIsland();
    AddSC_go_scripts();
    sScriptMgr->SwapScriptContext(true);
}

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
    for (uint32 spawn : {ShangTempleSpawn, AysaSpawn, ReedGiverSpawn, 244137u})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    for (uint32 spawn : MonkSpawns)
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
    auto quest = sQuestDataStore->GetQuestTemplate(DryFur);
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (pupil.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    Require(choice, "No suitable native belt reward");
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
