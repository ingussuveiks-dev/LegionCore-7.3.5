#define main SupplyChestProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GossipData.h"
#include "GossipDef.h"
#include "NPCPackets.h"
#include "ScriptsData.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"
#include "VehiclePackets.h"

void AddSC_SmartScripts();
void AddSC_generic_spell_scripts();

namespace
{
constexpr uint32 Betrayal = 24592;
constexpr uint32 Flank = 24677;
constexpr uint32 Liberation = 24575;
constexpr uint32 PhaseAura = 68483;
constexpr uint32 SlavesToNoOne = 24674;
constexpr uint32 LiveryAura = 69484;
constexpr uint32 MarcusEntry = 37803;
constexpr uint64 MarcusSpawn = 3101074;
constexpr uint32 HewellEntry = 38764;
constexpr uint64 HewellSpawn = 149553;
constexpr uint32 StoutHorse = 38765;
constexpr uint32 HewellMenu = 11079;

struct GilneasLootPet : LootPet
{
    GilneasLootPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
    }
};

void LoadFlankInputs()
{
    auto rows = WorldDatabase.Query(
#include "NativeGilneasFlank.inc"
    );
    Require(rows && rows->GetRowCount() == 6, "The original Gilneas quest templates are required");
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
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle "
        "FROM quest_template_addon WHERE ID IN(24575,24592,24672,24674,24677)");
    Require(addons && addons->GetRowCount() == 5, "Missing actual Gilneas addon routes");
    do
    {
        auto row = addons->Fetch();
        auto quest = quests[row[0].GetUInt32()];
        quest->LoadQuestTemplateAddon(row);
        if (quest->PrevQuestID)
            quest->prevQuests.push_back(quest->PrevQuestID);
        if (quest->NextQuestID && quests[uint32(quest->NextQuestID)])
            quests[uint32(quest->NextQuestID)]->prevQuests.push_back(quest->Id);
    } while (addons->NextRow());

    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID IN(24575,24592,24674,24677)");
    Require(objectives && objectives->GetRowCount() == 4, "Missing original Gilneas objectives");
    do
    {
        auto row = objectives->Fetch();
        auto quest = quests[row[1].GetUInt32()];
        quest->LoadQuestObjective(row);
        quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    } while (objectives->NextRow());

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature "
        "WHERE guid IN(148907,149547,149553,149554,149591,149605,149656,3101074)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter "
        "WHERE quest IN(24575,24592,24674,24677)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender "
        "WHERE quest IN(24575,24592,24674,24677)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_area AS SELECT * FROM legion_world.spell_area "
        "WHERE (spell=68483 AND area=4714 AND quest_start=14386) OR (spell=69484 AND area=4842 AND quest_start=24677)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_loot_template AS SELECT * FROM legion_world.creature_loot_template "
        "WHERE Entry=37802");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    Require((LootTemplates_Creature.*Access(LoadLootTableTag{}))() > 0, "The original Brothogg corpse loot is required");
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sSpellMgr->LoadSpellAreas();
    sObjectMgr->LoadCreatures();
}

void LoadHewellInputs()
{
    auto summon = sSpellMgr->GetSpellInfo(72772);
    auto forcecast = sSpellMgr->GetSpellInfo(72773);
    auto vehicle = sVehicleStore.LookupEntry(542);
    Require(summon && summon->GetEffect(EFFECT_0)->MiscValue == StoutHorse &&
        summon->GetEffect(EFFECT_0)->MiscValueB == 161 && forcecast &&
        forcecast->GetEffect(EFFECT_0)->TriggerSpell == 72772 && vehicle && vehicle->SeatID[0],
        "The native Hewell spell chain and horse seat are required");

    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE npc_text AS SELECT * FROM legion_world.npc_text WHERE ID=15408");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=11079");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=11079");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE source_type=0 AND entryorguid IN(38764,38765)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions "
        "WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=11079) OR (SourceTypeOrReferenceId=22 AND SourceEntry IN(38764,38765))");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE waypoints AS SELECT * FROM legion_world.waypoints WHERE entry=3876500");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=51254");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadNPCText();
    sGossipDataStore->LoadGossipMenu();
    sGossipDataStore->LoadGossipMenuItems();
    sSmartWaypointMgr->LoadFromDB();
    sSmartScriptMgr->LoadSmartAIFromDB();
    sConditionMgr->LoadConditions();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    AddSC_generic_spell_scripts();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->ValidateSpellScripts();
}

