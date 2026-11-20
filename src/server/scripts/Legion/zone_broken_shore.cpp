#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "AreaTriggerAI.h"
#include "AreaTrigger.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SmartAI.h"
#include "Spell.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace Feltotem
{
constexpr uint32 Tugar = 117230;
constexpr uint32 TotemMissile = 243224;
constexpr uint32 TotemSummon = 237903;
constexpr uint32 FelSurge = 242496;
constexpr uint32 Jormog = 117484;
constexpr uint32 Earthquake = 237950;
constexpr uint32 StalactiteWarning = 241395;
constexpr uint32 StalactiteImpact = 238027;
constexpr uint32 FallenStalactite = 238302;
constexpr uint32 StalactiteBreak = 243674;
constexpr uint32 FelHardenedScales = 238471;
constexpr uint32 BurrowedForm = 238703;
constexpr uint32 BurrowedDisplay = 75542;
constexpr uint32 BurrowDuration = 239942;
constexpr uint32 ChargeWarning = 243242;
constexpr uint32 JormogsFury = 241719;

constexpr uint32 ChargeMovement = 1;
constexpr float RockRadius = 2.75f;
constexpr float PlayerHitRadius = 2.5f;
Position const RockSites[] = {
    { 5207.0f, 5057.0f, 626.0f },
    { 5216.0f, 5049.0f, 626.0f },
    { 5218.0f, 5061.0f, 627.0f }
};

bool SegmentTouches(Position const& start, Position const& end, Position const& point, float radius)
{
    float const dx = end.GetPositionX() - start.GetPositionX();
    float const dy = end.GetPositionY() - start.GetPositionY();
    float const lengthSq = dx * dx + dy * dy;
    if (lengthSq < 0.0001f)
        return false;

    float const projection = ((point.GetPositionX() - start.GetPositionX()) * dx +
        (point.GetPositionY() - start.GetPositionY()) * dy) / lengthSq;
    float const fraction = std::clamp(projection, 0.0f, 1.0f);
    float const z = start.GetPositionZ() + (end.GetPositionZ() - start.GetPositionZ()) * fraction;
    return std::abs(point.GetPositionZ() - z) <= 3.0f &&
        point.GetExactDist2dSq(start.GetPositionX() + dx * fraction, start.GetPositionY() + dy * fraction) <= radius * radius;
}

struct TugarAI : SmartAI
{
    explicit TugarAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override
    {
        ++_attempt;
        _totemTimer = 0;
        StopEarthquake();
        SmartAI::Reset();
    }

    void EnterCombat(Unit* enemy) override
    {
        ++_attempt;
        // Legion DBM observed the first Fel Surge cast near 62s. The original
        // 15-yard missile travels at 2 yards/sec before the totem begins it.
        _totemTimer = 54500;
        _earthquakeTimer = 20500;
        SmartAI::EnterCombat(enemy);
    }

    Player* Participant() const
    {
        if (!me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1702 || me->HasUnitState(UNIT_STATE_EVADE))
            return nullptr;

        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1266 || scenario->GetCurrentStep() != 1 || scenario->IsCompleted(false))
            return nullptr;

        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player || !player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() ||
            !me->IsInMap(player) || !me->InSamePhase(player))
            return nullptr;

        return player;
    }

    uint64 ActiveAttempt() const { return Participant() ? _attempt : 0; }

    void JustDied(Unit* killer) override
    {
        StopEarthquake();
        SmartAI::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        UpdateEarthquake(diff);
        if (!_totemTimer || !ActiveAttempt())
            return;
        if (diff < _totemTimer)
        {
            _totemTimer -= diff;
            return;
        }

        _totemTimer = urand(25000, 33000);
        if (me->CastSpell(me, TotemMissile, true) != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Feltotem's Fall: Tugar's Fel Surge Totem missile cast failed in instance %u", me->GetInstanceId());
    }

    void HitStalactites(Creature* worm, Position const& start, Position const& end);
    void StopEarthquake()
    {
        _earthquakeTimer = 0;
        _rockFall = 0;
        RemoveRocks();
    }

private:
    void UpdateEarthquake(uint32 diff);
    void RemoveRocks()
    {
        for (ObjectGuid& guid : _rocks)
        {
            if (AreaTrigger* rock = ObjectAccessor::GetAreaTrigger(*me, guid))
                rock->Remove();
            guid.Clear();
        }
    }

    uint64 _attempt = 0;
    uint32 _totemTimer = 0;
    uint32 _earthquakeTimer = 0;
    uint32 _rockFall = 0;
    std::array<Position, 3> _rockPositions;
    std::array<ObjectGuid, 3> _rocks;
};

TugarAI* FindTugarAI(Unit* caster)
{
    Creature* boss = caster ? caster->ToCreature() : nullptr;
    if (!boss || boss->GetEntry() != Tugar || !boss->IsAIEnabled)
        return nullptr;
    return dynamic_cast<TugarAI*>(boss->AI());
}

uint64 ActiveTugarAttempt(Unit* caster)
{
    auto* ai = FindTugarAI(caster);
    return ai ? ai->ActiveAttempt() : 0;
}

struct JormogAI : SmartAI
{
    explicit JormogAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override
    {
        Surface();
        SmartAI::Reset();
    }

    void JustDied(Unit* killer) override
    {
        if (auto* tugar = FindTugarAI(ObjectAccessor::GetCreature(*me, _tugar)))
            tugar->StopEarthquake();
        Surface();
        SmartAI::JustDied(killer);
    }

