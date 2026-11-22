#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "GilneasQuestNative.h"
#include "ScriptsData.h"
#include "Vehicle.h"

void AddSC_silverpine_forest();

namespace
{
constexpr uint32 QuestId = 27088;
constexpr uint32 Previous = 27082;
constexpr uint32 Cage = 60808;
constexpr uint32 Release = 83902;
constexpr uint32 EttinEntry = 44367;
constexpr uint32 EttinSpawn = 128656;
constexpr uint32 WormcrudSpawn = 129267;
constexpr uint32 ChickenEntry = 44935;

Quest const* LoadQuest(bool baseline)
{
    auto rows = WorldDatabase.Query(
#include "NativeForestEttin.inc"
    );
    Require(rows && rows->GetRowCount() == 2, "Missing the original chicken cage quest");
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    do
    {
        auto quest = new Quest(rows->Fetch());
        quests[quest->Id] = quest;
        const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(quest->Id, quest);
    } while (rows->NextRow());
    auto quest = quests[QuestId];
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=27088");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[4].GetUInt32() == EttinEntry &&
        objective->Fetch()[5].GetUInt32() == 1 && quest->SourceItemId == Cage, "The original kill and cage changed");
    quest->LoadQuestObjective(objective->Fetch());
    auto addon = WorldDatabase.Query("SELECT ID,MaxLevel,AllowableClasses,SourceSpellID,PrevQuestID,NextQuestID,ExclusiveGroup,"
        "RewardMailTemplateID,RewardMailDelay,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction,"
        "RequiredMinRepValue,RequiredMaxRepValue,ProvidedItemCount,SpecialFlags,RewardMailTitle FROM quest_template_addon WHERE ID=27088");
    Require(addon && addon->Fetch()[4].GetUInt32() == Previous, "The original Apothecary route changed");
    quest->LoadQuestTemplateAddon(addon->Fetch());
    quest->prevQuests.push_back(Previous);
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL);
    auto vehicle = sVehicleStore.LookupEntry(1063);
    Require(vehicle && vehicle->SeatID[0] == 8429 && vehicle->SeatID[1] == 8430, "The matching 7.3.5 Ettin seats changed");

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sObjectMgr->LoadPlayerInfo();
    sObjectMgr->LoadEquipmentTemplates();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(128656,129267)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=27088");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=27088");
    if (!baseline)
        WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id IN(83902,83903,83904)");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    if (!baseline)
    {
        sScriptDataStore->LoadSpellScriptNames();
        sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
        AddSC_silverpine_forest();
        sScriptMgr->SwapScriptContext(true);
    }
    sObjectMgr->LoadCreatures();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    return quest;
}

struct ForestActor : Creature
{
    ForestActor(Map& map, uint32 spawn) : Creature(false)
    {
        auto data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Missing the actual forest actor placement");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true), "Could not load the actual forest actor");
    }

