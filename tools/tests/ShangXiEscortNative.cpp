#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "CreatureTextMgr.h"
#include "ConditionMgr.h"
#include "GilneasQuestNative.h"
#include "MoveSpline.h"
#include "ScriptedEscortAI.h"
#include "ScriptSystem.h"
#include "SpellPackets.h"
#include "Vehicle.h"

void AddSC_WanderingIsland();

namespace
{
constexpr uint32 Passing = 29787;
constexpr uint32 ShangXi = 56159;
constexpr uint32 Guardian = 56274;
constexpr uint32 GateEntry = 209922;
constexpr uint32 SummonEscort = 105333;
constexpr uint32 TempleReceiver = 55672;
constexpr uint32 TempleReceiverSpawn = 245086;
constexpr uint32 UnwelcomeNature = 29788;
constexpr uint32 SmallButSignificant = 29789;
constexpr uint32 Wisdom = 29790;
constexpr uint32 Suffering = 29791;
constexpr uint32 WisdomSpell = 106623;
constexpr uint32 WisdomMaster = 56686;
constexpr uint32 WisdomCredit = 56688;
constexpr uint32 Scamp = 55640;
constexpr uint32 RitualCharm = 74634;
constexpr uint32 BalloonVehicle = 55649;
constexpr uint32 BalloonStand = 55918;
constexpr uint32 BalloonStandSpawn = 245079;
constexpr uint32 BalloonRideCredit = 56378;
constexpr uint32 TurtleStoryCredit = 55939;

Quest const* LoadPassing()
{
    auto row = WorldDatabase.Query(
#include "NativeShangXiEscort.inc"
    );
    Require(row && row->GetRowCount() == 5, "Missing the original temple quest templates");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(row->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (row->NextRow());
    auto quest = quests[Passing];
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID BETWEEN 29787 AND 29791");
    Require(addon && addon->GetRowCount() == 5, "Missing the original temple prerequisites");
    do
    {
        auto fields = addon->Fetch();
        auto current = quests[fields[0].GetUInt32()];
        current->LoadQuestTemplateAddon(fields);
        if (current->PrevQuestID && quests[std::abs(current->PrevQuestID)])
            current->prevQuests.push_back(current->PrevQuestID);
        if (current->NextQuestID && quests[std::abs(current->NextQuestID)])
        {
            auto& previous = quests[std::abs(current->NextQuestID)]->prevQuests;
            int32 const dependency = current->NextQuestID < 0 ? -int32(current->Id) : int32(current->Id);
            if (std::find(previous.begin(), previous.end(), dependency) == previous.end())
                previous.push_back(dependency);
        }
    } while (addon->NextRow());
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=29787");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Guardian && objective->Fetch()[5].GetUInt32() == 1,
        "Missing real Guardian of the Elders objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    auto wisdomObjective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID BETWEEN 29788 AND 29791 ORDER BY QuestID,StorageIndex");
    Require(wisdomObjective && wisdomObjective->GetRowCount() == 5, "Missing the original temple preparation and staff planting objectives");
    do
    {
        auto fields = wisdomObjective->Fetch();
        quests[fields[1].GetUInt32()]->LoadQuestObjective(fields);
    } while (wisdomObjective->NextRow());
    quests[Wisdom]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    quests[UnwelcomeNature]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    quests[SmallButSignificant]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);
    quests[Suffering]->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(bool(WorldDatabase.Query("SELECT 1 FROM spell_area WHERE spell=105333 AND quest_start=29787 "
        "AND area=5829 AND quest_start_status=8 AND quest_end=29787 AND quest_end_status=8 AND autocast=1")),
        "Native escort activation requires the actual accepted quest");
    Require(bool(WorldDatabase.Query("SELECT 1 FROM creature c JOIN creature_questender q ON q.id=c.id "
        "WHERE q.quest=29787 AND c.id=55672 AND c.map=860")), "The original route receiver is unavailable");
    auto summon = sSpellMgr->GetSpellInfo(SummonEscort);
    Require(summon && summon->GetEffect(EFFECT_0)->MiscValue == ShangXi && summon->GetEffect(EFFECT_0)->MiscValueB == 3276,
        "The native Shang Xi spell was replaced");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE passing_spell_area AS SELECT * FROM spell_area WHERE spell IN (105333,105001,106623,105002) OR (spell=60922 AND quest_start=29791)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM passing_spell_area");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=860 AND (guid IN (245086,245096,245079,245127,244855,245235) OR id=55640)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest BETWEEN 29787 AND 29791");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest BETWEEN 29787 AND 29791");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry IN (55672,56686,57769)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE (SourceTypeOrReferenceId=19 AND SourceEntry=29790) OR (SourceTypeOrReferenceId=18 AND SourceGroup IN (55918,55649))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN (55918,55649)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura IN (55918,55649)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    auto& lootTable = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto const fullLootTable = lootTable;
    lootTable += " WHERE Entry IN (209671,209774)";
    auto loadedLoot = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    lootTable = fullLootTable;
    Require(loadedLoot == 4 && LootTemplates_Gameobject.GetLootFor(209671) && LootTemplates_Gameobject.GetLootFor(209774),
        "Missing the original ritual charm loot tables and bonus rows");
    auto& scampLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto const fullScampLoot = scampLoot;
    scampLoot += " WHERE Entry=55640";
    (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    scampLoot = fullScampLoot;
    Require(LootTemplates_Creature.GetLootFor(55640), "Missing the original Thornbranch Scamp loot");
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    if (WorldDatabase.Query("SELECT 1 FROM conditions LIMIT 1"))
        sConditionMgr->LoadConditions();
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

void CheckTempleArrival(Quest const* quest)
{
    auto const& path = sScriptSystemMgr->GetPointMoveList(ShangXi);
    auto const& firstPoint = path.front();
    auto const& lastPoint = path.back();
    Position start{firstPoint.fX, firstPoint.fY, firstPoint.fZ};
    Position finish{lastPoint.fX, lastPoint.fY, lastPoint.fZ};
    Map map(860, 301, 0, DIFFICULTY_NONE);
    PassingPlayer owner(map, 2978701, start);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    owner.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    owner.SetLevel(9);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 15000);
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    Require(owner.CastSpell(&owner, SummonEscort, false) == SPELL_CAST_OK, "The cave refused the original escort spell");
    Tick(owner, 100);
    Creature* escort = EscortFor(owner);
    Require(escort, "The normal cave spell did not summon its private escort");
    ObjectGuid const escortGuid = escort->GetGUID();
    bool guardianDefeated = false;
    bool reachedTemple = false;
    for (uint32 elapsed = 0; elapsed < 180000 && (escort = map.GetCreature(escortGuid)); elapsed += 100)
    {
        SupplyMove(owner, escort->GetPosition());
        escort->Unit::Update(100);
        escort->AI()->UpdateAI(100);
        Tick(owner, 100);
        map.MoveAllCreaturesInMoveList();
        if (!guardianDefeated)
            if (Creature* guardian = GuardianFor(*escort); guardian && owner.IsWithinDistInMap(guardian, 25.0f))
            {
                owner.DealDamage(guardian, uint32(guardian->GetMaxHealth()) * 2);
                guardianDefeated = !guardian->IsAlive();
            }
        reachedTemple |= escort->GetExactDist(finish) < 1.0f && escort->movespline->Finalized();
        map.RemoveAllObjectsInRemoveList();
    }
    Require(guardianDefeated && reachedTemple && !map.GetCreature(escortGuid), "The native escort and guardian did not reach the temple end of the route");
    SupplyMove(owner, finish);
    Creature* receiver = owner.FindNearestCreature(TempleReceiver, 10.0f, true);
    Require(receiver && receiver->GetDBTableGUIDLow() == TempleReceiverSpawn,
        "The actual temple receiver was unavailable");
    SupplyMove(owner, receiver->GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward turnIn{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    turnIn.QuestGiverGUID = receiver->GetGUID();
    turnIn.QuestID = quest->Id;
    owner.GetSession()->HandleQuestGiverChooseReward(turnIn);
    Require(owner.IsQuestRewarded(Passing), "The actual Shang Xi at the temple refused the completed quest");
    std::cout << "PASS actual Shang Xi movement, actual guardian death, original last waypoint, cleanup and temple turn-in\n";
}

void CompleteTemplePreparations(LootPlayer& owner, Creature& giver)
{
    Map& map = *owner.GetMap();
    auto acceptQuest = [&](uint32 quest)
    {
        MoveGilneasPlayer(owner, giver.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        accept.QuestGiverGUID = giver.GetGUID();
        accept.QuestID = quest;
        owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
        Require(owner.GetQuestStatus(quest) == QUEST_STATUS_INCOMPLETE, "The temple did not offer the original preparation quest");
    };
    auto rewardQuest = [&](uint32 quest)
    {
        MoveGilneasPlayer(owner, giver.GetPosition());
        WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
        reward.QuestGiverGUID = giver.GetGUID();
        reward.QuestID = quest;
        owner.GetSession()->HandleQuestGiverChooseReward(reward);
        Require(owner.IsQuestRewarded(quest), "The temple did not reward the genuine preparation objectives");
    };

    acceptQuest(UnwelcomeNature);
    auto scamps = WorldDatabase.Query("SELECT guid FROM creature WHERE id=55640 AND map=860 ORDER BY guid");
    Require(scamps && scamps->GetRowCount() == 64, "Missing the 64 original Thornbranch Scamps");
    PassingPet pet(owner, map);
    pet.setFaction(owner.getFaction());
    pet.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    for (uint32 combat = 0; combat < 8; ++combat)
    {
        auto spawn = scamps->Fetch()[0].GetUInt32();
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Missing an original Thornbranch Scamp during the encounter");
        map.LoadGrid(data->posX, data->posY);
        auto scamp = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(860, Scamp, spawn));
        Require(scamp && scamp->IsAlive(), "The original Thornbranch Scamp did not load alive");
        MoveGilneasPlayer(owner, scamp->GetPosition());
        map.CreatureRelocation(&pet, scamp->GetPositionX(), scamp->GetPositionY(), scamp->GetPositionZ(), scamp->GetOrientation());
        map.MoveAllCreaturesInMoveList();
        Unit& attacker = combat % 2 ? static_cast<Unit&>(pet) : static_cast<Unit&>(owner);
        Require(attacker.IsValidAttackTarget(scamp), "The original Thornbranch Scamp did not allow combat");
        attacker.DealDamage(scamp, uint32(scamp->GetMaxHealth()) * 2);
        Tick(owner, 150);
        Require(!scamp->IsAlive() && owner.GetQuestObjectiveData(UnwelcomeNature, Scamp) == int32(combat + 1),
            "A real Scamp defeat did not award its player or pet owner's progress");
        scamps->NextRow();
    }
    rewardQuest(UnwelcomeNature);
    Require(!owner.CanTakeQuest(sQuestDataStore->GetQuestTemplate(Wisdom), false), "A single rewarded preparation unlocked Passing Wisdom");

    acceptQuest(SmallButSignificant);
    auto charms = WorldDatabase.Query("SELECT id,guid,spawntimesecs,position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3 "
        "FROM gameobject WHERE id IN (209671,209774) AND map=860 ORDER BY id,guid");
    Require(charms && charms->GetRowCount() == 39, "Missing the 39 original ritual charm placements");
    std::vector<Placement> placements;
    do
    {
        auto row = charms->Fetch();
        placements.push_back({row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    } while (charms->NextRow());
    for (uint32 index = 0; index < 9; ++index)
    {
        auto const& location = placements[index];
        Supply charm(map, location);
        MoveGilneasPlayer(owner, location.position);
        Require(charm.ActivateToQuest(&owner), "The original ritual charm did not mark for its active objective");
        OpenSupply(owner, charm);
        Require(TakeSupply(owner, charm, RitualCharm), "An original charm did not yield its required loot");
        ReleaseSupply(owner, charm);
        if (!index)
        {
            AbandonSupply(owner, SmallButSignificant);
            Require(!owner.GetItemCount(RitualCharm), "Abandoning the preparation retained the gathered ritual charm");
            acceptQuest(SmallButSignificant);
        }
    }
    Require(owner.GetItemCount(RitualCharm) == 8, "The eight actual charms were not carried for turn-in");
    rewardQuest(SmallButSignificant);
    Require(!owner.GetItemCount(RitualCharm), "The preparation reward did not consume its charms");
    std::cout << "PASS eight actual Scamp defeats with player and pet, 39 charm placements, eight real charms and both temple rewards\n";
}

void CheckPassingWisdom()
{
    Map map(860, 302, 0, DIFFICULTY_NONE);
    CreatureData const* spawn = sObjectMgr->GetCreatureData(TempleReceiverSpawn);
    Require(spawn, "Missing the original Passing Wisdom giver");
    Position const temple{spawn->posX, spawn->posY, spawn->posZ, spawn->orientation};
    map.LoadGrid(spawn->posX, spawn->posY);
    LootPlayer owner(SupplySession(), map, 2979001, temple);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    owner.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    owner.SetLevel(10);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 15000);
    owner.SetRewardedQuest(Passing);
    auto wisdomQuest = sQuestDataStore->GetQuestTemplate(Wisdom);
#if defined(PASSING_WISDOM_BASELINE) || defined(PASSING_WISDOM_ORPHAN_BASELINE)
    owner.SetRewardedQuest(UnwelcomeNature);
#ifdef PASSING_WISDOM_ORPHAN_BASELINE
    Require(owner.CanTakeQuest(wisdomQuest, false), "One-preparation admission no longer reproduces");
#endif
    owner.SetRewardedQuest(SmallButSignificant);
#endif
    MoveGilneasPlayer(owner, temple);
    Creature* giver = owner.FindNearestCreature(TempleReceiver, 10.0f, true);
    Require(owner.GetAreaId() == 5832 && giver && giver->GetDBTableGUIDLow() == TempleReceiverSpawn, "Missing the actual Passing Wisdom area and giver");
#if !defined(PASSING_WISDOM_BASELINE) && !defined(PASSING_WISDOM_ORPHAN_BASELINE)
    CompleteTemplePreparations(owner, *giver);
#endif
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = giver->GetGUID();
    accept.QuestID = Wisdom;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Tick(owner, 250);
    owner.UpdateZoneDependentAuras(owner.GetZoneId());
    Tick(owner, 250);
    auto const actors = owner.GetSummonList(WisdomMaster);
    Require(owner.GetQuestStatus(Wisdom) == QUEST_STATUS_INCOMPLETE && !owner.GetQuestObjectiveData(Wisdom, WisdomCredit),
        "The native staff quest did not remain incomplete immediately after acceptance");
#ifdef PASSING_WISDOM_BASELINE
    Require(actors->empty() && !owner.HasAura(WisdomSpell), "The conflicting endpoint masks did not reproduce the absent Master Shang Xi");
    std::cout << "EXPECTED_BROKEN accepted Passing Wisdom never started its original area spell or summoned the story actor\n";
#else
    Require(actors->size() == 1 && owner.HasAura(WisdomSpell), "The original area spell did not publish one Master Shang Xi for the accepted quest");
    ObjectGuid const storyGuid = actors->front();
#ifdef PASSING_WISDOM_ORPHAN_BASELINE
    AbandonSupply(owner, Wisdom);
    Tick(owner, 250);
    map.RemoveAllObjectsInRemoveList();
    Require(map.GetCreature(storyGuid), "The previous version no longer kept an abandoned Master Shang Xi actor");
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Tick(owner, 250);
    owner.UpdateZoneDependentAuras(owner.GetZoneId());
    Tick(owner, 250);
    Require(actors->size() > 1 && owner.GetQuestStatus(Wisdom) == QUEST_STATUS_INCOMPLETE, "The previous actor did not overlap the newly accepted story");
    std::cout << "EXPECTED_BROKEN one prerequisite was enough and abandoning left the old story alongside the new attempt\n";
    return;
#else
    auto original = map.GetCreature(storyGuid);
    Require(original && original->HasQuestObjectScope(), "The original story actor was not bound to the accepted quest");
    AbandonSupply(owner, Wisdom);
    Tick(owner, 250);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(storyGuid) && !owner.GetQuestObjectiveData(Wisdom, WisdomCredit), "An abandoned story retained its actor or gave credit");
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Tick(owner, 250);
    owner.UpdateZoneDependentAuras(owner.GetZoneId());
    Tick(owner, 250);
    Require(actors->size() == 1 && actors->front() != storyGuid, "A new attempt did not receive its own Master Shang Xi");
#endif
    ObjectGuid const activeStory = actors->front();
    bool plantedStaff = false;
    bool reachedPlantingPoint = false;
    for (uint32 elapsed = 0; elapsed < 120000; elapsed += 100)
    {
        auto* shangXi = map.GetCreature(activeStory);
        if (!shangXi)
            break;
        shangXi->Unit::Update(100);
        shangXi->AI()->UpdateAI(100);
        Tick(owner, 100);
        map.MoveAllCreaturesInMoveList();
        reachedPlantingPoint |= shangXi->GetExactDist(Position{874.205f, 4464.75f, 241.3819f}) < 1.0f && shangXi->movespline->Finalized();
        if (Creature* staff = shangXi->FindNearestCreature(57874, 15.0f, true))
            plantedStaff |= staff->GetQuestObjectScope().lock() == shangXi->GetQuestObjectScope().lock();
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Passing Wisdom arrived=" << reachedPlantingPoint << " staff=" << plantedStaff << " quest=" << owner.GetQuestStatus(Wisdom)
        << " credit=" << owner.GetQuestObjectiveData(Wisdom, WisdomCredit) << std::endl;
    Require(reachedPlantingPoint && plantedStaff && owner.GetQuestObjectiveData(Wisdom, WisdomCredit) == 1 && owner.GetQuestStatus(Wisdom) == QUEST_STATUS_COMPLETE,
        "The native staff ceremony did not reach its planting point and grant its objective");
    auto aysaSpawn = sObjectMgr->GetCreatureData(245096);
    Require(aysaSpawn, "Missing the original Passing Wisdom receiver");
    MoveGilneasPlayer(owner, Position{aysaSpawn->posX, aysaSpawn->posY, aysaSpawn->posZ, aysaSpawn->orientation});
    Creature* aysa = owner.FindNearestCreature(56662, 10.0f, true);
    Require(aysa && aysa->GetDBTableGUIDLow() == 245096, "The original Aysa receiver did not load");
    uint32 choice = 0;
    if (auto package = sDB2Manager.GetQuestPackageItems(wisdomQuest->PackageID))
        for (auto item : *package)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    Require(choice, "Passing Wisdom did not offer an original belt reward");
    WorldPackets::Quest::QuestGiverChooseReward turnIn{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    turnIn.QuestGiverGUID = aysa->GetGUID();
    turnIn.QuestID = Wisdom;
    turnIn.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(turnIn);
    Require(owner.IsQuestRewarded(Wisdom) && owner.GetItemCount(choice) == 1, "Aysa did not award the completed ceremony");
    accept.QuestGiverGUID = aysa->GetGUID();
    accept.QuestID = Suffering;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(Suffering) == QUEST_STATUS_INCOMPLETE, "Aysa did not offer Shen-zin Su's journey after the story");
    std::cout << "PASS Passing Wisdom preparations, native restart, staff ceremony, Aysa reward and follow-up admission\n";
#endif
}

void CheckShenZinSu()
{
    Map map(860, 303, 0, DIFFICULTY_NONE);
    auto const* data = sObjectMgr->GetCreatureData(BalloonStandSpawn);
    Require(data, "The original clickable balloon is missing");
    Position const dock{data->posX, data->posY, data->posZ, data->orientation};
    map.LoadGrid(data->posX, data->posY);
    LootPlayer owner(SupplySession(), map, 2979101, dock);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    owner.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    owner.InitDisplayIds();
    owner.SetLevel(10);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 15000);
    owner.SetRewardedQuest(Wisdom);
    MoveGilneasPlayer(owner, dock);
    auto* placedBalloon = owner.FindNearestCreature(BalloonStand, 10.0f, true);
    auto* aysa = owner.FindNearestCreature(56662, 15.0f, true);
    Require(placedBalloon && placedBalloon->GetDBTableGUIDLow() == BalloonStandSpawn && aysa, "Missing the real balloon stand or Aysa");
    WorldPackets::Spells::SpellClick click{WorldPacket(CMSG_SPELL_CLICK)};
    click.SpellClickUnitGuid = placedBalloon->GetGUID();
    owner.GetSession()->HandleSpellClick(click);
    Tick(owner, 500);
    Require(!owner.GetVehicle() && owner.GetSummonList(BalloonVehicle)->empty(), "A balloon appeared before accepting the quest");

    MoveGilneasPlayer(owner, aysa->GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest accept{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    accept.QuestGiverGUID = aysa->GetGUID();
    accept.QuestID = Suffering;
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    Require(owner.GetQuestStatus(Suffering) == QUEST_STATUS_INCOMPLETE && !owner.GetQuestObjectiveData(Suffering, BalloonRideCredit),
        "Aysa did not accept Suffering, or supplied boarding credit before the player boarded");
    Position remote = dock;
    remote.m_positionX += 35.0f;
    MoveGilneasPlayer(owner, remote);
    Require(owner.GetAreaId() == 5832 && !owner.IsWithinDistInMap(placedBalloon, INTERACTION_DISTANCE),
        "The remote balloon test point must remain in the original dock area outside interaction range");
    owner.GetSession()->HandleSpellClick(click);
    Tick(owner, 500);
#ifdef WANDERING_ISLE_BALLOON_BASELINE
    Require(owner.GetVehicle() && owner.GetQuestObjectiveData(Suffering, BalloonRideCredit) == 1,
        "The remote click did not reproduce boarding and its original credit");
    auto oldFlight = owner.GetVehicleBase()->GetGUID();
    AbandonSupply(owner, Suffering);
    Tick(owner, 250);
    map.RemoveAllObjectsInRemoveList();
    Require(map.GetCreature(oldFlight) && owner.GetVehicle(), "Abandoning no longer reproduced the continuing unscoped ride");
    owner.ExitVehicle();
    Tick(owner, 2000);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN the distant dock click boarded a player and abandoning kept the original ride active\n";
    return;
#else
    Require(!owner.GetVehicle() && owner.GetSummonList(BalloonVehicle)->empty() && !owner.GetQuestObjectiveData(Suffering, BalloonRideCredit),
        "A distant click started the original balloon trip");
#endif
    MoveGilneasPlayer(owner, dock);
    owner.GetSession()->HandleSpellClick(click);
    Tick(owner, 500);
    auto list = owner.GetSummonList(BalloonVehicle);
    Creature* balloon = list->empty() ? nullptr : map.GetCreature(list->front());
    std::cout << "CASE Shen-zin Su balloons=" << list->size() << " seated=" << bool(owner.GetVehicle()) << " boarding="
        << owner.GetQuestObjectiveData(Suffering, BalloonRideCredit) << std::endl;
    Require(balloon && list->size() == 1 && owner.GetVehicleBase() == balloon && owner.GetQuestObjectiveData(Suffering, BalloonRideCredit) == 1,
        "The actual balloon click did not seat and credit the owner");

#ifndef WANDERING_ISLE_BALLOON_BASELINE
    ObjectGuid const cancelledFlight = balloon->GetGUID();
    auto aysaList = owner.GetSummonList(56661);
    Require(balloon->HasQuestObjectScope() && aysaList->size() == 1, "The first balloon and Aysa were not bound to the quest");
    ObjectGuid const cancelledAysa = aysaList->front();
    auto firstAysa = map.GetCreature(cancelledAysa);
    Require(firstAysa && firstAysa->GetQuestObjectScope().lock() == balloon->GetQuestObjectScope().lock(),
        "The first balloon and Aysa did not share the accepted attempt");
    AbandonSupply(owner, Suffering);
    map.RemoveAllObjectsInRemoveList();
    Tick(owner, 250);
    Require(!map.GetCreature(cancelledFlight) && !map.GetCreature(cancelledAysa) && !owner.GetVehicle(),
        "Abandoning retained the flight, the passenger, or the owner in a seat");
    MoveGilneasPlayer(owner, aysa->GetPosition());
    owner.GetSession()->HandleQuestGiverAcceptQuest(accept);
    MoveGilneasPlayer(owner, dock);
    owner.GetSession()->HandleSpellClick(click);
    Tick(owner, 500);
    balloon = list->empty() ? nullptr : map.GetCreature(list->front());
    Require(balloon && list->size() == 1 && balloon->GetGUID() != cancelledFlight && owner.GetVehicleBase() == balloon &&
        owner.GetQuestObjectiveData(Suffering, BalloonRideCredit) == 1, "A real second attempt did not start its own balloon and native boarding credit");
    Require(!owner.GetQuestObjectiveData(Suffering, TurtleStoryCredit), "Boarding awarded the story objective before flying the route");

    LootPlayer other(SupplySession(), map, 2979102, dock);
    other.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
    other.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    other.setFactionForRace(RACE_PANDAREN_NEUTRAL);
    other.InitDisplayIds();
    other.SetLevel(10);
    other.SetRewardedQuest(Wisdom);
    MoveGilneasPlayer(other, aysa->GetPosition());
    other.GetSession()->HandleQuestGiverAcceptQuest(accept);
    MoveGilneasPlayer(other, dock);
    other.GetSession()->HandleSpellClick(click);
    Tick(other, 500);
    auto otherFlight = other.GetVehicleBase();
    Require(otherFlight && otherFlight != balloon && owner.GetVehicleBase() == balloon && !otherFlight->CanTargetQuestObject(&owner) &&
        !balloon->CanTargetQuestObject(&other), "Two own attempts unexpectedly shared a balloon or an owner's interaction");
    auto otherFlightGuid = otherFlight->GetGUID();
    AbandonSupply(other, Suffering);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(otherFlightGuid) && owner.GetVehicleBase() == balloon, "Canceling the nearby test flight affected the real rider");
#endif

    auto const& path = sScriptSystemMgr->GetPointMoveList(BalloonVehicle);
    Require(path.size() == 21, "The original 21-point Shen-zin Su route is missing");
    ObjectGuid const balloonGuid = balloon->GetGUID();
    bool hadAysa = false;
    bool hadJi = false;
    bool reachedLanding = false;
    const Position end{path.back().fX, path.back().fY, path.back().fZ};
    for (uint32 elapsed = 0; elapsed < 480000 && (balloon = map.GetCreature(balloonGuid)); elapsed += 100)
    {
        for (uint32 entry : {56660u, 56661u, 65476u, 65477u})
        {
            std::list<Creature*> passengers;
            balloon->GetCreatureListWithEntryInGrid(passengers, entry, 15.0f);
            for (Creature* npc : passengers)
                if (auto summon = npc->ToTempSummon(); summon && (summon->GetSummonerGUID() == balloonGuid || summon->GetSummonerGUID() == owner.GetGUID()))
                    npc->Unit::Update(100);
        }
        balloon->Unit::Update(100);
        balloon->AI()->UpdateAI(100);
        Tick(owner, 100);
        map.MoveAllCreaturesInMoveList();
        if (auto vehicle = balloon->GetVehicleKit())
            for (int8 seat = 1; seat < 5; ++seat)
                if (auto passenger = vehicle->GetPassenger(seat))
                {
                    hadJi |= passenger->GetEntry() == 56660;
                    hadAysa |= passenger->GetEntry() == 56661;
                }
        reachedLanding |= balloon->GetExactDist(end) < 1.0f;
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Shen-zin Su Ji=" << hadJi << " Aysa=" << hadAysa << " last=" << reachedLanding << " seated=" << bool(owner.GetVehicle())
        << " story=" << owner.GetQuestObjectiveData(Suffering, TurtleStoryCredit) << std::endl;
    Require(hadAysa && hadJi && reachedLanding && !owner.GetVehicle() && owner.GetQuestObjectiveData(Suffering, TurtleStoryCredit) == 1,
        "The actual ride did not carry both original companions, complete the story and reach the final exit");

    auto shaopaiData = sObjectMgr->GetCreatureData(245235);
    Require(shaopaiData, "Missing the original Elder Shaopai placement");
    MoveGilneasPlayer(owner, Position{shaopaiData->posX, shaopaiData->posY, shaopaiData->posZ, shaopaiData->orientation});
    Creature* shaopai = owner.FindNearestCreature(56012, 10.0f, true);
    Require(shaopai && shaopai->GetDBTableGUIDLow() == 245235, "Elder Shaopai was not available after the flight");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestGiverGUID = shaopai->GetGUID();
    reward.QuestID = Suffering;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.IsQuestRewarded(Suffering), "Elder Shaopai did not accept the two genuine balloon objectives");
    std::cout << "PASS normal balloon click, abandoned flight cleanup, new attempt, both companions, 21 waypoints and Elder Shaopai reward\n";
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
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Shang Xi reward check requires isolated characters");
        LoginDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
        LoginDatabase.Execute("SET SESSION TRANSACTION READ ONLY");
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckPassing(quest);
        CheckTempleArrival(quest);
        CheckPassingWisdom();
#ifdef WANDERING_ISLE_BALLOON
        CheckShenZinSu();
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
