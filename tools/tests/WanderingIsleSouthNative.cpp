#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "ConditionMgr.h"
#include "CharacterPackets.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "CreatureTextMgr.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "MapManager.h"
#include "MiscPackets.h"
#include "MoveSpline.h"
#include "MovementPackets.h"
#include "NPCPackets.h"
#include "ObjectVisitors.hpp"
#include "ScriptedEscortAI.h"
#include "ScriptSystem.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"
#include "SpellPackets.h"
#include "Vehicle.h"
#include "WanderingIsleCartNative.h"

void AddSC_go_scripts();
void AddSC_WanderingIsland();

#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
struct FarewellMapsTag { using Type = std::vector<Map*> MapManager::*; friend Type Access(FarewellMapsTag); };
template struct FixtureAccess<FarewellMapsTag, &MapManager::i_maps>;
struct FarewellSessionsTag { using Type = SessionMap World::*; friend Type Access(FarewellSessionsTag); };
template struct FixtureAccess<FarewellSessionsTag, &World::m_sessions>;
#endif

namespace
{
constexpr uint32 Bidden = 29792;
constexpr uint32 Stocking = 29795;
constexpr uint32 Wreck = 30589;
constexpr uint32 Handle = 30590;
constexpr uint32 Preying = 30591;
constexpr uint32 EvilFromSeas = 29793;
constexpr uint32 NoneLeftBehind = 29794;
constexpr uint32 UrgentNews = 29796;
constexpr uint32 MedicalSupplies = 29797;
constexpr uint32 Tiger = 55946;
constexpr uint32 DarkenedHorror = 56007;
constexpr uint32 Bamboo = 80806;
constexpr uint32 Explosives = 74955;
constexpr uint32 MedicalCrateItem = 74958;
constexpr uint32 InjuredSource = 55999;
constexpr uint32 CarriedSailor = 56236;
constexpr uint32 CarryAura = 105520;
#ifdef WANDERING_ISLE_THORN
constexpr uint32 FromBadToWorse = 29665;
constexpr uint32 AncientEvil = 29798;
constexpr uint32 RiskingItAll = 30767;
constexpr uint32 Vordraka = 56009;
constexpr uint32 AysaEscort = 60729;
constexpr uint32 JiEscort = 60741;
constexpr uint32 ThornCredit = 60727;
#endif
#ifdef WANDERING_ISLE_HEALING
constexpr uint32 Healing = 29799;
constexpr uint32 HealingPower = 117783;
#endif
#ifdef WANDERING_ISLE_DEPARTURE
constexpr uint32 NewAllies = 29800;
constexpr uint32 NewFate = 31450;
constexpr uint32 CrashCartTender = 57741;
constexpr uint32 CrashCartYak = 57742;
#endif

struct SouthPet : LootPet
{
    SouthPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        setFaction(owner.getFaction());
        SetPhaseMask(owner.GetPhaseMask(), false);
        SetPhaseId(owner.GetPhases(), false);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }
};

void LoadSouth()
{
    auto templates = WorldDatabase.Query(
#include "NativeWanderingIsleSouth.inc"
    );
    std::string questIds = "29792,29793,29794,29795,29796,29797,30589,30590,30591";
    uint32 templateCount = 9, objectiveCount = 6;
#ifdef WANDERING_ISLE_THORN
    questIds += ",29665,29798,29799,30767";
    templateCount += 4;
    objectiveCount += 4;
#endif
#ifdef WANDERING_ISLE_DEPARTURE
    questIds += ",29800,31450";
    templateCount += 2;
    objectiveCount += 2;
#endif
    Require(templates && templates->GetRowCount() == templateCount, "The southern Wandering Isle quest templates are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.PQuery("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN (%s)", questIds.c_str());
    Require(addons && addons->GetRowCount() == templateCount, "The original southern prerequisites are missing");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        if (quest->PrevQuestID && quests[std::abs(quest->PrevQuestID)])
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.PQuery("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (%s) AND QuestID NOT IN (29792,29796,30589)", questIds.c_str());
    Require(objectives && objectives->GetRowCount() == objectiveCount, "The original forest, bamboo or wreck objectives are missing");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM ? QUEST_SPECIAL_FLAGS_DELIVER : QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=860 AND "
#ifdef WANDERING_ISLE_THORN
        "(guid IN (245135,245137,245146,245264,245266,245384,245389,245495,245497,12924836"
#ifdef WANDERING_ISLE_DEPARTURE
        ",245548,245583,245631"
#endif
        ") OR id IN (55946,55999,56007,56360))"
#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
        " OR (map=0 AND id=60566 AND guid=338298) OR (map=1 AND id=60570 AND guid=189104)"
#endif
    );
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM legion_world.spell_target_position WHERE id IN (117497,117597)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE script_waypoint AS SELECT * FROM legion_world.script_waypoint WHERE entry IN (60729,57742)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE phase_definitions AS SELECT * FROM legion_world.phase_definitions WHERE zoneId=5736 AND entry IN (1,2,3)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon "
        "WHERE entry IN (55999,56360,60729,60741,60780,60858,60878,60896,57208,57739,57741,57742,56013)");
#else
        "(guid IN (245135,245137,245264,245266,245384,245389) OR id IN (55946,55999,56007))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry=55999");
#endif
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (" + questIds + ")").c_str());
    WorldDatabase.DirectExecute(("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN (" + questIds + ")").c_str());
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE (SourceTypeOrReferenceId=18 AND SourceGroup=55999)"
#ifdef WANDERING_ISLE_THORN
        " OR (SourceTypeOrReferenceId=23 AND SourceGroup=5736 AND SourceEntry IN (2,3))"
#endif
#ifdef WANDERING_ISLE_DEPARTURE
        " OR (SourceTypeOrReferenceId=15 AND SourceGroup=13726)"
#endif
    );
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry IN (55999,57741)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura=-105520");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE (spell=56267 AND quest_start=29794) OR (spell=102613 AND quest_start=29798)"
#ifdef WANDERING_ISLE_HEALING
        " OR (spell=117783 AND quest_start=29799)"
#endif
#ifdef WANDERING_ISLE_DEPARTURE
        " OR (spell IN (105096,108914,108931) AND quest_start=29800)"
#endif
    );