    bool Burrow(Creature* tugar, uint64 attempt)
    {
        if (_attempt || !attempt || me->GetDBTableGUIDLow() != 14534072)
            return false;
        _attempt = attempt;
        _tugar = tugar->GetGUID();
        _nextCharge = 6500;
        _burrowTime = 30000;
        _chargesLaunched = 0;
        _combatMove = CanCombatMove();
        _addedFlags = (UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE) & ~me->GetUInt32Value(UNIT_FIELD_FLAGS);
        me->SetFlag(UNIT_FIELD_FLAGS, _addedFlags);
        SetCombatMove(false);
        me->CastStop();
        if (me->CastSpell(me, BurrowedForm, true) != SPELL_CAST_OK || !me->HasAura(BurrowedForm))
        {
            Surface();
            return false;
        }
        // The transform's WDB record includes an invisible fallback model.
        me->SetDisplayId(BurrowedDisplay);
        me->CastSpell(me, BurrowDuration, true);
        return true;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_attempt)
        {
            SmartAI::UpdateAI(diff);
            return;
        }

        auto* tugar = FindTugarAI(ObjectAccessor::GetCreature(*me, _tugar));
        Player* player = tugar && tugar->ActiveAttempt() == _attempt ? tugar->Participant() : nullptr;
        if (!player)
        {
            Surface();
            return;
        }
        if (_chargeInProgress)
        {
            ScanMovement(*tugar, *player);
            if (_chargeArrived || diff >= _chargeInProgress)
                _chargeInProgress = 0;
            else
                _chargeInProgress -= diff;
        }
        if (diff >= _burrowTime)
        {
            Surface();
            return;
        }
        _burrowTime -= diff;

        if (_chargesLaunched == 3)
            return;
        if (diff < _nextCharge)
        {
            _nextCharge -= diff;
            return;
        }
        if (_watchingChargeTarget)
        {
            ++_chargesLaunched;
            _watchingChargeTarget = false;
            _nextCharge = 7500;
            LaunchCharge();
            return;
        }

        _targetPosition = player->GetPosition();
        _watchingChargeTarget = true;
        _nextCharge = 1650;
        me->SetFacingToObject(player);
        me->CastSpell(me, ChargeWarning, true);
        Talk(0, player->GetGUID());
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (_chargeInProgress && type == POINT_MOTION_TYPE && id == ChargeMovement)
            _chargeArrived = true;
        SmartAI::MovementInform(type, id);
    }

private:
    void LaunchCharge()
    {
        float const x = _targetPosition.GetPositionX();
        float const y = _targetPosition.GetPositionY();
        float const z = me->GetHeight(x, y, _targetPosition.GetPositionZ() + 1.0f, true, 5.0f);
        if (z <= INVALID_HEIGHT || !me->IsWithinLOS(x, y, z + 1.0f))
            return;
        _previousPosition = me->GetPosition();
        _chargeInProgress = 5000;
        _chargeArrived = false;
        _playerHit = false;
        me->GetMotionMaster()->MoveCharge(x, y, z, 20.0f, ChargeMovement, true);
    }

    void ScanMovement(TugarAI& tugar, Player& player)
    {
        Position const current = me->GetPosition();
        tugar.HitStalactites(me, _previousPosition, current);
        if (!_playerHit && SegmentTouches(_previousPosition, current, player.GetPosition(), PlayerHitRadius))
        {
            _playerHit = true;
            me->CastSpell(&player, JormogsFury, true);
        }
        _previousPosition = current;
    }

    void Surface()
    {
        if (!_attempt)
            return;
        _attempt = 0;
        _watchingChargeTarget = false;
        _chargeInProgress = 0;
        me->StopMoving();
        me->GetMotionMaster()->Clear(false);
        me->RemoveAurasDueToSpell(BurrowedForm);
        me->RemoveAurasDueToSpell(BurrowDuration);
        me->RemoveFlag(UNIT_FIELD_FLAGS, _addedFlags);
        SetCombatMove(_combatMove && me->IsAlive());
    }

    ObjectGuid _tugar;
    Position _previousPosition;
    Position _targetPosition;
    uint64 _attempt = 0;
    uint32 _addedFlags = 0;
    uint32 _nextCharge = 0;
    uint32 _burrowTime = 0;
    uint32 _chargeInProgress = 0;
    uint8 _chargesLaunched = 0;
    bool _combatMove = false;
    bool _chargeArrived = false;
    bool _playerHit = false;
    bool _watchingChargeTarget = false;
};

void TugarAI::UpdateEarthquake(uint32 diff)
{
    uint64 const attempt = ActiveAttempt();
    if (!attempt)
    {
        _rockFall = 0;
        RemoveRocks();
        return;
    }

    if (_rockFall > diff)
        _rockFall -= diff;
    else if (_rockFall)
    {
        _rockFall = 0;
        for (std::size_t i = 0; i < _rocks.size(); ++i)
        {
            Position const& position = _rockPositions[i];
            if (position.GetPositionZ() <= INVALID_HEIGHT)
                continue;
            me->CastSpell(position, StalactiteImpact, true);
            me->CastSpell(position, FallenStalactite, true);
            std::list<AreaTrigger*> areaTriggers;
            me->GetAreaTriggersWithEntryInRange(areaTriggers, FallenStalactite, me->GetGUID(), 100.0f);
            for (AreaTrigger const* rock : areaTriggers)
                if (rock->GetSpellId() == FallenStalactite && rock->IsInDist2d(&position, 0.3f))
                    _rocks[i] = rock->GetGUID();
            if (!_rocks[i])
                TC_LOG_WARN("scripts", "Feltotem's Fall: stalactite did not publish at site %u in instance %u",
                    uint32(i), me->GetInstanceId());
        }
    }

    if (diff < _earthquakeTimer)
    {
        _earthquakeTimer -= diff;
        return;
    }
    if (!_earthquakeTimer)
        return;

    Creature* worm = me->FindNearestCreature(Jormog, 100.0f);
    auto* ai = worm && worm->IsAIEnabled ? dynamic_cast<JormogAI*>(worm->AI()) : nullptr;
    if (!ai)
    {
        _earthquakeTimer = 0;
        return;
    }
    if (me->CastSpell(me, Earthquake, false) != SPELL_CAST_OK || !me->HasAura(Earthquake))
    {
        _earthquakeTimer = 500;
        return;
    }
    _earthquakeTimer = 60000;
    if (!ai->Burrow(me, attempt))
        return;

    RemoveRocks();
    _rockFall = 2200;
    for (std::size_t i = 0; i < _rocks.size(); ++i)
    {
        Position position = RockSites[i];
        float const z = me->GetHeight(position.GetPositionX(), position.GetPositionY(),
            position.GetPositionZ() + 1.0f, true, 5.0f);
        position.m_positionZ = z;
        _rockPositions[i] = position;
        if (z > INVALID_HEIGHT)
            me->CastSpell(position, StalactiteWarning, true);
    }
}

