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

#include "Item.h"
#include "MoveSplineInit.h"
#include "ObjectAccessor.h"
#include "PassiveAI.h"
#include "QuestObjectScope.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
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

constexpr uint32 PoisonousChickenQuest = 27088;
constexpr uint32 ChickenCage = 60808;
constexpr uint32 ForestEttin = 44367;
constexpr uint32 BushChicken = 44935;
constexpr uint32 ReleaseBushChicken = 83902;
constexpr uint8 BushChickenSummonSlot = 6;
constexpr uint32 EttinVehicle = 1063;
constexpr uint32 EttinGrabChicken = 83904;
constexpr uint32 ChickenRide = 46598;
constexpr uint32 EttinMouth = 83907;
constexpr uint32 ChickenExplosion = 83903;
constexpr uint32 EttinBonk = 80146;
constexpr uint32 EttinSmash = 88421;
constexpr int8 ArmSeat = 0;
constexpr int8 MouthSeat = 1;

bool IsPlacedEttin(Creature const& ettin)
{
    uint64 spawn = ettin.GetDBTableGUIDLow();
    return ettin.GetEntry() == ForestEttin && ettin.GetMapId() == 0 && (spawn == 128656 || spawn == 128825);
}

bool CanReleaseChicken(Player const& player, Creature const& ettin)
{
    return player.IsAlive() && ettin.IsAlive() && IsPlacedEttin(ettin) &&
        player.GetQuestStatus(PoisonousChickenQuest) == QUEST_STATUS_INCOMPLETE && player.HasItemCount(ChickenCage, 1, false) &&
        player.InSamePhase(&ettin) && player.IsWithinDistInMap(&ettin, 40.0f);
}

struct npc_silverpine_forest_ettin : ScriptedAI
{
    explicit npc_silverpine_forest_ettin(Creature* creature) : ScriptedAI(creature) { }

    enum class FeedingStep { None, Approach, Notice, BoardArm, Lift, BoardMouth, Swallow, Explode };

    bool CanFeed() const
    {
        Vehicle* seats = me->GetVehicleKit();
        return _step == FeedingStep::None && seats && seats->GetVehicleInfo()->ID == EttinVehicle &&
            seats->HasEmptySeat(ArmSeat) && seats->HasEmptySeat(MouthSeat);
    }

    bool OfferChicken(Player& owner, TempSummon& chicken)
    {
        if (!CanFeed() || !CanReleaseChicken(owner, *me) || chicken.GetSummonerGUID() != owner.GetGUID() ||
            chicken.GetEntry() != BushChicken || !chicken.IsAlive() || chicken.GetVehicle())
            return false;

        _owner = owner.GetGUID();
        _chicken = chicken.GetGUID();
        _attempt = owner.GetQuestCancellationReceipt(PoisonousChickenQuest);
        _timeLeft = 20000;
        _step = FeedingStep::Approach;
        chicken.SetReactState(REACT_PASSIVE);
        chicken.SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
        Position point = me->GetNearPosition(1.5f, me->GetRelativeAngle(&chicken));
        chicken.GetMotionMaster()->MovePoint(1, point);
        return true;
    }

    bool IsGrabbing(Unit const* chicken) const
    {
        return chicken && _step == FeedingStep::BoardArm && chicken->GetGUID() == _chicken;
    }

    bool IsEating(Unit const* chicken) const
    {
        return chicken && _step == FeedingStep::Explode && chicken->GetGUID() == _chicken;
    }

    void PassengerBoarded(Unit* passenger, int8 seat, bool apply) override
    {
        if (!apply || passenger->GetGUID() != _chicken)
            return;
        if (_step == FeedingStep::BoardArm && seat == ArmSeat)
        {
            ResumePatrol();
            _step = FeedingStep::Lift;
            _wait = 4000;
        }
        else if (_step == FeedingStep::BoardMouth && seat == MouthSeat)
        {
            _step = FeedingStep::Swallow;
            _wait = 2000;
        }
    }

    void Reset() override
    {
        EndFeeding();
        _combat.Reset();
    }

    void JustDied(Unit*) override { EndFeeding(); }

    void EnterCombat(Unit*) override
    {
        _combat.ScheduleEvent(EttinBonk, 2000);
        _combat.ScheduleEvent(EttinSmash, 8000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_step != FeedingStep::None)
            UpdateFeeding(diff);

        if (!UpdateVictim())
            return;
        _combat.Update(diff);
        if (!me->HasUnitState(UNIT_STATE_CASTING))
            if (uint32 spell = _combat.ExecuteEvent())
            {
                DoCastVictim(spell);
                _combat.ScheduleEvent(spell, spell == EttinBonk ? urand(9000, 11000) : urand(15000, 16000));
            }
        DoMeleeAttackIfReady();
    }

private:
    void ResumePatrol()
    {
        if (_paused)
        {
            me->ResumeMovement();
            _paused = false;
        }
    }