    ~ForestActor()
    {
        CombatStop(true);
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

struct ForestPet : LootPet
{
    ForestPet(LootPlayer& owner, Map& map) : LootPet(owner, map)
    {
        m_ControlledByPlayer = true;
        SetLevel(owner.getLevel());
        setFaction(owner.getFaction());
    }
};

void CheckEttin(Quest const* quest, bool baseline)
{
    for (uint32 spawn : {EttinSpawn, WormcrudSpawn})
        sObjectMgr->RemoveCreatureFromGrid(spawn, sObjectMgr->GetCreatureData(spawn));
    Map map(0, 649, 0, DIFFICULTY_NONE);
    ForestActor wormcrud(map, WormcrudSpawn), ettin(map, EttinSpawn);
    LootPlayer owner(SupplySession(), map, 2708801, wormcrud.GetPosition());
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
    owner.SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
    owner.setFactionForRace(RACE_UNDEAD_PLAYER);
    owner.InitDisplayIds();
    owner.GetReputationMgr().LoadFromDB({});
    owner.SetLevel(15);
    owner.SetMaxHealth(50000);
    owner.SetHealth(50000);
    owner.SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
    for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE})
        owner.SetFloatValue(field, 1.0f);
    auto accept = [&]
    {
        MoveGilneasPlayer(owner, wormcrud.GetPosition());
        WorldPackets::Quest::QuestGiverAcceptQuest packet{WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST)};
        packet.QuestGiverGUID = wormcrud.GetGUID();
        packet.QuestID = QuestId;
        owner.GetSession()->HandleQuestGiverAcceptQuest(packet);
    };
    auto use = [&](Unit* target) { return owner.CastSpell(target, Release, false, owner.GetItemByEntry(Cage)); };
    accept();
    Require(!owner.GetItemCount(Cage), "Wormcrud granted an unearned cage");
    owner.SetRewardedQuest(Previous);
    accept();
    Require(owner.GetItemCount(Cage) == 1, "Wormcrud did not supply the actual cage");
    if (!baseline)
        Require(use(&wormcrud) != SPELL_CAST_OK, "The cage accepted a non-Ettin target");
    Position nearby = ettin.GetNearPosition(5.0f, 0);
    MoveGilneasPlayer(owner, nearby);
    auto health = ettin.GetHealth();
    Require(use(&ettin) == SPELL_CAST_OK, "The actual cage did not start on the placed Forest Ettin");
    Tick(owner, 750);
    auto chicken = [&]() -> Creature*
    {
        return map.GetCreature(owner.m_SummonSlot[6]);
    };
    Require(chicken() && chicken()->GetEntry() == ChickenEntry, "The native cage did not produce its real summon");
    if (baseline)
    {
        Require(!ettin.GetVehicleKit() && ettin.GetHealth() == health, "The original database unexpectedly had a feeding vehicle");
        std::cout << "EXPECTED_BROKEN original cage released a chicken with no Ettin feeding vehicle\n";
        return;
    }

    bool arm = false, mouth = false;
    auto advance = [&](uint32 duration)
    {
        for (uint32 time = 0; time < duration; time += 20)
        {
            Tick(owner, 20);
            if (Creature* actor = chicken())
                actor->Update(20);
            ettin.Update(20);
            map.RemoveAllObjectsInRemoveList();
            if (Vehicle* kit = ettin.GetVehicleKit())
            {
                arm |= kit->GetPassenger(0) && kit->GetPassenger(0) == chicken();
                mouth |= kit->GetPassenger(1) && kit->GetPassenger(1) == chicken();
            }
        }
    };
    Require(use(&ettin) != SPELL_CAST_OK, "A second chicken replaced an active feed");
    AbandonSupply(owner, QuestId);
    accept();
    advance(3000);
    Require(!chicken() && ettin.GetHealth() == health && !owner.GetQuestObjectiveData(QuestId, EttinEntry),
        "A canceled feed advanced the new quest attempt");
    owner.RemoveSpellCooldown(Release, true);
    owner.GetGlobalCooldownMgr().CancelGlobalCooldown(sSpellMgr->GetSpellInfo(Release));
    MoveGilneasPlayer(owner, nearby);
    auto restart = use(&ettin);
    std::cout << "CASE restart=" << uint32(restart) << " quest=" << owner.GetQuestStatus(QuestId)
        << " cage=" << owner.GetItemCount(Cage) << " distance=" << owner.GetDistance(&ettin) << '\n';
    Require(restart == SPELL_CAST_OK, "A restarted attempt could not release a chicken");
    advance(15000);
    std::cout << "CASE arm=" << arm << " mouth=" << mouth << " health=" << ettin.GetHealth() << '/' << health
        << " poison=" << ettin.HasAura(83903) << " chicken=" << bool(chicken()) << '\n';
    Require(arm && mouth && !chicken() && ettin.IsAlive() && ettin.GetHealth() < health / 2 && ettin.HasAura(83903),
        "The real arm and mouth seats did not produce the weakening explosion");
    Require(!owner.GetQuestObjectiveData(QuestId, EttinEntry), "Feeding awarded credit without a death");
    ForestPet pet(owner, map);
    pet.DealDamage(&ettin, uint32(ettin.GetMaxHealth()) * 2);
    Tick(owner, 100);
    Require(ettin.isDead(false) && owner.CanRewardQuest(quest, false) && owner.GetQuestObjectiveData(QuestId, EttinEntry) == 1,
        "A pet's killing blow did not record the native Ettin objective");
    MoveGilneasPlayer(owner, wormcrud.GetPosition());
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
    Require(choice && owner.CanRewardQuest(quest, choice, false), "The genuine Wormcrud package was unavailable");
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = wormcrud.GetGUID();
    reward.ItemChoiceID = choice;
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId) && !owner.GetItemCount(Cage), "Wormcrud did not accept the native turn-in");
    std::cout << "PASS real cage, genuine arm and mouth boarding, native poison, pet kill and Wormcrud turn-in\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The Ettin check requires isolated characters");
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
#ifdef FOREST_ETTIN_BASELINE
        CheckEttin(LoadQuest(true), true);
#else
        CheckEttin(LoadQuest(false), false);
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