Creature& GilneasActor(Map& map, uint32 entry, uint64 guid)
{
    auto data = sObjectMgr->GetCreatureData(guid);
    Require(data && data->id == entry, "Missing exact original Gilneas creature placement");
    map.LoadGrid(data->posX, data->posY);
    auto actor = map.GetCreature(ObjectGuid::Create<HighGuid::Creature>(map.GetId(), entry, guid));
    Require(actor && actor->IsAlive(), "Native Gilneas creature did not enter the grid");
    return *actor;
}

void MoveGilneas(LootPlayer& player, Position const& point)
{
    auto map = player.GetMap();
    SupplyMove(player, point);
    map->GetZoneAndAreaId(player.m_zoneId, player.m_areaId, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ());
    player.UpdateZoneDependentAuras(player.GetZoneId());
    player.UpdateAreaDependentAuras(player.GetAreaId());
    player.GetPhaseMgr().Recalculate();
    player.GetPhaseMgr().Update();
}

void AcceptGilneas(LootPlayer& player, Creature const& giver, uint32 id)
{
    MoveGilneas(player, giver.GetPosition());
    WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
    packet.QuestGiverGUID = giver.GetGUID();
    packet.QuestID = id;
    player.GetSession()->HandleQuestGiverAcceptQuest(packet);
}

void RewardGilneas(LootPlayer& player, Creature const& receiver, uint32 id, uint32 itemChoice = 0)
{
    MoveGilneas(player, receiver.GetPosition());
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = id;
    packet.ItemChoiceID = itemChoice;
    player.GetSession()->HandleQuestGiverChooseReward(packet);
}

void HewellHello(LootPlayer& player, Creature const& hewell)
{
    MoveGilneas(player, hewell.GetPosition());
    WorldPackets::NPC::Hello hello{WorldPacket(CMSG_TALK_TO_GOSSIP)};
    hello.Unit = hewell.GetGUID();
    player.GetSession()->HandleGossipHelloOpcode(hello);
}

void HewellSelect(LootPlayer& player, Creature const& hewell)
{
    WorldPackets::NPC::GossipSelectOption packet{WorldPacket(CMSG_GOSSIP_SELECT_OPTION)};
    packet.GossipUnit = hewell.GetGUID();
    packet.GossipID = HewellMenu;
    packet.GossipIndex = 0;
    player.GetSession()->HandleGossipSelectOption(packet);
}

void UpdateHorse(LootPlayer& player, ObjectGuid horseGuid, uint32 diff)
{
    auto map = player.GetMap();
    player.Unit::Update(diff);
    if (auto horse = map->GetCreature(horseGuid))
        horse->Update(diff);
    map->MoveAllCreaturesInMoveList();
    map->RemoveAllObjectsInRemoveList();
}

ObjectGuid BoardHewellHorse(LootPlayer& player, Creature const& hewell)
{
    HewellHello(player, hewell);
    Require(player.PlayerTalkClass->GetGossipMenu().GetMenuItemCount() == 1, "Hewell's native ride choice was not offered");
    HewellSelect(player, hewell);
    for (uint32 elapsed = 0; elapsed < 2000 && !player.GetVehicleBase(); elapsed += 20)
        player.Unit::Update(20);
    auto horse = player.GetVehicleCreatureBase();
    Require(horse && horse->GetEntry() == StoutHorse && horse->ToTempSummon() &&
        horse->ToTempSummon()->GetSummonerGUID() == player.GetGUID() && horse->GetVehicleKit()->GetPassenger(0) == &player,
        "The actual forcecast did not board its summoner in the native horse seat");
    return horse->GetGUID();
}