    void EndFeeding()
    {
        ObjectGuid chicken = _chicken;
        _step = FeedingStep::None;
        _wait = 0;
        _timeLeft = 0;
        _chicken.Clear();
        _owner.Clear();
        _attempt.reset();
        ResumePatrol();
        if (!chicken.IsEmpty())
            if (Creature* actor = ObjectAccessor::GetCreature(*me, chicken))
                actor->DespawnOrUnsummon(100);
    }

    void UpdateFeeding(uint32 diff)
    {
        Player* owner = ObjectAccessor::GetPlayer(*me, _owner);
        Creature* chicken = ObjectAccessor::GetCreature(*me, _chicken);
        if (!owner || !chicken || !chicken->IsAlive() || !_attempt || _attempt->load(std::memory_order_acquire) ||
            !CanReleaseChicken(*owner, *me) || diff >= _timeLeft)
        {
            EndFeeding();
            return;
        }
        _timeLeft -= diff;
        if (diff < _wait)
        {
            _wait -= diff;
            return;
        }
        _wait = 0;

        switch (_step)
        {
            case FeedingStep::Approach:
                if (me->IsWithinDistInMap(chicken, 2.5f))
                {
                    me->PauseMovement();
                    _paused = true;
                    me->SetFacingToObject(chicken);
                    _step = FeedingStep::Notice;
                    _wait = 1250;
                }
                break;
            case FeedingStep::Notice:
                _step = FeedingStep::BoardArm;
                me->CastSpell(chicken, EttinGrabChicken, true);
                break;
            case FeedingStep::Lift:
                _step = FeedingStep::BoardMouth;
                if (me->GetVehicleKit()->GetPassenger(ArmSeat) == chicken)
                    chicken->CastSpell(me, EttinMouth, true);
                break;
            case FeedingStep::Swallow:
                if (me->GetVehicleKit()->GetPassenger(MouthSeat) == chicken)
                {
                    _step = FeedingStep::Explode;
                    chicken->CastSpell(chicken, ChickenExplosion, true, nullptr, nullptr, owner->GetGUID());
                    if (me->IsAlive())
                        AttackStart(owner);
                }
                EndFeeding();
                break;
            default:
                break;
        }
    }

    FeedingStep _step = FeedingStep::None;
    ObjectGuid _owner;
    ObjectGuid _chicken;
    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    EventMap _combat;
    uint32 _timeLeft = 0;
    uint32 _wait = 0;
    bool _paused = false;
};

class spell_silverpine_release_bush_chicken : public SpellScript
{
    PrepareSpellScript(spell_silverpine_release_bush_chicken);

    SpellCastResult CheckCage()
    {
        Player* owner = GetCaster()->ToPlayer();
        Unit* explicitTarget = GetExplTargetUnit();
        Creature* ettin = explicitTarget ? explicitTarget->ToCreature() : nullptr;
        Item* cage = GetCastItem();
        if (!owner || !ettin || !cage || cage->GetEntry() != ChickenCage || !CanReleaseChicken(*owner, *ettin))
            return SPELL_FAILED_BAD_TARGETS;
        auto ai = CAST_AI(npc_silverpine_forest_ettin, ettin->AI());
        return ai && ai->CanFeed() ? SPELL_CAST_OK : SPELL_FAILED_NOT_READY;
    }

    void RememberPreviousChicken()
    {
        _previous = GetCaster()->m_SummonSlot[BushChickenSummonSlot];
    }

    TempSummon* FindReleasedChicken(Player& owner)
    {
        ObjectGuid summon = owner.m_SummonSlot[BushChickenSummonSlot];
        if (!summon || summon == _previous)
            return nullptr;
        Creature* creature = ObjectAccessor::GetCreature(owner, summon);
        TempSummon* chicken = creature ? creature->ToTempSummon() : nullptr;
        return chicken && chicken->GetEntry() == BushChicken && chicken->GetSummonerGUID() == owner.GetGUID() &&
            chicken->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == ReleaseBushChicken ? chicken : nullptr;
    }

    void SendChicken(SpellEffIndex)
    {
        Player* owner = GetCaster()->ToPlayer();
        Creature* ettin = GetHitCreature();
        if (!owner || !ettin || !CanReleaseChicken(*owner, *ettin))
            return;
        auto ai = CAST_AI(npc_silverpine_forest_ettin, ettin->AI());
        if (!ai)
            return;

        if (TempSummon* chicken = FindReleasedChicken(*owner))
            _feeding = ai->OfferChicken(*owner, *chicken);
    }

