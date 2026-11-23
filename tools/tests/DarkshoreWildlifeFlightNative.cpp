#define main SupplyChestsProbeMain
#include "QuestSupplyChestsNative.cpp"
#undef main

#include "DarkshoreWildlifeFlight.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "GilneasQuestNative.h"
#include "MoveSpline.h"
#include "Vehicle.h"

void AddSC_darkshore();

namespace
{
using namespace DarkshoreWildlifeFlight;
constexpr uint32 Giver = 122236;
constexpr uint32 Menu = 10509;

struct FlightActor : Creature
{
    FlightActor(::Map& map, uint32 spawn) : Creature(false)
    {
        CreatureData const* data = sObjectMgr->GetCreatureData(spawn);
        Require(data, "Original Darkshore actor placement is missing");
        map.LoadGrid(data->posX, data->posY);
        Require(LoadCreatureFromDB(spawn, &map, true) && IsInWorld(), "Original Darkshore actor did not load");
    }

    ~FlightActor()
    {
        if (IsInWorld())
            GetMap()->RemoveFromMap(static_cast<Creature*>(this), false);
    }
};

Quest const* LoadFlightInputs()
{
    auto row = WorldDatabase.Query(
#include "NativeDarkshoreWildlifeFlight.inc"
    );
    Require(bool(row), "Missing In Defense of Darkshore");
    auto quest = new Quest(row->Fetch());
    auto& quests = sQuestDataStore->*Access(QuestVectorTag{});
    quests.resize(45089);
    sQuestDataStore->*Access(QuestMaxTag{}) = uint32(quests.size());
    quests[QuestId] = quest;
    const_cast<QuestMap&>(sQuestDataStore->GetQuestTemplates()).emplace(QuestId, quest);
    auto objectives = WorldDatabase.Query("SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Description,Bugged "
        "FROM quest_objectives WHERE QuestID=13885 ORDER BY ID");
    Require(objectives && objectives->GetRowCount() == 3, "Missing the actual three wildlife objectives");
    do
    {
        auto fields = objectives->Fetch();
        Require(fields[2].GetUInt8() == QUEST_OBJECTIVE_MONSTER && fields[5].GetInt32() == 8, "Native wildlife objective changed");
        quest->LoadQuestObjective(fields);
    } while (objectives->NextRow());
    quest->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST);

    sObjectMgr->LoadItemTemplates();
    sObjectMgr->LoadPlayerInfo();
    sSpellMgr->LoadSpellBonusess();
    sSpellMgr->LoadSpellThreats();
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature AS SELECT * FROM legion_world.creature WHERE guid IN(122236,122516) OR "
        "(map=1 AND id IN(2165,34318,34396,2070,2071,2237) AND position_x BETWEEN 4500 AND 5700 AND position_y BETWEEN 0 AND 550)");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu AS SELECT * FROM legion_world.gossip_menu WHERE MenuID=10509");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gossip_menu_option AS SELECT * FROM legion_world.gossip_menu_option WHERE MenuID=10509");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE conditions AS SELECT * FROM legion_world.conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=10509");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_target_position AS SELECT * FROM legion_world.spell_target_position WHERE id=65372");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE spell_script_names AS SELECT * FROM legion_world.spell_script_names WHERE spell_id=65373");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_queststarter AS SELECT * FROM legion_world.creature_queststarter WHERE quest=13885");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE creature_questender AS SELECT * FROM legion_world.creature_questender WHERE quest=13885");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");
    sObjectMgr->LoadCreatures();
    sGossipDataStore->LoadGossipMenuItems();
    sConditionMgr->LoadConditions();
    sSpellMgr->LoadSpellTargetPositions();
    sQuestDataStore->LoadCreatureQuestRelations();
    sQuestDataStore->LoadCreatureInvolvedRelations();
    sScriptMgr->SetScriptContext(sScriptMgr->GetNameOfStaticContext());
    AddSC_darkshore();
    sScriptMgr->SwapScriptContext(true);
    sScriptDataStore->LoadSpellScriptNames();
    sScriptDataStore->ValidateSpellScripts();
    return quest;
}

Creature* CurrentFlight(LootPlayer& player)
{
    auto flights = player.GetSummonList(Hippogryph);
    Require(flights->size() <= 1, "Duplicate private hippogryph");
    return flights->empty() ? nullptr : player.GetMap()->GetCreature(flights->front());
}

void Borrow(LootPlayer& player, Creature& orseus)
{
    player.PrepareGossipMenu(&orseus, Menu, false);
    orseus.AI()->sGossipSelect(&player, Menu, 0);
}

