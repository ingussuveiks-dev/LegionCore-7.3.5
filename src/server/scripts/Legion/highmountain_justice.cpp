#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "ObjectAccessor.h"
#include "QuestDef.h"
#include "Vehicle.h"
#include "MoveSpline.h"
#include "highmountain_justice_route.h"
#include <map>
#include <mutex>
#include <set>

namespace Justice
{
constexpr uint32 Quest = 40594, SummonSpell = 198214;
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
    std::set<ObjectGuid> healed;
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
        player->HasPhaseId(5050) && player->GetExactDist(&Return) < 8.0f && !rides.count(player->GetGUID());
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
    // Spell/area-trigger targets use the caster's phase set. Keep the bombing
    // actors reachable, and follow the rider's phase change on completion.
    if (bird && bird->GetPhases() != player->GetPhases()) bird->SetPhaseId(player->GetPhases(),true);
    ride.elapsed += diff;
    if (ride.phase == Phase::Exiting)
    {
        if (!player->GetVehicle() && (player->movespline->Finalized() || ride.elapsed >= 10000))
            Finish(player,!ride.success || !player->movespline->Finalized());
        else if (ride.elapsed >= 10000) Finish(player,true);
        return;
    }
    if (!bird || !bird->IsAlive() || bird->IsDespawn() || ride.lifetime.expired() ||
        !player->IsAlive() || (player->GetQuestStatus(Quest) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(Quest) != QUEST_STATUS_COMPLETE) ||
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
            if (!player->GetQuestObjectiveData(Quest,100712)) player->KilledMonsterCredit(100712);
            ride.phase = Phase::Flying;
            ride.elapsed = 0;
            bird->GetMotionMaster()->MovePoint(1,Route[0],false,18.0f);
        }
        else if (ride.elapsed >= 10000) Exit(player,ride,bird);
        return;
    }
    if (player->GetVehicleBase() != bird || ride.elapsed >= 300000)
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

class spell_highmountain_justice : public SpellScript
{
    PrepareSpellScript(spell_highmountain_justice);
    SpellCastResult Check()
    { return Justice::CanStart(GetCaster()->ToPlayer()) ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT; }
    void After()
    {
        // A failed summon must
        // not leave that aura behind; a rejected duplicate must keep its ride.
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        if (Player* player = GetCaster()->ToPlayer())
            if (!Justice::rides.count(player->GetGUID())) player->RemoveAurasDueToSpell(Justice::SummonSpell);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_highmountain_justice::Check);
        AfterCast += SpellCastFn(spell_highmountain_justice::After);
    }
};

struct npc_highmountain_justice : ScriptedAI
{
    npc_highmountain_justice(Creature* npc) : ScriptedAI(npc) { }
    // Seat 16161 is a control seat. Keep this AI when the core charms the
    // vehicle; PossessedAI would swallow waypoint and dismount callbacks.
    void OnCharmed(bool) override { }
    ObjectGuid owner;
    void IsSummonedBy(Unit* summoner) override
    {
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        Player* player = summoner ? summoner->ToPlayer() : nullptr;
        if (!Justice::CanStart(player) || !me->GetVehicleKit() ||
            me->GetExactDist(&Justice::Return) > 15.0f || !player->IsWithinLOSInMap(me))
        { me->DespawnOrUnsummon(); return; }
        owner = player->GetGUID();
        player->AddQuestDelayedEvent(Justice::Quest,0,[] { });
        Justice::Ride ride;
        ride.bird = me->GetGUID();
        ride.lifetime = player->getQuestStatus(Justice::Quest)->ScriptLifetime;
        Justice::rides.emplace(owner,ride);
        me->AddPlayerInPersonnalVisibilityList(owner);
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        me->SetCanFly(true);
        me->SetDisableGravity(true);
        me->setActive(true);
        me->SetPhaseId(player->GetPhases(),true);
        me->NearTeleportTo(Justice::Start);
    }
    void PassengerBoarded(Unit* passenger, int8, bool apply) override
    {
        if (!apply)
            if (Player* player = passenger->ToPlayer()) Justice::Restore(player,me->GetGUID());
    }
    void MovementInform(uint32 type, uint32 id) override
    {
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        auto it = Justice::rides.find(owner);
        if (it != Justice::rides.end() && it->second.bird == me->GetGUID())
        {
            auto& ride = it->second;
            if (ride.phase == Justice::Phase::Flying && type == POINT_MOTION_TYPE &&
                id == ride.node+1 && me->GetExactDist(&Justice::Route[ride.node]) < 0.5f)
                ride.arrived = true;
        }
    }
    void CalcExitVehiclePos(Position& pos) override
    {
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        auto it = Justice::rides.find(owner);
        if (it != Justice::rides.end() && it->second.bird == me->GetGUID() && it->second.success)
            pos = Justice::Landing;
    }
};