#ifdef WANDERING_ISLE_HEALING
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid=60858");
#endif
#ifdef WANDERING_ISLE_DEPARTURE
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=115904");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=13726");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=13726");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID=19723");
#endif
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
    sObjectMgr->LoadInstanceTemplate();
#endif
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
#ifdef WANDERING_ISLE_DEPARTURE
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
#endif
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
#ifdef WANDERING_ISLE_THORN
    sObjectMgr->LoadPhaseDefinitions();
    sSpellMgr->LoadSpellTargetPositions();
    sScriptSystemMgr->LoadScriptWaypoints();
#endif
#ifdef WANDERING_ISLE_HEALING
    sSmartScriptMgr->LoadSmartAIFromDB();
#endif
    sCreatureTextMgr->LoadCreatureTexts();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();

    auto& gameobjectLoot = LootTemplates_Gameobject.*Access(LootTableNameTag{});
    auto const originalGameobjectLoot = gameobjectLoot;
    gameobjectLoot += " WHERE Entry IN (40474,209792,211394,211397,211398,211399,211400,211401)";
    auto const count = (LootTemplates_Gameobject.*Access(LoadLootTableTag{}))();
    gameobjectLoot = originalGameobjectLoot;
    Require(count == 26, "The eight objective rows and original bonus loot did not load together");
    auto& tigerLoot = LootTemplates_Creature.*Access(LootTableNameTag{});
    auto const originalTigerLoot = tigerLoot;
    tigerLoot += " WHERE Entry IN (55946,56007,56009,56360,60780,60858)";
    (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    tigerLoot = originalTigerLoot;
    Require(LootTemplates_Creature.GetLootFor(Tiger) && LootTemplates_Creature.GetLootFor(DarkenedHorror), "Missing the original tiger or horror loot");
}

Creature& FindSouthActor(Map& map, uint32 spawn)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data, "An original forest actor has no world placement");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(map.GetId(), data->id, spawn));
    Require(actor && actor->GetDBTableGUIDLow() == spawn && actor->IsAlive(), "Could not load the original forest actor");
    return *actor;
}