void TugarAI::HitStalactites(Creature* worm, Position const& start, Position const& end)
{
    for (ObjectGuid& guid : _rocks)
    {
        AreaTrigger* rock = ObjectAccessor::GetAreaTrigger(*me, guid);
        if (!rock || !SegmentTouches(start, end, rock->GetPosition(), RockRadius))
            continue;

        Position const position = rock->GetPosition();
        guid.Clear();
        rock->Remove();
        worm->RemoveAuraFromStack(FelHardenedScales, worm->GetGUID());
        worm->CastSpell(position, StalactiteBreak, true);
    }
}
}

class npc_mage_tower_tugar : public CreatureScript
{
public:
    npc_mage_tower_tugar() : CreatureScript("npc_mage_tower_tugar") {}
    CreatureAI* GetAI(Creature* creature) const override { return new Feltotem::TugarAI(creature); }
};

class npc_mage_tower_jormog : public CreatureScript
{
public:
    npc_mage_tower_jormog() : CreatureScript("npc_mage_tower_jormog") {}
    CreatureAI* GetAI(Creature* creature) const override { return new Feltotem::JormogAI(creature); }
};

class spell_mage_tower_stalactite_impact : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_stalactite_impact);

    bool Load() override { return Feltotem::ActiveTugarAttempt(GetCaster()) != 0; }

    void KnockBack(SpellEffIndex)
    {
        Player* player = GetHitPlayer();
        WorldLocation const* impact = GetExplTargetDest();
        if (player && impact && Feltotem::ActiveTugarAttempt(GetCaster()))
            player->KnockbackFrom(impact->GetPositionX(), impact->GetPositionY(), 10.0f, 5.0f);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_mage_tower_stalactite_impact::KnockBack, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

struct at_mage_tower_broken_stalactite : AreaTriggerAI
{
    explicit at_mage_tower_broken_stalactite(AreaTrigger* trigger) : AreaTriggerAI(trigger) {}
    void OnCreate() override { at->SetDuration(1500); }
};

class npc_mage_tower_fel_surge_totem : public CreatureScript
{
public:
    npc_mage_tower_fel_surge_totem() : CreatureScript("npc_mage_tower_fel_surge_totem") {}

    struct TotemAI : ScriptedAI
    {
        explicit TotemAI(Creature* creature) : ScriptedAI(creature) {}

        void InitializeAI() override
        {
            ScriptedAI::InitializeAI();
            if (me->isTotem() && me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == Feltotem::TotemSummon)
                _attempt = Feltotem::ActiveTugarAttempt(me->ToTempSummon()->GetSummoner());
            if (_attempt)
            {
                me->SetCreateHealth(1);
                me->SetMaxHealth(1);
                me->SetHealth(1);
            }
        }

        bool IsActive() const
        {
            return _attempt && me->IsAlive() &&
                _attempt == Feltotem::ActiveTugarAttempt(me->ToTempSummon()->GetSummoner());
        }

        void UpdateAI(uint32 diff) override
        {
            if (!IsActive())
            {
                me->DespawnOrUnsummon();
                return;
            }
            if (_started)
                return;
            if (_retryTimer > diff)
            {
                _retryTimer -= diff;
                return;
            }

            _started = me->CastSpell(me, Feltotem::FelSurge, false) == SPELL_CAST_OK;
            if (!_started && ++_failedCasts == 3)
            {
                TC_LOG_WARN("scripts", "Feltotem's Fall: Fel Surge Totem could not start its cast in instance %u", me->GetInstanceId());
                me->DespawnOrUnsummon();
            }
            _retryTimer = 250;
        }

        void SpellFinishCast(SpellInfo const* spell) override
        {
            if (spell->Id == Feltotem::FelSurge)
                me->DespawnOrUnsummon(500);
        }

    private:
        uint64 _attempt = 0;
        uint32 _retryTimer = 0;
        uint8 _failedCasts = 0;
        bool _started = false;
    };

    CreatureAI* GetAI(Creature* creature) const override { return new TotemAI(creature); }
};

class spell_mage_tower_fel_surge_missile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge_missile);

    bool Load() override
    {
        _attempt = Feltotem::ActiveTugarAttempt(GetCaster());
        return true;
    }

    void Land(SpellEffIndex effect)
    {
        if (!_attempt || _attempt != Feltotem::ActiveTugarAttempt(GetCaster()))
            PreventHitDefaultEffect(effect);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_mage_tower_fel_surge_missile::Land, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE); }

    uint64 _attempt = 0;
};

class spell_mage_tower_fel_surge_summon : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge_summon);

    void Summon(SpellEffIndex effect)
    {
        if (!Feltotem::ActiveTugarAttempt(GetCaster()))
            PreventHitDefaultEffect(effect);
        else
            SetEffectValue(1);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_mage_tower_fel_surge_summon::Summon, EFFECT_0, SPELL_EFFECT_SUMMON); }
};

class spell_mage_tower_fel_surge : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_fel_surge);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Creature* caster = GetCaster()->ToCreature();
        auto* ai = caster && caster->IsAIEnabled ? dynamic_cast<npc_mage_tower_fel_surge_totem::TotemAI*>(caster->AI()) : nullptr;
        if (!ai || !ai->IsActive())
            targets.clear();
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_fel_surge::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_fel_surge::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

