/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Darkshore
SD%Complete: 0
SDComment: Quest support:
SDCategory: Darkshore
EndScriptData */

/* ContentData
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ScriptedFollowerAI.h"
#include "ChatTextBuilder.h"
#include "DarkshoreWildlifeFlight.h"
#include "Map.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "ObjectAccessor.h"
#include "QuestObjectScope.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "WorldSession.h"

namespace
{
using namespace DarkshoreWildlifeFlight;

constexpr uint32 OrseusMenu = 10509;
constexpr uint32 BorrowHippogryph = 0;
constexpr uint32 RequestHippogryph = 65401;
constexpr uint32 RideHippogryph = 65384;
constexpr uint32 TakeOff = 65385;
Position const Landing{4978.0f, 134.0f, 52.45f, 0.95f};

struct FlightCue
{
    int32 PathIndex;
    uint32 Broadcast;
};

constexpr FlightCue KathrenaCues[] = {{1, 34580}, {3, 34589}, {7, 34590}, {12, 34591}, {18, 34592}, {24, 34593}};
G3D::Vector3 const BoardingPath[] =
{
    {4961.64f, 130.561f, 70.5063f}, {4971.0f, 124.0f, 74.0f},
    {4979.0f, 128.0f, 76.0f}, {4985.0f, 132.0f, 72.0f}, {4978.0f, 134.0f, 56.0f}
};

// Reconstructed from the 7.3.5 landscape and the deer, moonstalker and bear
// sections in the Legion flight. The final approach stays east of the grove huts.
G3D::Vector3 const WildlifeRoute[] =
{
    {4978.0f, 134.0f, 56.0f}, {4995.0f, 152.0f, 77.0f},
    {5034.0f, 183.0f, 82.0f}, {5105.0f, 173.0f, 79.0f},
    {5160.0f, 130.0f, 69.0f}, {5240.0f, 111.0f, 58.0f},
    {5320.0f, 95.0f, 68.0f}, {5400.0f, 161.0f, 59.0f},
    {5465.0f, 192.0f, 56.0f}, {5500.0f, 221.0f, 63.0f}, {5530.0f, 223.0f, 64.0f},
    {5565.0f, 245.0f, 66.0f}, {5515.0f, 309.0f, 58.0f},
    {5460.0f, 366.0f, 54.0f}, {5360.0f, 378.0f, 49.0f},
    {5270.0f, 395.0f, 55.0f}, {5190.0f, 450.0f, 60.0f},
    {5110.0f, 468.0f, 75.0f}, {5020.0f, 438.0f, 69.0f},
    {4920.0f, 422.0f, 67.0f}, {4830.0f, 383.0f, 72.0f},
    {4760.0f, 432.0f, 68.0f}, {4700.0f, 396.0f, 75.0f},
    {4645.0f, 334.0f, 78.0f}, {4600.0f, 283.0f, 83.0f},
    {4550.0f, 235.0f, 101.0f}, {4590.0f, 221.0f, 120.0f}, {4640.0f, 202.0f, 153.0f},
    {4690.0f, 174.0f, 150.0f}, {4735.0f, 150.0f, 110.0f}, {4763.0f, 125.0f, 95.0f}, {4830.0f, 130.0f, 85.0f},
    {4920.0f, 124.0f, 89.0f}, {4980.0f, 120.0f, 87.0f},
    {4985.0f, 132.0f, 70.0f}, {4978.0f, 134.0f, 53.1f}
};

class npc_darkshore_orseus : public ScriptedAI
{
public:
    explicit npc_darkshore_orseus(Creature* creature) : ScriptedAI(creature) { }

    void sGossipSelect(Player* player, uint32 menu, uint32 option) override
    {
        if (menu != OrseusMenu || option != BorrowHippogryph || me->GetEntry() != Orseus ||
            me->GetDBTableGUIDLow() != OrseusSpawn || me->GetMapId() != DarkshoreWildlifeFlight::Map || !me->IsAlive() ||
            player->PlayerTalkClass->GetGossipMenu().GetMenuId() != OrseusMenu ||
            !player->PlayerTalkClass->GetGossipMenu().GetItem(BorrowHippogryph) ||
            !player->IsAlive() || player->isInCombat() || player->GetVehicle() || player->isInFlight() ||
            player->IsBeingTeleported() || player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE ||
            !player->IsWithinDistInMap(me, INTERACTION_DISTANCE) || !player->IsWithinLOSInMap(me))
            return;

        player->PlayerTalkClass->SendCloseGossip();
        me->CastSpell(player, RequestHippogryph, true);
    }
};

class npc_darkshore_grove_hippogryph : public ScriptedAI
{
public:
    explicit npc_darkshore_grove_hippogryph(Creature* creature) : ScriptedAI(creature) { }

    // The player controls the spell bar. Kathrena retains the scripted flight.
    void OnCharmed(bool /*apply*/) override { }

    void IsSummonedBy(Unit* summoner) override
    {
        auto scope = me->GetQuestObjectScope().lock();
        if (!summoner || !summoner->IsPlayer() || !scope || scope->Owner != summoner->GetGUID() || !me->GetVehicleKit())
        {
            me->DespawnOrUnsummon(1);
            return;
        }

        _owner = summoner->GetGUID();
        me->SetReactState(REACT_PASSIVE);
        me->setFaction(summoner->getFaction());
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        _timeLeft = 15000;
        if (TempSummon* kathrena = me->SummonCreature(Kathrena, me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 300000))
        {
            _kathrena = kathrena->GetGUID();
            kathrena->SetReactState(REACT_PASSIVE);
            kathrena->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            kathrena->CastCustomSpell(RideHippogryph, SPELLVALUE_BASE_POINT0, KathrenaSeat + 1, me, true);
        }
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (!apply && ((seat == RiderSeat && passenger->GetGUID() == _owner) ||
            (seat == KathrenaSeat && passenger->GetGUID() == _kathrena)))
            _interrupted = true;
    }

    void UpdateAI(uint32 diff) override
    {
        Player* player = ObjectAccessor::GetPlayer(*me, _owner);
        Vehicle* vehicle = me->GetVehicleKit();
        if (_interrupted || !vehicle || !player || !player->IsAlive() || player->IsBeingTeleported() ||
            (player->GetQuestStatus(QuestId) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(QuestId) != QUEST_STATUS_COMPLETE) ||
            diff >= _timeLeft)
        {
            EndFlight();
            return;
        }
        _timeLeft -= diff;

        if (_splineId)
        {
            if (me->movespline->GetId() != _splineId || me->movespline->Finalized())
                EndFlight();
            else if (_nextCue < std::size(KathrenaCues) && me->movespline->currentPathIdx() >= KathrenaCues[_nextCue].PathIndex)
                if (Unit* guide = vehicle->GetPassenger(KathrenaSeat))
                    SendGuideLine(*guide, *player, KathrenaCues[_nextCue++].Broadcast);
            return;
        }

        Unit* kathrena = vehicle->GetPassenger(KathrenaSeat);
        if (!kathrena || kathrena->GetGUID() != _kathrena)
            return;

        if (!_approachId)
        {
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            if (Launch(BoardingPath, 10.0f) <= 0)
            {
                EndFlight();
                return;
            }
            _approachId = me->movespline->GetId();
            return;
        }
        if (_boardingRequested && vehicle->GetPassenger(RiderSeat) == player)
        {
            kathrena->CastSpell(me, TakeOff, true);
            SendGuideLine(*kathrena, *player, 34579);
            player->SetClientControl(me, false);
            int32 duration = Launch(WildlifeRoute, 13.0f);
            if (duration <= 0)
            {
                EndFlight();
                return;
            }
            _splineId = me->movespline->GetId();
            _timeLeft = uint32(duration) + 5000;
            return;
        }

        if (me->movespline->GetId() != _approachId)
        {
            EndFlight();
            return;
        }
        if (!me->movespline->Finalized())
            return;

        if (!_boardingRequested && !player->GetVehicle() && player->IsWithinDistInMap(me, 16.0f) && player->IsWithinLOSInMap(me))
        {
            _boardingRequested = true;
            SendGuideLine(*kathrena, *player, 34571);
            player->CastCustomSpell(RideHippogryph, SPELLVALUE_BASE_POINT0, RiderSeat + 1, me, true);
        }
    }

    void CalcExitVehiclePos(Position& pos) override
    {
        if (me->IsWithinDist3d(&Landing, 5.0f))
            pos.Relocate(Landing);
        else if (float ground = me->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() + 2.0f, true, 160.0f);
            ground > INVALID_HEIGHT)
            pos.m_positionZ = ground + 0.3f;
    }

    Player* GetActiveRider() const
    {
        Vehicle* vehicle = me->GetVehicleKit();
        if (!_splineId || _interrupted || me->IsQuestObjectExpired() || !vehicle || me->movespline->Finalized() ||
            me->movespline->GetId() != _splineId)
            return nullptr;
        Unit* rider = vehicle->GetPassenger(RiderSeat);
        Player* player = rider ? rider->ToPlayer() : nullptr;
        return player && player->IsAlive() && player->GetGUID() == _owner &&
            player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE ? player : nullptr;
    }

    void Protect(Creature* creature)
    {
        if (Player* player = GetActiveRider())
            if (uint32 credit = CreditForWildlife(creature->GetEntry()); credit && creature->IsAlive() &&
                creature->GetMapId() == DarkshoreWildlifeFlight::Map && creature->InSamePhase(player) &&
                _protected.insert(creature->GetGUID()).second)
                // Passing the living creature GUID would resolve to its actual
                // entry and record a kill instead of the protection marker.
                player->KilledMonsterCredit(credit, ObjectGuid::Empty, ProtectWildlife);
    }

