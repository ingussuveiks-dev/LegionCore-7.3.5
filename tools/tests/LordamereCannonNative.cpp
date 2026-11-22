#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "QuestObjectScope.h"
#include "SpellTargetInfo.h"
#include "Vehicle.h"

void AddSC_silverpine_forest();

namespace
{
constexpr uint32 Defense = 27232;
constexpr uint32 Cannon = 45263;
constexpr uint32 Worgen = 45270;
constexpr uint32 Launch = 84575;

void ShadowCannonRows(char const* table, char const* restriction)
{
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE cannon_%s AS SELECT * FROM %s WHERE %s",
        table, table, restriction).c_str());
    WorldDatabase.DirectExecute(Trinity::StringFormat("CREATE TEMPORARY TABLE %s AS SELECT * FROM cannon_%s", table, table).c_str());
}

Quest const* LoadDefense()
{
    auto row = WorldDatabase.Query(
#include "NativeLordamereCannon.inc"
    );
    Require(bool(row), "Missing The Waters Run Red quest");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[Defense] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(Defense, quest);
    auto objective = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=27232");
    Require(objective && objective->GetRowCount() == 1 && objective->Fetch()[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER &&
        objective->Fetch()[4].GetUInt32() == Worgen && objective->Fetch()[5].GetUInt32() == 50, "Missing the actual 50-death objective");
    quest->LoadQuestObjective(objective->Fetch());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);
    Require(bool(WorldDatabase.Query("SELECT 1 FROM gameobject g JOIN gameobject_queststarter s ON s.id=g.id "
        "WHERE s.quest=27232 AND g.id=205350 AND g.map=0")), "Missing the original Horde Communication Panel");
    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadItemTemplateAddon();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_silverpine_forest();
    sScriptMgr->SwapScriptContext(true);
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    ShadowCannonRows("conditions", "SourceTypeOrReferenceId=18 AND SourceGroup=45263");
    ShadowCannonRows("spell_script_names", "spell_id IN(84574,84575)");
    ShadowCannonRows("npc_spellclick_spells", "npc_entry=45263");
    ShadowCannonRows("creature_questender", "quest=27232");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sObjectMgr->LoadNPCSpellClickSpells();
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    sConditionMgr->LoadConditions();
    auto rocket = sSpellMgr->GetSpellInfo(Launch);
    Require(rocket && rocket->GetEffect(EFFECT_0)->TriggerSpell == 84574, "Missing native Rocket Blast missile and impact");
    return quest;
}

struct DockCannon : Creature
{
    DockCannon(Map& map, uint32 guid, Position const& point)
    {
        m_DBTableGuid = guid;
        Require(Create(guid, &map, 1, Cannon, 0, HORDE, point.GetPositionX(), point.GetPositionY(), point.GetPositionZ(), point.GetOrientation()) &&
            map.AddToMap(static_cast<Creature*>(this)), "Could not publish the original dock cannon");
        Require(GetVehicleKit() && GetVehicleKit()->GetVehicleInfo()->ID == 1094 && AI(), "Missing loaded cannon vehicle or controller");
    }

    ~DockCannon()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
        ResetMap();
    }
};

std::list<Creature*> GetSwimmers(DockCannon& cannon)
{
    std::list<Creature*> nearby;
    cannon.GetCreatureListWithEntryInGrid(nearby, Worgen, 270.0f);
    nearby.remove_if([&](Creature* creature)
    {
        auto summon = creature->ToTempSummon();
        return !summon || !summon->IsAlive() || summon->GetSummonerGUID() != cannon.GetGUID();
    });
    return nearby;
}