namespace GodQueen
{
constexpr uint32 Faljar = 116496;
constexpr uint32 AncestralShield = 237949;
constexpr uint32 AncestralBlast = 237952;
// The Legion DBM timeline measures the start of the 20-second cast.
constexpr uint32 KnowledgeIntervals[] = { 69200, 118400, 66300, 26700, 27100, 27500 };

struct FaljarAI : SmartAI
{
    explicit FaljarAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override
    {
        StopKnowledge();
        SmartAI::Reset();
    }

    void EnterCombat(Unit* enemy) override
    {
        _engaged = true;
        _interval = 0;
        _remaining = 98400;
        SmartAI::EnterCombat(enemy);
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (id == 3 && value == 3 && me->GetMapId() == 1703)
            StopKnowledge();
        SmartAI::SetData(id, value);
    }

    void JustDied(Unit* killer) override
    {
        StopKnowledge();
        SmartAI::JustDied(killer);
    }

    bool AllowsBlast() const { return (_handoff || _castingBlast) && IsActive(); }

    void UpdateAI(uint32 diff) override
    {
        if (!IsActive())
        {
            if (_handoff || _castingBlast)
                StopKnowledge();
            SmartAI::UpdateAI(diff);
            return;
        }

        Spell const* current = me->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        bool isBlast = current && current->GetSpellInfo()->Id == AncestralBlast;
        if (_handoff)
        {
            if (isBlast)
            {
                _handoff = 0;
                _castingBlast = true;
                _remaining = _interval < std::size(KnowledgeIntervals) ? KnowledgeIntervals[_interval++] : 25000;
            }
            else if (_handoff > diff)
            {
                _handoff -= diff;
                return; // Let the native10ms effect160 start the cast before another Shadowbolt.
            }
            else
            {
                _handoff = 0;
                me->RemoveAurasDueToSpell(AncestralShield, me->GetGUID());
                _remaining = 15000;
                TC_LOG_WARN("scripts", "The God-Queen's Fury: Faljar's shield did not start Ancestral Knowledge in instance %u", me->GetInstanceId());
            }
        }

        if (_castingBlast && !isBlast)
        {
            _castingBlast = false;
            me->RemoveAurasDueToSpell(AncestralShield, me->GetGUID());
        }

        _remaining = _remaining > diff ? _remaining - diff : 0;
        if (!_remaining && !me->IsNonMeleeSpellCast(false))
        {
            _handoff = 250;
            if (me->CastSpell(me, AncestralShield, false) == SPELL_CAST_OK)
                return;
            _handoff = 0;
            _remaining = 1000;
        }
        SmartAI::UpdateAI(diff);
    }

private:
    bool IsActive() const
    {
        if (!_engaged || !me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1703 || me->HasUnitState(UNIT_STATE_EVADE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1265 || scenario->GetCurrentStep() != 1 || scenario->IsCompleted(false))
            return false;
        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        return player && player->IsAlive() && player->IsInWorld() && !player->HasPendingTeleport() &&
            me->IsInMap(player) && me->InSamePhase(player) && !me->IsFriendlyTo(player);
    }

    void StopKnowledge()
    {
        _engaged = false;
        _handoff = 0;
        _castingBlast = false;
        me->InterruptSpell(CURRENT_GENERIC_SPELL, true, true, AncestralBlast);
        me->RemoveAurasDueToSpell(AncestralShield, me->GetGUID());
    }

    uint32 _remaining = 0;
    uint32 _handoff = 0;
    size_t _interval = 0;
    bool _engaged = false;
    bool _castingBlast = false;
};
}

class npc_mage_tower_faljar : public CreatureScript
{
public:
    npc_mage_tower_faljar() : CreatureScript("npc_mage_tower_faljar") {}
    CreatureAI* GetAI(Creature* creature) const override { return new GodQueen::FaljarAI(creature); }
};

class spell_mage_tower_ancestral_blast : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_ancestral_blast);

    SpellCastResult CheckAttempt()
    {
        Creature* caster = GetCaster()->ToCreature();
        auto* ai = caster && caster->GetEntry() == GodQueen::Faljar && caster->IsAIEnabled ?
            dynamic_cast<GodQueen::FaljarAI*>(caster->AI()) : nullptr;
        return ai && ai->AllowsBlast() ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT;
    }

    void Register() override { OnCheckCast += SpellCheckCastFn(spell_mage_tower_ancestral_blast::CheckAttempt); }
};

namespace GodQueen
{
constexpr uint32 Sigryn = 116484;
constexpr uint32 Valkyr = 119696;
constexpr uint32 SummonValkyr = 237730;
constexpr uint32 DarkWingsPrepare = 237769;
constexpr uint32 DarkWingsCharge = 237759;
constexpr uint32 DarkWingsDamage = 237761;

struct SigrynAI;

struct ValkyrAI : ScriptedAI
{
    explicit ValkyrAI(Creature* creature) : ScriptedAI(creature) {}
    bool Prepare(Position const& destination, uint64 wave);
    SigrynAI* ActiveSigryn() const;
    void SpellFinishCast(SpellInfo const* spell) override;
    bool Covers(Unit const& target) const;

private:
    Position _destination;
    uint64 _wave = 0;
    bool _released = false;
};

struct SigrynAI : SmartAI
{
    explicit SigrynAI(Creature* creature) : SmartAI(creature), _wings(creature) {}

    void Reset() override
    {
        StopWaves();
        SmartAI::Reset();
    }

    void EnterCombat(Unit* enemy) override
    {
        _timer = 146000;
        SmartAI::EnterCombat(enemy);
    }

    void JustDied(Unit* killer) override
    {
        StopWaves();
        SmartAI::JustDied(killer);
    }

