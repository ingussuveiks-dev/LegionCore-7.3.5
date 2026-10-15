#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "ObjectMgr.h"
#include "QuestData.h"
#include "Map.h"
#include <cmath>
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "stormheim_dragons_route.h"
#include <map>
#include <mutex>

namespace StormheimDragons
{
constexpr uint32 QuestId = 39652;
struct Offer { ObjectGuid source; std::weak_ptr<uint8> lifetime; };
std::map<ObjectGuid,Offer> offers;
std::map<ObjectGuid,ObjectGuid> starting;
bool NativeQuest()
{
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    if (!quest) return false;
    bool ride = false, talk = false;
    for (auto const& objective : quest->GetObjectives())
        if (objective.Type == QUEST_OBJECTIVE_MONSTER && objective.Amount == 1 && !(objective.Flags & QUEST_OBJECTIVE_FLAG_OPTIONAL))
        {
            ride |= objective.ObjectID == 96466 && objective.StorageIndex == 0;
            talk |= objective.ObjectID == 96467 && objective.StorageIndex == 1 && (objective.Flags & QUEST_OBJECTIVE_FLAG_SEQUENCED);
        }
    return ride && talk;
}
bool Grounded(Player* player)
{
    if (!player || !player->IsInWorld() || !player->IsAlive() || player->GetMapId() != 1220 ||
        player->GetZoneId() != 7541 || player->GetInstanceId() || player->IsBeingTeleported() ||
        player->GetVehicle() || player->GetTransport() || player->isInFlight() || player->IsFlying() ||
        player->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR) || !player->movespline->Finalized()) return false;
    float floor = player->GetMap()->GetHeight(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()+2.0f, true, 5.0f);
    return std::isfinite(floor) && std::fabs(player->GetPositionZ()-floor) < 1.0f;
}
void Credit(Player* player, uint32 entry)
{
    if (!NativeQuest() || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
        player->GetQuestObjectiveData(QuestId,entry) ||
        (entry == 96467 && !player->GetQuestObjectiveData(QuestId,96466))) return;
    auto quest = sQuestDataStore->GetQuestTemplate(QuestId);
    for (auto const& objective : quest->GetObjectives())
        if (objective.ObjectID == entry)
        {
            player->SetQuestObjectiveData(quest,&objective,1);
            player->SendQuestUpdateAddCredit(quest,ObjectGuid::Empty,objective,1);
            if (player->CanCompleteQuest(QuestId)) player->CompleteQuest(QuestId);
            return;
        }
}
constexpr size_t RouteSize = sizeof(Route) / sizeof(Route[0]);
enum class Phase { Boarding, Flying, Exiting, Recovering };
struct Ride
{
    ObjectGuid bird;
    std::weak_ptr<uint8> lifetime;
    Phase phase = Phase::Boarding;
    uint32 elapsed = 0;
    size_t node = 0;
    bool requested = false, arrived = false, success = false;
};
std::map<ObjectGuid,Ride> rides;
// Vehicle removal can call PassengerBoarded synchronously during OnUpdate.
std::recursive_mutex mutex;