void InterruptHewellRide(LootPlayer& player)
{
    auto seat = player.GetVehicle()->GetSeatForPassenger(&player);
    Require(seat && !seat->CanEnterOrExit(), "The original scripted horseback seat restricts player exits");
    WorldPackets::Vehicle::RequestVehicleExit exitPacket{WorldPacket(CMSG_REQUEST_VEHICLE_EXIT)};
    player.GetSession()->HandleRequestVehicleExit(exitPacket);
    Require(player.GetVehicleBase(), "A manual exit ignored the original scripted seat restriction");
    player.ExitVehicle();
}

void CheckHewell(Map& map, LootPlayer& owner, Creature const& genn, Creature const& lorna, bool baseline)
{
    auto& hewell = GilneasActor(map, HewellEntry, HewellSpawn);
    if (baseline)
    {
        Require(!hewell.HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) &&
            sObjectMgr->GetCreatureTemplate(StoutHorse)->VehicleId == 0,
            "The old world no longer reproduces Hewell's disabled gossip and missing vehicle");
        std::cout << "EXPECTED_BROKEN Hewell had no usable horse choice or vehicle kit\n";
        return;
    }

    LootPlayer early(SupplySession(), map, 2467702, hewell.GetPosition());
    early.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
    early.setFactionForRace(RACE_WORGEN);
    early.InitDisplayIds();
    early.SetLevel(15);
    early.SetRewardedQuest(14386);
    HewellHello(early, hewell);
    Require(!early.PlayerTalkClass->GetGossipMenu().GetMenuItemCount(), "A pre-quest Worgen saw Hewell's ride");
    HewellSelect(early, hewell);
    Require(!early.GetVehicleBase(), "An early rider passed the SmartAI admission");

    auto firstHorse = BoardHewellHorse(owner, hewell);
    HewellSelect(owner, hewell);
    Require(owner.GetVehicleBase() && owner.GetVehicleBase()->GetGUID() == firstHorse, "Repeated selection replaced the actual horse");
    InterruptHewellRide(owner);
    for (unsigned tick = 0; tick < 25 && map.GetCreature(firstHorse); ++tick)
        UpdateHorse(owner, firstHorse, 20);
    Require(!owner.GetVehicleBase() && !map.GetCreature(firstHorse) && owner.GetQuestStatus(Flank) == QUEST_STATUS_COMPLETE,
        "Dismount did not clean up the ride and preserve the normal turn-in");

    auto horseGuid = BoardHewellHorse(owner, hewell);
    Require(horseGuid != firstHorse, "Retry reused an ended ride");
    early.SetRewardedQuest(Betrayal);
    AcceptGilneas(early, genn, Flank);
    auto secondRiderHorse = BoardHewellHorse(early, hewell);
    Require(secondRiderHorse != horseGuid && owner.GetVehicleBase() && owner.GetVehicleBase()->GetGUID() == horseGuid,
        "A second rider displaced the first rider from their horse");
    InterruptHewellRide(early);
    for (unsigned tick = 0; tick < 25 && map.GetCreature(secondRiderHorse); ++tick)
        UpdateHorse(early, secondRiderHorse, 20);
    Require(!early.GetVehicleBase() && !map.GetCreature(secondRiderHorse) && owner.GetVehicleBase() && owner.GetVehicleBase()->GetGUID() == horseGuid,
        "The second rider's exit affected the first ride");
    for (uint32 elapsed = 0; elapsed < 240000 && owner.GetVehicleBase(); elapsed += 100)
        UpdateHorse(owner, horseGuid, 100);
    Require(!owner.GetVehicleBase() && owner.GetDistance2d(&lorna) < 25.0f &&
        owner.GetQuestStatus(Flank) == QUEST_STATUS_COMPLETE && !owner.GetQuestRewardStatus(Flank),
        "The actual route did not unload a still-unrewarded rider at Livery Outpost");
    for (unsigned tick = 0; tick < 25 && map.GetCreature(horseGuid); ++tick)
        UpdateHorse(owner, horseGuid, 20);
    Require(!map.GetCreature(horseGuid), "The completed horse survived its cleanup");
    std::cout << "PASS Hewell gossip, pre-quest and duplicate refusal, actual two-owner seats, interrupted ride cleanup, retry and original route arrival\n";
}