    void DespawnUnassignedChicken()
    {
        if (!_feeding)
            if (Player* owner = GetCaster()->ToPlayer())
                if (TempSummon* chicken = FindReleasedChicken(*owner))
                    chicken->DespawnOrUnsummon();
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_silverpine_release_bush_chicken::CheckCage);
        BeforeCast += SpellCastFn(spell_silverpine_release_bush_chicken::RememberPreviousChicken);
        OnEffectHitTarget += SpellEffectFn(spell_silverpine_release_bush_chicken::SendChicken, EFFECT_1, SPELL_EFFECT_DUMMY);
        AfterCast += SpellCastFn(spell_silverpine_release_bush_chicken::DespawnUnassignedChicken);
    }

    ObjectGuid _previous;
    bool _feeding = false;
};

class spell_silverpine_ettin_grab : public SpellScript
{
    PrepareSpellScript(spell_silverpine_ettin_grab);

    void GrabChicken(SpellEffIndex)
    {
        Creature* ettin = GetCaster()->ToCreature();
        Unit* chicken = GetHitUnit();
        auto ai = ettin ? CAST_AI(npc_silverpine_forest_ettin, ettin->AI()) : nullptr;
        if (ai && ai->IsGrabbing(chicken) && GetEffectValue() == ChickenRide)
            chicken->CastSpell(ettin, ChickenRide, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_silverpine_ettin_grab::GrabChicken, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_silverpine_chicken_explosion : public SpellScript
{
    PrepareSpellScript(spell_silverpine_chicken_explosion);

    void SelectFedEttin(std::list<WorldObject*>& targets)
    {
        Unit* chicken = GetCaster();
        Creature* ettin = chicken->GetVehicleBase() ? chicken->GetVehicleBase()->ToCreature() : nullptr;
        auto ai = ettin ? CAST_AI(npc_silverpine_forest_ettin, ettin->AI()) : nullptr;
        if (!ai || !ai->IsEating(chicken))
        {
            targets.clear();
            return;
        }
        targets.remove_if([ettin](WorldObject const* target) { return target != ettin; });
    }

    void WeakenEttin()
    {
        if (Creature* ettin = GetHitCreature(); ettin && IsPlacedEttin(*ettin) && GetHitDamage() > 0)
            SetHitDamage(int32(CalculatePct(ettin->GetMaxHealth(), 80)));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_silverpine_chicken_explosion::SelectFedEttin, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_silverpine_chicken_explosion::SelectFedEttin, EFFECT_2, TARGET_UNIT_SRC_AREA_ENTRY);
        OnHit += SpellHitFn(spell_silverpine_chicken_explosion::WeakenEttin);
    }
};

constexpr uint32 AtaericQuest = 27518;
constexpr uint32 ShieldCredit = 45769;
constexpr uint32 ArcaneFiend = 45766;
constexpr uint32 ArcaneExpulsion = 87249;
constexpr uint32 ArcaneShield = 87251;
constexpr uint32 SummonArcaneFiend = 87253;
constexpr uint32 AtaericSuicide = 87255;
constexpr float ExpulsionDistance = 15.0f;

struct npc_silverpine_ataeric : ScriptedAI
{
    explicit npc_silverpine_ataeric(Creature* creature) : ScriptedAI(creature), _fiends(me) { }

    void Reset() override
    {
        _owner.Clear();
        _expellingFiend.Clear();
        _attempt.reset();
        _finished = false;
        _summonTimer = 3500;
        _missingOwner = 0;
        _fiends.DespawnAll();

        me->SetReactState(REACT_PASSIVE);
        me->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
        me->SetPowerType(POWER_ENERGY);
        me->SetMaxPower(POWER_ENERGY, 100);
        me->SetPower(POWER_ENERGY, 0);
        if (!me->HasAura(ArcaneShield))
            DoCastSelf(ArcaneShield, true);
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!_owner.IsEmpty() || _finished || me->GetMapId() != 0 || me->GetDBTableGUIDLow() != 254632)
            return;

        Player* player = who->ToPlayer();
        if (!IsEligible(player, 24.0f))
            return;

        _attempt = player->GetQuestCancellationReceipt(AtaericQuest);
        if (!_attempt || _attempt->load())
            return;

        _owner = player->GetGUID();
        Talk(0);
        Talk(7, player->GetGUID());
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->GetEntry() != ArcaneFiend ||
            summon->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != SummonArcaneFiend)
            return;

        _fiends.Summon(summon);
        if (auto player = ActiveOwner())
            summon->AI()->AttackStart(player);
    }

    void SummonedCreatureDespawn(Creature* summon) override { _fiends.Despawn(summon); }

