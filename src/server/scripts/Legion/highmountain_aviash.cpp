#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "highmountain_aviash_route.h"
#include <map>
#include <mutex>

namespace Aviash
{
constexpr uint32 Quest = 39387, SummonSpell = 194359;
constexpr size_t RouteSize = sizeof(Route) / sizeof(Route[0]);
enum class Phase { Boarding, Flying, Exiting };
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
    return player && player->IsAlive() && player->IsInWorld() && player->GetMapId() == 1220 &&
        player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE &&
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
    player->RemoveAurasDueToSpell(SummonSpell);
    // Successful rides use the native exit spline. Interrupted rides return to
    // the source only after that spline, never override an external teleport.
    if (recover && player->IsInWorld() && player->GetMapId() == 1220 &&
        !player->IsBeingTeleported() && !player->GetVehicle())
        player->NearTeleportTo(ride.success ? Landing : Return);
    rides.erase(it);
    if (bird) bird->DespawnOrUnsummon(1000);
}
void Update(Player* player, uint32 diff)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    auto it = rides.find(player->GetGUID());
    if (it == rides.end()) return;
    Ride& ride = it->second;
    if (player->GetMapId() != 1220 || player->IsBeingTeleported())
    { Finish(player,false); return; }
    Creature* bird = ObjectAccessor::GetCreature(*player,ride.bird);
    ride.elapsed += diff;
    if (ride.phase == Phase::Exiting)
    {
        if (!player->GetVehicle() && (player->movespline->Finalized() || ride.elapsed >= 10000))
            Finish(player,!ride.success || !player->movespline->Finalized());
        else if (ride.elapsed >= 10000) Finish(player,true);
        return;
    }
    if (!bird || !bird->IsAlive() || bird->IsDespawn() || ride.lifetime.expired() ||
        !player->IsAlive() || player->GetQuestStatus(Quest) != QUEST_STATUS_INCOMPLETE ||
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
    if (!ride.arrived) return;
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

class spell_highmountain_aviash : public SpellScript
{
    PrepareSpellScript(spell_highmountain_aviash);
    SpellCastResult Check()
    { return Aviash::CanStart(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT; }
    void After()
    {
        // The dummy aura is effect 1, after IsSummonedBy. A failed summon must
        // not leave that aura behind; a rejected duplicate must keep its ride.
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        if (Player* player = GetCaster()->ToPlayer())
            if (!Aviash::rides.count(player->GetGUID())) player->RemoveAurasDueToSpell(Aviash::SummonSpell);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_highmountain_aviash::Check);
        AfterCast += SpellCastFn(spell_highmountain_aviash::After);
    }
};

struct npc_highmountain_aviash : ScriptedAI
{
    npc_highmountain_aviash(Creature* npc) : ScriptedAI(npc) { }
    ObjectGuid owner;
    void IsSummonedBy(Unit* summoner) override
    {
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!Aviash::CanStart(player) || !me->GetVehicleKit() ||
            me->GetExactDist(&Aviash::Return) > 15.0f || !player->IsWithinLOSInMap(me))
        { me->DespawnOrUnsummon(); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(Aviash::Quest,0,[] { });
        Aviash::Ride ride;
        ride.bird = me->GetGUID();
        ride.lifetime = player->getQuestStatus(Aviash::Quest)->ScriptLifetime;
        Aviash::rides.emplace(owner,ride);
        me->AddPlayerInPersonnalVisibilityList(owner);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->setActive(true);
        me->NearTeleportTo(Aviash::Start);
    }
    void PassengerBoarded(Unit* passenger, int8, bool apply) override
    {
        if (!apply)
            if (Player* player = passenger->ToPlayer()) Aviash::Restore(player,me->GetGUID());
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        auto it = Aviash::rides.find(owner);
        if (it != Aviash::rides.end() && it->second.bird == me->GetGUID())
        {
            auto& ride = it->second;
            if (ride.phase == Aviash::Phase::Flying && type == POINT_MOTION_TYPE &&
                id == ride.node+1 && me->GetExactDist(&Aviash::Route[ride.node]) < 0.5f)
                ride.arrived = true;
        }
    }
    void CalcExitVehiclePos(Position& pos) override
    {
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        auto it = Aviash::rides.find(owner);
        if (it != Aviash::rides.end() && it->second.bird == me->GetGUID() && it->second.success)
            pos = Aviash::Landing;
    }
};

class player_highmountain_aviash : public PlayerScript
{
public:
    player_highmountain_aviash() : PlayerScript("player_highmountain_aviash") { }
    void OnUpdate(Player* player, uint32 diff) override { Aviash::Update(player,diff); }
    void OnMapChanged(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        Aviash::Finish(player,false);
    }
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(Aviash::mutex);
        if (!Aviash::rides.count(player->GetGUID())) return;
        bool recover = player->IsInWorld() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
        Aviash::Finish(player,recover);
        // This core calls OnLogout after the ordinary save. Save the recovery
        // teleport destination and aura removal so login cannot resume in midair.
        if (recover) player->SaveToDB();
    }
};

void AddSC_highmountain_aviash()
{
    RegisterSpellScript(spell_highmountain_aviash);
    RegisterCreatureAI(npc_highmountain_aviash);
    new player_highmountain_aviash();
}
