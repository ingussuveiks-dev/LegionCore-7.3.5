#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GameObjectPackets.h"
#include "GilneasQuestNative.h"
#include "MoveSpline.h"
#include "MovementPackets.h"
#include "SmartScriptMgr.h"
#include "SpellPackets.h"
#include "Vehicle.h"

void AddSC_darkshore();

namespace
{
constexpr uint32 GreymistQuest = 13560;
constexpr uint32 DecoyEntry = 34207;
constexpr uint32 Counter = 32852;

Quest const* LoadGreymist()
{
    auto row = WorldDatabase.Query(
#include "NativeGreymist.inc"
    );
    Require(row && row->GetRowCount() == 1, "Missing An Ocean Not So Deep");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[GreymistQuest] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(GreymistQuest, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13560");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Counter && objective->Fetch()[5].GetInt32() == 50, "Missing the native 50-murloc objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(121889,121992)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject AS SELECT * FROM legion_world.gameobject WHERE guid=37921");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid IN(34207,3420700)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM legion_world.spell_target_position WHERE id=64923");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(62682,62684)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_linked_spell AS SELECT * FROM legion_world.spell_linked_spell WHERE spell_trigger IN(-62684,62682)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadGameobjects();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sSpellMgr->LoadSpellTargetPositions();
    sSpellMgr->LoadSpellLinked();
    if (WorldDatabase.Query("SELECT spell_id FROM spell_script_names WHERE spell_id=62682"))
    {
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_darkshore();
        sScriptMgr->SwapScriptContext(true);
        sScriptDataStore->LoadSpellScriptNames();
        sScriptDataStore->ValidateSpellScripts();
    }
    return quest;
}

struct GreymistConsole : GameObject
{
    explicit GreymistConsole(::Map& map)
    {
        auto spawn = sObjectMgr->GetGOData(37921);
        Require(spawn && spawn->id == 195006, "The original console placement is missing");
        sObjectMgr->RemoveGameobjectFromGrid(37921, spawn);
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadGameObjectFromDB(37921, &map, true), "Could not publish the original console");
        Update(100);
        Require(GetGoType() == GAMEOBJECT_TYPE_GOOBER && GetGOInfo()->goober.spell == 64924 &&
            GetGOInfo()->goober.questID == GreymistQuest, "Native console spell or quest admission changed");
    }

    ~GreymistConsole()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

struct GreymistVictim : Creature
{
    GreymistVictim(::Map& map, uint32 guid) : Creature(false)
    {
        auto spawn = sObjectMgr->GetCreatureData(guid);
        Require(spawn, "Missing original Greymist placement");
        map.LoadGrid(spawn->posX, spawn->posY);
        Require(LoadCreatureFromDB(guid, &map, true) && IsInWorld(), "Original Greymist creature did not load");
        SetReactState(REACT_PASSIVE);
    }