    Player* Participant() const
    {
        if (!me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1703 || me->HasUnitState(UNIT_STATE_EVADE))
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1265 || scenario->GetCurrentStep() != 1 || scenario->IsCompleted(false))
            return nullptr;
        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        return player && player->IsAlive() && player->IsInWorld() && !player->HasPendingTeleport() &&
            me->IsInMap(player) && me->InSamePhase(player) && !me->IsFriendlyTo(player) ? player : nullptr;
    }

    bool OwnsWave(uint64 wave) const { return wave && wave == _wave && Participant(); }

    bool ClaimHit(Unit& target)
    {
        Player* player = Participant();
        return player && target.GetCharmerOrOwnerPlayerOrPlayerItself() == player && _hits.insert(target.GetGUID()).second;
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!_timer)
            return;
        if (!Participant())
        {
            // A temporary missing victim before a wave must not erase the pull timer.
            if (_published)
                StopWaves();
            return;
        }
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        if (me->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING))
        {
            _timer = 100;
            return;
        }

        _timer = 20000;
        StartWave();
    }

    void JustSummoned(Creature* summon) override
    {
        SmartAI::JustSummoned(summon);
        if (summon->GetEntry() != Valkyr || summon->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != SummonValkyr)
            return;
        auto* ai = summon->IsAIEnabled ? dynamic_cast<ValkyrAI*>(summon->AI()) : nullptr;
        if (!_arming || !ai || !ai->Prepare(_landing, _wave))
        {
            summon->DespawnOrUnsummon();
            return;
        }
        _wings.Summon(summon);
        ++_published;
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon->GetEntry() == Valkyr)
            _wings.Despawn(summon);
        SmartAI::SummonedCreatureDespawn(summon);
    }

private:
    void StartWave()
    {
        ++_wave;
        _wings.DespawnAll();
        _hits.clear();
        _published = 0;
        // These three axes have a full-width, unobstructed flight corridor
        // in the installed 7.3.5 arena. The reverse directions use the same corridors.
        static constexpr float directions[] = { 0.0f, 0.78539816f, 2.3561945f };
        uint32 direction = urand(0, 5);
        float angle = directions[direction % 3] + (direction >= 3 ? 3.14159265f : 0.0f);
        float dx = std::cos(angle), dy = std::sin(angle);
        uint32 gap = urand(1, 7);
        for (uint32 lane = 0; lane < 10; ++lane)
        {
            if (lane == gap || lane == gap + 1)
                continue;
            float offset = (float(lane) - 4.5f) * 6.0f;
            float x = 3300.0f - offset * dy, y = 529.0f + offset * dx;
            _landing.Relocate(x + dx * 28.0f, y + dy * 28.0f, 640.0f, angle);
            _arming = true;
            me->CastSpell(x - dx * 28.0f, y - dy * 28.0f, 640.0f, SummonValkyr, true);
            _arming = false;
        }
        if (_published != 8)
        {
            _wings.DespawnAll();
            _published = 0;
            TC_LOG_WARN("scripts", "The God-Queen's Fury: Sigryn could not publish the full Dark Wings formation in instance %u", me->GetInstanceId());
            return;
        }
        me->CastSpell(me, 237772, true); // Native boss event for the formation warning.
        Talk(8);
        Talk(9);
    }

    void StopWaves()
    {
        _timer = 0;
        _arming = false;
        _published = 0;
        ++_wave;
        _wings.DespawnAll();
        _hits.clear();
    }

    SummonList _wings;
    GuidSet _hits;
    Position _landing;
    uint32 _timer = 0;
    uint32 _published = 0;
    uint64 _wave = 0;
    bool _arming = false;
};

SigrynAI* ValkyrAI::ActiveSigryn() const
{
    TempSummon* summon = me->ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    Creature* boss = owner ? owner->ToCreature() : nullptr;
    auto* ai = boss && boss->GetEntry() == Sigryn && boss->IsAIEnabled ? dynamic_cast<SigrynAI*>(boss->AI()) : nullptr;
    return ai && me->IsAlive() && ai->OwnsWave(_wave) ? ai : nullptr;
}

bool ValkyrAI::Prepare(Position const& destination, uint64 wave)
{
    _wave = wave;
    _destination = destination;
    if (!ActiveSigryn())
        return false;
    me->SetReactState(REACT_PASSIVE);
    me->SetCanFly(true);
    me->SetDisableGravity(true);
    me->SetFacingTo(_destination.GetOrientation());
    return me->CastSpell(me, DarkWingsPrepare, false) == SPELL_CAST_OK;
}

void ValkyrAI::SpellFinishCast(SpellInfo const* spell)
{
    if (spell->Id != DarkWingsPrepare || _released)
        return;
    _released = true;
    if (!ActiveSigryn())
    {
        me->DespawnOrUnsummon();
        return;
    }
    if (me->CastSpell(_destination, DarkWingsCharge, true) == SPELL_CAST_OK)
        me->CastSpell(_destination, DarkWingsDamage, true);
    me->DespawnOrUnsummon(2000);
}

bool ValkyrAI::Covers(Unit const& target) const
{
    float dx = _destination.GetPositionX() - me->GetPositionX();
    float dy = _destination.GetPositionY() - me->GetPositionY();
    float along = (target.GetPositionX() - me->GetPositionX()) * dx +
        (target.GetPositionY() - me->GetPositionY()) * dy;
    return _released && along >= 0.0f && along <= dx * dx + dy * dy;
}
}

class npc_mage_tower_sigryn : public CreatureScript
{
public:
    npc_mage_tower_sigryn() : CreatureScript("npc_mage_tower_sigryn") {}
    CreatureAI* GetAI(Creature* creature) const override { return new GodQueen::SigrynAI(creature); }
};