uint32 GilneasRewardChoice(LootPlayer const& player, Quest const& quest)
{
    if (auto rewards = sDB2Manager.GetQuestPackageItems(quest.PackageID))
        for (auto item : *rewards)
            if (player.CanSelectQuestPackageItem(item))
                return item->ItemID;

    if (auto fallback = sDB2Manager.GetQuestPackageItemsFallback(quest.PackageID))
        if (!fallback->empty())
            return fallback->front()->ItemID;

    throw std::runtime_error("The Gilneas reward package has no selectable item");
}

void CheckMarcus(Map& map, LootPlayer& owner, Creature const& lorna)
{
    auto& marcus = GilneasActor(map, MarcusEntry, MarcusSpawn);
    auto& brothogg = GilneasActor(map, 37802, 148907);
    MoveGilneas(owner, marcus.GetPosition());
    float ground = map.GetHeight(marcus.GetPositionX(), marcus.GetPositionY(), marcus.GetPositionZ() + 2.0f);
    Require(owner.GetAreaId() == 4842 && std::abs(ground - marcus.GetPositionZ()) < 2.0f &&
        owner.HasAura(LiveryAura) && owner.InSamePhase(&marcus) && owner.InSamePhase(&lorna),
        "The original Livery terrain and phase must reveal Marcus while retaining Lorna");

    LootPlayer early(SupplySession(), map, 2467402, marcus.GetPosition());
    early.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
    early.setFactionForRace(RACE_WORGEN);
    early.InitDisplayIds();
    early.SetLevel(15);
    early.SetRewardedQuest(14386);
    AcceptGilneas(early, marcus, SlavesToNoOne);
    Require(!early.HasAura(LiveryAura) && !early.InSamePhase(&marcus) && early.GetQuestStatus(SlavesToNoOne) == QUEST_STATUS_NONE,
        "A Worgen outside the Livery quest phase could start Marcus's quest");

    AcceptGilneas(owner, marcus, SlavesToNoOne);
    Require(owner.GetQuestStatus(SlavesToNoOne) == QUEST_STATUS_INCOMPLETE, "Marcus did not offer his original quest");
    MoveGilneas(owner, brothogg.GetPosition());
    Require(owner.GetAreaId() == 4732 && !owner.HasAura(LiveryAura) && owner.HasAura(PhaseAura) && owner.InSamePhase(&brothogg),
        "Leaving Livery must remove its local aura and retain the actual mine phase");
    {
        GilneasLootPet pet(owner, map);
        Require(pet.DealDamage(&brothogg, uint32(brothogg.GetMaxHealth()), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
            brothogg.isDead(false), "The original Brothogg pet kill failed");
        Tick(owner, 300);
    }
    Require(owner.GetQuestObjectiveData(SlavesToNoOne, brothogg.GetEntry()) == 1 &&
        owner.GetQuestStatus(SlavesToNoOne) == QUEST_STATUS_COMPLETE &&
        early.GetQuestStatus(SlavesToNoOne) == QUEST_STATUS_NONE, "The native Brothogg kill did not credit only the eligible owner");
    MoveGilneas(owner, marcus.GetPosition());
    Require(owner.HasAura(LiveryAura) && owner.InSamePhase(&marcus), "The Livery phase was not restored on return");
    uint32 choice = GilneasRewardChoice(owner, *sQuestDataStore->GetQuestTemplate(SlavesToNoOne));
    auto beforeChoice = owner.GetItemCount(choice);
    RewardGilneas(owner, marcus, SlavesToNoOne, choice);
    Require(owner.GetQuestRewardStatus(SlavesToNoOne) && owner.GetItemCount(choice) > beforeChoice,
        "Marcus did not accept the objective and award the actual package item");
    auto afterChoice = owner.GetItemCount(choice);
    RewardGilneas(owner, marcus, SlavesToNoOne, choice);
    Require(owner.GetItemCount(choice) == afterChoice, "Marcus awarded a duplicate reward");
    std::cout << "PASS Marcus native placement, Worgen phase isolation and re-entry, Brothogg pet kill, normal turn-in and exactly-once reward\n";
}

void CheckFlank(bool baseline, bool checkMarcus, bool expectMissingMarcus, bool checkHewell, bool expectMissingHewell)
{
    Map map(654, 616, 0, DIFFICULTY_NONE);
    auto& krennan = GilneasActor(map, 37874, 149547);
    auto& genn = GilneasActor(map, 37876, 149554);
    auto& walden = GilneasActor(map, 37733, 149591);
    auto& ashbury = GilneasActor(map, 37735, 149605);
    auto& lorna = GilneasActor(map, 37783, 149656);
    auto betrayal = sQuestDataStore->GetQuestTemplate(Betrayal);
    auto flank = sQuestDataStore->GetQuestTemplate(Flank);
    auto liberation = sQuestDataStore->GetQuestTemplate(Liberation);
    LootPlayer owner(SupplySession(), map, 2459201, genn.GetPosition());
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
    owner.setFactionForRace(RACE_WORGEN);
    owner.InitDisplayIds();
    owner.SetLevel(15);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    owner.SetRewardedQuest(14386);
    owner.SetRewardedQuest(24672);
    MoveGilneas(owner, genn.GetPosition());
    Require(owner.HasAura(PhaseAura) && owner.InSamePhase(&genn), "The actual Worgen phase must reveal the reward actor");

    if (baseline)
    {
        Require(owner.CanTakeQuest(flank, false) && !owner.GetNextQuest(genn.GetGUID(), betrayal),
            "The original data no longer reproduces the early offer and missing handoff");
        std::cout << "EXPECTED_BROKEN Genn admitted Flank before Betrayal and the native handoff was missing\n";
        return;
    }

    Require(!owner.CanTakeQuest(flank, false) && !owner.CanTakeQuest(liberation, false),
        "A later quest was available before Betrayal");
    AcceptGilneas(owner, genn, Flank);
    Require(owner.GetQuestStatus(Flank) == QUEST_STATUS_NONE, "The early stock accept packet bypassed the prerequisite");
    AcceptGilneas(owner, krennan, Betrayal);
    Require(owner.GetQuestStatus(Betrayal) == QUEST_STATUS_INCOMPLETE, "Krennan did not offer Betrayal after Onwards and Upwards");
    MoveGilneas(owner, walden.GetPosition());
    Require(owner.DealDamage(&walden, uint32(walden.GetMaxHealth()), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
        walden.isDead(false), "The native Walden kill failed");
    Tick(owner, 300);
    Require(owner.GetQuestObjectiveData(Betrayal, walden.GetEntry()) == 1 && owner.GetQuestStatus(Betrayal) == QUEST_STATUS_INCOMPLETE &&
        !owner.CanTakeQuest(flank, false), "One real kill allowed early progression");

    MoveGilneas(owner, ashbury.GetPosition());
    {
        GilneasLootPet pet(owner, map);
        Require(pet.DealDamage(&ashbury, uint32(ashbury.GetMaxHealth()), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false) &&
            ashbury.isDead(false), "The native Ashbury pet kill failed");
        Tick(owner, 300);
    }
    Require(owner.GetQuestObjectiveData(Betrayal, ashbury.GetEntry()) == 1 && owner.GetQuestStatus(Betrayal) == QUEST_STATUS_COMPLETE &&
        !owner.CanTakeQuest(flank, false), "Both kills must complete Betrayal without yet admitting Flank");

    RewardGilneas(owner, genn, Betrayal);
    Require(owner.GetQuestStatus(Betrayal) == QUEST_STATUS_COMPLETE && !owner.CanTakeQuest(flank, false),
        "Rejecting an absent choice must retain both kills and the prerequisite");
    auto reward = GilneasRewardChoice(owner, *betrayal);
    auto beforeChoice = owner.GetItemCount(reward);
    RewardGilneas(owner, genn, Betrayal, reward);
    Require(owner.GetQuestRewardStatus(Betrayal) && owner.CanTakeQuest(flank, false) && owner.GetNextQuest(genn.GetGUID(), betrayal) == flank,
        "The actual Betrayal reward did not make Genn's next quest available");
    Require(owner.GetItemCount(reward) > beforeChoice, "The actual package reward was not stored");
    AcceptGilneas(owner, genn, Flank);
    Require(owner.GetQuestStatus(Flank) == QUEST_STATUS_COMPLETE && !owner.CanTakeQuest(liberation, false),
        "The actual Flank travel quest did not become ready for its receiver");
    if (checkHewell || expectMissingHewell)
        CheckHewell(map, owner, genn, lorna, expectMissingHewell);
    RewardGilneas(owner, lorna, Flank);
    Require(owner.GetQuestRewardStatus(Flank) && owner.CanTakeQuest(liberation, false) && owner.GetNextQuest(lorna.GetGUID(), flank) == liberation,
        "Lorna did not accept the real Flank turn-in or expose Liberation Day");
    if (checkHewell)
    {
        auto& hewell = GilneasActor(map, HewellEntry, HewellSpawn);
        HewellHello(owner, hewell);
        HewellSelect(owner, hewell);
        Require(!owner.PlayerTalkClass->GetGossipMenu().GetMenuItemCount() && !owner.GetVehicleBase(),
            "A player who turned in Flank could still take Hewell's ride");
    }
    std::cout << "PASS on real Gilneas actors: both player and pet kills, early packet refusal, Genn reward and offer, Lorna reward and Liberation Day\n";
    if (expectMissingMarcus)
    {
        MoveGilneas(owner, lorna.GetPosition());
        Require(!sObjectMgr->GetCreatureData(MarcusSpawn) && !owner.HasAura(LiveryAura),
            "The old world no longer reproduces the missing Marcus and phase");
        std::cout << "EXPECTED_BROKEN Marcus was unavailable after the actual Flank turn-in\n";
    }
    else if (checkMarcus)
        CheckMarcus(map, owner, lorna);
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{654, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{654, {}}});
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        auto characters = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth" &&
            characters.substr(characters.find_last_of(';') + 1) == "fixture_char", "This probe requires private login and character schemas");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        CharacterDatabase.SetConnectionInfo(characters, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        LoadFlankInputs();
        bool checkHewell = sConfigMgr->GetBoolDefault("NativeGilneasFlank.CheckHewell", false);
        if (checkHewell)
            LoadHewellInputs();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<Pet>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFlank(sConfigMgr->GetBoolDefault("NativeGilneasFlank.ExpectOldAdmission", false),
            sConfigMgr->GetBoolDefault("NativeGilneasFlank.CheckMarcus", false),
            sConfigMgr->GetBoolDefault("NativeGilneasFlank.ExpectMissingMarcus", false), checkHewell,
            sConfigMgr->GetBoolDefault("NativeGilneasFlank.ExpectMissingHewell", false));
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