private:
    static void SendGuideLine(Unit const& guide, Player& rider, uint32 broadcast)
    {
        Trinity::BroadcastTextBuilder builder(&guide, CHAT_MSG_MONSTER_SAY, broadcast, &rider);
        std::unique_ptr<WorldPackets::Packet> packet(builder(rider.GetSession()->GetSessionDbLocaleIndex()));
        rider.SendDirectMessage(packet->Write());
    }

    template <size_t Size>
    int32 Launch(G3D::Vector3 const (&path)[Size], float speed)
    {
        Movement::MoveSplineInit movement(*me);
        movement.Path().assign(std::begin(path), std::end(path));
        movement.SetFly();
        movement.SetSmooth();
        movement.SetUncompressed();
        movement.SetVelocity(speed);
        return movement.Launch();
    }

    void EndFlight()
    {
        _interrupted = true;
        if (Vehicle* vehicle = me->GetVehicleKit())
            if (Unit* rider = vehicle->GetPassenger(RiderSeat); rider && rider->GetGUID() == _owner)
            {
                if (_splineId && me->movespline->GetId() == _splineId && me->movespline->Finalized() &&
                    me->IsWithinDist3d(&Landing, 3.0f))
                    if (Unit* guide = vehicle->GetPassenger(KathrenaSeat))
                        SendGuideLine(*guide, *rider->ToPlayer(), 34594);
                rider->ExitVehicle();
            }
        if (Creature* kathrena = ObjectAccessor::GetCreature(*me, _kathrena))
            kathrena->DespawnOrUnsummon(1);
        me->DespawnOrUnsummon(1);
    }

    ObjectGuid _owner;
    ObjectGuid _kathrena;
    GuidUnorderedSet _protected;
    uint32 _approachId = 0;
    uint32 _splineId = 0;
    uint32 _timeLeft = 0;
    size_t _nextCue = 0;
    bool _boardingRequested = false;
    bool _interrupted = false;
};