class npc_mage_tower_dark_valkyr : public CreatureScript
{
public:
    npc_mage_tower_dark_valkyr() : CreatureScript("npc_mage_tower_dark_valkyr") {}
    CreatureAI* GetAI(Creature* creature) const override { return new GodQueen::ValkyrAI(creature); }
};

class spell_mage_tower_dark_wings : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_dark_wings);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Creature* caster = GetCaster()->ToCreature();
        auto* valkyr = caster && caster->IsAIEnabled ? dynamic_cast<GodQueen::ValkyrAI*>(caster->AI()) : nullptr;
        auto* sigryn = valkyr ? valkyr->ActiveSigryn() : nullptr;
        if (!sigryn)
            targets.clear();
        else
            targets.remove_if([valkyr, sigryn](WorldObject* object)
            {
                Unit* target = object->ToUnit();
                return !target || !valkyr->Covers(*target) || !sigryn->ClaimHit(*target);
            });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_dark_wings::FilterTargets, EFFECT_0, TARGET_UNIT_BETWEEN_ENEMY);
    }
};

namespace Twins
{
constexpr uint32 Raest = 116409;
constexpr uint32 RuneWarning = 236468;
constexpr uint32 RuneMissile = 236469;
constexpr uint32 Rune = 236460;
constexpr uint32 NightmareSummon = 236461;
constexpr uint32 Nightmare = 119098;
constexpr uint32 SoakDuration = 3000;

struct RaestAI : SmartAI
{
    explicit RaestAI(Creature* creature) : SmartAI(creature), _nightmares(creature) {}

    void Reset() override
    {
        StopRunes();
        _phaseStarted = false;
        SmartAI::Reset();
    }

    void JustDied(Unit* killer) override
    {
        StopRunes();
        SmartAI::JustDied(killer);
    }

    void SetData(uint32 id, uint32 value) override
    {
        SmartAI::SetData(id, value);
        if (id != 5 || value != 5 || !InFight() || _phaseStarted)
            return;

        // This existing DATA event follows Karam's second Purgatory. The
        // encounter's LFG entry admits a single player.
        if (Player* player = me->SelectNearestPlayerNotGM(100.0f))
        {
            if (!player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() || !me->InSamePhase(player))
                return;
            _phaseStarted = true;
            _participant = player->GetGUID();
            ++_generation;
            _timer = 18200;
        }
    }

    uint64 ActiveGeneration() const { return GetRunePlayer() ? _generation : 0; }

    Player* GetRunePlayer() const
    {
        if (!_participant || !InFight())
            return nullptr;
        Player* player = ObjectAccessor::GetPlayer(*me, _participant);
        return player && player->IsAlive() && player->IsInWorld() && !player->HasPendingTeleport() &&
            me->IsInMap(player) && me->InSamePhase(player) ? player : nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!_participant)
            return;
        if (!GetRunePlayer())
        {
            StopRunes();
            return;
        }
        if (_timer > diff)
        {
            _timer -= diff;
            return;
        }
        _timer = 35000;

        // The original casts land near the central disk, even when the
        // player is kiting on the perimeter. Raest's home is above that disk.
        Position const& home = me->GetHomePosition();
        float angle = frand(-1.6f, 0.4f);
        for (uint8 attempt = 0; attempt < 4; ++attempt, angle += 0.6f)
        {
            float x = home.GetPositionX() + 8.0f * std::cos(angle);
            float y = home.GetPositionY() + 8.0f * std::sin(angle);
            float ground = me->GetHeight(x, y, home.GetPositionZ() + 1.0f, true, 10.0f);
            if (ground < 435.0f || ground > 439.0f || !me->IsWithinLOS(x, y, ground + 1.0f))
                continue;

            if (me->CastSpell(x, y, ground, RuneMissile, true) == SPELL_CAST_OK)
            {
                me->CastSpell(me, RuneWarning, true);
                Talk(7);
                return;
            }
            break;
        }
        TC_LOG_WARN("scripts", "Thwarting the Twins: Raest could not project a Rune of Summoning in instance %u", me->GetInstanceId());
    }

    void JustSummoned(Creature* summon) override
    {
        SmartAI::JustSummoned(summon);
        if (summon->GetEntry() != Nightmare || summon->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != NightmareSummon)
            return;
        Player* player = GetRunePlayer();
        if (!player)
        {
            summon->DespawnOrUnsummon();
            return;
        }
        _nightmares.Summon(summon);
        summon->AI()->AttackStart(player);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon->GetEntry() == Nightmare)
            _nightmares.Despawn(summon);
        SmartAI::SummonedCreatureDespawn(summon);
    }

private:
    bool InFight() const
    {
        if (!me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1684 || me->HasUnitState(UNIT_STATE_EVADE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1273 && scenario->GetCurrentStep() == 1 && !scenario->IsCompleted(false);
    }

    void StopRunes()
    {
        _participant.Clear();
        _timer = 0;
        me->RemoveAreaObject(Rune);
        _nightmares.DespawnAll();
    }

    SummonList _nightmares;
    ObjectGuid _participant;
    uint64 _generation = 0;
    uint32 _timer = 0;
    bool _phaseStarted = false;
};

RaestAI* GetRaest(Unit* caster)
{
    Creature* boss = caster ? caster->ToCreature() : nullptr;
    return boss && boss->GetEntry() == Raest && boss->IsAIEnabled ? dynamic_cast<RaestAI*>(boss->AI()) : nullptr;
}
}

class npc_mage_tower_raest : public CreatureScript
{
public:
    npc_mage_tower_raest() : CreatureScript("npc_mage_tower_raest") {}
    CreatureAI* GetAI(Creature* creature) const override { return new Twins::RaestAI(creature); }
};

