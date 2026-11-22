#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "ScriptedEscortAI.h"
#include "ScriptSystem.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Passing = 29787;
constexpr uint32 ShangXi = 56159;
constexpr uint32 Guardian = 56274;
constexpr uint32 GateEntry = 209922;
constexpr uint32 SummonEscort = 105333;

Quest const* LoadPassing()
{
    auto row = WorldDatabase.Query(
#include "NativeShangXiEscort.inc"
    );
    Require(bool(row), "Missing Worthy of Passing template");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Passing] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Passing, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29787");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Guardian && objective->Fetch()[5].GetUInt32() == 1,
        "Missing real Guardian of the Elders objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(bool(WorldDatabase.Query("SELECT 1 FROM spell_area WHERE spell=105333 AND quest_start=29787 "
        "AND area=5829 AND quest_start_status=8 AND quest_end=29787 AND quest_end_status=8 AND autocast=1")),
        "Native escort activation requires the actual accepted quest");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_questender q ON q.id=c.id "
        "WHERE q.quest=29787 AND c.id=55672 AND c.map=860")), "The original route receiver is unavailable");
    auto summon = sSpellMgr->GetSpellInfo(SummonEscort);
    Require(summon && summon->GetEffect(EFFECT_0)->MiscValue == ShangXi && summon->GetEffect(EFFECT_0)->MiscValueB == 3276,
        "The native Shang Xi spell was replaced");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE passing_spell_area AS SELECT * FROM spell_area WHERE spell=105333");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM passing_spell_area");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sScriptSystemMgr->LoadScriptWaypoints();
    sCreatureTextMgr->LoadCreatureTexts();
    return quest;
}

struct PassingPet : LootPet
{
    PassingPet(LootPlayer& owner, Map& map) : LootPet(owner, map) { m_ControlledByPlayer = true; }
};

struct PassingPlayer : LootPlayer
{
    PassingPlayer(Map& map, uint64 guid, Position const& point) : LootPlayer(SupplySession(), map, guid, point)
    {
        map.GetZoneAndAreaId(m_zoneId, m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
        Require(m_areaId == 5829, "Native spell area is outside the escort cave");
    }
};

struct PassingGate : GameObject
{
    ~PassingGate()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<GameObject*>(this), false);
        ResetMap();
    }
};

Creature* EscortFor(LootPlayer& owner)
{
    auto creatures = owner.GetSummonList(ShangXi);
    Require(creatures->size() <= 1, "Duplicate native Shang Xi escort");
    return creatures->empty() ? nullptr : owner.GetMap()->GetCreature(creatures->front());
}

Creature* GuardianFor(Creature& escort)
{
    std::list<Creature*> guardians;
    escort.GetCreatureListWithEntryInGrid(guardians, Guardian, 100.0f);
    for (auto guardian : guardians)
        if (guardian->IsAlive() && guardian->GetQuestObjectScope().lock() == escort.GetQuestObjectScope().lock())
            return guardian;
    return nullptr;
}

void MoveEscort(Creature& escort, Player& player, Position const& point)
{
    auto map = escort.GetMap();
    map->LoadGrid(point.GetPositionX(), point.GetPositionY());
    SupplyMove(player, point);
    map->CreatureRelocation(&escort, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation());
    map->MoveAllCreaturesInMoveList();
}

