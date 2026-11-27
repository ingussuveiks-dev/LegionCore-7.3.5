#define ROCKAWAY_NATIVE_SUPPORT_ONLY
#include "RockawayOffensiveNative.cpp"
#include "AreaTriggerData.h"
#include "GameObjectPackets.h"
#include "LFGMgr.h"
#include "SmartScriptMgr.h"
#include <chrono>
#include <thread>

namespace
{
constexpr uint32 Portal = 246926;
constexpr uint32 Meryl = 102700;
constexpr uint32 VioletHold = 1494;
constexpr uint32 Dalaran = 1220;
constexpr uint32 Transfer = 203241;

void LoadPortal(bool oldAction)
{
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ WRITE");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE gameobject_template AS SELECT * FROM legion_world.gameobject_template WHERE entry=246926");
    WorldDatabase.DirectExecute("CREATE TEMPORARY TABLE smart_scripts AS SELECT * FROM legion_world.smart_scripts WHERE entryorguid=246926 AND source_type=1");
    WorldDatabase.DirectExecute("SET SESSION TRANSACTION READ ONLY");

    auto rows = WorldDatabase.Query("SELECT id FROM smart_scripts WHERE id=0 AND event_type=64 AND action_type=41 AND target_type=1");
    Require(bool(rows) == oldAction, "Fixture must match the requested portal migration state");
    sObjectMgr->LoadGameObjectTemplate();
    if (oldAction)
        sSmartScriptMgr->LoadSmartAIFromDB();
    auto entry = sObjectMgr->GetGameObjectTemplate(Portal);
    Require(entry && entry->type == GAMEOBJECT_TYPE_GOOBER && entry->goober.spell == Transfer &&
        entry->GetAutoCloseTime() == 3 && entry->AIName == (oldAction ? "SmartGameObjectAI" : ""),
        "The actual portal template must retain its original transfer and auto-close");
    sSpellMgr->LoadSpellTargetPositions();
    sAreaTriggerDataStore->LoadAreaTriggerTeleports();
    sObjectMgr->LoadInstanceTemplate();
    sLFGMgr->LoadLFGDungeons();
}

void ExercisePortal(bool oldAction)
{
    GameTime::UpdateGameTimers();
    HashMapHolder<Player>::SetSize(64);
    Map map(Dalaran, 300, 0, DIFFICULTY_NONE);
    auto closeSession = [](WorldSession* session)
    {
        session->SetPlayer(nullptr);
        delete session;
    };
    std::unique_ptr<WorldSession, decltype(closeSession)> session(new WorldSession(0, "Violet Hold native report", {},
        SEC_PLAYER, 6, 0, "Win", LOCALE_enUS, 0, false, AT_AUTH_FLAG_NONE, {}), closeSession);
    NativePlayer player(session.get(), map, 101, {-945.9f, 4342.9f, 737.3f, 0});
    player.SetClass(CLASS_MAGE);
    player.setFactionForRace(player.getRace());
    player.SetScenarioId(1034);
    Actor meryl(&map, Meryl, 10010, player.GetPosition());
    Require(map.AddToMap(static_cast<Creature*>(&meryl)), "The real summoner's template was not published");
    auto portal = meryl.SummonGameObject(Portal, player.GetPosition(), 0, 0, 0, 0, 0, 120);
    Require(portal && portal->IsInWorld() && !portal->isSpawnedByDefault(), "The temporary goober was not published");
    auto guid = portal->GetGUID();

    WorldPackets::GameObject::GameObjReportUse report{WorldPacket(CMSG_GAME_OBJ_REPORT_USE)};
    report.Guid = guid;
    session->HandleGameobjectReportUse(report);
    Require(!player.HasPendingTeleport(), "Report-use must leave the normal activation to the use packet");
    if (oldAction)
    {
        portal->Delete();
        map.RemoveAllObjectsInRemoveList();
        std::cout << "EXPECTED_BROKEN native report-use dispatched the historical gameobject action 41\n";
        return;
    }

    WorldPackets::GameObject::GameObjectUse use{WorldPacket(CMSG_GAME_OBJ_USE)};
    use.Guid = guid;
    {
        Player::ScopedTeleportDelay delay(player);
        session->HandleGameObjectUse(use);
        player.Unit::Update(100);
    }
    Require(player.HasPendingTeleport() && player.GetTeleportDest().GetMapId() == VioletHold &&
        player.GetTeleportDest().GetExactDist(1276.16f, -263.27f, 44.36f) < 0.08f,
        "The native goober use must prepare the original Violet Hold transfer");
    Require(portal->getLootState() == GO_ACTIVATED, "The real goober did not activate");

    std::this_thread::sleep_for(std::chrono::seconds(4));
    GameTime::UpdateGameTimers();
    portal->Update(4000);
    Require(portal->getLootState() == GO_JUST_DEACTIVATED, "The native goober did not auto-close");
    portal->Update(4000);
    map.RemoveAllObjectsInRemoveList();
    Require(!map.GetGameObject(guid), "The temporary goober remained after its native cleanup");
    std::cout << "PASS native report/use, Violet Hold transfer request and temporary goober cleanup\n";
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
        auto login = sConfigMgr->GetStringDefault("LoginDatabaseInfo", "");
        Require(login.substr(login.find_last_of(';') + 1) == "fixture_auth", "The probe needs a private login pool");
        LoginDatabase.SetConnectionInfo(login, 1, 1);
        Require(!LoginDatabase.Open() && LoginDatabase.PrepareStatements(), "Private login pool failed");
        auto oldAction = sConfigMgr->GetBoolDefault("NativeDreadlordsPrize.ExpectOldAction", false);
        LoadPortal(oldAction);
        ExercisePortal(oldAction);
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
