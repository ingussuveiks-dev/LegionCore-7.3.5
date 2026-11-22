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

#include "MoveSplineInit.h"
#include "PassiveAI.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "Vehicle.h"

namespace
{
constexpr uint32 WatersRunRed = 27232;
constexpr uint32 HordeCannon = 45263;
constexpr uint32 HillsbradWorgen = 45270;
constexpr uint32 RocketImpact = 84574;
constexpr uint32 RocketLaunch = 84575;

// Approach the three pier cannons from the water between the island and dock.
// Z for both ends is resolved from the extracted Lordamere Lake surface.
struct LakeApproach
{
    float islandX, islandY, pierX, pierY;
};

constexpr LakeApproach WorgenApproaches[] = {
    {700.6f, 749.2f, 670.0f, 921.0f},
    {710.0f, 785.0f, 703.0f, 920.0f},
    {762.0f, 795.0f, 748.0f, 923.0f},
    {771.0f, 759.0f, 785.0f, 911.0f},
};

Player* GetCannonRider(Unit* cannon)
{
    if (!cannon || cannon->GetEntry() != HordeCannon || cannon->GetMapId() != 0)
        return nullptr;
    auto vehicle = cannon->GetVehicleKit();
    auto passenger = vehicle ? vehicle->GetPassenger(0) : nullptr;
    return passenger ? passenger->ToPlayer() : nullptr;
}

bool IsActiveCannonRider(Player* player)
{
    return player && player->IsAlive() && player->GetQuestStatus(WatersRunRed) == QUEST_STATUS_INCOMPLETE;
}

struct npc_lordamere_horde_cannon : PassiveAI
{
    explicit npc_lordamere_horde_cannon(Creature* creature) : PassiveAI(creature), _worgen(me) { }

    // The controlling seat drives the cannon, while this controller still
    // owns wave timing and receives native boarding and dismount callbacks.
    void OnCharmed(bool) override { }

    void PassengerBoarded(Unit* passenger, int8 seat, bool boarded) override
    {
        if (seat != 0 || !passenger->IsPlayer())
            return;

        ++_rideSequence;
        _worgen.DespawnAll();
        _waveTimer = boarded ? 1 : 0;
        _wave = 0;
    }

    void JustSummoned(Creature* summon) override { _worgen.Summon(summon); }
    void SummonedCreatureDespawn(Creature* summon) override { _worgen.Despawn(summon); }
    void SummonedCreatureDies(Creature* summon, Unit*) override { _worgen.Despawn(summon); }
    uint32 GetRideSequence() const { return _rideSequence; }

    void UpdateAI(uint32 diff) override
    {
        auto rider = GetCannonRider(me);
        if (!rider)
            return;
        if (!IsActiveCannonRider(rider))
        {
            rider->ExitVehicle();
            return;
        }
        if (_waveTimer > diff)
        {
            _waveTimer -= diff;
            return;
        }
        _waveTimer = 4500;
        if (_worgen.size() >= 24)
            return;

        LakeApproach const& route = WorgenApproaches[_wave++ % std::size(WorgenApproaches)];
        for (float spacing : {-4.0f, 0.0f, 4.0f})
            LaunchSwimmer(route, spacing);
    }

private:
    void LaunchSwimmer(LakeApproach const& route, float spacing)
    {
        Map* map = me->GetMap();
        float const islandX = route.islandX + spacing;
        float const pierX = route.pierX + spacing;
        LiquidData startWater, endWater;
        float const dockHeight = me->GetPositionZ();
        if (!map->getLiquidStatus(islandX, route.islandY, dockHeight, MAP_LIQUID_TYPE_WATER, &startWater) ||
            !map->getLiquidStatus(pierX, route.pierY, dockHeight, MAP_LIQUID_TYPE_WATER, &endWater))
            return;

        Position start(islandX, route.islandY, startWater.level - 0.25f);
        start.SetOrientation(start.GetAngle(pierX, route.pierY));
        auto properties = sSummonPropertiesStore.LookupEntry(64);
        if (auto swimmer = map->SummonCreature(HillsbradWorgen, start, properties, 50000, me))
        {
            swimmer->SetTempSummonType(TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN);
            swimmer->SetSwim(true);
            Movement::MoveSplineInit movement(*swimmer);
            movement.MoveTo(pierX, route.pierY, endWater.level - 0.25f, false);
            movement.SetVelocity(4.5f);
            movement.Launch();
        }
    }