class spell_mage_tower_rune_missile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_rune_missile);

    bool Load() override
    {
        if (auto* ai = Twins::GetRaest(GetCaster()))
            _generation = ai->ActiveGeneration();
        return true;
    }

    void Land(SpellEffIndex effect)
    {
        auto* ai = Twins::GetRaest(GetCaster());
        if (!_generation || !ai || _generation != ai->ActiveGeneration())
            PreventHitDefaultEffect(effect);
    }

    void Register() override { OnEffectHit += SpellEffectFn(spell_mage_tower_rune_missile::Land, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE); }

    uint64 _generation = 0;
};

struct at_mage_tower_summoning_rune : AreaTriggerAI
{
    explicit at_mage_tower_summoning_rune(AreaTrigger* trigger) : AreaTriggerAI(trigger) {}

    void OnCreate() override
    {
        if (auto* ai = Twins::GetRaest(at->GetCaster()))
            _generation = ai->ActiveGeneration();
        _lifetime = at->GetDuration();
    }

    void OnUpdate(uint32) override
    {
        Player* player = RunePlayer();
        if (!player || IsSoaked(*player, at->GetElapsedTime()))
            at->Remove(false);
    }

    void OnRemove() override
    {
        if (_lifetime <= 0 || at->GetElapsedTime() < static_cast<uint32>(_lifetime))
            return;
        if (Player* player = RunePlayer())
            if (!IsSoaked(*player, static_cast<uint32>(_lifetime)))
            {
                _generation = 0;
                at->GetCaster()->CastSpell(at->GetPosition(), Twins::NightmareSummon, true);
            }
    }

private:
    Player* RunePlayer() const
    {
        auto* ai = Twins::GetRaest(at->GetCaster());
        return _generation && ai && _generation == ai->ActiveGeneration() ? ai->GetRunePlayer() : nullptr;
    }

    bool IsInside(Unit const& target) const
    {
        return target.IsAlive() && target.IsInWorld() && at->IsInMap(&target) && at->InSamePhase(&target) &&
            at->GetExactDist2d(&target) <= at->GetRadius() &&
            std::abs(at->GetPositionZ() - target.GetPositionZ()) <= 2.5f;
    }

    bool IsSoaked(Player const& player, uint32 elapsed)
    {
        if (!_soaked)
        {
            bool occupied = IsInside(player);
            if (!occupied)
                for (ObjectGuid const& guid : player.m_Controlled)
                {
                    Unit* controlled = ObjectAccessor::GetUnit(player, guid);
                    if (controlled && (controlled->isPet() || controlled->isGuardian()) &&
                        controlled->GetCharmerOrOwnerPlayerOrPlayerItself() == &player && IsInside(*controlled))
                    {
                        occupied = true;
                        break;
                    }
                }
            if (!occupied)
                _occupied = false;
            else if (!_occupied)
            {
                _occupied = true;
                _occupiedSince = elapsed;
            }
            else
                _soaked = elapsed - _occupiedSince >= Twins::SoakDuration;
        }
        return _soaked;
    }

    int32 _lifetime = 0;
    uint64 _generation = 0;
    uint32 _occupiedSince = 0;
    bool _occupied = false;
    bool _soaked = false;
};

class npc_mage_tower_agatha : public CreatureScript
{
public:
    npc_mage_tower_agatha() : CreatureScript("npc_mage_tower_agatha") {}

    struct AgathaAI : SmartAI
    {
        explicit AgathaAI(Creature* creature) : SmartAI(creature) {}

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (!summon || summon->IsAlive() || !me->IsAlive() || !me->isInCombat() || me->GetMapId() != 1616)
                return;

            switch (summon->GetEntry())
            {
                case 115640: // Fuming Imp
                case 115641: // Smoldering Imp
                case 115642: // Umbral Imp
                case 115643: // Imp Servant
                case 115719: // Imp Servant, opposite side
                    break;
                default:
                    return;
            }

            TempSummon* imp = summon->ToTempSummon();
            if (!imp || imp->GetSummonerGUID() != me->GetGUID() || !me->IsInMap(imp))
                return;

            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1221 || scenario->IsCompleted(false))
                return;

            Unit* victim = me->getVictim();
            Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player || !player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() ||
                !me->IsInMap(player) || !me->InSamePhase(player))
                return;

            me->CastSpell(player->GetPosition(), 242879, true); // Agatha's Fury at the player's feet.
        }
    };

    CreatureAI* GetAI(Creature* creature) const override { return new AgathaAI(creature); }
};

// 117470
class boss_sivash : public CreatureScript
{
public:
    boss_sivash() : CreatureScript("boss_sivash") {}

    struct boss_sivashAI : public ScriptedAI
    {
        boss_sivashAI(Creature* creature) : ScriptedAI(creature), summons(me) {}

        EventMap events;
        SummonList summons;

        void Reset() override
        {
            events.Reset();
            me->RemoveAllAreaObjects();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 25000); // summons 233968
            events.RescheduleEvent(EVENT_2, 13000); // 241433
            events.RescheduleEvent(EVENT_3, 23000); // 233996 sea with help 233977
        }

        void JustDied(Unit* who) override
        {
            summons.DespawnAll();
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() != 117487)
                return;

            summons.Summon(summon);
            if (auto target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                summon->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        me->SummonCreature(117487, me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ());
                        me->SummonCreature(117487, me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ());
                        events.RescheduleEvent(EVENT_1, 25000);
                        break;
                    case EVENT_2:
                        DoCast(241433);
                        events.RescheduleEvent(EVENT_2, 13000);
                        break;
                    case EVENT_3:
                        DoCast(233996);
                        for (uint8 i = 0; i < urand(1, 2); ++i)
                            me->CastSpell(me->GetPositionX() + irand(-35, 35), me->GetPositionY() + irand(-35, 35), me->GetPositionZ(), 233970, true);
                        events.RescheduleEvent(EVENT_3, 23000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_sivashAI(creature);
    }
};

// 13481
class areatrigger_at_tidal_wave : public AreaTriggerScript
{
    public:
        areatrigger_at_tidal_wave() : AreaTriggerScript("areatrigger_at_tidal_wave") {}