bool CanStart(Player* player)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return NativeQuest() && Grounded(player) &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE &&
        !player->IsBeingTeleported() && !player->isInFlight() && !player->IsFlying() &&
        !player->GetVehicle() && !player->GetTransport() && !player->isInCombat() &&
        player->GetExactDist(&Return) < 8.0f && !rides.count(player->GetGUID());
}
void Restore(Player* player, ObjectGuid bird)
{
    if (player->GetUnitBeingMoved()->GetGUID() == bird)
        player->SetClientControl(player,true);
    else if (auto viewpoint = player->GetViewpoint())
        if (viewpoint->GetGUID() == bird) player->SetViewpoint(viewpoint,false);
}
void Exit(Player* player, Ride& ride, Creature* bird)
{
    ride.phase = Phase::Exiting;
    ride.elapsed = 0;
    Restore(player,ride.bird);
    if (bird) bird->StopMoving();
    if (Unit* base = player->GetVehicleBase())
        if (base->GetGUID() == ride.bird) player->ExitVehicle();
}
void Finish(Player* player, bool recover)
{
    // Caller holds mutex. Erase only after ExitVehicle/CalcExitVehiclePos.
    auto it = rides.find(player->GetGUID());
    if (it == rides.end()) return;
    Ride ride = it->second;
    Creature* bird = player->IsInWorld() ? ObjectAccessor::GetCreature(*player,ride.bird) : nullptr;
    Restore(player,ride.bird);
    if (Unit* base = player->GetVehicleBase())
        if (base->GetGUID() == ride.bird) player->ExitVehicle();

    // Successful rides use the native exit spline. Interrupted rides return to
    // the source only after that spline, never override an external teleport.
    if (recover && player->IsInWorld() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && !player->GetVehicle())
    {
        if (ride.success)
        {
            // A near teleport still needs its acknowledgement. Keep this ride
            // until the fallback has actually placed the player on the summit.
            it->second.phase = Phase::Recovering;
            it->second.elapsed = 0;
            player->NearTeleportTo(Landing);
            if (bird) bird->DespawnOrUnsummon(1000);
            return;
        }
        player->NearTeleportTo(Return);
    }
    // Native exit spline must finish before credit, and an abandoned/reaccepted
    // quest must not inherit an earlier ride's completion.
    if (ride.success && !ride.lifetime.expired() && player->getQuestStatus(QuestId) &&
        ride.lifetime.lock() == player->getQuestStatus(QuestId)->ScriptLifetime &&
        Grounded(player) && player->GetExactDist(&Landing) < 3.0f)
        Credit(player,96466);
    rides.erase(it);
    if (bird) bird->DespawnOrUnsummon(1000);
}
void Update(Player* player, uint32 diff)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = rides.find(player->GetGUID());
    if (it == rides.end()) return;
    Ride& ride = it->second;
    if (ride.phase == Phase::Recovering)
    {
        ride.elapsed += diff;
        if (player->GetMapId() != 1220) { ride.success = false; Finish(player,false); }
        else if (!player->IsBeingTeleported() && Grounded(player) && player->GetExactDist(&Landing) < 3.0f)
            Finish(player,false);
        else if (ride.elapsed >= 10000) { ride.success = false; Finish(player,false); }
        return;
    }
    if (player->GetMapId() != 1220 || player->IsBeingTeleported())
    { Finish(player,false); return; }
    Creature* bird = ObjectAccessor::GetCreature(*player,ride.bird);
    ride.elapsed += diff;
    if (ride.phase == Phase::Exiting)
    {
        if (!player->GetVehicle() && (player->movespline->Finalized() || ride.elapsed >= 10000))
            Finish(player,!ride.success || !player->movespline->Finalized() || !Grounded(player) || player->GetExactDist(&Landing) >= 3.0f);
        else if (ride.elapsed >= 10000) Finish(player,true);
        return;
    }
    if (!bird || !bird->IsAlive() || bird->IsDespawn() || ride.lifetime.expired() ||
        !player->IsAlive() || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
        !player->getQuestStatus(QuestId) || ride.lifetime.lock() != player->getQuestStatus(QuestId)->ScriptLifetime ||
        !player->InSamePhase(bird) || !player->canSeeOrDetect(bird) || player->GetDistance(bird) > 20.0f)
    { Exit(player,ride,bird); return; }
    if (ride.phase == Phase::Boarding)
    {
        if (!ride.requested)
        {
            ride.requested = true;
            player->RemoveAurasByType(SPELL_AURA_MOUNTED);
            player->StopMoving();
            player->EnterVehicle(bird,0,true);
        }
        if (player->GetVehicleBase() == bird && bird->GetVehicleKit()->GetPassenger(0) == player &&
            player->movespline->Finalized())
        {
            bird->GetVehicleKit()->RelocatePassengers();
            player->SendMovementFlagUpdate(true);
            player->SetClientControl(bird,false);
            ride.phase = Phase::Flying;
            ride.elapsed = 0;
            bird->GetMotionMaster()->MovePoint(1,Route[0],false,18.0f);
        }
        else if (ride.elapsed >= 10000) Exit(player,ride,bird);
        return;
    }
    if (player->GetVehicleBase() != bird || ride.elapsed >= 180000)
    { Exit(player,ride,bird); return; }
    if (!ride.arrived || !bird->movespline->Finalized() || bird->GetExactDist(&Route[ride.node]) >= 0.5f) return;
    ride.arrived = false;
    if (++ride.node == RouteSize)
    {
        ride.success = true;
        bird->SetFacingTo(Landing.GetOrientation());
        Exit(player,ride,bird);
    }
    else bird->GetMotionMaster()->MovePoint(uint32(ride.node+1),Route[ride.node],false,18.0f);
}
}

struct npc_stormheim_dragons : ScriptedAI
{
    npc_stormheim_dragons(Creature* npc) : ScriptedAI(npc) { }
    ObjectGuid owner;
    void IsSummonedBy(Unit* summoner) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!player || !StormheimDragons::starting.count(player->GetGUID())) return;
        if (!StormheimDragons::CanStart(player) || !me->GetVehicleKit() || me->GetVehicleKit()->GetVehicleInfo()->ID != 4367 ||
            me->GetExactDist(&StormheimDragons::Return) > 15.0f || !player->IsWithinLOSInMap(me))
        { me->DespawnOrUnsummon(); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(StormheimDragons::QuestId,0,[] { });
        StormheimDragons::Ride ride;
        ride.bird = me->GetGUID();
        ride.lifetime = player->getQuestStatus(StormheimDragons::QuestId)->ScriptLifetime;
        StormheimDragons::rides.emplace(owner,ride);
        me->AddPlayerInPersonnalVisibilityList(owner);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->setActive(true);
        me->NearTeleportTo(StormheimDragons::Start);
    }
    void PassengerBoarded(Unit* passenger, int8, bool apply) override
    {
        if (!apply)
            if (Player* player = passenger->ToPlayer()) StormheimDragons::Restore(player,me->GetGUID());
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        auto it = StormheimDragons::rides.find(owner);
        if (it != StormheimDragons::rides.end() && it->second.bird == me->GetGUID())
        {
            auto& ride = it->second;
            if (ride.phase == StormheimDragons::Phase::Flying && type == POINT_MOTION_TYPE &&
                id == ride.node+1 && me->GetExactDist(&StormheimDragons::Route[ride.node]) < 0.5f)
                ride.arrived = true;
        }
    }
    void CalcExitVehiclePos(Position& pos) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        auto it = StormheimDragons::rides.find(owner);
        if (it != StormheimDragons::rides.end() && it->second.bird == me->GetGUID() && it->second.success)
            pos = StormheimDragons::Landing;
    }
};

