#define ADERIC_REPOSE_NATIVE_SUPPORT_ONLY
#include "AdericReposeNative.cpp"

#include "SpellPackets.h"
#include "Vehicle.h"
#include "VehiclePackets.h"

namespace
{
constexpr uint32 SiegeQuest = 24681;
constexpr uint32 EndgameQuest = 26706;
constexpr uint32 LaunchRack = 38150;
constexpr uint32 GlaiveThrower = 37927;
constexpr uint32 LaunchGlaive = 70735;
constexpr uint32 GlaiveBarrage = 70732;

void LoadHarborData()
{
    auto rows = WorldDatabase.Query(
#include "NativeKeelHarbor.inc"
    );
    Require(rows && rows->GetRowCount() == 8, "Missing the actual Gilneas quest templates");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon "
        "WHERE ID IN(24681,26706)");
    Require(addons && addons->GetRowCount() == 2, "Missing original quest addon fields");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(24681,26706) ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 4, "Missing original siege and Endgame counters");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN(24681,26706)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN(24681,26706)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell IN(69486,70695,74093)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId=4714");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN(38150,37927,43747)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE (SourceTypeOrReferenceId=23 AND SourceGroup=4714) "
        "OR (SourceTypeOrReferenceId=18 AND SourceGroup IN(38150,37927,43747)) OR (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(24681,26706))");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadPhaseDefinitions();
    sObjectMgr->LoadNPCSpellClickSpells();
    sConditionMgr->LoadConditions();
}

void Click(LootPlayer& player, Creature& rack)
{
    WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
    packet.SpellClickUnitGuid = rack.GetGUID();
    player.GetSession()->HandleSpellClick(packet);
    Tick(player, 1000);
}

void Aim(LootPlayer& player, Creature& machine, uint32 spell, Position const& point)
{
    // The isolated map is stepped through impacts, without waiting out each wall-clock cooldown.
    machine.RemoveCreatureSpellCooldown(spell);
    machine.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(spell));
    WorldPackets::Spells::CastSpell packet{WorldPacket(CMSG_CAST_SPELL)};
    packet.Cast.SpellID = spell;
    packet.Cast.SpellGuid = ObjectGuid::Create<HighGuid::Cast>(machine.GetMapId(), spell,
        sObjectMgr->GetGenerator<HighGuid::Cast>()->Generate(), SPELL_CAST_TYPE_NORMAL);
    packet.Cast.Target.Flags = TARGET_FLAG_SOURCE_LOCATION | TARGET_FLAG_DEST_LOCATION;
    packet.Cast.Target.SrcLocation = WorldPackets::Spells::TargetLocation{ObjectGuid::Empty, machine.GetPosition()};
    packet.Cast.Target.DstLocation = WorldPackets::Spells::TargetLocation{ObjectGuid::Empty, point};
    packet.Cast.MissileTrajectory.Speed = 25.0f;
    player.GetSession()->HandleCastSpellOpcode(packet);
}

void Advance(LootPlayer& player, Creature& machine, uint32 duration)
{
    for (uint32 elapsed = 0; elapsed < duration; elapsed += 100)
    {
        Tick(player, 100);
        machine.Update(100);
        player.GetMap()->MoveAllCreaturesInMoveList();
    }
}

void ExitVehicle(LootPlayer& owner)
{
    WorldPackets::Vehicle::RequestVehicleExit exit{WorldPacket(CMSG_REQUEST_VEHICLE_EXIT)};
    owner.GetSession()->HandleRequestVehicleExit(exit);
    Require(!owner.GetVehicle() && owner.GetUnitBeingMoved() == &owner, "Native exit did not restore the player as mover");
}