    struct areatrigger_at_tidal_waveAI : AreaTriggerAI
    {
        areatrigger_at_tidal_waveAI(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) {}

        bool CalculateSpline(Position const* pos, Position& startPos, Position& endPos, std::vector<Position>& path) override
        {
            endPos.m_positionX = pos->GetPositionX() + irand(-30, 30);
            endPos.m_positionY = pos->GetPositionY() + 50;
            endPos.m_positionZ = pos->GetPositionZ() - 5;
            if (Unit* caster = at->GetCaster())
            {
                startPos.m_positionX = caster->GetPositionX() + irand(-55, 55);
                startPos.m_positionY = caster->GetPositionY() - 90;
                startPos.m_positionZ = caster->GetPositionZ() - 7;
                return true;
            }
            else
                return false;
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areatrigger) const override
    {
        return new areatrigger_at_tidal_waveAI(areatrigger);
    }
};

// 121124
class boss_apocron : public CreatureScript
{
public:
    boss_apocron() : CreatureScript("boss_apocron") {}

    struct boss_apocronAI : public ScriptedAI
    {
        boss_apocronAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 23000); // 241458
            events.RescheduleEvent(EVENT_2, 35000); // 241518 (random)
            events.RescheduleEvent(EVENT_3, 11000); // 241498 script (241507)
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            if (auto victim = me->getVictim())
            {
                if (abs(victim->GetPositionZ() - me->GetPositionZ()) >= 4.0f)
                {
                    EnterEvadeMode();
                    return;
                }
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(241458);
                        events.RescheduleEvent(EVENT_1, 23000); // 241458
                        break;
                    case EVENT_2:
                        DoCast(241518);
                        events.RescheduleEvent(EVENT_2, 24000); // 241518 (random)
                        break;
                    case EVENT_3:
                        DoCast(241498);
                        me->AddDelayedEvent(2100, [actor = me] () -> void
                        {
                            uint8 r = urand(5, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        me->AddDelayedEvent(3100, [actor = me] () -> void
                        {
                            uint8 r = urand(5, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        me->AddDelayedEvent(4100, [actor = me] () -> void
                        {
                            uint8 r = urand(8, 11);
                            for (uint8 i = 0; i < r; ++i)
                                { if (auto ai = actor->AI()) ai->DoCast(241507); }
                        });

                        events.RescheduleEvent(EVENT_3, 16000); // 241498 script (241507)
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_apocronAI(creature);
    }
};

// 117239
class boss_bs_brutallus : public CreatureScript
{
public:
    boss_bs_brutallus() : CreatureScript("boss_bs_brutallus") {}

    struct boss_bs_brutallusAI : public ScriptedAI
    {
        boss_bs_brutallusAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 19000); // 233484
            events.RescheduleEvent(EVENT_2, 30000); // 233566
            events.RescheduleEvent(EVENT_3, 20000); // 233515
        }


        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(233484);
                        events.RescheduleEvent(EVENT_1, 19000); // 233484
                        break;
                    case EVENT_2:
                        DoCast(233566);
                        events.RescheduleEvent(EVENT_2, 18000); // 233566
                        break;
                    case EVENT_3:
                        DoCast(233515);
                        events.RescheduleEvent(EVENT_3, 20+18000); // 233515
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_bs_brutallusAI(creature);
    }
};

//233484
class spell_meteor_slash : public SpellScript
{
    PrepareSpellScript(spell_meteor_slash);

    uint8 targetsCount = 0;

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targetsCount = targets.size();
    }

    void HandleDamage(SpellEffIndex /*effectIndex*/)
    {
        if (targetsCount)
            SetHitDamage(GetHitDamage() / targetsCount);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_meteor_slash::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
        OnEffectHitTarget += SpellEffectFn(spell_meteor_slash::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 117303
class boss_malificus : public CreatureScript
{
public:
    boss_malificus() : CreatureScript("boss_malificus") {}

    struct boss_malificusAI : public ScriptedAI
    {
        boss_malificusAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* unit) override
        {
            events.RescheduleEvent(EVENT_1, 17000); // 234452
            events.RescheduleEvent(EVENT_2, 15000); // 233614
            events.RescheduleEvent(EVENT_3, 16000); // 233569
        }


        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(234452);
                        events.RescheduleEvent(EVENT_1, 19000); // 234452
                        break;
                    case EVENT_2:
                        DoCast(233614);
                        events.RescheduleEvent(EVENT_2, 16000); // 233614
                        break;
                    case EVENT_3:
                        DoCast(233569);
                        events.RescheduleEvent(EVENT_3, 18000); // 233569
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_malificusAI(creature);
    }
};

void AddSC_broken_shore()
{
    new npc_mage_tower_tugar();
    new npc_mage_tower_jormog();
    RegisterSpellScript(spell_mage_tower_stalactite_impact);
    RegisterAreaTriggerAI(at_mage_tower_broken_stalactite);
    new npc_mage_tower_fel_surge_totem();
    RegisterSpellScript(spell_mage_tower_fel_surge_missile);
    RegisterSpellScript(spell_mage_tower_fel_surge_summon);
    RegisterSpellScript(spell_mage_tower_fel_surge);
    new npc_mage_tower_faljar();
    RegisterSpellScript(spell_mage_tower_ancestral_blast);
    new npc_mage_tower_sigryn();
    new npc_mage_tower_dark_valkyr();
    RegisterSpellScript(spell_mage_tower_dark_wings);
    new npc_mage_tower_raest();
    RegisterSpellScript(spell_mage_tower_rune_missile);
    RegisterAreaTriggerAI(at_mage_tower_summoning_rune);
    new npc_mage_tower_agatha();
    new boss_sivash();
    new areatrigger_at_tidal_wave();
    new boss_apocron();
    new boss_bs_brutallus();
    RegisterSpellScript(spell_meteor_slash);
    new boss_malificus();
}