class spell_darkshore_protect_wildlife : public SpellScript
{
    PrepareSpellScript(spell_darkshore_protect_wildlife);

    npc_darkshore_grove_hippogryph* GetFlight()
    {
        Creature* mount = GetCaster()->ToCreature();
        if (Player* player = GetCaster()->ToPlayer())
            mount = player->GetVehicleCreatureBase();
        if (!mount || mount->GetEntry() != Hippogryph || mount->GetMapId() != DarkshoreWildlifeFlight::Map)
            return nullptr;
        return CAST_AI(npc_darkshore_grove_hippogryph, mount->AI());
    }

    SpellCastResult CheckCast()
    {
        auto flight = GetFlight();
        Player* rider = flight ? flight->GetActiveRider() : nullptr;
        return rider && (!GetCaster()->IsPlayer() || GetCaster() == rider) ? SPELL_CAST_OK : SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    }

    void SelectWildlife(std::list<WorldObject*>& targets)
    {
        targets.remove_if([](WorldObject* target)
        {
            Creature const* creature = target->ToCreature();
            return !creature || !creature->IsAlive() || !CreditForWildlife(creature->GetEntry());
        });
    }

    void HandleWildlife(SpellEffIndex /*effect*/)
    {
        if (auto flight = GetFlight())
            if (Creature* creature = GetHitCreature())
                flight->Protect(creature);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_darkshore_protect_wildlife::CheckCast);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkshore_protect_wildlife::SelectWildlife,
            EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_darkshore_protect_wildlife::HandleWildlife, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};
}

class spell_darkshore_buried_artifact_detector : public SpellScript
{
    PrepareSpellScript(spell_darkshore_buried_artifact_detector);

    void SelectNearestMarker(std::list<WorldObject*>& targets)
    {
        if (targets.size() < 2)
            return;

        auto caster = GetCaster();
        auto nearest = *std::min_element(targets.begin(), targets.end(), [caster](WorldObject const* first, WorldObject const* second)
        {
            return caster->GetExactDistSq(first) < caster->GetExactDistSq(second);
        });
        targets.clear();
        targets.push_back(nearest);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkshore_buried_artifact_detector::SelectNearestMarker,
            EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
    }
};

void AddSC_darkshore()
{
    RegisterCreatureAI(npc_darkshore_orseus);
    RegisterCreatureAI(npc_darkshore_grove_hippogryph);
    RegisterSpellScript(spell_darkshore_protect_wildlife);
    RegisterSpellScript(spell_darkshore_buried_artifact_detector);
}
