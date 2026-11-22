#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "SmartScriptMgr.h"
#include "Vehicle.h"

void AddSC_kezan();
void AddSC_SmartScripts();

namespace
{
constexpr uint32 QuestId = 14071;
constexpr uint32 Previous = 28349;
constexpr uint32 Keys = 46856;
constexpr uint32 KeysCredit = 48323;
constexpr uint32 UseKeys = 66393;
constexpr uint32 Rod = 34840;
constexpr uint32 MegsSpawn = 189554;
constexpr uint32 FriendSpawns[] = {240082, 240081, 240080};
constexpr uint32 Crew[] = {34957, 34958, 34959};

Quest const* LoadRide()
{
    auto rows = WorldDatabase.Query(
#include "NativeKezanHotRod.inc"
    );
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    Require(rows && rows->GetRowCount() == 2, "Missing the Kezan route or Megs in Marketing");
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto quest = quests[QuestId];
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=14071 ORDER BY StorageIndex");
    Require(objectives && objectives->GetRowCount() == 4, "Missing Hot Rod key or passenger objectives");
    for (uint32 entry : {48323, 34959, 34957, 34958})
    {
        auto row = objectives->Fetch();
        Require(row[4].GetUInt32() == entry && row[5].GetUInt32() == 1, "An original Hot Rod objective changed");
        quest->LoadQuestObjective(row);
        objectives->NextRow();
    }
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=14071");
    Require(addon && addon->Fetch()[4].GetUInt32() == Previous && quest->SourceItemId == Keys && quest->PackageID == 571,
        "The original Kezan prerequisite, key or package changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Previous);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellProcEvents();
    sSpellMgr->LoadSpellProcs();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(189554,240082,240081,240080)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=14071");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=14071");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_template_addon AS SELECT * FROM legion_world.creature_template_addon "
        "WHERE entry IN(34890,34892,34954)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts "
        "WHERE source_type=0 AND entryorguid IN(34840,34957,34958,34959)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sSmartScriptMgr->LoadSmartAIFromDB();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_SmartScripts();
    AddSC_kezan();
    sScriptMgr->SwapScriptContext(true);
    sObjectMgr->LoadCreatureTemplateAddons();
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct KezanActor : Creature
{
    KezanActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Missing original Kezan actor placement");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the actual Kezan actor");
    }

