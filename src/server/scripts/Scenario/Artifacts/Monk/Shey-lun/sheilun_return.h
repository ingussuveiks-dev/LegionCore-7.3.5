#ifndef SHEILUN_RETURN_H
#define SHEILUN_RETURN_H

#include "Spline.h"
#include "ModelIgnoreFlags.h"
#include <map>
#include <mutex>

namespace SheilunAcquisition
{
constexpr uint32 FlightPoint = 0x53484c01;

inline Creature* KiteDriver(Creature* kite)
{
    Vehicle* vehicle = kite ? kite->GetVehicleKit() : nullptr;
    Unit* driver = vehicle ? vehicle->GetPassenger(1) : nullptr;
    Creature* actor = driver ? driver->ToCreature() : nullptr;
    return actor && actor->GetEntry() == GuideEntry && actor->IsAlive() && actor->GetVehicleBase() == kite &&
        actor->HasQuestObjectScope() && kite->CanTargetQuestObject(actor) ? actor : nullptr;
}

// Only value identities cross the two map update threads. The destination map
// resolves its own actors; the source instance never accesses a remote Map.
enum class Leg : uint8 { Transfer, Summoning, Boarding, Flight, Exit, Abort };
struct Transit
{
    ObjectGuid owner, item, kite;
    std::weak_ptr<uint8> quest;
    uint64 epoch = 0;
    uint32 instance = 0, elapsed = 0, retry = 0;
    uint8 tries = 0;
    Leg leg = Leg::Transfer;
    bool boarded = false, ended = false, summon = false;
};
static std::mutex transitLock;
static std::map<ObjectGuid, Transit> transits;

inline bool ReadTransit(ObjectGuid owner, Transit& result)
{
    std::lock_guard<std::mutex> lock(transitLock);
    auto it = transits.find(owner);
    if (it == transits.end()) return false;
    result = it->second; return true;
}

template<class Fn> inline bool EditTransit(Transit const& expected, Fn change)
{
    std::lock_guard<std::mutex> lock(transitLock);
    auto it = transits.find(expected.owner);
    if (it == transits.end() || it->second.epoch != expected.epoch || it->second.instance != expected.instance) return false;
    change(it->second); return true;
}

inline void EraseTransit(Transit const& expected)
{
    std::lock_guard<std::mutex> lock(transitLock);
    auto it = transits.find(expected.owner);
    if (it != transits.end() && it->second.epoch == expected.epoch && it->second.instance == expected.instance) transits.erase(it);
}

inline bool TransitItem(Player* player, Transit const& transit)
{
    Item* item = player ? player->GetItemByGuid(transit.item) : nullptr;
    Item* inventory = InventoryArtifact(player);
    return player && player->GetGUID() == transit.owner && item && item == inventory && item->GetEntry() == Artifact &&
        item->GetOwnerGUID() == transit.owner && item->GetCount();
}

inline bool TransitLease(Player* player, Transit const& transit)
{
    auto quest = player ? player->getQuestStatus(QuestId) : nullptr;
    return Active(player) && quest && !transit.quest.expired() &&
        quest->ScriptLifetime == transit.quest.lock() && TransitItem(player, transit);
}

inline void RestoreControl(Player* player, ObjectGuid kite)
{
    if (!player || kite.IsEmpty()) return;
    if (WorldObject* view = player->GetViewpoint())
        if (view->GetGUID() == kite) player->SetViewpoint(view, false);
    if (Unit* mover = player->GetUnitBeingMoved())
        if (mover->GetGUID() == kite) player->SetClientControl(player, true);
}

// Check the curve that MoveSmoothFlyPath really installs, including phased
// dynamic obstacles. The separately retained native probe checks all static
// route samples against the matching extracted terrain and WMO assets.
inline bool FlightClear(Player* player, std::vector<G3D::Vector3> const& path)
{
    if (!player || path.size() < 2 || path.size() > 16) return false;
    for (auto const& point : path) if (!SheilunReturnRoute::Valid(point)) return false;
    Movement::SplineBase spline;
    spline.init_spline(path.data(), int(path.size()), Movement::SplineBase::ModeCatmullrom);
    unsigned budget = 0;
    for (int segment = spline.first(); segment < spline.last(); ++segment)
    {
        float length = spline.SegLength(segment);
        if (!std::isfinite(length) || length < 0.f || length > 2048.f) return false;
        int samples = std::max(16, int(std::ceil(length * 2.f)));
        if ((budget += samples) > 8192) return false;
        G3D::Vector3 previous;
        for (int sample = 0; sample <= samples; ++sample)
        {
            G3D::Vector3 point;
            spline.evaluate_percent(segment, float(sample) / samples, point);
            for (float dx : {-2.f, 0.f, 2.f}) for (float dy : {-2.f, 0.f, 2.f})
            {
                float height = player->GetMap()->GetHeight(player->GetPhases(), point.x+dx, point.y+dy, point.z+4.f, true, 10000.f);
                if (!std::isfinite(height) || height < -10000.f || height > point.z + .05f) return false;
                if (sample) for (float dz : {.1f, 2.f, 4.f})
                    if (!player->GetMap()->isInLineOfSight(previous.x+dx, previous.y+dy, previous.z+dz,
                        point.x+dx, point.y+dy, point.z+dz, player->GetPhases(), VMAP::ModelIgnoreFlags::Nothing)) return false;
            }
            previous = point;
        }
    }
    return true;
}

inline bool Instance::Guide(Player* player, Creature* guide) const
{
    return Present(player) && state.claimed && EarnedItem(player) && state.guideReady &&
        guide && guide->GetEntry() == GuideEntry && guide->GetGUID() == state.guide && At(player, guide);
}

inline bool Instance::Request(Player* player, Creature* guide, uint32 menu, uint32 option)
{
    bool offered = SheilunPlan::Menu(player, menu, option, 19257, 0);
    SheilunPlan::Close(player);
    if (!offered || !Guide(player, guide) || state.travel != Travel::None || player->GetVehicle() || player->waitOnSeat ||
        player->GetTransport() || player->isInFlight() || player->isInCombat()) return false;
    state.travel = Travel::Requested; ++state.request; state.travelTimer = 5000; state.retryTimer = 1;
    state.travelElapsed = 0; state.farewellTimer = 0; state.farewellStarted = false;
    return true;
}

inline bool Instance::ReturnCast(Player* player, uint64& epoch, uint64& request) const
{
    if (!Present(player) || !state.claimed || !EarnedItem(player) ||
        (state.travel != Travel::Requested && state.travel != Travel::Boarding && state.travel != Travel::Credit)) return false;
    epoch = state.epoch; request = state.request; return true;
}

inline bool Instance::ReturnEffect(Player* player, uint64 epoch, uint64 request, uint8 effect)
{
    if (!Current(player, epoch) || request != state.request || !EarnedItem(player)) return false;
    if (!effect)
    {
        if (state.travel != Travel::Requested || !state.kite.IsEmpty() || state.spawningKite) return false;
        state.spawningKite = true; return true;
    }
    Creature* kite = CreatureAt(instance, state.kite);
    if (!kite || !KiteDriver(kite) || !state.boarded || player->GetVehicleBase() != kite || !kite->GetVehicleKit() ||
        kite->GetVehicleKit()->GetPassenger(0) != player || effect > 2 || (state.returnEffects & (1 << effect))) return false;
    state.returnEffects |= 1 << effect;
    return true;
}

inline void Instance::EndReturnCast(Player* player, uint64 epoch, uint64 request, uint8)
{
    if (!Current(player, epoch) || request != state.request) return;
    state.spawningKite = false;
    Quest const* quest = sQuestDataStore->GetQuestTemplate(QuestId);
    bool receipt = false;
    if (quest) for (auto const& objective : quest->GetObjectives())
        if (objective.ID == 281599 && objective.ObjectID == GuideEntry && objective.StorageIndex == 2)
            receipt = player->GetQuestObjectiveData(quest, objective.StorageIndex) >= objective.Amount;
    Creature* kite = CreatureAt(instance, state.kite);
    if ((state.returnEffects & 6) == 6 && state.boarded && receipt && KiteDriver(kite))
    {
        state.returnCredited = true; state.travel = Travel::Departure;
        state.travelTimer = 3339; state.retryTimer = 0;
        if (Creature* kite = CreatureAt(instance, state.kite))
            if (Vehicle* vehicle = kite->GetVehicleKit())
                if (Unit* driver = vehicle->GetPassenger(1))
                    if (Creature* tak = driver->ToCreature()) sCreatureTextMgr->SendChat(tak, 13, state.owner);
    }
}

inline void Instance::Kite(Creature* kite, Player* player)
{
    if (!Present(player) || !state.spawningKite || !state.kite.IsEmpty() || !kite || kite->GetEntry() != KiteEntry) return;
    state.kite = kite->GetGUID(); state.travel = Travel::Boarding;
    player->BindQuestObject(QuestId, kite);
    kite->setActive(true);
}

inline void Instance::Boarded(Creature* kite, Player* player, int8 seat, bool apply)
{
    if (!kite || !player || player->GetGUID() != state.owner || seat != 0 || kite->GetGUID() != state.kite) return;
    if (!apply)
    {
        state.boarded = false;
        if (state.travel == Travel::FirstFlight)
        {
            RestoreControl(player, state.kite);
            state.aborting = true; // Native exit spline, then the ordinary safe exit.
            return;
        }
        if (state.travel != Travel::ExitFirst && state.travel != Travel::Transfer)
        {
            RestoreControl(player, state.kite); state.travel = Travel::None;
            // Keep already-issued native credit/event receipts across a retry.
            ObjectGuid old = state.kite; state.kite.Clear();
            if (Creature* actor = CreatureAt(instance, old)) actor->DespawnOrUnsummon(2000);
            state.retryTimer = 0; state.travelTimer = 0;
        }
        return;
    }
    if (!Present(player) || !EarnedItem(player)) return;
    state.boarded = true;
    // Native join callbacks can run during Effect0 of the first cast. Replay
    // on the instance update, after the containing native spell has returned.
    state.travel = Travel::Credit; state.retryTimer = 1;
}

inline void Instance::FlightEnded(Creature* kite)
{
    if (!kite || kite->GetGUID() != state.kite || state.travel != Travel::FirstFlight) return;
    state.travel = Travel::ExitFirst; state.travelTimer = 0;
}

inline bool Instance::Transfer(Player* player) const
{
    return Present(player) && state.travel == Travel::Transfer && state.returnCredited &&
        EarnedItem(player) && !player->GetVehicle() && player->movespline->Finalized();
}

inline void Instance::TravelUpdate(Player* player, uint32 diff)
{
    if (state.travel == Travel::None || state.travel == Travel::Transfer) return;
    state.travelElapsed += diff;
    if (state.farewellTimer) Elapsed(state.farewellTimer, diff);
    if (state.travelElapsed > 90000)
    {
        RestoreControl(player, state.kite);
        if (player->GetVehicle()) player->ExitVehicle();
        ObjectGuid old = state.kite; state.kite.Clear(); state.boarded = false; state.travel = Travel::None;
        if (Creature* actor = CreatureAt(instance, old)) actor->DespawnOrUnsummon(2000);
        TC_LOG_ERROR("scripts", "Sheilun: source flight timed out; guide retained for retry");
        return;
    }
    Creature* kite = CreatureAt(instance, state.kite);
    if (state.travel == Travel::Requested || state.travel == Travel::Boarding || state.travel == Travel::Credit)
    {
        if (Elapsed(state.travelTimer, diff))
        {
            RestoreControl(player, state.kite);
            if (player->GetVehicle()) player->ExitVehicle();
            ObjectGuid old = state.kite; state.kite.Clear(); state.boarded = false;
            state.travel = Travel::None; state.spawningKite = false;
            if (Creature* actor = CreatureAt(instance, old)) actor->DespawnOrUnsummon(2000);
            TC_LOG_ERROR("scripts", "Sheilun: return boarding/credit timed out; guide retained for retry");
            return;
        }
        if (state.retryTimer && !Elapsed(state.retryTimer, diff)) return;
        state.retryTimer = 1000;
        if (state.travel == Travel::Requested || state.travel == Travel::Credit) player->CastSpell(player, 202828, true);
        else if (kite && !player->GetVehicle() && !player->waitOnSeat && player->movespline->Finalized() &&
            kite->GetVehicleKit() && kite->GetVehicleKit()->HasEmptySeat(0)) player->CastSpell(kite, 46598, true);
        return;
    }
    if (!kite && state.travel != Travel::ExitFirst)
    {
        RestoreControl(player, state.kite); state.travel = Travel::None; state.kite.Clear(); state.boarded = false;
        TC_LOG_ERROR("scripts", "Sheilun: return vehicle disappeared; guide retained for retry");
        return;
    }
    if (state.travel == Travel::Departure)
    {
        if (!state.boarded || !KiteDriver(kite) || player->GetVehicleBase() != kite || !Elapsed(state.travelTimer, diff) ||
            !player->movespline->Finalized() || player->IsMapObjectArrivalProtected()) return;
        auto path = SheilunReturnRoute::First(G3D::Vector3(kite->GetPositionX(),kite->GetPositionY(),kite->GetPositionZ()));
        if (!FlightClear(player, path))
        {
            state.travelTimer = 1000;
            TC_LOG_ERROR("scripts", "Sheilun: return takeoff collision check failed; flight withheld");
            return;
        }
        player->SetClientControl(kite, false);
        kite->SetDisableGravity(true); kite->SetCanFly(true); kite->SetSpeed(MOVE_FLIGHT, 4.f);
        state.travel = Travel::FirstFlight; state.travelTimer = 4000;
        kite->GetMotionMaster()->MoveSmoothFlyPath(FlightPoint, path.data(), uint32(path.size()));
        return;
    }
    if (state.travel == Travel::FirstFlight)
    {
        if (!state.farewellStarted && Elapsed(state.travelTimer, diff))
        {
            Speak(18); state.farewellStarted = true; state.farewellTimer = 6746;
        }
        return;
    }
    if (state.travel != Travel::ExitFirst) return;
    if (!state.farewellStarted) { Speak(18); state.farewellStarted = true; state.farewellTimer = 6746; }
    if (state.farewellTimer) return; // Keep the player aboard while the full clip finishes.
    RestoreControl(player, state.kite);
    if (player->GetVehicle()) { player->ExitVehicle(); return; }
    if (!player->movespline->Finalized() || state.farewellTimer || (state.travelTimer && !Elapsed(state.travelTimer, diff))) return;
    Transit transit; transit.owner = state.owner; transit.item = state.item;
    transit.quest = state.quest; transit.epoch = state.epoch; transit.instance = instance->GetInstanceId();
    {
        std::lock_guard<std::mutex> lock(transitLock);
        transits[transit.owner] = transit;
    }
    state.travel = Travel::Transfer;
    player->CastSpell(player, 202831, true);
    // A failed teleport must leave a usable return guide and visible evidence.
    if (player->GetMap() == instance && !player->HasPendingTeleport())
    {
        EraseTransit(transit); state.travel = Travel::ExitFirst; state.travelTimer = 1000;
        TC_LOG_ERROR("scripts", "Sheilun: native return transfer failed; retry pending");
    }
}

inline void RegisterKite(Creature* kite, Player* player)
{
    if (!kite || !player) return;
    if (Instance* controller = Controller(player)) { controller->Kite(kite, player); return; }
    Transit transit;
    if (player->GetMapId() != SheilunReturnRoute::ReturnMap || !ReadTransit(player->GetGUID(), transit) ||
        !TransitLease(player, transit) || transit.leg != Leg::Summoning || !transit.summon || !transit.kite.IsEmpty()) return;
    EditTransit(transit, [&](Transit& current) { current.kite = kite->GetGUID(); current.leg = Leg::Boarding; current.retry = 1; });
    player->BindQuestObject(QuestId, kite);
    kite->setActive(true);
}

inline void TransitBoarded(Creature* kite, Player* player, int8 seat, bool apply)
{
    Transit transit;
    if (!kite || !player || seat != 0 || !ReadTransit(player->GetGUID(), transit) || transit.kite != kite->GetGUID()) return;
    EditTransit(transit, [&](Transit& current) { current.boarded = apply; });
    if (!apply) RestoreControl(player, transit.kite);
}

inline void TransitEnded(Creature* kite)
{
    std::lock_guard<std::mutex> lock(transitLock);
    for (auto& pair : transits)
        if (pair.second.kite == kite->GetGUID() && pair.second.leg == Leg::Flight) pair.second.ended = true;
}

inline void CancelTransit(Player* player, Transit const& transit)
{
    EraseTransit(transit);
    // Logout can occur between removal and far-map admission. Its source
    // instance already retired the old actors; no remote/null Map access here.
    if (!player || !player->IsInWorld()) return;
    RestoreControl(player, transit.kite);
    if (player->GetVehicleBase() && player->GetVehicleBase()->GetGUID() == transit.kite) player->ExitVehicle();
    if (Creature* kite = CreatureAt(player->GetMap(), transit.kite)) kite->DespawnOrUnsummon(2000);
}

inline void AbortTransit(Player* player, Transit const& transit)
{
    if (!player || !player->CanContact() || player->GetMapId() != SheilunReturnRoute::ReturnMap ||
        player->HasPendingTeleport() || player->isInFlight() || player->GetTransport() ||
        (player->GetVehicleBase() && player->GetVehicleBase()->GetGUID() != transit.kite))
    {
        CancelTransit(player, transit); return;
    }
    // Keep only value identities while the ordinary vehicle-exit spline
    // finishes. This cleanup survives quest cancellation but grants nothing.
    EditTransit(transit, [](Transit& current) { current.leg = Leg::Abort; current.retry = 0; });
    RestoreControl(player, transit.kite);
    if (player->GetVehicleBase()) player->ExitVehicle();
}

inline void UpdateAbort(Player* player, Transit const& transit, uint32 diff)
{
    if (!player->CanContact() || player->GetMapId() != SheilunReturnRoute::ReturnMap ||
        player->HasPendingTeleport() || player->isInFlight() || player->GetTransport() ||
        (player->GetVehicleBase() && player->GetVehicleBase()->GetGUID() != transit.kite))
    {
        CancelTransit(player, transit); return;
    }
    RestoreControl(player, transit.kite);
    if (player->GetVehicleBase()) { player->ExitVehicle(); return; }
    // Never replace another issued mover or an unfinished native exit spline.
    if (player->GetUnitBeingMoved() != player) { CancelTransit(player, transit); return; }
    if (!player->movespline->Finalized()) return;
    if (!player->IsAlive()) { CancelTransit(player, transit); return; }
    if (transit.retry > diff) { EditTransit(transit, [=](Transit& current) { current.retry -= diff; }); return; }
    Position landing = Landing();
    if (player->GetDistance(landing) < 3.f) { CancelTransit(player, transit); return; }
    player->TeleportTo(SheilunReturnRoute::ReturnMap, landing.GetPositionX(), landing.GetPositionY(),
        landing.GetPositionZ(), landing.GetOrientation());
    if (player->HasPendingTeleport()) { CancelTransit(player, transit); return; }
    EditTransit(transit, [](Transit& current) { current.retry = 1000; });
    TC_LOG_ERROR("scripts", "Sheilun: interrupted destination flight could not reach its native safe landing; retry pending");
}

inline void UpdateTransit(Player* player, uint32 diff)
{
    Transit transit;
    if (!player || !ReadTransit(player->GetGUID(), transit)) return;
    if (transit.leg == Leg::Abort) { UpdateAbort(player, transit, diff); return; }
    if (!TransitLease(player, transit)) { AbortTransit(player, transit); return; }
    EditTransit(transit, [=](Transit& current) { current.elapsed += diff; });
    if (transit.elapsed > 120000)
    {
        TC_LOG_ERROR("scripts", "Sheilun: destination flight timed out; owned artifact retained");
        AbortTransit(player, transit); return;
    }
    if (!player->IsInWorld() || !player->CanContact() || player->HasPendingTeleport()) return;
    if (player->GetMapId() != SheilunReturnRoute::ReturnMap)
    {
        if (transit.leg != Leg::Transfer || player->GetMapId() != MapId) CancelTransit(player, transit);
        return;
    }
    if (!player->IsAlive()) { CancelTransit(player, transit); return; }
    if (transit.leg == Leg::Transfer)
    {
        EditTransit(transit, [](Transit& current) { current.leg = Leg::Summoning; current.retry = 0; });
        return;
    }
    Creature* kite = CreatureAt(player->GetMap(), transit.kite);
    if (transit.leg == Leg::Flight && (!kite || !transit.boarded || player->GetVehicleBase() != kite))
    {
        AbortTransit(player, transit); return;
    }
    if (transit.leg == Leg::Summoning || transit.leg == Leg::Boarding)
    {
        if (!kite && !transit.kite.IsEmpty())
        {
            TC_LOG_ERROR("scripts", "Sheilun: destination vehicle disappeared");
            AbortTransit(player, transit); return;
        }
        if (kite && KiteDriver(kite) && transit.boarded && player->GetVehicleBase() == kite && kite->GetVehicleKit() &&
            kite->GetVehicleKit()->GetPassenger(0) == player && player->movespline->Finalized() && !player->IsMapObjectArrivalProtected())
        {
            std::vector<G3D::Vector3> path;
            if (sTaxiPathNodesByPath.size() <= SheilunReturnRoute::CandidatePath ||
                !SheilunReturnRoute::Second(G3D::Vector3(kite->GetPositionX(),kite->GetPositionY(),kite->GetPositionZ()),
                    sTaxiPathNodesByPath[SheilunReturnRoute::CandidatePath], path) || !FlightClear(player, path))
            {
                TC_LOG_ERROR("scripts", "Sheilun: destination route data/collision check failed");
                AbortTransit(player, transit); return;
            }
            EditTransit(transit, [](Transit& current) { current.leg = Leg::Flight; });
            player->SetClientControl(kite, false);
            kite->SetDisableGravity(true); kite->SetCanFly(true); kite->SetSpeed(MOVE_FLIGHT, 4.f);
            kite->GetMotionMaster()->MoveSmoothFlyPath(FlightPoint, path.data(), uint32(path.size()));
            return;
        }
        if (transit.retry > diff) { EditTransit(transit, [=](Transit& current) { current.retry -= diff; }); return; }
        if (transit.tries >= 5 && !transit.boarded)
        {
            TC_LOG_ERROR("scripts", "Sheilun: destination boarding failed; same vehicle retry exhausted");
            AbortTransit(player, transit); return;
        }
        if (transit.boarded) return;
        EditTransit(transit, [](Transit& current) { current.retry = 1000; ++current.tries; current.summon = false; });
        if (!kite) player->CastSpell(player, 202828, true);
        else if (!player->GetVehicle() && !player->waitOnSeat && player->movespline->Finalized() &&
            kite->GetVehicleKit() && kite->GetVehicleKit()->HasEmptySeat(0)) player->CastSpell(kite, 46598, true);
        return;
    }
    if (transit.leg == Leg::Flight && transit.ended)
    {
        EditTransit(transit, [](Transit& current) { current.leg = Leg::Exit; });
        RestoreControl(player, transit.kite);
        Position landing = Landing();
        player->ExitVehicle(&landing);
        return;
    }
    if (transit.leg == Leg::Exit && !player->GetVehicle() && player->movespline->Finalized()) CancelTransit(player, transit);
}
}
#endif
