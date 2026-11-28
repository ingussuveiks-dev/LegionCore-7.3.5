#define KEEL_HARBOR_NATIVE_SUPPORT_ONLY
#include "KeelHarborNative.cpp"

#include "Transport.h"

namespace
{
constexpr uint32 Gunship = 203428;
constexpr uint32 DockHippogryph = 43747;
constexpr uint32 SummonHippogryph = 81877;
constexpr uint32 FlyingHippogryph = 43751;
constexpr uint32 Gilneas = 654;

bool CanUseHippogryph(LootPlayer& player, Creature& hippogryph)
{
    auto conditions = sConditionMgr->GetConditionsForSpellClickEvent(DockHippogryph, SummonHippogryph);
    ConditionSourceInfo info(&player, &hippogryph);
    return sConditionMgr->IsObjectMeetToConditions(info, conditions);
}

void CheckDeck(Map& map, Transport& ship, LootPlayer& owner, LootPlayer& visitor)
{
    bool found = false;
    for (int localX = -45; localX <= 45 && !found; localX += 5)
        for (int localY = -15; localY <= 15 && !found; localY += 5)
        {
            float x = float(localX), y = float(localY), z = 95.0f;
            ship.CalculatePassengerPosition(x, y, z);
            float eligible = map.GetHeight(owner.GetPhases(), x, y, z, false, 110.0f, nullptr, &owner);
            float foreign = map.GetHeight(visitor.GetPhases(), x, y, z, false, 110.0f, nullptr, &visitor);
            found = eligible > ship.GetPositionZ() + 5.0f && foreign < ship.GetPositionZ();
        }
    Require(found, "The native ship model did not provide phased deck collision");
}

void CheckEndgameGunship(bool observe)
{
    Map map(Gilneas, 597, 0, DIFFICULTY_NONE);
    auto dock = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE guid=148159 AND id=43747 AND map=654");
    auto lornaData = WorldDatabase.Query("SELECT guid,id,position_x,position_y,position_z,orientation,phaseMask FROM creature WHERE guid=148161 AND id=43727 AND map=654");
    Require(dock && lornaData, "The original dock actors are missing");
    ReposeQuestGiver hippogryph(map, dock->Fetch()), lorna(map, lornaData->Fetch());
    LootPlayer owner(SupplySession(), map, 2670601, lorna.GetPosition()), visitor(SupplySession(), map, 2670602, lorna.GetPosition());
    for (auto player : { &owner, &visitor })
    {
        player->SetByteValue(UNIT_FIELD_BYTES_0, 0, RACE_WORGEN);
        player->setFactionForRace(RACE_WORGEN);
        player->SetLevel(12);
        for (uint32 quest : {14386u, 14466u, 24903u, 24678u, 24679u, 24680u})
            player->SetRewardedQuest(quest);
        Travel(*player, lorna.GetPosition());
    }
    auto row = WorldDatabase.Query("SELECT guid FROM transports WHERE entry=203428");
    if (observe)
    {
        Require(!row && CanUseHippogryph(visitor, hippogryph), "The pre-migration missing ship or unguarded click was not reproduced");
        std::cout << "EXPECTED_BROKEN no continent spawn for 203428 and no active Endgame gate for 81877\n";
        return;
    }
    Require(row && row->GetRowCount() == 1, "The continent transport is not registered exactly once");
    Require(!CanUseHippogryph(visitor, hippogryph), "A character without Endgame is admitted to the dock interaction");
    Travel(visitor, hippogryph.GetPosition());
    Click(visitor, hippogryph);
    Require(visitor.GetSummonList(FlyingHippogryph)->empty() && !visitor.GetVehicle(), "The old unqualified summon path is still reachable");
    owner.SetRewardedQuest(SiegeQuest);
    Travel(owner, lorna.GetPosition());
    Accept(owner, lorna, EndgameQuest);
    Require(owner.GetQuestStatus(EndgameQuest) == QUEST_STATUS_INCOMPLETE && CanUseHippogryph(owner, hippogryph),
        "The normal Lorna acceptance did not enable the original dock click");
    AbandonSupply(owner, EndgameQuest);
    Require(!CanUseHippogryph(owner, hippogryph), "Abandonment did not close the native dock interaction");
    Accept(owner, lorna, EndgameQuest);
    Require(CanUseHippogryph(owner, hippogryph), "A real retry did not recover dock admission");

    auto ship = sTransportMgr->CreateTransport(Gunship, row->Fetch()[0].GetUInt64(), &map);
    Require(ship && ship->IsInWorld() && ship->GetMap() == &map, "The actual Core transport could not be published");
    auto removeShip = [&map](Transport* object) { map.RemoveFromMap(object, true); };
    std::unique_ptr<Transport, decltype(removeShip)> published(ship, removeShip);
    Require(ship->GetPhaseMask() == 4194304 && ship->GetPhases() == std::set<uint32>{191} &&
        owner.InSamePhaseId(ship) && owner.InSamePhase(ship) && !visitor.InSamePhaseId(ship) && !visitor.InSamePhase(ship),
        "The native gunship is visible in an unrelated phase or is missing for the eligible phase");
    Require(ship->GetGOInfo()->moTransport.taxiPathID == 2338 && ship->GetGOInfo()->GetSpawnMap() == 749,
        "The actual taxi path or passenger map differs");
    auto const& frames = ship->GetKeyFrames();
    auto harborStop = std::find_if(frames.begin(), frames.end(), [](KeyFrame const& frame)
        { return frame.Node->ArrivalEventID == 25670; });
    Require(harborStop != frames.end() && harborStop->Node->DepartureEventID == 25664 &&
        harborStop->DepartureTime - harborStop->ArriveTime == 180000,
        "The authored boarding window is missing");
    for (uint32 elapsed = 0; elapsed < harborStop->ArriveTime + 1000; elapsed += 100)
        ship->Update(100);
    Require(!ship->IsMoving() && ship->GetExactDist2d(harborStop->Node->Loc.X, harborStop->Node->Loc.Y) < 1.0f,
        "The ship did not reach its native offshore stop");
    CheckDeck(map, *ship, owner, visitor);
    auto stopped = ship->GetPosition();
    for (uint32 elapsed = ship->GetPathProgress(); elapsed < harborStop->DepartureTime + 3500; elapsed += 100)
        ship->Update(100);
    Require(ship->IsMoving() && ship->GetExactDist(stopped) > 10.0f, "The ship remained at the stop after the authored departure");
    auto endgame = sQuestDataStore->GetQuestTemplate(EndgameQuest);
    Require(owner.GetQuestStatus(EndgameQuest) == QUEST_STATUS_INCOMPLETE &&
        !owner.HasQuestObjectiveComplete(endgame, endgame->GetObjectives().front()),
        "Transport publication awarded premature finale progress");
    std::cout << "PASS native offshore stop, real phase-filtered deck collision, departure and authenticated dock click conditions\n";
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
        dynamic_cast<VMAP::VMapManager2*>(VMAP::VMapFactory::createOrGetVMapManager())->InitializeThreadUnsafe({{Gilneas, {}}});
        MMAP::MMapFactory::createOrGetMMapManager()->InitializeThreadUnsafe({{Gilneas, {}}});
        LoginDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("LoginDatabaseInfo", ""), 1, 1);
        CharacterDatabase.SetConnectionInfo(sConfigMgr->GetStringDefault("CharacterDatabaseInfo", ""), 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        Require(!CharacterDatabase.Open() && CharacterDatabase.PrepareStatements(), "Private character pool failed");
        auto schema = CharacterDatabase.Query("SELECT DATABASE()");
        Require(schema && schema->Fetch()[0].GetString() == "fixture_char", "Gunship probes require isolated characters");
        LoadHarborData();
        sObjectMgr->LoadPageTexts();
        sObjectMgr->LoadGameObjectTemplate();
        LoadGameObjectModelList(argv[2]);
        sTransportMgr->LoadTransportTemplates();
        HashMapHolder<Player>::SetSize(32);
        HashMapHolder<GameObject>::SetSize(32);
        GameTime::UpdateGameTimers();
        CheckEndgameGunship(sConfigMgr->GetBoolDefault("NativeEndgameGunship.Observe", false));
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