void AcceptSouth(LootPlayer& player, Creature& actor, uint32 quest)
{
    MoveGilneasPlayer(player, actor.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest request{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    request.QuestGiverGUID = actor.GetGUID();
    request.QuestID = quest;
    player.GetSession()->HandleQuestGiverAcceptQuest(request);
    Require(player.GetQuestStatus(quest) != QUEST_STATUS_NONE, "The actual actor did not offer its forest quest");
}

void RewardSouth(LootPlayer& player, Creature& actor, uint32 quest)
{
    MoveGilneasPlayer(player, actor.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward request{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    request.QuestGiverGUID = actor.GetGUID();
    request.QuestID = quest;
    if (auto package = sDB2Manager.GetQuestPackageItems(sQuestDataStore->GetQuestTemplate(quest)->PackageID))
        for (auto item : *package)
            if (player.CanSelectQuestPackageItem(item))
            {
                request.ItemChoiceID = item->ItemID;
                break;
            }
    player.GetSession()->HandleQuestGiverChooseReward(request);
    Require(player.IsQuestRewarded(quest), "The actual actor did not reward the forest quest");
}

std::vector<Placement> SouthSupplies(uint32 item, uint32 count)
{
    auto rows = WorldDatabase.PQuery("SELECT g.id,g.guid,g.spawntimesecs,g.position_x,g.position_y,g.position_z,g.orientation,"
        "g.rotation0,g.rotation1,g.rotation2,g.rotation3 FROM gameobject g JOIN gameobject_template t ON t.entry=g.id JOIN gameobject_loot_template l ON l.Entry=t.Data1 "
        "WHERE g.map=860 AND l.Item=%u AND l.QuestRequired=1 ORDER BY g.id,g.guid", item);
    Require(rows && rows->GetRowCount() == count, "The original bamboo or bundle placements changed");
    std::vector<Placement> placements;
    do
    {
        auto row = rows->Fetch();
        placements.push_back({row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(),
            {row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat()},
            G3D::Quat(row[7].GetFloat(), row[8].GetFloat(), row[9].GetFloat(), row[10].GetFloat())});
    } while (rows->NextRow());
    return placements;
}

void CheckSouthSupplies(LootPlayer& player, LootPlayer& visitor, Creature& giver, uint32 quest, uint32 item, uint32 needed, uint32 count, bool exclusiveFirst,
    bool expectAbandonRetention = false)
{
    auto locations = SouthSupplies(item, count);
    Map& map = *player.GetMap();
    AcceptSouth(player, giver, quest);
    if (exclusiveFirst)
    {
        Supply first(map, locations[0]);
        MoveGilneasPlayer(visitor, locations[0].position);
        OpenSupply(visitor, first);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
        Require(visitor.GetLootGUID() == first.GetGUID() && first.getLootState() == GO_ACTIVATED && first.loot.isLooted(),
            "The previous exclusive source did not reproduce an empty foreign loot activation");
        ReleaseSupply(visitor, first);
#else
        Require(!visitor.GetLootGUID() && first.getLootState() == GO_READY, "The wrong player consumed exclusive forest supplies");
#endif
    }
    auto collect = [&](Placement const& location)
    {
        Supply source(map, location);
        MoveGilneasPlayer(player, location.position);
        OpenSupply(player, source);
        Require(TakeSupply(player, source, item), "The genuine forest supply did not yield its required item");
        ReleaseSupply(player, source);
    };
    collect(locations[1]);
    AbandonSupply(player, quest);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
    Require(player.GetItemCount(item) == 1, "The previous quest did not reproduce abandoned item retention");
    if (exclusiveFirst)
        std::cout << "EXPECTED_BROKEN the exclusively quest-only bamboo source opened without its quest\n";
    std::cout << "EXPECTED_BROKEN abandoning the original quest retained its required item " << item << '\n';
#else
    if (expectAbandonRetention)
    {
        Require(player.GetItemCount(item) == 1, "The previous medical quest did not reproduce item retention");
        std::cout << "EXPECTED_BROKEN abandoning Medical Supplies kept its required bundle\n";
        return;
    }
    Require(!player.GetItemCount(item), "Abandoning a supply quest retained its exclusive item");
    AcceptSouth(player, giver, quest);
    for (uint32 index = 0; index < needed; ++index)
        collect(locations[index + 2]);
    Require(player.GetItemCount(item) == needed && player.GetQuestStatus(quest) == QUEST_STATUS_COMPLETE, "The real forest supplies did not reach their quota");
    RewardSouth(player, giver, quest);
    Require(!player.GetItemCount(item), "The native reward did not consume its supplies");
    std::cout << "PASS " << needed << " real supplies from " << count << " placements, abandon, retry and reward for " << quest << '\n';
#endif
}

void CheckSouthCombat(LootPlayer& pupil, Creature& giver, uint32 quest, uint32 entry, uint32 needed, uint32 placements)
{
    Map& map = *pupil.GetMap();
    auto spawns = WorldDatabase.PQuery("SELECT guid FROM creature WHERE map=860 AND id=%u ORDER BY ((phaseMask & 1)=0),guid", entry);
    Require(spawns && spawns->GetRowCount() == placements, "The original combat placements changed");
    AcceptSouth(pupil, giver, quest);
    SouthPet pet(pupil, map);
    for (uint32 index = 0; index < needed; ++index)
    {
        auto& opponent = FindSouthActor(map, spawns->Fetch()[0].GetUInt32());
        MoveGilneasPlayer(pupil, opponent.GetPosition());
        map.CreatureRelocation(&pet, opponent.GetPositionX(), opponent.GetPositionY(), opponent.GetPositionZ(), opponent.GetOrientation());
        map.MoveAllCreaturesInMoveList();
        Unit& attacker = index % 2 == 0 ? static_cast<Unit&>(pupil) : static_cast<Unit&>(pet);
        Require(attacker.IsValidAttackTarget(&opponent), "A placed opponent could not be attacked");
        attacker.DealDamage(&opponent, uint32(opponent.GetMaxHealth()) * 2);
        Tick(pupil, 150);
        Require(!opponent.IsAlive() && pupil.GetQuestObjectiveData(quest, entry) == int32(index + 1), "A real player or pet defeat did not progress the quest");
        spawns->NextRow();
    }
    RewardSouth(pupil, giver, quest);
    std::cout << "PASS " << needed << " actual player or pet defeats and original reward for " << quest << '\n';
}

void CheckRescue(LootPlayer& owner, LootPlayer& visitor, Creature& ji)
{
    Map& map = *owner.GetMap();
    auto rows = WorldDatabase.Query("SELECT guid FROM creature WHERE map=860 AND id=55999 ORDER BY guid");
    Require(rows && rows->GetRowCount() == 22, "The original injured sailor placements changed");
    std::vector<uint32> spawns;
    do
        spawns.push_back(rows->Fetch()[0].GetUInt32());
    while (rows->NextRow());
    auto click = [](LootPlayer& player, Creature& sailor)
    {
        WorldPackets::Spells::SpellClick packet{WorldPacket(CMSG_SPELL_CLICK)};
        packet.SpellClickUnitGuid = sailor.GetGUID();
        player.GetSession()->HandleSpellClick(packet);
        Tick(player, 100);
    };
    auto carry = [&](Creature& source) -> Creature*
    {
        MoveGilneasPlayer(owner, source.GetPosition());
        click(owner, source);
        std::list<Creature*> nearby;
        owner.GetCreatureListWithEntryInGrid(nearby, CarriedSailor, 10.0f);
        for (Creature* sailor : nearby)
            if (auto summon = sailor->ToTempSummon(); summon && summon->GetSummonerGUID() == owner.GetGUID())
                sailor->Unit::Update(100);
        auto kit = owner.GetVehicleKit();
        auto passenger = kit ? kit->GetPassenger(0) : nullptr;
        Require(kit && kit->GetRecAura() == CarryAura && owner.HasAura(CarryAura) && passenger && passenger->GetEntry() == CarriedSailor,
            "A legitimate click did not produce the native sailor passenger in seat zero");
        source.AI()->UpdateAI(150);
        return passenger->ToCreature();
    };

    AcceptSouth(owner, ji, NoneLeftBehind);
    auto& first = FindSouthActor(map, spawns[0]);
    MoveGilneasPlayer(visitor, first.GetPosition());
    ObjectGuid const firstGuid = first.GetGUID();
    click(visitor, first);
    first.AI()->UpdateAI(2500);
    first.Unit::Update(5100);
    map.RemoveAllObjectsInRemoveList();
    Require(!visitor.HasAura(CarryAura), "The native quest condition admitted an ineligible visitor");
#ifdef WANDERING_ISLE_RESCUE_BASELINE
    auto dismissed = map.GetCreature(firstGuid);
    std::cout << "CASE ineligible click left source alive=" << bool(dismissed && dismissed->IsAlive()) << '\n';
    auto& second = FindSouthActor(map, spawns[1]);
    Position remote = second.GetPosition();
    remote.m_positionX += 25.0f;
    MoveGilneasPlayer(owner, remote);
    click(owner, second);
    Require(owner.HasAura(CarryAura) && owner.GetVehicleKit(), "A remote click did not reproduce a carry vehicle in the original implementation");
    AbandonSupply(owner, NoneLeftBehind);
    Tick(owner, 500);
    Require(owner.HasAura(CarryAura) && owner.GetVehicleKit(), "The abandoned native carry no longer left its vehicle aura on the player");
    owner.RemoveAurasDueToSpell(CarryAura);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "EXPECTED_BROKEN a remote click began a carry and abandoning kept the carry vehicle\n";
#else
    auto preserved = map.GetCreature(firstGuid);
    Require(preserved && preserved->IsAlive(), "An ineligible click consumed an unrescued world actor");
    auto& second = FindSouthActor(map, spawns[1]);
    Position remote = second.GetPosition();
    remote.m_positionX += 25.0f;
    MoveGilneasPlayer(owner, remote);
    click(owner, second);
    Require(!owner.HasAura(CarryAura), "A distant click picked up a native sailor");
    auto passenger = carry(second);
    ObjectGuid const abandonedPassenger = passenger->GetGUID();
    Require(passenger->HasQuestObjectScope() && !passenger->CanTargetQuestObject(&visitor), "The carried sailor lacked its owner's accepted scope");
    AbandonSupply(owner, NoneLeftBehind);
    map.RemoveAllObjectsInRemoveList();
    Tick(owner, 500);
    std::cout << "CASE abandoned carry aura=" << owner.HasAura(CarryAura) << " kit=" << bool(owner.GetVehicleKit())
        << " old actor=" << bool(map.GetCreature(abandonedPassenger)) << '\n';
    Require(!map.GetCreature(abandonedPassenger) && !owner.HasAura(CarryAura) && !owner.GetVehicleKit(),
        "Abandoning kept the old sailor or carry vehicle active");
    AcceptSouth(owner, ji, NoneLeftBehind);

    auto& controller = FindSouthActor(map, 245137);
    Require(controller.GetEntry() == 56476, "The original camp rescue controller did not load");
    auto arriveAtCamp = [&](Creature& sailor)
    {
        MoveGilneasPlayer(owner, controller.GetPosition());
        owner.GetVehicleKit()->RelocatePassengers();
        map.MoveAllCreaturesInMoveList();
        Require(sailor.IsWithinDistInMap(&controller, 10.0f), "The native carried passenger did not reach the camp radius");
        Trinity::AIRelocationNotifier relocation(owner);
        Trinity::VisitNearbyObject(&owner, owner.GetVisibilityRange(), relocation);
        Trinity::VisitNearbyObject(&owner, owner.GetVisibilityRange(), relocation);
    };
    auto pendingSailor = carry(FindSouthActor(map, spawns[2]));
    arriveAtCamp(*pendingSailor);
    AbandonSupply(owner, NoneLeftBehind);
    controller.Unit::Update(250);
    Tick(owner, 250);
    controller.AI()->UpdateAI(250);
    map.RemoveAllObjectsInRemoveList();
    AcceptSouth(owner, ji, NoneLeftBehind);
    Require(!owner.GetQuestObjectiveData(NoneLeftBehind, InjuredSource) && !owner.GetVehicleKit(),
        "An abandoned camp handoff affected the next accepted rescue attempt");

    for (uint32 index = 0; index < 3; ++index)
    {
        auto& source = FindSouthActor(map, spawns[index + 3]);
        auto sailor = carry(source);
        Require(owner.GetQuestObjectiveData(NoneLeftBehind, InjuredSource) == int32(index), "Pickup gave delivery credit early");
        arriveAtCamp(*sailor);
        controller.Unit::Update(250);
        Tick(owner, 250);
        controller.AI()->UpdateAI(250);
        std::cout << "CASE camp handoff=" << index + 1 << " credit=" << owner.GetQuestObjectiveData(NoneLeftBehind, InjuredSource)
            << " aura=" << owner.HasAura(CarryAura) << " visibility=" << owner.GetVisibilityRange()
            << " controller sees=" << controller.canSeeOrDetect(&owner, false, true) << '\n';
        Require(owner.GetQuestObjectiveData(NoneLeftBehind, InjuredSource) == int32(index + 1) && !owner.HasAura(CarryAura),
            "A genuine sailor handoff did not credit exactly once and end the carry");
        map.RemoveAllObjectsInRemoveList();
    }
    RewardSouth(owner, ji, NoneLeftBehind);
    std::cout << "PASS refused foreign and distant casts, abandoned carry cleanup, three actual sailor handoffs and Ji's reward\n";
#endif
}

#ifdef WANDERING_ISLE_THORN
Creature* FindThornEscort(Player& player)
{
    for (ObjectGuid guid : *player.GetSummonList(AysaEscort))
        if (auto actor = player.GetMap()->GetCreature(guid); actor && actor->IsAlive() && !actor->IsQuestObjectExpired())
            return actor;
    std::list<Creature*> escorts;
    player.GetCreatureListWithEntryInGrid(escorts, AysaEscort, 100.0f);
    for (auto actor : escorts)
        if (auto summon = actor->ToTempSummon(); summon && summon->GetSummonerGUID() == player.GetGUID() && summon->IsAlive() && !summon->IsQuestObjectExpired())
            return summon;
    return nullptr;
}

void CheckThorn(LootPlayer& pupil, LootPlayer& visitor)
{
    for (auto [spellId, entry] : {std::pair{117497u, AysaEscort}, std::pair{117597u, JiEscort}})
    {
        auto spell = sSpellMgr->GetSpellInfo(spellId);
        auto effect = spell ? spell->GetEffect(EFFECT_0) : nullptr;
        Require(effect && effect->IsEffect(SPELL_EFFECT_SUMMON) && effect->MiscValue == int32(entry) &&
            effect->MiscValueB == 3276 && sSpellMgr->GetSpellTargetPosition(spellId), "The original personal destination summon changed");
    }
    auto& map = *pupil.GetMap();
    auto& admiral = FindSouthActor(map, 245146);
    CheckSouthCombat(pupil, admiral, FromBadToWorse, 56360, 8, 22);
    AcceptSouth(pupil, admiral, AncientEvil);
    auto& nightmare = FindSouthActor(map, 12924836);
    auto& aysa = FindSouthActor(map, 245495);
    MoveGilneasPlayer(pupil, nightmare.GetPosition());
    MoveGilneasPlayer(visitor, nightmare.GetPosition());
    SouthPet pet(pupil, map);
    Require(pet.IsValidAttackTarget(&nightmare), "The pet could not attack the original Vordraka");
    pet.DealDamage(&nightmare, uint32(nightmare.GetMaxHealth()) * 2);
    Tick(pupil, 150);
    Require(!nightmare.IsAlive() && pupil.GetQuestObjectiveData(AncientEvil, Vordraka) == 1 && !visitor.GetQuestObjectiveData(AncientEvil, Vordraka),
        "The actual pet defeat did not credit the eligible owner once");
    RewardSouth(pupil, aysa, AncientEvil);
    AcceptSouth(pupil, aysa, RiskingItAll);
    Tick(pupil, 250);
    auto escort = FindThornEscort(pupil);
    Require(escort, "Normal Aysa acceptance did not cast the original personal escort");
    ObjectGuid abandonedEscort = escort->GetGUID();
    AbandonSupply(pupil, RiskingItAll);
    map.RemoveAllObjectsInRemoveList();
#ifdef WANDERING_ISLE_THORN_BASELINE
    Require(map.GetCreature(abandonedEscort), "The previous escort did not reproduce retention after abandonment");
    std::cout << "EXPECTED_BROKEN the accepted Risking It All escort remained alive after abandoning the quest\n";
    map.GetCreature(abandonedEscort)->DespawnOrUnsummon();
    map.RemoveAllObjectsInRemoveList();
#else
    Require(!map.GetCreature(abandonedEscort), "The abandoned escort survived cancellation");
    AcceptSouth(pupil, aysa, RiskingItAll);
    visitor.SetRewardedQuest(AncientEvil);
    AcceptSouth(visitor, aysa, RiskingItAll);
    Tick(pupil, 100);
    Tick(visitor, 100);
    escort = FindThornEscort(pupil);
    auto second = FindThornEscort(visitor);
    Require(escort && second && escort != second && escort->HasQuestObjectScope() && second->HasQuestObjectScope() &&
        escort->CanTargetQuestObject(&pupil) && !escort->CanTargetQuestObject(&visitor), "The two original escorts did not have separate owners");
    pupil.CastSpell(&pupil, 117497, true);
    Tick(pupil, 100);
    Require(pupil.GetSummonList(AysaEscort)->size() == 1 && pupil.GetSummonList(JiEscort)->size() == 1 &&
        visitor.GetSummonList(JiEscort)->size() == 1, "Duplicate summon or missing personal Ji");
    auto escortGuid = escort->GetGUID();
    auto secondGuid = second->GetGUID();
    AbandonSupply(visitor, RiskingItAll);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetCreature(secondGuid) && map.GetCreature(escortGuid), "One participant canceled another participant's escort");

    auto const& route = sScriptSystemMgr->GetPointMoveList(AysaEscort);
    Require(route.size() == 10 && route[7].uiPointId == 8, "The original escort route is absent");
    Position debate{route[7].fX, route[7].fY, route[7].fZ};
    bool reachedDebate = false, startedExit = false;
    Position last{route.back().fX, route.back().fY, route.back().fZ};
    Position lastObserved = escort->GetPosition();
    uint32 duration = 0;
    for (uint32 elapsed = 0; elapsed < 190000 && (escort = map.GetCreature(escortGuid)); elapsed += 100)
    {
        duration = elapsed;
        if (!pupil.isWatchingMovie() && !pupil.GetQuestObjectiveData(RiskingItAll, ThornCredit))
            SupplyMove(pupil, escort->GetPosition());
        escort->Unit::Update(100);
        escort->AI()->UpdateAI(100);
        Tick(pupil, 100);
        map.MoveAllCreaturesInMoveList();
        reachedDebate |= escort->movespline->Finalized() && escort->GetExactDist(debate) < 1.0f;
        startedExit |= reachedDebate && !escort->movespline->Finalized() && escort->GetPositionY() < 3990.0f;
        lastObserved = escort->GetPosition();
        map.RemoveAllObjectsInRemoveList();
    }
    std::cout << "CASE Thorn debate=" << reachedDebate << " exit motion=" << startedExit << " duration=" << duration
        << " last actor distance from scripted exit=" << lastObserved.GetExactDist(last) << std::endl;
    Require(reachedDebate && startedExit && !map.GetCreature(escortGuid), "The native escort did not reach the debate, depart and clean up");
    if (sConfigMgr->GetBoolDefault("NativeThornExit.ExpectInterrupted", false))
    {
        Require(lastObserved.GetExactDist(last) > 30.0f, "The previous exit no longer reproduces its incomplete waypoint");
        std::cout << "EXPECTED_BROKEN the deck route stopped above the intended ground waypoint\n";
    }
    else
        Require(lastObserved.GetExactDist(last) < 1.0f, "Aysa despawned before actually reaching the ground waypoint");
    Require(pupil.GetQuestObjectiveData(RiskingItAll, ThornCredit) == 1 && pupil.isWatchingMovie(), "The original escort did not launch its movie and credit its owner");
    Require(!visitor.GetQuestObjectiveData(RiskingItAll, ThornCredit) && !visitor.isWatchingMovie(), "A canceled quest participant received the movie or credit");
    Require(pupil.HasPendingTeleport() && pupil.GetTeleportDest().GetExactDist(Position{249.38f, 3939.55f, 65.61f}) < 1.0f,
        "The original conversation did not queue the landing beside Ji");
    WorldPackets::Misc::CompleteMovie movie{WorldPacket(CMSG_COMPLETE_MOVIE)};
    pupil.GetSession()->HandleCompleteMovie(movie);
    pupil.Update(100);
    WorldPackets::Movement::MoveTeleportAck teleport{WorldPacket(CMSG_MOVE_TELEPORT_ACK)};
    teleport.MoverGUID = pupil.GetGUID();
    pupil.GetSession()->HandleMoveTeleportAck(teleport);
    Require(!pupil.isWatchingMovie() && !pupil.IsBeingTeleported() &&
        pupil.GetExactDist(Position{249.38f, 3939.55f, 65.61f}) < 1.0f, "The native movie and movement acknowledgements did not complete the landing");
    auto& ji = FindSouthActor(map, 245497);
    RewardSouth(pupil, ji, RiskingItAll);
    auto healing = sQuestDataStore->GetQuestTemplate(29799);
    Require(healing && pupil.CanTakeQuest(healing, false), "Ji's wounded Shen-zin Su follow-up was unavailable");
    std::cout << "PASS actual Vordraka pet defeat, private escorts, cancellation, debate, movie and landing acknowledgements, and Ji reward\n";
#endif
}
#endif

#ifdef WANDERING_ISLE_HEALING
void CheckHealing(LootPlayer& pupil, LootPlayer& visitor)
{
    auto& map = *pupil.GetMap();
    auto& ji = FindSouthActor(map, 245497);
    auto findActors = [&](std::initializer_list<uint32> entries)
    {
        std::list<Creature*> actors;
        for (uint32 entry : entries)
            ji.GetCreatureListWithEntryInGrid(actors, entry, ji.GetVisibilityRange());
        actors.remove_if([&](Creature* creature)
        {
            auto summon = creature->ToTempSummon();
            return !summon || !summon->IsAlive() || summon->GetSummonerGUID() != ji.GetGUID();
        });
        return actors;
    };
    auto approach = [&](LootPlayer& player)
    {
        MoveGilneasPlayer(player, ji.GetPosition());
        Trinity::AIRelocationNotifier notifier(player);
        Trinity::VisitNearbyObject(&player, ji.GetVisibilityRange(), notifier);
    };
    uint32 defeated = 0;
    SouthPet pet(pupil, map);
    auto run = [&](uint32 duration, bool defend = false)
    {
        for (uint32 elapsed = 0; elapsed < duration; elapsed += 250)
        {
            ji.Unit::Update(250);
            ji.AI()->UpdateAI(250);
            if (defend)
            {
                for (auto enemy : findActors({60780,60858}))
                {
                    map.LoadGrid(enemy->GetPositionX(), enemy->GetPositionY());
                    enemy->Unit::Update(250);
                    if (enemy->getVictim() && (enemy->getVictim()->GetEntry() == 60878 || enemy->getVictim()->GetEntry() == 60896))
                    {
                        Unit& attacker = defeated % 2 ? static_cast<Unit&>(pupil) : static_cast<Unit&>(pet);
                        if (attacker.IsValidAttackTarget(enemy))
                        {
                            attacker.DealDamage(enemy, uint32(enemy->GetMaxHealth()) * 2);
                            defeated += !enemy->IsAlive();
                        }
                    }
                }
            }
            Tick(pupil, 250);
            Tick(visitor, 250);
            map.MoveAllCreaturesInMoveList();
            map.RemoveAllObjectsInRemoveList();
        }
    };
    AcceptSouth(pupil, ji, Healing);
    approach(pupil);
    run(22000);
    auto healers = findActors({60878,60896});
    std::cout << "CASE Healing status=" << pupil.GetQuestStatus(Healing) << " zone=" << pupil.GetZoneId()
        << " area=" << pupil.GetAreaId() << " phase=" << pupil.GetPhaseMask() << " healers=" << healers.size()
        << " aura=" << pupil.HasAura(HealingPower) << " power=" << pupil.GetPower(POWER_ALTERNATE)
        << " max=" << pupil.GetMaxPower(POWER_ALTERNATE) << " credit=" << pupil.GetQuestObjectiveData(Healing, 56011) << std::endl;
#ifdef WANDERING_ISLE_HEALING_BASELINE
    Require(!healers.empty() && !pupil.HasAura(HealingPower) && !pupil.GetQuestObjectiveData(Healing, 56011),
        "The earlier quest did not reproduce a missing power bar with live healers");
    std::cout << "EXPECTED_BROKEN the original spell area refused the active quest's healing power bar\n";
#else
    Require(!healers.empty() && pupil.HasAura(HealingPower) && pupil.GetPower(POWER_ALTERNATE) > 0 &&
        !pupil.GetQuestObjectiveData(Healing, 56011), "An accepted participant did not join real active healing");
#endif
    AbandonSupply(pupil, Healing);
#ifndef WANDERING_ISLE_HEALING_BASELINE
    Require(!pupil.HasAura(HealingPower) && !pupil.GetPower(POWER_ALTERNATE), "Native abandonment did not remove the power bar immediately");
#endif
    Position away = ji.GetPosition();
    away.m_positionY -= 165.0f;
    MoveGilneasPlayer(pupil, away);
    run(18000);
#ifdef WANDERING_ISLE_HEALING_BASELINE
    visitor.SetRewardedQuest(RiskingItAll);
    approach(visitor);
    Require(findActors({60878,60896}).size() >= 2,
        "The earlier controller did not reproduce spawning more healers after the only participant abandoned");
    std::cout << "EXPECTED_BROKEN the original controller continued the defense after its only participant abandoned\n";
#else
    Require(findActors({60878,60896,60780,60858}).empty() && !pupil.HasAura(HealingPower) && !pupil.GetPower(POWER_ALTERNATE),
        "An abandoned participant retained the wound-healing aura or alternate power");

    AcceptSouth(pupil, ji, Healing);
    approach(pupil);
    run(14500);
    healers = findActors({60878,60896});
    auto enemies = findActors({60780,60858});
    Require(healers.size() == 1 && !enemies.empty() && pupil.HasAura(HealingPower), "A fresh attempt did not begin with a real healer and an attacker");
    Creature* healer = healers.front();
    enemies.front()->DealDamage(healer, uint32(healer->GetMaxHealth()) * 2);
    Require(!healer->IsAlive(), "The real attacker did not kill the first healer");
    auto power = pupil.GetPower(POWER_ALTERNATE);
    run(3500);
    Require(findActors({60878,60896}).empty() && pupil.GetPower(POWER_ALTERNATE) == power,
        "A dead healer still increased healing progress");
    MoveGilneasPlayer(pupil, away);
    run(500);
    Require(!pupil.HasAura(HealingPower) && !pupil.GetPower(POWER_ALTERNATE), "Leaving the defense did not clear the power bar");

    visitor.SetRewardedQuest(RiskingItAll);
    AcceptSouth(visitor, ji, Healing);
    approach(pupil);
    approach(visitor);
    run(16500, true);
    Require(pupil.HasAura(HealingPower) && visitor.HasAura(HealingPower), "Two valid participants did not join the same defense");
    MoveGilneasPlayer(visitor, away);
    run(250, true);
    Require(pupil.HasAura(HealingPower) && !visitor.HasAura(HealingPower), "One departure canceled another participant or left a stale power bar");
    for (uint32 round = 0; round < 42 && !pupil.GetQuestObjectiveData(Healing, 56011); ++round)
        run(10000, true);
    Require(pupil.GetQuestObjectiveData(Healing, 56011) == 1 && defeated >= 2 &&
        !visitor.GetQuestObjectiveData(Healing, 56011) && visitor.GetQuestStatus(Healing) == QUEST_STATUS_INCOMPLETE,
        "Actual healing and defense did not credit only the remaining eligible participant");
    Require(findActors({60878,60896,60780,60858}).empty() && !pupil.HasAura(HealingPower) && !pupil.GetPower(POWER_ALTERNATE),
        "Completing healing left active attackers, healers or the quest power bar");
    RewardSouth(pupil, ji, Healing);
    AbandonSupply(visitor, Healing);
    std::cout << "PASS actual healing bar, abandonment, range departure, dead healer stop, shared defense and Ji reward; actual defeats=" << defeated << '\n';
#endif
}
#endif

#ifdef WANDERING_ISLE_DEPARTURE
#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
void FinishCapitalArrival(LootPlayer& player, uint32 mapId, uint32 receiver)
{
    auto& sessions = sWorld->*Access(FarewellSessionsTag{});
    WorldSession* session = player.GetSession();
    uint32 account = session->GetAccountId();
    Require(sessions.try_emplace(account, WorldSessionPtr(session, [](WorldSession*) {})).second,
        "The isolated map transfer already had that fixture account");
    try
    {
        player.Update(100);
        Map* destination = sMapMgr->FindBaseMap(mapId);
        Require(destination && player.GetTeleportMap() == destination && session->GetMap() == destination, "Movie completion did not issue the real capital transfer");
        WorldPackets::Movement::WorldPortResponse arrival{WorldPacket(CMSG_WORLD_PORT_RESPONSE)};
        session->HandleWorldPortResponse(arrival);
        Require(player.GetMap() == destination && player.IsInWorld() && !player.HasPendingTeleport(), "Native capital transfer acknowledgement did not enter the map");
        auto& actor = FindSouthActor(*destination, receiver);
        RewardSouth(player, actor, NewFate);
    }
    catch (...)
    {
        sessions.erase(account);
        throw;
    }
    sessions.erase(account);
}

struct FarewellMapFixture
{
    Map eastern{0, 300, 0, DIFFICULTY_NONE};
    Map kalimdor{1, 300, 0, DIFFICULTY_NONE};
    std::vector<Map*>& maps = sMapMgr->*Access(FarewellMapsTag{});

    FarewellMapFixture()
    {
        Require(!maps.at(0) && !maps.at(1), "The isolated capital maps must be unowned");
        maps[0] = &eastern;
        maps[1] = &kalimdor;
    }

    ~FarewellMapFixture()
    {
        maps[0] = nullptr;
        maps[1] = nullptr;
    }
};
#endif

void CheckDeparture(LootPlayer& pupil, LootPlayer& visitor)
{
    auto& map = *pupil.GetMap();
    auto& giver = FindSouthActor(map, 245548);
    auto& tender = FindSouthActor(map, 245583);
    auto& shang = FindSouthActor(map, 245631);
    Require(pupil.IsQuestRewarded(Healing), "The farewell probe requires the actual wound defense reward");
    AcceptSouth(pupil, giver, NewAllies);
    Require(pupil.GetQuestStatus(NewAllies) == QUEST_STATUS_COMPLETE && !pupil.GetQuestObjectiveData(NewAllies, CrashCartTender),
        "The crash-site ride should be an optional objective");
    MoveGilneasPlayer(pupil, tender.GetPosition());
    auto travel = [&]()
    {
        WorldPackets::Spells::SpellClick click{WorldPacket(CMSG_SPELL_CLICK)};
        click.SpellClickUnitGuid = tender.GetGUID();
        pupil.GetSession()->HandleSpellClick(click);
        Creature* cart = FindOwnedCartActor(pupil, 57208);
        Creature* yak = FindOwnedCartActor(pupil, CrashCartYak);
        Require(cart && yak && cart->GetVehicleKit(), "The original Wreck tender did not publish a cart and yak");
        Tick(pupil, 2000);
        Require(cart->GetVehicleKit()->GetPassenger(0) == &pupil, "The real Wreck cart did not board the owner");
        return std::pair{cart, yak};
    };
    auto [cart, yak] = travel();
    auto const& route = sScriptSystemMgr->GetPointMoveList(CrashCartYak);
    Require(route.size() == 24, "The original crash-site route did not load");
    FinishOwnedCartRoute(map, pupil, *cart, *yak);
    auto const& last = route.back();
    Require(pupil.GetExactDist(Position{last.fX, last.fY, last.fZ}) < 8.0f &&
        pupil.GetQuestObjectiveData(NewAllies, CrashCartTender) == 1, "The actual optional ride did not reach and record its landing");
    RewardSouth(pupil, shang, NewAllies);
    AcceptSouth(pupil, shang, NewFate);
    std::cout << "PASS native crash-site cart, 24 original route points, optional arrival credit and Spirit of Master Shang Xi follow-up\n";
#ifdef WANDERING_ISLE_DEPARTURE_BASELINE
    WorldPackets::Character::NeutralPlayerSelectFaction choice{WorldPacket(CMSG_NEUTRAL_PLAYER_SELECT_FACTION)};
    choice.Faction = 2;
    pupil.GetSession()->HandleNeutralPlayerSelectFaction(choice);
    Require(pupil.getRace() == RACE_PANDAREN_NEUTRAL && pupil.GetQuestStatus(NewFate) == QUEST_STATUS_COMPLETE &&
        !pupil.GetQuestObjectiveData(NewFate, 64594) && pupil.isWatchingMovie(),
        "The original handler did not reproduce a completed quest without a faction");
    std::cout << "EXPECTED_BROKEN an unsupported faction choice completed the active quest and issued the movie without a faction or objective\n";
#else
    auto choose = [&](LootPlayer& player, uint32 faction)
    {
        WorldPackets::Character::NeutralPlayerSelectFaction choice{WorldPacket(CMSG_NEUTRAL_PLAYER_SELECT_FACTION)};
        choice.Faction = faction;
        player.GetSession()->HandleNeutralPlayerSelectFaction(choice);
    };
    auto requireNeutral = [](LootPlayer& player)
    {
        Require(player.getRace() == RACE_PANDAREN_NEUTRAL && !player.HasPendingTeleport() &&
            !player.isWatchingMovie() && !player.GetQuestObjectiveData(NewFate, 64594), "An inadmissible selection changed faction, transfer or credit");
    };
    choose(pupil, 2);
    requireNeutral(pupil);
    MoveGilneasPlayer(pupil, giver.GetPosition());
    choose(pupil, 1);
    requireNeutral(pupil);

    visitor.SetRewardedQuest(Healing);
    MoveGilneasPlayer(visitor, shang.GetPosition());
    choose(visitor, 0);
    requireNeutral(visitor);
    AcceptSouth(visitor, giver, NewAllies);
    RewardSouth(visitor, shang, NewAllies);
    AcceptSouth(visitor, shang, NewFate);
    visitor.CompleteQuest(NewFate);
    Require(visitor.GetQuestStatus(NewFate) == QUEST_STATUS_COMPLETE && !visitor.GetQuestObjectiveData(NewFate, 64594),
        "The legacy complete character could not be prepared");

    auto finish = [&](LootPlayer& player, uint32 faction, uint8 race, uint32 mapId, uint32 area, uint32 language)
    {
        MoveGilneasPlayer(player, shang.GetPosition());
        WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};
        hello.Unit = shang.GetGUID();
        player.GetSession()->HandleGossipHelloOpcode(hello);
        auto const& menu = player.PlayerTalkClass->GetGossipMenu();
        Require(menu.GetMenuId() == 13726 && menu.GetItem(0) && !menu.GetItem(1) && !menu.GetItem(2), "Neutral Shang Xi gossip did not publish its real option");
        WorldPackets::NPC::GossipSelectOption selection{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
        selection.GossipUnit = shang.GetGUID();
        selection.GossipID = 13726;
        selection.GossipIndex = 0;
        player.GetSession()->HandleGossipSelectOption(selection);
        Require(menu.Empty(), "Selecting the actual option did not consume and close the gossip");

        choose(player, faction);
        std::cout << "CASE faction choice=" << faction << " race=" << uint32(player.getRace()) << " status=" << player.GetQuestStatus(NewFate)
            << " credit=" << player.GetQuestObjectiveData(NewFate, 64594) << " pending=" << player.HasPendingTeleport()
            << " dest=" << player.GetTeleportDest().GetMapId() << " movie=" << player.isWatchingMovie()
            << " bind=" << player.m_homebindMapId << "/" << player.m_homebindAreaId << " language=" << player.HasSpell(language)
            << " Pandaren=" << player.HasSpell(108127) << std::endl;
        Require(player.getRace() == race && player.GetQuestObjectiveData(NewFate, 64594) == 1 &&
            player.GetQuestStatus(NewFate) == QUEST_STATUS_COMPLETE && player.HasPendingTeleport() &&
            player.GetTeleportDest().GetMapId() == mapId && player.isWatchingMovie() &&
            player.m_homebindMapId == mapId && player.m_homebindAreaId == area && player.HasSpell(language) && player.HasSpell(108127),
            "The native faction handoff did not schedule its original capital, language, home bind and objective");
        choose(player, 1 - faction);
        Require(player.getRace() == race && player.GetQuestObjectiveData(NewFate, 64594) == 1 && player.GetTeleportDest().GetMapId() == mapId,
            "A repeated faction selection changed the completed handoff");
        WorldPackets::Misc::CompleteMovie movie{WorldPacket(CMSG_COMPLETE_MOVIE)};
        player.GetSession()->HandleCompleteMovie(movie);
        Require(!player.isWatchingMovie() && player.HasPendingTeleport(), "The movie acknowledgement lost the approved capital transfer");
    };
    finish(pupil, 1, RACE_PANDAREN_ALLIANCE, 0, 9, 668);
    finish(visitor, 0, RACE_PANDAREN_HORDE, 1, 363, 669);
    std::cout << "PASS both native faction selections, stale complete recovery, one objective, matching capital, home and languages\n";
#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
    FinishCapitalArrival(pupil, 0, 338298);
    FinishCapitalArrival(visitor, 1, 189104);
    std::cout << "PASS native capital transfer acknowledgements and original capital quest turn-ins\n";
#endif
#endif
}
#endif

void CheckSouth()
{
#if defined(WANDERING_ISLE_DEPARTURE) && !defined(WANDERING_ISLE_DEPARTURE_BASELINE)
    FarewellMapFixture capitals;
#endif
    Map map(860, 305, 0, DIFFICULTY_NONE);
    auto& korga = FindSouthActor(map, 245264);
    auto& wei = FindSouthActor(map, 245266);
    auto& makael = FindSouthActor(map, 245384);
    LootPlayer pupil(SupplySession(), map, 2979501, korga.GetPosition());
    LootPlayer visitor(SupplySession(), map, 2979502, korga.GetPosition());
    for (auto player : {&pupil, &visitor})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_PANDAREN_NEUTRAL);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_PANDAREN_NEUTRAL);
        player->InitDisplayIds();
        player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
        player->SetLevel(11);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 20000);
    }
    pupil.SetRewardedQuest(Bidden);
    CheckSouthSupplies(pupil, visitor, korga, Stocking, Bamboo, 10, 84, true);