void CheckHarbor(bool observe)
{
    Map map(654, 596, 0, DIFFICULTY_NONE);
    auto rackRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE guid=148959 AND id=38150 AND map=654");
    auto dariusRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE guid=148175 AND id=38149 AND map=654");
    auto lornaRow = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE guid=148161 AND id=43727 AND map=654");
    Require(rackRow && dariusRow && lornaRow, "Missing original rack, Darius or Lorna placement");
    ReposeQuestGiver rack(map, rackRow->Fetch()), darius(map, dariusRow->Fetch()), lorna(map, lornaRow->Fetch());
    LootPlayer owner(SupplySession(), map, 2468101, darius.GetPosition()), observer(SupplySession(), map, 2468102, darius.GetPosition());
    for (auto player : {&owner, &observer})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->InitDisplayIds();
        player->GetReputationMgr().LoadFromDB({});
        player->SetLevel(12);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint32 quest : {14386u, 14466u, 24903u, 24678u, 24679u, 24680u})
            player->SetRewardedQuest(quest);
        Travel(*player, darius.GetPosition());
    }
    Travel(observer, lorna.GetPosition());
    Require(observer.InSamePhase(&lorna) && lorna.hasQuest(EndgameQuest), "The original Endgame giver is unavailable in Keel Harbor");
    auto endgame = sQuestDataStore->GetQuestTemplate(EndgameQuest);
    if (observe)
    {
        Accept(observer, lorna, EndgameQuest);
        Require(observer.GetQuestStatus(EndgameQuest) == QUEST_STATUS_INCOMPLETE, "Pre-migration data did not reproduce early Endgame acceptance");
        std::cout << "EXPECTED_BROKEN Lorna accepted Endgame before the siege quest was started\n";
        return;
    }
    Require(!observer.CanTakeQuest(endgame, false), "Endgame still opens before the siege");
    Travel(observer, rack.GetPosition());
    Click(observer, rack);
    Require(!observer.GetVehicle() && observer.GetSummonList(GlaiveThrower)->empty(), "An unrelated player could start a Glaive Thrower");
    Accept(owner, darius, SiegeQuest);
    Require(owner.GetQuestStatus(SiegeQuest) == QUEST_STATUS_INCOMPLETE, "Darius did not open the original siege quest");
    Travel(owner, rack.GetPosition());
    Click(owner, rack);
    auto& summons = *owner.GetSummonList(GlaiveThrower);
    auto vehicle = owner.GetVehicleCreatureBase();
    if (!vehicle && !summons.empty())
    {
        vehicle = map.GetCreature(summons.front());
        if (vehicle)
            Advance(owner, *vehicle, 1800);
    }
    vehicle = owner.GetVehicleCreatureBase();
    Require(vehicle && vehicle->GetEntry() == GlaiveThrower && vehicle->GetVehicleKit() &&
        vehicle->GetVehicleKit()->GetPassenger(0) == &owner, "Original click did not board the driver");
    Require(vehicle->HasSpell(LaunchGlaive) && vehicle->HasSpell(GlaiveBarrage) && vehicle->HasSpell(59737),
        "Original abilities are missing from the vehicle");
    auto firstVehicle = vehicle->GetGUID();
    Click(owner, rack);
    Require(summons.size() == 1 && owner.GetVehicleCreatureBase() == vehicle, "Repeated click replaced the active vehicle");
    ExitVehicle(owner);
    Advance(owner, *vehicle, 100);
    map.RemoveAllObjectsInRemoveList();
    Travel(owner, rack.GetPosition());
    Click(owner, rack);
    vehicle = owner.GetVehicleCreatureBase();
    if (!vehicle && !summons.empty())
        if (auto next = map.GetCreature(summons.back()))
            Advance(owner, *next, 1800);
    vehicle = owner.GetVehicleCreatureBase();
    Require(vehicle && vehicle->GetVehicleKit()->GetPassenger(0) == &owner, "The rider could not board again after exiting");
    std::cout << "BOARD initial=" << firstVehicle.GetGUIDLow() << " retry=" << vehicle->GetGUIDLow() << '\n';

    auto quest = sQuestDataStore->GetQuestTemplate(SiegeQuest);
    for (auto const& objective : quest->GetObjectives())
    {
        auto enemyRow = WorldDatabase.PQuery("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature "
            "WHERE map=654 AND areaId=4725 AND id=%u ORDER BY guid LIMIT 1", uint32(objective.ObjectID));
        Require(bool(enemyRow), "Missing an original siege target in the battlefield");
        for (int32 killed = 0; killed < objective.Amount; ++killed)
        {
            ReposeQuestGiver enemy(map, enemyRow->Fetch());
            Position source = enemy.GetPosition();
            source.m_positionX -= 14.0f;
            source.SetOrientation(0.0f);
            map.CreatureRelocation(vehicle, source.GetPositionX(), source.GetPositionY(), source.GetPositionZ(), 0.0f);
            map.MoveAllCreaturesInMoveList();
            Travel(owner, source);
            Require(vehicle->IsValidAttackTarget(&enemy) && vehicle->IsWithinLOSInMap(&enemy), "A native siege target is not attackable");
            auto spell = killed ? LaunchGlaive : GlaiveBarrage;
            for (unsigned hit = 0; hit < 30 && enemy.IsAlive(); ++hit)
            {
                Aim(owner, *vehicle, spell, enemy.GetPosition());
                Advance(owner, *vehicle, 1600);
            }
            Require(!enemy.IsAlive() && owner.GetQuestObjectiveData(quest, objective.StorageIndex) == killed + 1,
                "Vehicle damage did not award one normal death credit for this target");
        }
        std::cout << "PASS target=" << objective.ObjectID << " credited=" << objective.Amount << '\n';
    }
    Require(owner.GetQuestStatus(SiegeQuest) == QUEST_STATUS_COMPLETE && !owner.CanTakeQuest(endgame, false),
        "Completing the counters alone must not unlock Endgame");
    ExitVehicle(owner);
    map.RemoveAllObjectsInRemoveList();
    Travel(owner, darius.GetPosition());
    Reward(owner, darius, SiegeQuest);
    Travel(owner, lorna.GetPosition());
    Require(owner.HasAura(74093) && owner.HasPhaseId(191) && !observer.HasAura(74093),
        "Only the rewarded siege player should gain the harbor phase");
    Accept(owner, lorna, EndgameQuest);
    Require(owner.GetQuestStatus(EndgameQuest) == QUEST_STATUS_INCOMPLETE &&
        observer.GetQuestStatus(EndgameQuest) == QUEST_STATUS_NONE, "Lorna did not offer Endgame only to the eligible player");
    std::cout << "PASS actual launch, all three kill counters, exit and reboarding, Darius reward and Lorna's follow-up\n";
}
}

#ifndef KEEL_HARBOR_NATIVE_SUPPORT_ONLY
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(CrashTrace);
    try
    {
        InitializeNative(argc, argv);
        opcodeTable.Initialize();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{654, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{654, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Siege probe requires isolated characters");
        LoadHarborData();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckHarbor(sConfigMgr->GetBoolDefault("NativeKeelHarbor.Observe", false));
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
#endif