void CheckPassing(Quest const* quest)
{
    auto const& path = sScriptSystemMgr->GetPointMoveList(ShangXi);
    Require(path.size() == 12 && path[7].uiPointId == 8 && path[10].uiPointId == 11, "Native guardian and pause waypoints changed");
    auto point = [&](size_t index)
    {
        auto const& waypoint = path[index];
        return Position{waypoint.fX, waypoint.fY, waypoint.fZ, 0.0f};
    };
    auto start = point(0);
    Map map(860, 300, 0, DIFFICULTY_NONE);
    PassingPlayer owner(map, 29787, start), other(map, 29788, start);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
        player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
        player->SetLevel(9);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 15000);
    }
    owner.CastSpell(&owner, SummonEscort, true);
    Require(!EscortFor(owner), "An unaccepted quest summoned Master Shang Xi");
    for (auto player : {&owner, &other})
    {
        player->AddQuestAndCheckCompletion(quest, nullptr);
        Require(player->CastSpell(player, SummonEscort, false) == SPELL_CAST_OK, "The active native spell area refused the escort");
        Tick(*player, 100);
        player->CastSpell(player, SummonEscort, true);
    }
    auto first = EscortFor(owner), second = EscortFor(other);
    Require(first && second && first != second && first->HasQuestObjectScope() && second->HasQuestObjectScope() &&
        first->IsInPersonnalVisibilityList(owner.GetGUID()) && !first->IsInPersonnalVisibilityList(other.GetGUID()),
        "Native spell did not publish two personal escorts");
    auto firstEscort = dynamic_cast<npc_escortAI*>(first->AI());
    auto secondEscort = dynamic_cast<npc_escortAI*>(second->AI());
    Require(firstEscort && secondEscort && firstEscort->HasEscortState(STATE_ESCORT_ESCORTING) &&
        secondEscort->HasEscortState(STATE_ESCORT_ESCORTING), "Native waypoint escort did not start");

    auto gateRow = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id=209922 AND map=860");
    Require(gateRow && gateRow->GetRowCount() == 1, "Missing the original shared gate");
    auto gateData = gateRow->Fetch();
    Position gatePoint{gateData[1].GetFloat(),gateData[2].GetFloat(),gateData[3].GetFloat(),gateData[4].GetFloat()};
    PassingGate gate;
    Require(gate.Create(gateData[0].GetUInt32(), GateEntry, &map, 1, gatePoint,
        G3D::Quat(gateData[5].GetFloat(),gateData[6].GetFloat(),gateData[7].GetFloat(),gateData[8].GetFloat()), 255, GO_STATE_READY) &&
        map.AddToMap(static_cast<GameObject*>(&gate)), "Could not publish the original gate");
    MoveEscort(*first, owner, point(7));
    MoveEscort(*second, other, point(7));
    firstEscort->WaypointReached(8);
    secondEscort->WaypointReached(8);
    auto firstGuardian = GuardianFor(*first), secondGuardian = GuardianFor(*second);
    Require(firstGuardian && secondGuardian && firstGuardian != secondGuardian && gate.GetGoState() == GO_STATE_READY &&
        firstGuardian->IsInPersonnalVisibilityList(owner.GetGUID()) && !firstGuardian->IsInPersonnalVisibilityList(other.GetGUID()) &&
        firstGuardian->CanTargetQuestObject(&owner) && !firstGuardian->CanTargetQuestObject(&other),
        "The original challenge did not publish private guardians beside the closed gate");

    MoveEscort(*second, other, point(10));
    secondEscort->WaypointReached(11);
    Require(secondEscort->HasEscortState(STATE_ESCORT_PAUSED), "Escort did not wait for the undefeated guardian");
    auto previous = secondGuardian->GetGUID();
    secondGuardian->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
    Require(!other.GetQuestObjectiveData(Passing, Guardian) && gate.GetGoState() == GO_STATE_READY,
        "An alive guardian's disappearance completed the challenge");
    secondEscort->UpdateAI(1000);
    secondGuardian = GuardianFor(*second);
    Require(secondGuardian && secondGuardian->GetGUID() != previous, "The live attempt did not recover a lost guardian");

    MoveEscort(*first, owner, point(10));
    PassingPet pet(owner, map);
    pet.SetLevel(9);
    pet.DealDamage(firstGuardian, uint32(firstGuardian->GetMaxHealth()) * 2);
    Tick(owner, 100);
    firstEscort->WaypointReached(11);
    Require(firstGuardian->isDead(false) && owner.GetQuestObjectiveData(Passing, Guardian) == 1 &&
        owner.CanRewardQuest(quest, false) && gate.GetGoState() == GO_STATE_ACTIVE &&
        !firstEscort->HasEscortState(STATE_ESCORT_PAUSED) && !other.GetQuestObjectiveData(Passing, Guardian),
        "The real pet kill failed native credit or left its escort paused after an early death");
    auto cancelledEscort = second->GetGUID(), cancelledGuardian = secondGuardian->GetGUID();
    AbandonSupply(other, Passing);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(cancelledEscort) && !map.GetCreature(cancelledGuardian) && owner.CanRewardQuest(quest, false),
        "Abandon did not remove only the current escort and guardian");
    AbandonSupply(owner, Passing);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS native Shang Xi spell, two private guardians, live despawn/retry, real pet death, gate and early-kill continuation\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
        LoadGameObjectModelList(argv[2]);
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_WanderingIsland();
        sScriptMgr->SwapScriptContext(true);
        auto quest = LoadPassing();
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPassing(quest);
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