    void SummonedCreatureDies(Creature* summon, Unit* killer) override
    {
        auto found = std::find(_fiends.begin(), _fiends.end(), summon->GetGUID());
        if (found == _fiends.end())
            return;

        _fiends.Despawn(summon);
        Player* owner = ActiveOwner();
        Player* recipient = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!owner || !IsEligible(recipient, 35.0f) ||
            (recipient != owner && (!owner->GetGroup() || owner->GetGroup() != recipient->GetGroup())) ||
            !me->IsWithinDistInMap(summon, ExpulsionDistance) || !me->IsWithinLOSInMap(summon))
            return;

        int32 previous = me->GetPower(POWER_ENERGY);
        _expellingFiend = summon->GetGUID();
        summon->CastSpell(summon, ArcaneExpulsion, true);
        _expellingFiend.Clear();
        int32 energy = me->GetPower(POWER_ENERGY);
        if (energy <= previous)
            return;

        if (energy >= 100)
        {
            _finished = true;
            Talk(5);
            Talk(6);
            me->RemoveAurasDueToSpell(ArcaneShield);
            DoCastSelf(AtaericSuicide, true);
        }
        else if (energy / 20 > previous / 20)
            Talk(uint8(energy / 20));
    }

    bool AcceptsExpulsion(Unit const& caster) const
    {
        return !_finished && caster.GetGUID() == _expellingFiend && !caster.IsAlive() &&
            me->HasAura(ArcaneShield);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        if (!_finished && me->HasAura(ArcaneShield) && damage >= me->GetHealth(attacker))
            damage = 0;
    }

    void JustDied(Unit* killer) override
    {
        _fiends.DespawnAll();
        if (_finished && killer == me && me->GetPower(POWER_ENERGY) >= 100)
            if (Player* owner = ActiveOwner())
                // Native 85376 uses effect 134 with the player as reward source.
                owner->RewardPlayerAndGroupAtEvent(ShieldCredit, owner);

        _owner.Clear();
        _attempt.reset();
    }

    void UpdateAI(uint32 diff) override
    {
        if (_owner.IsEmpty() || _finished)
            return;

        if (!ActiveOwner())
        {
            _missingOwner += diff;
            if (!_attempt || _attempt->load() || _missingOwner >= 6000)
                Reset();
            return;
        }
        _missingOwner = 0;

        if (_fiends.size() >= 3)
            return;
        if (diff < _summonTimer)
        {
            _summonTimer -= diff;
            return;
        }

        _summonTimer = 7000;
        DoCastSelf(SummonArcaneFiend, true);
    }

private:
    bool IsEligible(Player const* player, float distance) const
    {
        return player && player->IsAlive() && !player->isGameMaster() &&
            player->GetQuestStatus(AtaericQuest) == QUEST_STATUS_INCOMPLETE &&
            me->IsWithinDistInMap(player, distance) && me->InSamePhase(player) && me->IsWithinLOSInMap(player);
    }

    Player* ActiveOwner() const
    {
        if (!_attempt || _attempt->load())
            return nullptr;
        auto player = ObjectAccessor::GetPlayer(*me, _owner);
        return IsEligible(player, 35.0f) ? player : nullptr;
    }

    SummonList _fiends;
    ObjectGuid _owner;
    ObjectGuid _expellingFiend;
    std::shared_ptr<QuestCancellationReceipt const> _attempt;
    uint32 _summonTimer = 3500;
    uint32 _missingOwner = 0;
    bool _finished = false;
};

class spell_silverpine_arcane_expulsion : public SpellScript
{
    PrepareSpellScript(spell_silverpine_arcane_expulsion);

    void SelectShield(std::list<WorldObject*>& targets)
    {
        targets.clear();
        TempSummon* fiend = GetCaster()->ToTempSummon();
        if (!fiend || fiend->GetEntry() != ArcaneFiend)
            return;
        Unit* summoner = fiend->GetSummoner();
        auto boss = summoner ? summoner->ToCreature() : nullptr;
        auto ai = boss ? CAST_AI(npc_silverpine_ataeric, boss->AI()) : nullptr;
        if (ai && ai->AcceptsExpulsion(*fiend))
            targets.push_back(boss);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_silverpine_arcane_expulsion::SelectShield, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};
}

void AddSC_silverpine_forest()
{
    RegisterCreatureAI(npc_lordamere_horde_cannon);
    RegisterCreatureAI(npc_lordamere_hillsbrad_worgen);
    RegisterSpellScript(spell_lordamere_rocket_blast);
    RegisterCreatureAI(npc_silverpine_forest_ettin);
    RegisterSpellScript(spell_silverpine_release_bush_chicken);
    RegisterSpellScript(spell_silverpine_ettin_grab);
    RegisterSpellScript(spell_silverpine_chicken_explosion);
    RegisterCreatureAI(npc_silverpine_ataeric);
    RegisterSpellScript(spell_silverpine_arcane_expulsion);
}
