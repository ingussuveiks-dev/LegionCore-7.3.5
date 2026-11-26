#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "ConditionMgr.h"
#include "CreatureTextMgr.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "ObjectVisitors.hpp"
#include "SpellPackets.h"
#include "Vehicle.h"

void AddSC_go_scripts();
void AddSC_WanderingIsland();

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
    Require(templates && templates->GetRowCount() == 9, "The southern Wandering Isle quest templates are missing");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(templates->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (templates->NextRow());
    auto addons = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID IN (29792,29793,29794,29795,29796,29797,30589,30590,30591)");
    Require(addons && addons->GetRowCount() == 9, "The original southern prerequisites are missing");
    do
    {
        auto fields = addons->Fetch();
        auto quest = quests[fields[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(fields);
        if (quest->PrevQuestID && quests[std::abs(quest->PrevQuestID)])
            quest->prevQuests.push_back(quest->PrevQuestID);
    } while (addons->NextRow());
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN (29793,29794,29795,29797,30590,30591)");
    Require(objectives && objectives->GetRowCount() == 6, "The original forest, bamboo or wreck objectives are missing");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(row[2].GetUInt8() == QUEST_OBJECTIVE_ITEM ? QUEST_SPECIAL_FLAGS_DELIVER : QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    } while (objectives->NextRow());

    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE map=860 AND (guid IN (245135,245137,245264,245266,245384,245389) OR id IN (55946,55999,56007))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest IN (29793,29794,29795,29796,29797,30589,30590,30591)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest IN (29793,29794,29795,29796,29797,30589,30590,30591)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon WHERE entry=55999");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=55999");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_spellclick_spells AS SELECT * FROM legion_world.npc_spellclick_spells WHERE npc_entry=55999");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE vehicle_template_accessory AS SELECT * FROM legion_world.vehicle_template_accessory WHERE EntryOrAura=-105520");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area WHERE spell=56267 AND quest_start=29794");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadPageTexts();
    sObjectMgr->LoadGameObjectTemplate();
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatures();
    sObjectMgr->LoadNPCSpellClickSpells();
    sObjectMgr->LoadVehicleTemplateAccessories();
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellAreas();
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
    tigerLoot += " WHERE Entry IN (55946,56007)";
    (LootTemplates_Creature.*Access(LoadLootTableTag{}))();
    tigerLoot = originalTigerLoot;
    Require(LootTemplates_Creature.GetLootFor(Tiger) && LootTemplates_Creature.GetLootFor(DarkenedHorror), "Missing the original tiger or horror loot");
}

Creature& FindSouthActor(Map& map, uint32 spawn)
{
    auto data = sObjectMgr->GetCreatureData(spawn);
    Require(data, "An original forest actor has no world placement");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(860, data->id, spawn));
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

void CheckSouth()
{
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{860, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{860, {}}});
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