void TickFlight(::Map& map, LootPlayer& player, Creature& mount, uint32 diff)
{
    auto* vehicle = mount.GetVehicleKit();
    if (Unit* kathrena = vehicle->GetPassenger(KathrenaSeat))
        kathrena->Unit::Update(diff);
    else if (auto kathrena = mount.FindNearestCreature(Kathrena, 10.0f, true))
        kathrena->Unit::Update(diff);
    mount.Update(diff);
    map.MoveAllCreaturesInMoveList();
    Tick(player, diff);
}

void CheckFlight(Quest const* quest)
{
    ::Map map(DarkshoreWildlifeFlight::Map, 300, 0, DIFFICULTY_NONE);
    for (uint32 spawn : {OrseusSpawn, Giver})
        if (CreatureData const* data = sObjectMgr->GetCreatureData(spawn))
            sObjectMgr->RemoveCreatureFromGrid(spawn, data);
    FlightActor orseusActor(map, OrseusSpawn), kathrenaActor(map, Giver);
    Creature* orseus = &orseusActor;
    Creature* kathrenaGiver = &kathrenaActor;
    Position start = orseus->GetNearPosition(2.0f, 0.0f);
    Require(kathrenaGiver->hasQuest(QuestId) && kathrenaGiver->hasInvolvedQuest(QuestId), "Actual Kathrena quest relations must load");

    LootPlayer owner(SupplySession(), map, 1388501, start), other(SupplySession(), map, 1388502, start);
    for (auto player : {&owner, &other})
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_NIGHTELF);
        player->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_HUNTER);
        player->setFactionForRace(RACE_NIGHTELF);
        player->InitDisplayIds();
        player->SetLevel(20);
        player->SetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP, 30000);
        for (uint16 field : {UNIT_FIELD_MOD_TIME_RATE, UNIT_FIELD_MOD_CASTING_SPEED, UNIT_FIELD_MOD_SPELL_HASTE, UNIT_FIELD_MOD_HASTE, UNIT_FIELD_MOD_HASTE_REGEN})
            player->SetFloatValue(field, 1.0f);
    }

    Borrow(owner, *orseus);
    Require(!owner.PlayerTalkClass->GetGossipMenu().GetItem(0) && !CurrentFlight(owner), "A player without the quest received the ride option or vehicle");
    owner.AddQuestAndCheckCompletion(quest, kathrenaGiver);
    other.AddQuestAndCheckCompletion(quest, kathrenaGiver);
    owner.CastSpell(&owner, Summon, true);
    Require(!CurrentFlight(owner), "A player summoned a mount without Orseus");
    Borrow(owner, *orseus);
    Creature* mount = CurrentFlight(owner);
    Require(mount && mount->HasQuestObjectScope() && mount->HasSpell(ProtectWildlife) &&
        owner.canSeeOrDetect(mount) && !other.canSeeOrDetect(mount), "Real Orseus gossip did not create a private and armed hippogryph");
    Borrow(owner, *orseus);
    Require(CurrentFlight(owner) == mount, "Repeating gossip replaced the pending ride");
    auto vehicle = mount->GetVehicleKit();
    Require(vehicle, "The matching two-seat layout did not load");
    for (unsigned i = 0; i < 160 && (!owner.GetVehicle() || mount->movespline->Finalized()); ++i)
        TickFlight(map, owner, *mount, 50);
    auto guide = vehicle->GetPassenger(KathrenaSeat);
    auto summonedGuide = mount->FindNearestCreature(Kathrena, 10.0f, true);
    std::cout << "CASE board guide " << (guide ? guide->GetEntry() : 0) << " spawned " << bool(summonedGuide)
        << " guide aura " << (summonedGuide && mount->HasAura(65384, summonedGuide->GetGUID()))
        << " player aura " << mount->HasAura(65384, owner.GetGUID()) << " player seat " << bool(owner.GetVehicle())
        << " spline " << !mount->movespline->Finalized() << " mount distance " << owner.GetDistance(mount) << '\n';
    Require(guide && guide->GetEntry() == Kathrena && guide->HasQuestObjectScope() && !other.canSeeOrDetect(guide) &&
        vehicle->GetPassenger(RiderSeat) == &owner && owner.GetVehicleBase() == mount && !other.GetVehicle() &&
        !mount->movespline->Finalized(), "Native seats or takeoff failed");
    other.CastCustomSpell(65384, SPELLVALUE_BASE_POINT0, RiderSeat + 1, mount, true);
    Tick(other, 50);
    Require(!other.GetVehicle() && vehicle->GetPassenger(RiderSeat) == &owner, "Foreign rider can steal a personal seat");
    std::cout << "CASE actual private passenger seats and takeoff\n";

    GuidUnorderedSet aimed;
    std::map<SpellCastResult, unsigned> casts;
    constexpr uint32 wildlife[] = {2165, 34318, 34396, 2070, 2071, 2237};
    for (unsigned step = 0; step < 900 && owner.GetVehicleBase() == mount; ++step)
    {
        if (owner.GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE)
        {
            std::list<Creature*> animals;
            for (uint32 entry : wildlife)
                mount->GetCreatureListWithEntryInGrid(animals, entry, 90.0f);
            for (Creature* animal : animals)
            {
                if (!animal->IsAlive() || !mount->IsWithinLOSInMap(animal) || !aimed.insert(animal->GetGUID()).second)
                    continue;
                ++casts[mount->CastSpell(animal->GetPositionX(), animal->GetPositionY(), animal->GetPositionZ(), ProtectWildlife, false)];
                break;
            }
        }
        TickFlight(map, owner, *mount, 300);
    }
    std::cout << "CASE landing " << mount->GetPositionX() << ' ' << mount->GetPositionY() << ' ' << mount->GetPositionZ()
        << " aimed " << aimed.size() << '\n';
    for (auto const& [result, count] : casts)
        std::cout << "CASE protection cast status " << int(result) << " count " << count << '\n';
    for (uint32 objective : {34323, 34325, 34324})
        std::cout << "CASE objective " << objective << ' ' << owner.GetQuestObjectiveData(QuestId, objective) << '\n';
    for (uint32 objective : {34323, 34325, 34324})
        Require(owner.GetQuestObjectiveData(QuestId, objective) == 8 && !other.GetQuestObjectiveData(QuestId, objective),
            "Actual Protect Wildlife hits must yield eight objectives apiece, for the real owner only");
    Tick(owner, 1000);
    float ground = map.GetHeight(owner.GetPositionX(), owner.GetPositionY(), owner.GetPositionZ() + 2.0f);
    Require(!owner.GetVehicle() && owner.GetUnitBeingMoved() == &owner && std::abs(owner.GetPositionZ() - ground) < 0.2f &&
        owner.GetQuestStatus(QuestId) == QUEST_STATUS_COMPLETE, "Landing, mover restoration or native completion failed");
    MoveGilneasPlayer(owner, kathrenaGiver->GetNearPosition(2.0f, 0.0f));
    WorldPackets::Quest::QuestGiverChooseReward reward{WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD)};
    reward.QuestID = QuestId;
    reward.QuestGiverGUID = kathrenaGiver->GetGUID();
    owner.GetSession()->HandleQuestGiverChooseReward(reward);
    Require(owner.GetQuestRewardStatus(QuestId), "Original Kathrena did not accept native turn-in after landing");
    map.RemoveAllObjectsInRemoveList();

    Borrow(other, *orseus);
    Creature* repeat = CurrentFlight(other);
    Require(repeat, "A second eligible player could not obtain their own flight");
    auto repeatGuid = repeat->GetGUID();
    for (unsigned i = 0; i < 160 && !other.GetVehicle(); ++i)
        TickFlight(map, other, *repeat, 50);
    Require(other.GetVehicleBase() == repeat, "Abandonment check needs a real seat");
    AbandonSupply(other, QuestId);
    for (unsigned i = 0; i < 30 && map.GetCreature(repeatGuid); ++i)
    {
        if (Creature* actor = map.GetCreature(repeatGuid); actor && actor->GetVehicleKit())
            TickFlight(map, other, *actor, 100);
        map.RemoveAllObjectsInRemoveList();
    }
    Require(!map.GetCreature(repeatGuid) && !other.GetVehicle() && other.GetUnitBeingMoved() == &other,
        "Canceled quest retained a mount or the owner's transport controls");
    MoveGilneasPlayer(other, start);
    other.AddQuestAndCheckCompletion(quest, kathrenaGiver);
    Borrow(other, *orseus);
    Require(CurrentFlight(other) && CurrentFlight(other)->GetGUID() != repeatGuid, "Reacceptance did not create a fresh flight");
    AbandonSupply(other, QuestId);
    map.RemoveAllObjectsInRemoveList();
    std::cout << "PASS Darkshore native gossip, two seats, full flight, manual protection, landing, turn-in and abandonment/retry\n";
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
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Darkshore checks require isolated characters");
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{1, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{1, {}}});
        auto quest = LoadFlightInputs();
        HashMapHolder<Player>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckFlight(quest);
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
