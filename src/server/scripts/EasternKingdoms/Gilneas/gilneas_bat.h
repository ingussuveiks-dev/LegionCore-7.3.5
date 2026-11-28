#ifndef GILNEAS_BAT_H
#define GILNEAS_BAT_H

#include "GilneasBatFlight.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "ScriptSystem.h"
#include "Vehicle.h"

namespace GilneasBatFlight
{
enum class Leg : uint8 { Boarding, Outbound, Bombing, Returning, Done };
constexpr uint32 FirstBombingPoint = 8;
constexpr uint32 FirstReturnPoint = 28;
constexpr uint32 FinalPoint = 36;
constexpr uint32 BombingTime = 120000;

class BatAI : public ScriptedAI
{
public:
    explicit BatAI(Creature* actor) : ScriptedAI(actor) { }

    void OnCharmed(bool /*apply*/) override { }

    void IsSummonedBy(Unit* summoner) override
    {
        auto scope = me->GetQuestObjectScope().lock();
        if (!summoner || !summoner->IsPlayer() || !scope || scope->Owner != summoner->GetGUID() ||
            me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != Summon || !me->GetVehicleKit())
        {
            Finish();
            return;
        }
        owner = summoner->GetGUID();
        launchPosition = summoner->GetPosition();
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }

    Player* GetRider() const
    {
        auto kit = me->GetVehicleKit();
        Unit* rider = kit ? kit->GetPassenger(RiderSeat) : nullptr;
        Player* player = rider && rider->GetGUID() == owner ? rider->ToPlayer() : nullptr;
        if (!player || !player->IsAlive() || player->IsBeingTeleported() || me->IsQuestObjectExpired())
            return nullptr;
        QuestStatus status = player->GetQuestStatus(Quest);
        return status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE ? player : nullptr;
    }

    bool OwnsSeat(Player const* player) const
    {
        auto kit = me->GetVehicleKit();
        return kit && kit->GetPassenger(RiderSeat) == player && player->GetGUID() == owner;
    }

    Player* GetBombardier() const
    {
        Player* player = GetRider();
        return leg == Leg::Bombing && !returnRequested && player && player->GetQuestStatus(Quest) == QUEST_STATUS_INCOMPLETE ? player : nullptr;
    }

    bool CanReturn() const { return leg == Leg::Bombing && !returnRequested && GetRider(); }

    void RequestReturn() { returnRequested = true; }

    uint32 GetData(uint32 id) const override
    {
        if (id == FlyBack)
            return returnRequested;
        return id == Quest ? uint32(leg) : 0;
    }

    void PassengerBoarded(Unit* rider, int8 seat, bool apply) override
    {
        if (!apply && seat == RiderSeat && rider->GetGUID() == owner)
            interrupted = true;
    }

    void UpdateAI(uint32 diff) override
    {
        if (leg == Leg::Done)
            return;

        Player* rider = GetRider();
        if (interrupted || (!rider && leg != Leg::Boarding))
        {
            Finish();
            return;
        }
        if (!rider)
        {
            if (remaining <= diff)
                Finish();
            else
                remaining -= diff;
            return;
        }

        if (leg == Leg::Boarding)
        {
            rider->SetClientControl(me, false);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            leg = Leg::Outbound;
            Launch(1, FirstBombingPoint - 1, 22.0f);
            return;
        }

        if (me->movespline->GetId() != spline)
        {
            Finish();
            return;
        }
        if (leg == Leg::Bombing && !returnRequested)
        {
            if (remaining <= diff)
                returnRequested = true;
            else
                remaining -= diff;
        }
        if (!me->movespline->Finalized())
            return;
        if (me->GetExactDist(destination) > 2.0f)
        {
            if (arrivalWait <= diff)
                Finish();
            else
                arrivalWait -= diff;
            return;
        }

        if (leg == Leg::Returning)
            Finish();
        else if (returnRequested)
        {
            leg = Leg::Returning;
            Launch(FirstReturnPoint, FinalPoint, 24.0f);
        }
        else
        {
            if (leg == Leg::Outbound)
                remaining = BombingTime;
            leg = Leg::Bombing;
            Launch(FirstBombingPoint, FirstReturnPoint - 1, 22.0f);
        }
    }

    void CalcExitVehiclePos(Position& pos) override
    {
        float ground = me->GetMap()->GetHeight(me->GetPhases(), pos.GetPositionX(), pos.GetPositionY(),
            pos.GetPositionZ() + 2.0f, true, 180.0f, nullptr, me);
        if (ground > INVALID_HEIGHT && ground <= pos.GetPositionZ() + 1.0f)
            pos.m_positionZ = ground + 0.25f;
        else if (owner)
            pos.Relocate(launchPosition);
    }

