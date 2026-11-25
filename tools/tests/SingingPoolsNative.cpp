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
constexpr uint32 NextQuest = 29676;
constexpr uint32 BellCredit = 55077;
constexpr uint32 BellEntry = 209608;
constexpr uint32 BellSpawn = 102318;
constexpr uint32 AysaSpawn = 244270;
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
    Require(rows && rows->GetRowCount() == 3, "Missing the Singing Pools quests");
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
        "WHERE ID IN (29521,29661,29676)");
    Require(rowsAddon && rowsAddon->GetRowCount() == 3, "Missing the original Pools prerequisites");
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

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN (244137,244270)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN (29521,29661,29676)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN (29521,29661,29676)");
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
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sSpellMgr->LoadSpellProcEvents();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sAreaTriggerDataStore->LoadAreaTriggerScripts();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_WanderingIsland();
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

void CheckBell()
{
    for (uint32 spawn : {AysaSpawn, 244137u})
        if (auto data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    Map map(860, 663, 0, DIFFICULTY_NONE);
    Teacher aysa(map);
    Bell bell(map);
    LootPlayer pupil(SupplySession(), map, 2966101, aysa.GetPosition());
    pupil.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    pupil.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    pupil.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    pupil.InitDisplayIds();
    pupil.GetReputationMgr().LoadFromDB({});
    pupil.SetLevel(10);
    pupil.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        pupil.SetFloatValue(field, 1.0f);
    pupil.SetRewardedQuest(29423);
    pupil.AddQuestAndCheckCompletion(sQuestDataStore->GetQuestTemplate(Introduction), nullptr);
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