    ~KezanActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

void CheckRide(Quest const* quest, bool baseline)
{
    for (uint32 spawn : {MegsSpawn, 240082u, 240081u, 240080u})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(648, 654, 0, DIFFICULTY_NONE);
    KezanActor megs(map, MegsSpawn);
    std::array<KezanActor, 3> friends{{{map, FriendSpawns[0]}, {map, FriendSpawns[1]}, {map, FriendSpawns[2]}}};
    LootPlayer owner(SupplySession(), map, 1407101, megs.GetPosition());
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_GOBLIN);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_ROGUE);
    owner.setFactionForRace(RACE_GOBLIN);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(5);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 10000);
    owner.SetPhaseMask(megs.GetPhaseMask(), false);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        owner.SetFloatValue(field, 1.0f);

    auto accept = [&]
    {
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = megs.GetGUID();
        packet.QuestID = QuestId;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    accept();
    Require(!owner.GetItemCount(Keys), "Megs gave a key without the prerequisite");
    owner.SetRewardedQuest(Previous);
    accept();
    Require(owner.GetItemCount(Keys) == 1 && owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE,
        "Megs did not supply the actual key");

    auto decoy = owner.SummonCreature(Rod, owner.GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 15000);
    Require(decoy && decoy->GetVehicleKit(), "Could not publish an unrelated real vehicle for the case");
    owner.EnterVehicle(decoy, 0, true);
    Tick(owner, 300);
    Require(owner.GetVehicleBase() == decoy, "Could not board the unrelated real vehicle");
    owner.CastSpell(&owner, 66599, true);
    Tick(owner, 300);
    if (baseline)
    {
        Require(owner.GetQuestObjectiveData(QuestId, Crew[1]) == 1, "The original guard did not over-credit the wrong vehicle");
        owner.ExitVehicle();
        std::cout << "EXPECTED_BROKEN Gobber credited immediately on the unrelated vehicle\n";
        return;
    }
    Require(!owner.GetQuestObjectiveData(QuestId, Crew[1]), "Gobber credited the unrelated vehicle");
    owner.ExitVehicle();
    decoy->DespawnOrUnsummon(1);
    decoy->Update(20);
    for (uint32 entry : Crew)
    {
        GuidList unrelatedCrew = *owner.GetSummonList(entry);
        for (ObjectGuid guid : unrelatedCrew)
            if (Creature* passenger = map.GetCreature(guid))
                passenger->Update(2500);
    }
    map.RemoveAllObjectsInRemoveList();
    Tick(owner, 300);

    auto start = [&]
    {
        owner.RemoveSpellCooldown(UseKeys, true);
        owner.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(UseKeys));
        Require(owner.CastSpell(&owner, UseKeys, false, owner.GetItemByEntry(Keys)) == SPELL_CAST_OK, "The actual key failed to summon the Hot Rod");
        Tick(owner, 1200);
        Unit* base = owner.GetVehicleBase();
        Creature* vehicle = base ? base->ToCreature() : nullptr;
        Require(vehicle && vehicle->GetEntry() == Rod && vehicle->GetVehicleKit()->GetPassenger(0) == &owner &&
            vehicle->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == UseKeys, "The key did not seat its owner in the actual Hot Rod");
        return vehicle;
    };
    auto rod = start();
    Require(owner.GetQuestObjectiveData(QuestId, KeysCredit) == 1, "A confirmed driver seat did not record key use");
    owner.CastSpell(&owner, 66597, true);
    GuidList unsolicitedCrew = *owner.GetSummonList(Crew[0]);
    for (ObjectGuid guid : unsolicitedCrew)
        if (Creature* passenger = map.GetCreature(guid))
            passenger->Unit::Update(250);
    Require(!rod->GetVehicleKit()->GetPassenger(1) && !owner.GetQuestObjectiveData(QuestId, Crew[0]),
        "An unsolicited cast in the real car supplied an original pickup");
    map.RemoveAllObjectsInRemoveList();
    for (uint8 index = 0; index < friends.size(); ++index)
    {
        KezanActor& actor = friends[index];
        rod->Relocate(actor.GetPosition());
        rod->GetVehicleKit()->RelocatePassengers();
        Require(owner.canSeeOrDetect(&actor), "The original nearby friend was invisible to the actual driver");
        actor.AI()->MoveInLineOfSight_Safe(rod);
        GuidList passengers = *owner.GetSummonList(Crew[index]);
        for (ObjectGuid guid : passengers)
            if (Creature* passenger = map.GetCreature(guid))
                for (uint32 elapsed = 0; elapsed < 800 && !passenger->IsDespawn() && !passenger->GetVehicleBase(); elapsed += 20)
                    passenger->Update(20);
        Tick(owner, 300);
        Unit* passenger = rod->GetVehicleKit()->GetPassenger(index + 1);
        std::cout << "CASE seat=" << (index + 1) << " entry=" << (passenger ? passenger->GetEntry() : 0)
            << " owned=" << passengers.size() << " objective=" << owner.GetQuestObjectiveData(QuestId, Crew[index]) << '\n';
        Require(passenger && passenger->GetEntry() == Crew[index] && owner.GetQuestObjectiveData(QuestId, Crew[index]) == 1,
            "An original pickup failed to board and record its assigned seat");
        actor.AI()->MoveInLineOfSight_Safe(rod);
    }
    Require(owner.CanRewardQuest(quest, false), "The actual seats did not make the quest rewardable");
    rod->Relocate(megs.GetPosition());
    rod->GetVehicleKit()->RelocatePassengers();
    uint32 choice = 0;
    if (auto items = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto item : *items)
            if (owner.CanSelectQuestPackageItem(item))
            {
                choice = item->ItemID;
                break;
            }
    if (!choice)
        if (auto items = sDB2Manager.GetQuestPackageItemsFallback(quest->PackageID); items && !items->empty())
            choice = items->front()->ItemID;
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The actual Megs reward package could not be selected");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = megs.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && owner.GetItemCount(choice) == 1, "Megs did not give the selected reward");
    Require(owner.GetItemCount(Keys) == 1, "The actual reward removed the Kezan key needed later");
    owner.ExitVehicle();
    rod->DespawnOrUnsummon(1);
    rod->Update(20);
    map.RemoveAllObjectsInRemoveList();
    Require(start() != nullptr, "The actual car could not be reused after turn-in");
    owner.ExitVehicle();
    std::cout << "PASS native key, original pickups, actual seats, Megs turn-in and later key reuse\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{648, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{648, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Hot Rod check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef KEZAN_HOT_ROD_BASELINE
        CheckRide(LoadRide(), true);
#else
        CheckRide(LoadRide(), false);
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