class player_highmountain_justice : public PlayerScript
{
public:
    player_highmountain_justice() : PlayerScript("player_highmountain_justice") { }
    void OnUpdate(Player* player, uint32 diff) override { Justice::Update(player,diff); }
    void OnMapChanged(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        Justice::Finish(player,false);
    }
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        if (!Justice::rides.count(player->GetGUID())) return;
        bool recover = player->IsInWorld() && player->GetMapId() == 1220 && !player->IsBeingTeleported();
        Justice::Finish(player,recover);
        // This core calls OnLogout after the ordinary save. Save the recovery
        // teleport destination and aura removal so login cannot resume in midair.
        if (recover) player->SaveToDB();
    }
};

// Only the wounded bombing-phase actors need injury initialization. Other uses
// of these templates keep their normal health and regeneration.
struct npc_highmountain_justice_wounded : ScriptedAI
{
    npc_highmountain_justice_wounded(Creature* npc) : ScriptedAI(npc) { }
    void Reset() override
    {
        if (me->GetMapId() == 1220 && me->HasPhaseId(5165))
        {
            me->setRegeneratingHealth(false);
            me->SetHealth(me->CountPctFromMaxHealth(50));
        }
    }
    void UpdateAI(uint32) override
    {
        if (UpdateVictim()) DoMeleeAttackIfReady();
    }
};

class spell_highmountain_justice_heal : public SpellScript
{
    PrepareSpellScript(spell_highmountain_justice_heal);
    bool wounded = false;
    void Before()
    {
        Unit* target = GetHitUnit();
        wounded = target && target->IsAlive() && !target->IsFullHealth();
    }
    void After()
    {
        Unit* target = GetHitUnit();
        Unit* caster = GetCaster();
        if (!wounded || GetHitHeal() <= 0 || !target || !target->IsAlive() ||
            (target->GetEntry() != 97498 && target->GetEntry() != 97506) ||
            !target->HasPhaseId(5165) || target->GetMapId() != 1220 || caster->GetEntry() != 97508)
            return;
        Unit* owner = caster;
        for (unsigned i = 0; owner && !owner->ToPlayer() && i < 3; ++i) owner = owner->GetAnyOwner();
        Player* player = owner ? owner->ToPlayer() : nullptr;
        if (!player || !player->IsAlive() || player->GetQuestStatus(Justice::Quest) != QUEST_STATUS_INCOMPLETE ||
            !player->InSamePhase(target) || player->GetDistance(target) > 150.0f)
            return;
        std::lock_guard<std::recursive_mutex> lock(Justice::mutex);
        auto it = Justice::rides.find(player->GetGUID());
        if (it == Justice::rides.end() || it->second.lifetime.expired() || it->second.phase != Justice::Phase::Flying ||
            !player->GetVehicleBase() || player->GetVehicleBase()->GetGUID() != it->second.bird)
            return;
        // Native Healing Stream ticks every 2s. A target contributes 5% once
        // during this sortie, not once per tick. Actual healing is unchanged.
        if (it->second.healed.insert(target->GetGUID()).second) player->KilledMonsterCredit(97498);
    }
    void Register() override
    {
        BeforeHit += SpellHitFn(spell_highmountain_justice_heal::Before);
        AfterHit += SpellHitFn(spell_highmountain_justice_heal::After);
    }
};

void AddSC_highmountain_justice()
{
    RegisterSpellScript(spell_highmountain_justice);
    RegisterCreatureAI(npc_highmountain_justice);
    RegisterCreatureAI(npc_highmountain_justice_wounded);
    RegisterSpellScript(spell_highmountain_justice_heal);
    new player_highmountain_justice();
}