    void Finish(bool beforeLogout = false)
    {
        leg = Leg::Done;
        if (auto kit = me->GetVehicleKit())
            if (Unit* passenger = kit->GetPassenger(RiderSeat); passenger && passenger->GetGUID() == owner)
            {
                Position ground = me->GetPosition();
                if (beforeLogout)
                    CalcExitVehiclePos(ground);
                passenger->ExitVehicle();
                if (beforeLogout && !passenger->GetVehicle())
                    passenger->UpdatePosition(ground, true);
            }
        me->DespawnOrUnsummon(1);
    }

private:
    void Launch(uint32 first, uint32 last, float speed)
    {
        ScriptPointVector const& path = sScriptSystemMgr->GetPointMoveList(Bat);
        if (path.size() != FinalPoint || path[first - 1].uiPointId != first || path[last - 1].uiPointId != last)
        {
            TC_LOG_ERROR("scripts", "Gilneas riding bat: incomplete script waypoint range %u-%u", first, last);
            Finish();
            return;
        }
        Movement::MoveSplineInit movement(*me);
        movement.Path().emplace_back(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        for (uint32 index = first - 1; index < last; ++index)
            movement.Path().emplace_back(path[index].fX, path[index].fY, path[index].fZ);
        destination.Relocate(path[last - 1].fX, path[last - 1].fY, path[last - 1].fZ);
        movement.SetFly();
        movement.SetSmooth();
        movement.SetUncompressed();
        movement.SetVelocity(speed);
        if (movement.Launch() <= 0)
        {
            Finish();
            return;
        }
        spline = me->movespline->GetId();
        arrivalWait = 1000;
    }

    ObjectGuid owner;
    Position launchPosition;
    Position destination;
    uint32 spline = 0;
    uint32 remaining = 12000;
    uint32 arrivalWait = 1000;
    Leg leg = Leg::Boarding;
    bool returnRequested = false;
    bool interrupted = false;
};

BatAI* GetFlight(Unit* unit)
{
    auto actor = unit ? unit->ToCreature() : nullptr;
    return actor && actor->GetEntry() == Bat && actor->GetMapId() == Map && actor->HasQuestObjectScope() ?
        dynamic_cast<BatAI*>(actor->AI()) : nullptr;
}

BatAI* GetSpellFlight(Unit* caster)
{
    if (Player* rider = caster->ToPlayer())
    {
        auto flight = GetFlight(rider->GetVehicleBase());
        return flight && flight->OwnsSeat(rider) ? flight : nullptr;
    }
    return GetFlight(caster);
}

class npc_captured_riding_bat : public CreatureScript
{
public:
    npc_captured_riding_bat() : CreatureScript("npc_captured_riding_bat") { }
    CreatureAI* GetAI(Creature* actor) const override { return new BatAI(actor); }
};

class spell_gilneas_bat_fly_back : public SpellScript
{
    PrepareSpellScript(spell_gilneas_bat_fly_back);

    SpellCastResult Check()
    {
        auto flight = GetSpellFlight(GetCaster());
        return flight && flight->CanReturn() ? SPELL_CAST_OK : SPELL_FAILED_NOT_READY;
    }

    void Return(SpellEffIndex /*index*/)
    {
        if (auto flight = GetSpellFlight(GetCaster()); flight && flight->CanReturn())
            flight->RequestReturn();
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_gilneas_bat_fly_back::Check);
        OnEffectHitTarget += SpellEffectFn(spell_gilneas_bat_fly_back::Return, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_gilneas_bat_iron_bomb : public SpellScript
{
    PrepareSpellScript(spell_gilneas_bat_iron_bomb);

    SpellCastResult Check()
    {
        auto flight = GetSpellFlight(GetCaster());
        return flight && flight->GetBombardier() ? SPELL_CAST_OK : SPELL_FAILED_NOT_READY;
    }

    void Register() override { OnCheckCast += SpellCheckCastFn(spell_gilneas_bat_iron_bomb::Check); }
};

class spell_gilneas_bat_bomb_impact : public SpellScript
{
    PrepareSpellScript(spell_gilneas_bat_bomb_impact);

    void Filter(std::list<WorldObject*>& targets)
    {
        auto flight = GetSpellFlight(GetCaster());
        auto rider = flight ? flight->GetBombardier() : nullptr;
        targets.remove_if([rider](WorldObject* target)
        {
            auto creature = target->ToCreature();
            return !rider || !creature || !creature->IsAlive() || !creature->GetDBTableGUIDLow() ||
                !creature->InSamePhase(rider) || (creature->GetEntry() != 38287 && creature->GetEntry() != 38363 && creature->GetEntry() != 38364);
        });
    }

    void CheckImpact(SpellEffIndex index)
    {
        auto flight = GetSpellFlight(GetCaster());
        if (!flight || !flight->GetBombardier())
            PreventHitDefaultEffect(index);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gilneas_bat_bomb_impact::Filter, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_gilneas_bat_bomb_impact::CheckImpact, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class player_gilneas_bat_flight : public PlayerScript
{
public:
    player_gilneas_bat_flight() : PlayerScript("player_gilneas_bat_flight") { }
    void OnBeforeLogout(Player* player) override
    {
        if (!player->HasPendingTeleport())
            if (auto flight = GetFlight(player->GetVehicleBase()); flight && flight->OwnsSeat(player))
                flight->Finish(true);
    }
};
}

#endif