    SummonList _worgen;
    uint32 _waveTimer = 0;
    uint32 _wave = 0;
    uint32 _rideSequence = 0;
};

struct npc_lordamere_hillsbrad_worgen : PassiveAI
{
    explicit npc_lordamere_hillsbrad_worgen(Creature* creature) : PassiveAI(creature)
    {
        // Template 2166 is neutral to the seated Horde gunner. Only these
        // private attackers should become eligible enemies for Rocket Blast.
        if (me->HasQuestObjectScope())
            me->setFaction(14);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        auto swimmer = me->ToTempSummon();
        if (!swimmer || !attacker || type != SPELL_DIRECT_DAMAGE || swimmer->GetSummonerGUID() != attacker->GetGUID() ||
            !IsActiveCannonRider(GetCannonRider(attacker)))
            damage = 0;
    }
};

class spell_lordamere_rocket_blast : public SpellScript
{
    PrepareSpellScript(spell_lordamere_rocket_blast);

    SpellCastResult CheckRider()
    {
        return IsActiveCannonRider(GetCannonRider(GetCaster())) ? SPELL_CAST_OK : SPELL_FAILED_CASTER_AURASTATE;
    }

    void RememberLaunchRider()
    {
        if (auto rider = GetCannonRider(GetCaster()))
            if (auto ai = CAST_AI(npc_lordamere_horde_cannon, GetCaster()->GetAI()))
            {
                _rideSequence = ai->GetRideSequence();
                _attempt = rider->GetQuestCancellationReceipt(WatersRunRed);
            }
    }

    void CheckLaunchImpact(SpellEffIndex effect)
    {
        auto rider = GetCannonRider(GetCaster());
        auto ai = CAST_AI(npc_lordamere_horde_cannon, GetCaster()->GetAI());
        if (!IsActiveCannonRider(rider) || !ai || ai->GetRideSequence() != _rideSequence || !_attempt ||
            rider->GetQuestCancellationReceipt(WatersRunRed) != _attempt)
            PreventHitDefaultEffect(effect);
    }

    void SelectSwimmers(std::list<WorldObject*>& targets)
    {
        Unit* cannon = GetCaster();
        if (!IsActiveCannonRider(GetCannonRider(cannon)))
        {
            targets.clear();
            return;
        }
        targets.remove_if([&](WorldObject* target)
        {
            auto creature = target->ToCreature();
            auto swimmer = creature ? creature->ToTempSummon() : nullptr;
            return !swimmer || swimmer->GetEntry() != HillsbradWorgen || !swimmer->IsAlive() || swimmer->IsQuestObjectExpired() ||
                swimmer->GetSummonerGUID() != cannon->GetGUID();
        });
    }

    void Register() override
    {
        if (m_scriptSpellId == RocketLaunch)
        {
            OnCheckCast += SpellCheckCastFn(spell_lordamere_rocket_blast::CheckRider);
            BeforeCast += SpellCastFn(spell_lordamere_rocket_blast::RememberLaunchRider);
            OnEffectHit += SpellEffectFn(spell_lordamere_rocket_blast::CheckLaunchImpact, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
        }
        else if (m_scriptSpellId == RocketImpact)
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_lordamere_rocket_blast::SelectSwimmers,
                EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
    }

    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    uint32 _rideSequence = 0;
};
}

void AddSC_silverpine_forest()
{
    RegisterCreatureAI(npc_lordamere_horde_cannon);
    RegisterCreatureAI(npc_lordamere_hillsbrad_worgen);
    RegisterSpellScript(spell_lordamere_rocket_blast);
}