#ifdef WANDERING_ISLE_SOUTH_BASELINE
    pupil.SetRewardedQuest(Stocking);
#endif
    AcceptSouth(pupil, korga, Wreck);
    RewardSouth(pupil, makael, Wreck);
    CheckSouthSupplies(pupil, visitor, makael, Handle, Explosives, 6, 35, false);
#ifndef WANDERING_ISLE_SOUTH_BASELINE
    CheckSouthCombat(pupil, wei, Preying, Tiger, 9, 98);
    std::cout << "PASS Makael handoff and nine distinct player or pet tiger defeats with Wei's reward\n";
    auto& aysa = FindSouthActor(map, 245389);
    auto& ji = FindSouthActor(map, 245135);
    CheckSouthCombat(pupil, aysa, EvilFromSeas, DarkenedHorror, 8, 40);
    AcceptSouth(pupil, aysa, UrgentNews);
    RewardSouth(pupil, ji, UrgentNews);
    std::cout << "PASS actual Urgent News handoff from Aysa to Ji at the crash site\n";
#ifdef WANDERING_ISLE_MEDICAL_BASELINE
    CheckSouthSupplies(pupil, visitor, ji, MedicalSupplies, MedicalCrateItem, 8, 31, false, true);
#else
    CheckSouthSupplies(pupil, visitor, ji, MedicalSupplies, MedicalCrateItem, 8, 31, false);
#endif
#ifdef WANDERING_ISLE_RESCUE
    CheckRescue(pupil, visitor, ji);
#endif
#ifdef WANDERING_ISLE_THORN
    CheckThorn(pupil, visitor);
#endif
#ifdef WANDERING_ISLE_HEALING
    CheckHealing(pupil, visitor);
#endif
#ifdef WANDERING_ISLE_DEPARTURE
    CheckDeparture(pupil, visitor);
#endif
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}, {1, {}}, {860, {}}, {975, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}, {1, {}}, {860, {}}, {975, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The native southern quest test requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        LoadSouth();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_go_scripts();
        AddSC_WanderingIsland();
        sScriptMgr->SwapScriptContext(true);
#ifdef WANDERING_ISLE_DEPARTURE
        sScriptDataStore->LoadSpellScriptNames();
        sScriptDataStore->ValidateSpellScripts();
#endif
        CheckSouth();
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