namespace StormheimDragons
{
bool Scope(Creature* actor, uint32 entry, uint64 spawn)
{
    return actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn && actor->GetMapId() == 1220 &&
        actor->GetZoneId() == 7541 && !actor->GetInstanceId();
}
bool Interact(Player* player, Creature* actor)
{
    return Grounded(player) && actor->IsAlive() && actor->IsInWorld() && player->InSamePhase(actor) &&
        player->canSeeOrDetect(actor) && player->IsWithinLOSInMap(actor) && player->GetExactDist(actor) < 15.0f;
}
}
struct npc_stormheim_dragons_giver : SmartAI
{
    explicit npc_stormheim_dragons_giver(Creature* creature) : SmartAI(creature) { }
    void sGossipHello(Player* player) override
    {
        using namespace StormheimDragons;
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (Scope(me,92218,146706157))
        {
            if (Interact(player,me) && !rides.count(player->GetGUID())) Credit(player,96467);
            return; // Replace only this spawn's unconditional old hello credit.
        }
        SmartAI::sGossipHello(player);
        if (Scope(me,96465,146706156) && CanStart(player) && Interact(player,me))
        {
            player->AddQuestDelayedEvent(QuestId,0,[] { });
            offers[player->GetGUID()] = {me->GetGUID(),player->getQuestStatus(QuestId)->ScriptLifetime};
        }
    }
    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        using namespace StormheimDragons;
        if (!Scope(me,96465,146706156) || menu != 18693)
        { SmartAI::sGossipSelect(player,menu,option); return; }
        std::lock_guard<std::recursive_mutex> lock(mutex);
        auto offer = offers.find(player->GetGUID());
        if (option || !CanStart(player) || !Interact(player,me) || offer == offers.end() ||
            offer->second.source != me->GetGUID() || offer->second.lifetime.expired() ||
            offer->second.lifetime.lock() != player->getQuestStatus(QuestId)->ScriptLifetime ||
            player->PlayerTalkClass->GetGossipMenu().GetMenuId() != 18693 ||
            !player->PlayerTalkClass->GetGossipMenu().GetItem(option)) return;
        auto info = sObjectMgr->GetCreatureTemplate(92302);
        if (!info || sObjectMgr->GetScriptName(info->ScriptID) != "npc_stormheim_dragons") return;
        offers.erase(offer);
        player->PlayerTalkClass->ClearMenus();
        player->PlayerTalkClass->SendCloseGossip();
        starting[player->GetGUID()] = me->GetGUID();
        // Reuse native Ride Vethir182921's actor92302, with this quest giver's
        // non-control vehicle4367/seat15925. The older4133 has a control seat.
        Creature* dragon = player->SummonCreature(92302,Return,TEMPSUMMON_MANUAL_DESPAWN,0,4367);
        starting.erase(player->GetGUID());
        if (dragon && !rides.count(player->GetGUID())) dragon->DespawnOrUnsummon();
    }
};

class player_stormheim_dragons : public PlayerScript
{
public:
    player_stormheim_dragons() : PlayerScript("player_stormheim_dragons") { }
    void OnUpdate(Player* player, uint32 diff) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        StormheimDragons::Update(player,diff);
        auto offer = StormheimDragons::offers.find(player->GetGUID());
        if (offer != StormheimDragons::offers.end() && (offer->second.lifetime.expired() || player->GetQuestStatus(StormheimDragons::QuestId) != QUEST_STATUS_INCOMPLETE))
            StormheimDragons::offers.erase(offer);
    }
    void OnMapChanged(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        StormheimDragons::Finish(player,false);
        StormheimDragons::offers.erase(player->GetGUID());
    }
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(StormheimDragons::mutex);
        StormheimDragons::offers.erase(player->GetGUID());
        if (!StormheimDragons::rides.count(player->GetGUID())) return;
        bool recover = player->IsInWorld() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
        StormheimDragons::Finish(player,recover);
        // A logout cannot wait for a local recovery teleport acknowledgement.
        auto pending = StormheimDragons::rides.find(player->GetGUID());
        if (pending != StormheimDragons::rides.end())
        {
            pending->second.success = false;
            StormheimDragons::Finish(player,false);
        }
        // OnLogout follows the ordinary save; persist the safe destination.
        if (recover) player->SaveToDB();
    }
};

void AddSC_stormheim_dragons()
{
    RegisterCreatureAI(npc_stormheim_dragons_giver);
    RegisterCreatureAI(npc_stormheim_dragons);
    new player_stormheim_dragons();
}