void CheckDefense(Quest const* quest)
{
    auto rows = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature WHERE id=45263 "
        "AND map=0 AND areaId=1338 ORDER BY guid");
    Require(rows && rows->GetRowCount() == 3, "Expected all three original dock cannon placements");
    auto placements = std::vector<std::pair<uint32, Position>>{};
    do
    {
        auto row = rows->Fetch();
        placements.emplace_back(row[0].GetUInt32(), Position{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()});
    } while (rows->NextRow());
    Map map(0, 300, 0, DIFFICULTY_NONE);
    DockCannon cannon(map, placements[0].first, placements[0].second);
    DockCannon second(map, placements[1].first, placements[1].second);
    LootPlayer owner(SupplySession(), map, 27232, placements[0].second), other(SupplySession(), map, 27233, placements[1].second);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_UNDEAD_PLAYER);
        player->setFactionForRace(RACE_UNDEAD_PLAYER);
        player->InitDisplayIds();
        player->SetLevel(13);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 20000);
    }
    Require(!cannon.HandleSpellClick(&owner) && !owner.GetVehicle() && GetSwimmers(cannon).empty(),
        "A player without the quest accessed the cannon route");
    owner.AddQuestAndCheckCompletion(quest, nullptr);
    other.AddQuestAndCheckCompletion(quest, nullptr);
    Require(cannon.HandleSpellClick(&owner) && second.HandleSpellClick(&other), "The actual dock boarding spell failed");
    Tick(owner, 500);
    Tick(other, 500);
    Require(cannon.GetVehicleKit()->GetPassenger(0) == &owner && second.GetVehicleKit()->GetPassenger(0) == &other,
        "The eligible riders did not enter real control seats");
    Require(cannon.IsAIEnabled && second.IsAIEnabled, "Vehicle control suspended a native wave controller");
    cannon.Update(1000);
    second.Update(1000);
    auto firstWave = GetSwimmers(cannon);
    auto secondWave = GetSwimmers(second);
    Require(firstWave.size() == 3 && secondWave.size() == 3, "Each real rider must create a separate three-swimmer wave");
    for (Creature* swimmer : firstWave)
    {
        auto scope = swimmer->GetQuestObjectScope().lock();
        Require(scope && scope->Owner == owner.GetGUID() &&
            swimmer->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING), "Swimmer lacks the rider scope or real water movement");
    }
    auto target = firstWave.front();
    owner.DealDamage(target, target->GetMaxHealth() * 2);
    Require(target->IsAlive() && !owner.GetQuestObjectiveData(Defense, Worgen), "An on-foot attack could substitute for Rocket Blast");

    auto advanceSwimmers = [](std::list<Creature*> const& swimmers)
    {
        for (uint32 elapsed = 0; elapsed < 25000; elapsed += 50)
            for (Creature* swimmer : swimmers)
                swimmer->Unit::Update(50);
    };
    Position startPoint(target->GetPosition());
    advanceSwimmers(firstWave);
    advanceSwimmers(secondWave);
    Require(target->GetExactDist2d(startPoint) > 25.0f, "The real motion spline did not move the first swimmer");

    auto launch = [](Creature& fireFrom, Position const& impact)
    {
        SpellCastTargets targets;
        fireFrom.SetOrientation(fireFrom.GetAngle(impact));
        targets.SetSrc(fireFrom);
        targets.SetDst(impact);
        targets.SetPitch(0.05f);
        targets.SetSpeed(70.0f);
        Require(fireFrom.CastSpell(targets, sSpellMgr->GetSpellInfo(Launch), nullptr) == SPELL_CAST_OK,
            "The native Rocket Blast launch was rejected");
    };
    auto advanceShot = [&](Creature& fireFrom, uint32 milliseconds)
    {
        for (uint32 elapsed = 0; elapsed < milliseconds; elapsed += 10)
        {
            fireFrom.Unit::Update(10);
            owner.Unit::Update(10);
        }
    };
    Position targetPoint(target->GetPosition());
    launch(cannon, targetPoint);
    advanceShot(cannon, 6000);
    auto count = owner.GetQuestObjectiveData(Defense, Worgen);
    std::cout << "CASE first native launch credit=" << count << " target-alive=" << target->IsAlive()
        << " health=" << target->GetHealth() << " progress=" << unsigned(owner.GetQuestStatus(Defense))
        << " distance=" << cannon.GetExactDist2d(targetPoint) << " hostile=" << cannon.IsHostileTo(target)
        << " allowed-target=" << cannon.IsValidAttackTarget(target) << " charmer=" << cannon.GetCharmerGUID().GetCounter()
        << " range=" << sSpellMgr->GetSpellInfo(Launch)->GetMaxRange(false, &cannon) << std::endl;
    Require(count > 0 && count <= 3 && other.GetQuestObjectiveData(Defense, Worgen) == 0 && GetSwimmers(second).size() == 3,
        "Native rocket did not credit its rider or spread progress to the other cannon");
    map.RemoveAllObjectsInRemoveList();
    other.ExitVehicle();
    map.RemoveAllObjectsInRemoveList();
    Require(!other.GetVehicle() && GetSwimmers(second).empty(), "Dismount left a private wave active");

    launch(cannon, targetPoint);
    advanceShot(cannon, 10);
    owner.ExitVehicle();
    map.RemoveAllObjectsInRemoveList();
    Require(!owner.GetVehicle() && cannon.HandleSpellClick(&owner), "The first rider could not retry the same cannon");
    Tick(owner, 500);
    cannon.Update(1000);
    auto retryWave = GetSwimmers(cannon);
    Require(retryWave.size() == 3, "Retry did not start a new first wave");
    advanceSwimmers(retryWave);
    advanceShot(cannon, 6000);
    Require(owner.GetQuestObjectiveData(Defense, Worgen) == count && GetSwimmers(cannon).size() == 3,
        "An earlier missile could damage the new ride's waves");

    for (uint32 wave = 0; wave < 30 && !owner.CanRewardQuest(quest, false); ++wave)
    {
        cannon.Update(5000);
        auto active = GetSwimmers(cannon);
        Require(!active.empty(), "The next wave had no actual swimming creature");
        advanceSwimmers(active);
        launch(cannon, active.front()->GetPosition());
        advanceShot(cannon, 6000);
        map.RemoveAllObjectsInRemoveList();
    }
    Require(owner.GetQuestObjectiveData(Defense, Worgen) == 50 && owner.CanRewardQuest(quest, false), "Fifty actual cannon deaths did not complete the quest");
    cannon.Update(1000);
    map.RemoveAllObjectsInRemoveList();
    Require(!owner.GetVehicle() && GetSwimmers(cannon).empty() && !cannon.HandleSpellClick(&owner),
        "Completion did not restore player control and clean up the wave");
    AbandonSupply(other, Defense);
    Require(!second.HandleSpellClick(&other), "Abandon did not remove access to a cannon");
    auto giver = WorldDatabase.Query("SELECT guid,position_x,position_y,position_z,orientation FROM creature "
        "WHERE id=44916 AND map=0 AND areaId=5386");
    Require(giver && giver->GetRowCount() == 1, "Missing Admiral Hatchet's original placement");
    auto row = giver->Fetch();
    Position receiverPoint{row[1].GetFloat(),row[2].GetFloat(),row[3].GetFloat(),row[4].GetFloat()};
    SupplyMove(owner, receiverPoint);
    Actor receiver(&map, 44916, row[0].GetUInt32(), receiverPoint);
    Require(map.AddToMap(static_cast<Creature*>(&receiver)), "Could not publish Admiral Hatchet");
    uint32 reward = 0;
    if (auto choices = sDB2Manager.GetQuestPackageItems(quest->PackageID))
        for (auto choice : *choices)
            if (owner.CanSelectQuestPackageItem(choice))
            {
                reward = choice->ItemID;
                break;
            }
    Require(reward, "Missing an eligible DB2 reward package item");
    WorldPackets::Quest::QuestGiverChooseReward packet{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    packet.QuestGiverGUID = receiver.GetGUID();
    packet.QuestID = Defense;
    packet.ItemChoiceID = reward;
    owner.GetSession()->HandleQuestGiverChooseReward(packet);
    Require(owner.GetQuestRewardStatus(Defense) && owner.GetItemCount(reward), "Admiral Hatchet did not accept the completed defense");
    std::cout << "PASS actual boarding, private swimming waves, stale missile rejection, 50 native Rocket Blast deaths and the original reward\n";
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
        sSpellMgr->LoadSpellBonusess();
        sSpellMgr->LoadSpellThreats();
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{0, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{0, {}}});
        LoadGameObjectModelList(argv[2]);
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "The cannon test requires the isolated character schema");
        auto quest = LoadDefense();
        sObjectMgr->LoadPlayerInfo();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckDefense(quest);
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