    ~GreymistVictim()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

void UseGreymistConsole(LootPlayer& player, GreymistConsole& console)
{
    WorldPackets::GameObject::GameObjectUse packet{WorldPacket(CMSG_GAME_OBJ_USE)};
    packet.Guid = console.GetGUID();
    player.GetSession()->HandleGameObjectUse(packet);
}

TempSummon* GreymistDecoy(LootPlayer& player)
{
    auto summons = player.GetSummonList(DecoyEntry);
    Require(summons->size() <= 1, "The console published duplicate decoys");
    auto decoy = summons->empty() ? nullptr : player.GetMap()->GetCreature(summons->front());
    return decoy ? decoy->ToTempSummon() : nullptr;
}

void GreymistTick(::Map& map, LootPlayer& driver, TempSummon* decoy, uint32 duration)
{
    auto guid = decoy ? decoy->GetGUID() : ObjectGuid::Empty;
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 50)
    {
        Tick(driver, 50);
        if (auto actor = map.GetCreature(guid))
            actor->Update(50);
        map.MoveAllCreaturesInMoveList();
        map.RemoveAllObjectsInRemoveList();
    }
}

void CheckGreymist(Quest const* quest)
{
    for (uint32 guid : {121889, 121992})
        if (auto spawn = sObjectMgr->GetCreatureData(guid))
            sObjectMgr->RemoveCreatureFromGrid(guid, spawn);
    ::Map map(1, 300, 0, DIFFICULTY_NONE);
    GreymistConsole console(map);
    auto start = console.GetNearPosition(1.0f, 1.0f);
    auto session = [] { return new WorldSession(0, "Greymist native probe", {}, SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {}); };
    LootPlayer driver(session(), map, 1356001, start), member(session(), map, 1356002, start), visitor(session(), map, 1356003, start);
    for (auto player : {&driver, &member, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_NIGHTELF);
        player->InitDisplayIds();
        player->SetLevel(15);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }

    UseGreymistConsole(visitor, console);
    Require(!GreymistDecoy(visitor), "A player without the quest spawned a decoy");
    driver.AddQuestAndCheckCompletion(quest, nullptr);
    UseGreymistConsole(driver, console);
    Tick(driver, 200);
    auto decoy = GreymistDecoy(driver);
    Require(decoy, "Original console did not cast native 64924/64923");
    GreymistTick(map, driver, decoy, 300);

    float x = decoy->GetPositionX(), y = decoy->GetPositionY(), z = decoy->GetPositionZ();
    if (!map.IsInWater(x, y, z) || z >= -10.0f)
    {
        auto dbDestination = WorldDatabase.Query("SELECT id FROM spell_target_position WHERE id=64923");
        Require(!dbDestination, "The repaired destination failed the native underwater check");
        AbandonSupply(driver, GreymistQuest);
        GreymistTick(map, driver, decoy, 300);
        std::cout << "EXPECTED_BROKEN console leaves decoy at the surface without the native DB destination\n";
        return;
    }

    float floor = map.GetHeight(x, y, z + 3.0f);
    Require(floor >= z - 4.0f && floor <= z + 1.0f && driver.GetVehicleBase() == decoy &&
        decoy->GetVehicleKit()->GetPassenger(0) == &driver && decoy->HasQuestObjectScope() && decoy->HasSpell(62684),
        "Underwater decoy lacks a supported floor, real seat, accepted attempt or native ability");
    UseGreymistConsole(driver, console);
    Require(GreymistDecoy(driver) == decoy, "Repeat use replaced a live decoy");

    GreymistVictim tidehunter(map, 121889), warrior(map, 121992);
    map.CreatureRelocation(&warrior, x, y - 4, z, 0);
    map.MoveAllCreaturesInMoveList();
    Require(warrior.IsWithinDistInMap(decoy, 15.0f) && tidehunter.IsWithinDistInMap(decoy, 15.0f), "Expected two real victims in the blast radius");
    // A controlled survivor distinguishes real death credit from the old SPELLHIT behavior.
    warrior.SetMaxHealth(100000);
    warrior.SetHealth(100000);
    warrior.SetModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT, 100.0f);
    member.AddQuestAndCheckCompletion(quest, nullptr);
    visitor.AddQuestAndCheckCompletion(quest, nullptr);
    MoveGilneasPlayer(member, decoy->GetPosition());
    MoveGilneasPlayer(visitor, decoy->GetPosition());
    Group group;
    group.*Access(GroupGuidTag{}) = ObjectGuid::Create<HighGuid::Party>(13560, 0);
    for (auto player : {&driver, &member})
    {
        Group::MemberSlot slot;
        slot.Guid = player->GetGUID();
        slot.Class = player->getClass();
        const_cast<Group::MemberSlotList&>(group.GetMemberSlots()).push_back(slot);
        player->SetGroup(&group, 0);
    }
    sGroupMgr->AddGroup(&group);
    Require(driver.GetGroup() == &group && member.IsAtGroupRewardDistance(&driver),
        "The group fixture requires a nearby eligible member");

    Require(decoy->CastSpell(decoy, 62684, false) == SPELL_CAST_OK, "The real driver could not arm the native countdown");
    GreymistTick(map, driver, decoy, 200);
    Require(decoy->HasAura(62684) && decoy->CastSpell(decoy, 62684, false) != SPELL_CAST_OK,
        "A duplicate cast restarted the active countdown");
    GreymistTick(map, driver, decoy, 7000);
    Require(tidehunter.IsAlive() && warrior.GetHealth() == 100000 && !driver.GetQuestObjectiveData(GreymistQuest, Counter),
        "Detonation or credit fired before expiration");
    auto decoyGuid = decoy->GetGUID();
    GreymistTick(map, driver, decoy, 2200);
    bool ejected = !driver.GetVehicle() && driver.GetUnitBeingMoved() == &driver && driver.IsBeingTeleportedNear();
    Require(ejected, "Natural detonation did not return controls and transfer the driver from the seabed");
    WorldPackets::Movement::MoveTeleportAck ack{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};
    ack.MoverGUID = driver.GetGUID();
    driver.GetSession()->HandleMoveTeleportAck(ack);
    Require(!driver.IsBeingTeleported() && driver.IsWithinDistInMap(&console, INTERACTION_DISTANCE) &&
        driver.IsWithinLOSInMap(&console), "The return acknowledgement did not land at the original console");
    if (auto remaining = map.GetCreature(decoyGuid))
        GreymistTick(map, driver, remaining->ToTempSummon(), 6000);
    auto driverCount = driver.GetQuestObjectiveData(GreymistQuest, Counter);
    auto groupCount = member.GetQuestObjectiveData(GreymistQuest, Counter);
    auto visitorCount = visitor.GetQuestObjectiveData(GreymistQuest, Counter);
    driver.SetGroup(nullptr);
    member.SetGroup(nullptr);
    sGroupMgr->RemoveGroup(&group);

    std::cout << "Greymist detonation diagnostics: tidehunter alive=" << tidehunter.IsAlive() << " warrior health=" << warrior.GetHealth()
        << " driver=" << driverCount << " group=" << groupCount << " visitor=" << visitorCount
        << " ejected at detonation=" << ejected << " vehicle remains=" << (map.GetCreature(decoyGuid) != nullptr) << '\n';
    Require(!tidehunter.IsAlive() && warrior.GetHealth() > 0 && warrior.GetHealth() < 100000 &&
        driverCount == 1 && groupCount == 1 && !visitorCount && !driver.GetVehicle() && !map.GetCreature(decoyGuid),
        "Expected one lethal hit, one survivor, native group credit, ejection and cleanup");

    console.ResetDoorOrButton();
    console.Update(50);
    UseGreymistConsole(driver, console);
    Tick(driver, 200);
    auto retry = GreymistDecoy(driver);
    Require(retry && retry->GetGUID() != decoyGuid, "Owner could not obtain a new decoy");
    GreymistTick(map, driver, retry, 300);
    Require(retry->CastSpell(retry, 62684, false) == SPELL_CAST_OK, "Could not arm the retry");
    GreymistTick(map, driver, retry, 200);
    auto retryGuid = retry->GetGUID();
    auto survivorHealth = warrior.GetHealth();
    AbandonSupply(driver, GreymistQuest);
    GreymistTick(map, driver, retry, 9000);
    Require(!map.GetCreature(retryGuid) && !driver.GetVehicle() && driver.GetUnitBeingMoved() == &driver &&
        warrior.GetHealth() == survivorHealth, "Abandonment retained the vehicle or detonated its canceled aura");

    MoveGilneasPlayer(driver, start);
    driver.AddQuestAndCheckCompletion(quest, nullptr);
    driver.SetQuestObjectiveData(quest, &quest->GetObjectives().front(), 49);
    warrior.SetHealth(1);
    console.ResetDoorOrButton();
    console.Update(50);
    UseGreymistConsole(driver, console);
    Tick(driver, 200);
    auto finalDecoy = GreymistDecoy(driver);
    Require(finalDecoy && finalDecoy->GetGUID() != retryGuid, "Reaccepted quest could not use the console");
    GreymistTick(map, driver, finalDecoy, 300);
    Require(finalDecoy->CastSpell(finalDecoy, 62684, false) == SPELL_CAST_OK, "Could not arm the final charge");
    GreymistTick(map, driver, finalDecoy, 15000);
    Require(!warrior.IsAlive() && driver.GetQuestObjectiveData(GreymistQuest, Counter) == 50 &&
        driver.GetQuestStatus(GreymistQuest) == QUEST_STATUS_COMPLETE && driver.CanRewardQuest(quest, false) &&
        driver.IsBeingTeleportedNear() && !driver.GetVehicle() && !GreymistDecoy(driver),
        "The last genuine kill did not complete the quest and return the driver");
    driver.GetSession()->HandleMoveTeleportAck(ack);
    Require(!driver.IsBeingTeleported() && driver.IsWithinDistInMap(&console, INTERACTION_DISTANCE),
        "The completed quest lost the final return");
    UseGreymistConsole(driver, console);
    Require(!GreymistDecoy(driver), "The completed quest can start another ride");
    std::cout << "PASS Greymist original console, seabed, boarding, countdown, both lethal victims, a survivor, group credit, cancellation, retry and 50/50 return\n";
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
        LoadGameObjectModelList(argv[2]);
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "Expected a disposable realm");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        auto quest = LoadGreymist();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckGreymist(quest);
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
