#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "AreaTriggerAI.h"
#include "AreaTrigger.h"
#include "ConditionMgr.h"
#include "GossipData.h"
#include "GossipDef.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SmartAI.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <set>

namespace ClosingTheEye
{
constexpr uint32 VictoriaMenu = 20778;
constexpr uint32 PaidEntry = 6;
constexpr uint32 FreeEntry = 7;
constexpr uint32 AdmissionCurrency = 1226;
constexpr int32 AdmissionCost = 100;
constexpr uint32 FirstAttemptMarker = 45635;
constexpr uint32 TeleportSpell = 229196;
constexpr uint32 ScenarioMap = 1673;

bool IsAcceptedXylemEntry(bool farTransfer, WorldLocation const& actual, SpellTargetPosition const* destination)
{
    return farTransfer && destination && destination->target_mapId == ScenarioMap &&
        actual.GetMapId() == ScenarioMap &&
        std::abs(actual.GetPositionX() - destination->target_X) < 0.01f &&
        std::abs(actual.GetPositionY() - destination->target_Y) < 0.01f &&
        std::abs(actual.GetPositionZ() - destination->target_Z) < 0.01f;
}

struct VictoriaAI : SmartAI
{
    explicit VictoriaAI(Creature* creature) : SmartAI(creature) {}

    void sGossipSelect(Player* player, uint32 menu, uint32 selection) override
    {
        if (menu != VictoriaMenu || (selection != PaidEntry && selection != FreeEntry))
        {
            SmartAI::sGossipSelect(player, menu, selection);
            return;
        }

        if (!player || !player->IsAlive() || !player->IsInWorld() || player->HasPendingTeleport() ||
            !me->IsInMap(player) || !me->InSamePhase(player) || me->GetMapId() != 1220 || me->GetDBTableGUIDLow() != 12742696)
            return;

        GossipMenu const& shown = player->PlayerTalkClass->GetGossipMenu();
        if (shown.GetMenuId() != menu || !shown.GetItem(selection))
            return;

        auto const options = sGossipDataStore->GetGossipMenuItemsMapBounds(menu);
        auto const option = std::find_if(options.first, options.second,
            [selection](auto const& item) { return item.second.OptionIndex == selection; });
        if (option == options.second || !sConditionMgr->IsObjectMeetToConditions(player, me, option->second.Conditions) ||
            (selection == PaidEntry && !player->HasCurrency(AdmissionCurrency, AdmissionCost)))
            return;

        // Preserve the SmartAI specialization setup, scenario selection and native teleport.
        uint16 const previousScenario = player->GetScenarioId();
        SmartAI::sGossipSelect(player, menu, selection);
        if (!IsAcceptedXylemEntry(player->IsBeingTeleportedFar(), player->GetTeleportDest(),
            sSpellMgr->GetSpellTargetPosition(TeleportSpell)))
        {
            if (!player->HasPendingTeleport())
                player->SetScenarioId(previousScenario);
            return;
        }

        if (selection == PaidEntry)
            player->ModifyCurrency(AdmissionCurrency, -AdmissionCost, false, true);
        else if (!player->GetQuestRewardStatus(FirstAttemptMarker))
            player->SetRewardedQuest(FirstAttemptMarker);
    }
};
}

class npc_mage_tower_victoria : public CreatureScript
{
public:
    npc_mage_tower_victoria() : CreatureScript("npc_mage_tower_victoria") {}
    CreatureAI* GetAI(Creature* creature) const override { return new ClosingTheEye::VictoriaAI(creature); }
};

namespace ClosingTheEye
{
constexpr uint32 Xylem = 115244;
constexpr uint32 FocusingIris = 64693;
constexpr uint32 DrawPowerChannel = 231522;
constexpr uint32 DrawPowerStack = 231610;

enum XylemIntermissionSpells : uint32
{
    MirrorImage = 115543,
    MirrorPosition = 116492,
    PrismaticShock = 231914,
    TimeAcceleration = 231980,
    ArcaneBarrier = 231982,
    DarknessAura = 232006,
    DarknessDamage = 232007,
    ArcaneAnnihilation = 234728,
    CloakOfGlitteringRunes = 241048,
    GlitteringRunesVisual = 241062,
    FrostPhase = 242394,
    ArcanePhase = 242386,
    CancelAnnihilation = 164393,
    HunterFlare = 132950,
    ArcaneBarrierTrigger = 9255,
    ArcaneBarrierTemplate = 90231982,
    SetCombatPhase = 2401
};

struct XylemAI : SmartAI
{
    explicit XylemAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override;
    void UpdateAI(uint32 diff) override;
    void SummonedCreatureDies(Creature* summon, Unit* killer) override;
    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override;
    void MoveInLineOfSight(Unit* target) override;

    // BEGIN Draw Power controller
    bool CanDrawPower(Creature const* focus)
    {
        if (!focus || focus->GetEntry() != FocusingIris || focus->GetDBTableGUIDLow() != 14534039 ||
            !focus->IsAlive() || !focus->IsInWorld() || !me->IsInMap(focus) || !me->InSamePhase(focus) ||
            !me->IsAlive() || !me->isInCombat())
            return false;

        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1204 || scenario->GetCurrentStep() != 1 || scenario->IsCompleted(false))
            return false;

        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player || !player->IsAlive() || !player->IsInWorld() ||
            player->HasPendingTeleport() || !me->IsInMap(player) || !me->InSamePhase(player))
            return false;

        Spell const* channel = me->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        return channel && channel->GetSpellInfo()->Id == DrawPowerChannel;
    }
    // END Draw Power controller

    bool OwnsMirror(Creature const* image) const;
    bool CanHitInIntermission(Unit* target) const;
    bool IsProtected(Unit const& target);
    bool IsCastingAnnihilation() const;
    bool IsInIntermission() const { return _phase == Phase::Intermission; }

private:
    enum class Phase : uint32
    {
        FirstFrost = 1,
        Arcane,
        Intermission,
        Frost
    };

    Player* GetAttemptPlayer() const;
    bool IsEncounterActive() const;
    bool CanReveal(Player& player);
    void StartIntermission(Player& player);
    void ClearIntermission(Player* player);
    void ResumeFight(Player& player);
    void UpdateEncounter(uint32 diff);

    ObjectGuid _player;
    std::set<ObjectGuid> _images;
    Phase _phase = Phase::FirstFrost;
    Phase _nextPhase = Phase::Arcane;
    uint32 _phaseRemaining = 90000;
    ReactStates _previousReaction = REACT_AGGRESSIVE;
};

XylemAI* GetXylemAI(Unit* caster)
{
    Creature* xylem = caster ? caster->ToCreature() : nullptr;
    if (!xylem || xylem->GetEntry() != Xylem || xylem->GetMapId() != ScenarioMap ||
        xylem->GetDBTableGUIDLow() != 14534036 || !xylem->IsAIEnabled)
        return nullptr;

    return dynamic_cast<XylemAI*>(xylem->AI());
}

bool XylemAI::IsEncounterActive() const
{
    if (me->GetMapId() != ScenarioMap || me->GetDBTableGUIDLow() != 14534036 || !me->IsAlive() || !me->isInCombat())
        return false;

    Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
    return scenario && scenario->GetScenarioId() == 1204 && scenario->GetCurrentStep() == 1 && !scenario->IsCompleted(false);
}

Player* XylemAI::GetAttemptPlayer() const
{
    Player* player = ObjectAccessor::GetPlayer(*me, _player);
    return player && player->IsAlive() && player->IsInWorld() && !player->IsPreDelete() &&
        !player->IsBeingTeleportedFar() && me->IsInMap(player) && me->InSamePhase(player) &&
        player->GetQuestStatus(44925) == QUEST_STATUS_INCOMPLETE ? player : nullptr;
}

bool XylemAI::IsCastingAnnihilation() const
{
    Spell const* cast = me->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    return cast && cast->getState() != SPELL_STATE_FINISHED && cast->GetSpellInfo()->Id == ArcaneAnnihilation;
}

bool XylemAI::OwnsMirror(Creature const* image) const
{
    return IsInIntermission() && image && image->GetEntry() == MirrorImage &&
        me->IsInMap(image) && _images.count(image->GetGUID()) && GetAttemptPlayer();
}

bool XylemAI::CanHitInIntermission(Unit* target) const
{
    Player* player = IsInIntermission() && IsEncounterActive() ? GetAttemptPlayer() : nullptr;
    return player && target && target->IsAlive() && me->IsInMap(target) && me->InSamePhase(target) &&
        target->GetCharmerOrOwnerPlayerOrPlayerItself() == player;
}

bool XylemAI::IsProtected(Unit const& target)
{
    std::list<AreaTrigger*> barriers;
    me->GetAreaObjectList(barriers, ArcaneBarrier);
    for (AreaTrigger const* barrier : barriers)
    {
        int32 const remaining = barrier->GetDuration();
        if (remaining <= 0 || barrier->GetRealEntry() != ArcaneBarrierTrigger)
            continue;

        // The client scale animates down to 10% over the native ten-second life.
        float const radius = 8.0f * (0.1f + 0.9f * std::min(remaining, 10000) / 10000.0f);
        if (std::abs(barrier->GetPositionZ() - target.GetPositionZ()) < 4.0f &&
            barrier->GetExactDist2d(target.GetPositionX(), target.GetPositionY()) <= radius)
            return true;
    }
    return false;
}

bool XylemAI::CanReveal(Player& player)
{
    if (!IsCastingAnnihilation() || player.HasPendingTeleport() ||
        std::abs(me->GetPositionZ() - player.GetPositionZ()) > 4.0f)
        return false;

    if (me->GetExactDist2d(player.GetPositionX(), player.GetPositionY()) <= 4.0f)
        return true;

    std::list<AreaTrigger*> flares;
    player.GetAreaObjectList(flares, HunterFlare);
    return std::any_of(flares.begin(), flares.end(), [this, &player](AreaTrigger const* flare)
    {
        return flare->GetCasterGUID() == player.GetGUID() && flare->GetRealEntry() == 510 && flare->GetDuration() > 0 &&
            me->IsInMap(flare) && me->InSamePhase(flare) &&
            std::abs(me->GetPositionZ() - flare->GetPositionZ()) < 4.0f &&
            me->GetExactDist2d(flare->GetPositionX(), flare->GetPositionY()) <= 10.0f;
    });
}

void XylemAI::StartIntermission(Player& player)
{
    std::list<Creature*> anchors;
    me->GetCreatureListWithEntryInGrid(anchors, MirrorPosition, 105.0f);
    anchors.remove_if([this](Creature const* anchor)
    {
        uint64 const spawn = anchor->GetDBTableGUIDLow();
        return spawn < 14534029 || spawn > 14534043 || !me->InSamePhase(anchor);
    });
    std::vector<Creature const*> rim;
    for (Creature const* anchor : anchors)
        if (anchor->GetDBTableGUIDLow() < 14534039 && player.GetExactDist2d(anchor->GetPositionX(), anchor->GetPositionY()) >= 22.0f)
            rim.push_back(anchor);

    if (anchors.size() != 12 || rim.empty())
    {
        TC_LOG_ERROR("scripts", "Closing the Eye: expected 12 mirror positions with a usable rim location, found %u", uint32(anchors.size()));
        SmartAI::EnterEvadeMode();
        return;
    }

    Creature const* destination = rim[urand(0, uint32(rim.size() - 1))];
    _nextPhase = _phase == Phase::Arcane ? Phase::Frost : Phase::Arcane;
    _phase = Phase::Intermission;
    SmartAI::SetData(SetCombatPhase, uint32(_phase));
    me->InterruptNonMeleeSpells(true);
    me->StopMoving();
    me->GetMotionMaster()->Clear();
    me->GetMotionMaster()->MoveIdle();
    me->AttackStop();
    _previousReaction = me->GetReactState();
    me->SetReactState(REACT_ATTACK_OFF);
    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    me->NearTeleportTo(destination->GetPosition());

    if (me->CastSpell(me, ArcaneAnnihilation, false) != SPELL_CAST_OK || !IsCastingAnnihilation())
    {
        TC_LOG_ERROR("scripts", "Closing the Eye: Xylem could not start Arcane Annihilation");
        ClearIntermission(&player);
        SmartAI::EnterEvadeMode();
        return;
    }

    me->CastSpell(me, CloakOfGlitteringRunes, true);
    me->CastSpell(me, DarknessAura, true);
    for (Creature const* anchor : anchors)
    {
        if (anchor == destination)
            continue;

        if (TempSummon* image = me->SummonCreature(MirrorImage, anchor->GetPosition(), TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45000))
            _images.insert(image->GetGUID());
    }

    if (_images.size() != 11)
    {
        TC_LOG_ERROR("scripts", "Closing the Eye: expected 11 mirror images, published %u", uint32(_images.size()));
        ClearIntermission(&player);
        SmartAI::EnterEvadeMode();
    }
}

void XylemAI::SummonedCreatureDies(Creature* summon, Unit* killer)
{
    if (!OwnsMirror(summon) || !_images.erase(summon->GetGUID()) || !CanHitInIntermission(killer))
        return;

    auto const* spell = sSpellMgr->GetSpellInfo(ArcaneBarrier);
    if (!spell)
        return;

    auto barrier = std::make_unique<AreaTrigger>();
    Position const pos = summon->GetPosition();
    if (!barrier->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
        ArcaneBarrierTrigger, me, spell, pos, pos, nullptr, ObjectGuid::Empty, ArcaneBarrierTemplate))
    {
        TC_LOG_ERROR("scripts", "Closing the Eye: Xylem could not publish the Arcane Barrier of Replenishment");
        return;
    }
    barrier->SetSphereScale(-0.9f, barrier->GetDuration(), false, 0.05f);
    barrier->UpdateOverrideScale();
    barrier.release();
}

void XylemAI::ClearIntermission(Player* player)
{
    me->InterruptNonMeleeSpells(true, ArcaneAnnihilation);
    me->RemoveAurasDueToSpell(DarknessAura);
    me->RemoveAurasDueToSpell(CloakOfGlitteringRunes);
    me->RemoveAurasDueToSpell(GlitteringRunesVisual);
    me->RemoveAreaObject(ArcaneBarrier);
    if (player)
    {
        player->RemoveAurasDueToSpell(TimeAcceleration, me->GetGUID());
        player->RemoveAurasDueToSpell(DarknessDamage, me->GetGUID());
    }

    std::set<ObjectGuid> images;
    images.swap(_images);
    for (ObjectGuid const& guid : images)
        if (Creature* image = ObjectAccessor::GetCreature(*me, guid))
            image->DespawnOrUnsummon();

    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    me->SetReactState(_previousReaction);
}

void XylemAI::ResumeFight(Player& player)
{
    ClearIntermission(&player);
    me->CastSpell(me, CancelAnnihilation, true);
    _phase = _nextPhase;
    _phaseRemaining = _phase == Phase::Arcane ? 60000 : 90000;
    SmartAI::SetData(SetCombatPhase, uint32(_phase));
    me->CastSpell(me, _phase == Phase::Arcane ? ArcanePhase : FrostPhase, true);
    SmartAI::AttackStart(&player);
}

void XylemAI::UpdateEncounter(uint32 diff)
{
    if (!IsEncounterActive())
    {
        if (IsInIntermission())
        {
            ClearIntermission(ObjectAccessor::GetPlayer(*me, _player));
            _phase = Phase::FirstFrost;
        }
        return;
    }

    if (_player.IsEmpty())
    {
        Unit* victim = me->getVictim();
        Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player)
            return;
        _player = player->GetGUID();
        me->CastSpell(me, FrostPhase, true);
    }

    Player* player = GetAttemptPlayer();
    if (!player)
    {
        if (IsInIntermission())
            ClearIntermission(ObjectAccessor::GetPlayer(*me, _player));
        SmartAI::EnterEvadeMode();
        return;
    }

    if (IsInIntermission())
    {
        if (IsProtected(*player))
        {
            if (Aura* shield = player->GetAura(TimeAcceleration, me->GetGUID()))
            {
                if (shield->GetDuration() < 500)
                    shield->SetDuration(2000);
            }
            else
                me->AddAura(TimeAcceleration, player, nullptr, 0, 2000, 2000);
            player->RemoveAurasDueToSpell(DarknessDamage, me->GetGUID());
        }
        else
            player->RemoveAurasDueToSpell(TimeAcceleration, me->GetGUID());

        if (CanReveal(*player))
            ResumeFight(*player);
        else if (!IsCastingAnnihilation())
        {
            // The native cast deals its own damage. Surviving it does not reveal Xylem.
            ClearIntermission(player);
            SmartAI::EnterEvadeMode();
        }
        return;
    }

    if (me->GetHealthPct() < 11.0f)
        return;
    if (diff >= _phaseRemaining)
        StartIntermission(*player);
    else
        _phaseRemaining -= diff;
}

void XylemAI::Reset()
{
    if (IsInIntermission())
        ClearIntermission(ObjectAccessor::GetPlayer(*me, _player));
    _player.Clear();
    _phase = Phase::FirstFrost;
    _phaseRemaining = 90000;
    // BEGIN Draw Power reset
    me->RemoveAurasDueToSpell(DrawPowerStack);
    SmartAI::Reset();
    // END Draw Power reset
}

void XylemAI::UpdateAI(uint32 diff)
{
    UpdateEncounter(diff);
    SmartAI::UpdateAI(diff);
}

void XylemAI::DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type)
{
    if (IsInIntermission())
        damage = 0;
    else
        SmartAI::DamageTaken(attacker, damage, type);
}

void XylemAI::MoveInLineOfSight(Unit* target)
{
    if (!IsInIntermission())
        SmartAI::MoveInLineOfSight(target);
}

struct XylemMirrorAI : ScriptedAI
{
    explicit XylemMirrorAI(Creature* creature) : ScriptedAI(creature) {}

    void UpdateAI(uint32 diff) override
    {
        TempSummon* image = me->ToTempSummon();
        auto* xylem = GetXylemAI(image ? image->GetSummoner() : nullptr);
        if (!xylem || !xylem->OwnsMirror(me))
            return;

        if (diff >= _shockRemaining)
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
            _shockRemaining = me->CastSpell(me, PrismaticShock, false) == SPELL_CAST_OK ? 4000 : 1000;
        }
        else
            _shockRemaining -= diff;
    }

    void AttackStart(Unit*) override {}
    void MoveInLineOfSight(Unit*) override {}

private:
    uint32 _shockRemaining = urand(1000, 3000);
};
}

class npc_mage_tower_xylem : public CreatureScript
{
public:
    npc_mage_tower_xylem() : CreatureScript("npc_mage_tower_xylem") {}
    CreatureAI* GetAI(Creature* creature) const override { return new ClosingTheEye::XylemAI(creature); }
};

class npc_mage_tower_xylem_mirror : public CreatureScript
{
public:
    npc_mage_tower_xylem_mirror() : CreatureScript("npc_mage_tower_xylem_mirror") {}
    CreatureAI* GetAI(Creature* creature) const override { return new ClosingTheEye::XylemMirrorAI(creature); }
};

class spell_mage_tower_xylem_darkness : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_xylem_darkness);

    bool Load() override { return ClosingTheEye::GetXylemAI(GetCaster()) != nullptr; }

    void CastDarkness(AuraEffect const*)
    {
        PreventDefaultAction();
        Unit* caster = GetCaster();
        if (auto* ai = ClosingTheEye::GetXylemAI(caster))
            if (ai->IsInIntermission() && ai->IsCastingAnnihilation())
                caster->CastSpell(caster, ClosingTheEye::DarknessDamage, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_tower_xylem_darkness::CastDarkness, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

class spell_mage_tower_xylem_intermission_damage : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_xylem_intermission_damage);

    ClosingTheEye::XylemAI* Source()
    {
        Unit* caster = GetCaster();
        if (GetSpellInfo()->Id != ClosingTheEye::PrismaticShock)
            return ClosingTheEye::GetXylemAI(caster);

        TempSummon* image = caster ? caster->ToTempSummon() : nullptr;
        auto* ai = ClosingTheEye::GetXylemAI(image ? image->GetSummoner() : nullptr);
        return ai && ai->OwnsMirror(image) ? ai : nullptr;
    }

    bool Load() override { return Source() != nullptr; }

    void Filter(std::list<WorldObject*>& targets)
    {
        auto* ai = Source();
        bool const darkness = GetSpellInfo()->Id == ClosingTheEye::DarknessDamage;
        targets.remove_if([ai, darkness](WorldObject* object)
        {
            Unit* target = object ? object->ToUnit() : nullptr;
            return !ai || !ai->CanHitInIntermission(target) || (darkness && ai->IsProtected(*target));
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_xylem_intermission_damage::Filter, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_xylem_intermission_damage::Filter, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

class spell_mage_tower_xylem_time_acceleration : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_xylem_time_acceleration);

    bool Load() override { return ClosingTheEye::GetXylemAI(GetCaster()) != nullptr; }

    void CalculateAbsorption(AuraEffect const*, float& amount, bool&)
    {
        amount = -1;
    }

    void Absorb(AuraEffect*, DamageInfo& damage, float& amount)
    {
        auto* ai = ClosingTheEye::GetXylemAI(GetCaster());
        Unit* target = GetTarget();
        SpellInfo const* spell = damage.GetSpellInfo();
        if (ai && ai->CanHitInIntermission(target) && ai->IsProtected(*target) &&
            (!spell || spell->Id != ClosingTheEye::ArcaneAnnihilation))
            amount = damage.GetDamage();
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_tower_xylem_time_acceleration::CalculateAbsorption, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
        OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_tower_xylem_time_acceleration::Absorb, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
    }
};

class spell_mage_tower_xylem_draw_power : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_xylem_draw_power);

    bool Load() override
    {
        Unit* caster = GetCaster();
        Creature* xylem = caster ? caster->ToCreature() : nullptr;
        return xylem && xylem->GetEntry() == ClosingTheEye::Xylem && xylem->GetMapId() == ClosingTheEye::ScenarioMap &&
            xylem->GetDBTableGUIDLow() == 14534036;
    }

    void DrawPower(AuraEffect const*)
    {
        PreventDefaultAction();
        Unit* caster = GetCaster();
        Creature* xylem = caster ? caster->ToCreature() : nullptr;
        Unit* focus = GetTarget();
        auto* ai = xylem && xylem->IsAIEnabled ? dynamic_cast<ClosingTheEye::XylemAI*>(xylem->AI()) : nullptr;
        if (ai && focus && ai->CanDrawPower(focus->ToCreature()))
            xylem->CastSpell(xylem, ClosingTheEye::DrawPowerStack, true);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_tower_xylem_draw_power::DrawPower, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

namespace ClosingTheEye
{
constexpr uint32 CorruptingShadows = 116839;
constexpr uint32 GrowingDarkness = 231817;
constexpr uint32 CreepingShadowsArea = 232664;
constexpr uint32 CreepingShadowsDamage = 232672;
constexpr uint32 CreepingShadowsTrigger = 9346;
constexpr uint32 CreepingShadowsTemplate = 90232664;
constexpr uint32 SeedOfDarkness = 233248;
constexpr uint32 FeedingOnDarkness = 233164;
constexpr uint32 SummonDarknessWithin = 233165;
constexpr uint32 DarknessWithin = 116840;
constexpr uint32 MergeDarkness = 232991;
constexpr float CreepingShadowsInitialRadius = 5.0f;

float CreepingShadowsRadius(uint32 age)
{
    return std::min(100.0f, CreepingShadowsInitialRadius + float(age) * 0.00038f);
}

bool IsInCreepingShadows(AreaTrigger const& ground, Unit const& target, float radius)
{
    return target.IsAlive() && target.IsInMap(&ground) && target.InSamePhase(&ground) &&
        std::abs(ground.GetPositionZ() - target.GetPositionZ()) <= 4.0f &&
        ground.GetExactDist2d(target.GetPositionX(), target.GetPositionY()) <= radius;
}

struct CorruptingShadowsAI : SmartAI
{
    explicit CorruptingShadowsAI(Creature* creature) : SmartAI(creature) {}

    Player* GetSeedPlayer() const
    {
        if (!IsGroundEncounter())
            return nullptr;
        Player* player = ObjectAccessor::GetPlayer(*me, _player);
        return IsParticipant(player) ? player : nullptr;
    }

    void ReleaseSeeds(Player& player)
    {
        if (GetSeedPlayer() != &player || player.HasPendingTeleport())
            return;

        uint32 const previous = _seedPublished;
        for (uint32 count = 0; count < 3; ++count)
            player.CastSpell(&player, SummonDarknessWithin, true, nullptr, nullptr, me->GetGUID());

        if (_seedPublished - previous != 3)
            TC_LOG_ERROR("scripts", "Closing the Eye: Seed of Darkness published %u Darkness Within actors",
                _seedPublished - previous);
    }

    bool OwnsSeed(Creature const& creature) const
    {
        return creature.GetEntry() == DarknessWithin && creature.GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == SummonDarknessWithin &&
            me->IsInMap(&creature) && me->InSamePhase(&creature) && _seedAdds.count(creature.GetGUID()) && GetSeedPlayer();
    }

    void JustSummoned(Creature* summon) override
    {
        SmartAI::JustSummoned(summon);
        if (summon->GetEntry() == DarknessWithin && summon->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == SummonDarknessWithin &&
            GetSeedPlayer() && _seedAdds.insert(summon->GetGUID()).second)
        {
            ++_seedPublished;
            summon->DespawnOrUnsummon(45000);
        }
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        _seedAdds.erase(summon->GetGUID());
        SmartAI::SummonedCreatureDespawn(summon);
    }

    void Reset() override
    {
        ClearGround();
        SmartAI::Reset();
    }

    void JustDied(Unit* killer) override
    {
        ClearGround();
        SmartAI::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!IsGroundEncounter())
        {
            if (!_player.IsEmpty())
                ClearGround();
            return;
        }

        if (_player.IsEmpty())
        {
            Unit* victim = me->getVictim();
            Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!IsParticipant(player))
                return;

            _player = player->GetGUID();
            _warmup = 3000;
            me->CastSpell(me, GrowingDarkness, true);
            return;
        }

        Player* player = ObjectAccessor::GetPlayer(*me, _player);
        if (!IsParticipant(player))
        {
            ClearGround();
            SmartAI::EnterEvadeMode();
            return;
        }

        if (_ground.IsEmpty())
        {
            if (diff < _warmup)
            {
                _warmup -= diff;
                return;
            }
            if (!PublishGround())
            {
                TC_LOG_ERROR("scripts", "Closing the Eye: Corrupting Shadows could not publish its ground hazard");
                ClearGround();
                SmartAI::EnterEvadeMode();
            }
            return;
        }

        AreaTrigger* ground = ObjectAccessor::GetAreaTrigger(*me, _ground);
        if (!ground || ground->GetDuration() <= 0)
        {
            ClearGround();
            SmartAI::EnterEvadeMode();
            return;
        }

        _age += std::min(diff, 300000 - _age);
        if (diff < _update)
        {
            _update -= diff;
            return;
        }
        _update = 250;
        UpdateGround(*ground, *player);
    }

private:
    bool IsGroundEncounter() const
    {
        if (me->GetEntry() != CorruptingShadows || me->GetMapId() != ScenarioMap || !me->IsAlive() || !me->isInCombat())
            return false;

        TempSummon* boss = me->ToTempSummon();
        if (!boss || !GetXylemAI(boss->GetSummoner()))
            return false;

        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1204 && scenario->GetCurrentStep() == 2 && !scenario->IsCompleted(false);
    }

    bool IsParticipant(Player const* player) const
    {
        return player && player->IsAlive() && player->IsInWorld() && !player->IsPreDelete() &&
            !player->IsBeingTeleportedFar() && me->IsInMap(player) && me->InSamePhase(player) &&
            player->GetQuestStatus(44925) == QUEST_STATUS_INCOMPLETE;
    }

    bool PublishGround()
    {
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(CreepingShadowsArea);
        if (!spell)
            return false;

        auto ground = std::make_unique<AreaTrigger>();
        Position origin = me->GetPosition();
        me->UpdateGroundPositionZ(origin.GetPositionX(), origin.GetPositionY(), origin.m_positionZ);
        if (!ground->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
            CreepingShadowsTrigger, me, spell, origin, origin, nullptr, ObjectGuid::Empty, CreepingShadowsTemplate))
            return false;

        _ground = ground.release()->GetGUID();
        return true;
    }

    void UpdateGround(AreaTrigger& ground, Player& player)
    {
        float const radius = CreepingShadowsRadius(_age);
        if (std::abs(ground.GetRadius() - radius) > 0.02f)
        {
            ground.SetSphereScale(radius / CreepingShadowsInitialRadius, 250, true);
            ground.UpdateOverrideScale();
        }

        auto updateTarget = [this, &ground, radius](Unit& target)
        {
            if (!IsInCreepingShadows(ground, target, radius))
            {
                target.RemoveAurasDueToSpell(CreepingShadowsDamage, me->GetGUID());
                _affected.erase(target.GetGUID());
                return;
            }

            Aura* aura = target.GetAura(CreepingShadowsDamage, me->GetGUID());
            if (aura)
            {
                if (aura->GetDuration() < 500)
                    aura->SetDuration(1500);
            }
            else if (me->AddAura(CreepingShadowsDamage, &target, nullptr, 0, 1500, 1500))
                _affected.insert(target.GetGUID());
        };

        updateTarget(player);
        Unit::ControlList const controlled = player.m_Controlled;
        for (ObjectGuid const& guid : controlled)
            if (Unit* unit = ObjectAccessor::GetUnit(player, guid))
                if (unit->GetCharmerOrOwnerPlayerOrPlayerItself() == &player)
                    updateTarget(*unit);
    }

    void ClearGround()
    {
        if (!_ground.IsEmpty())
            me->RemoveAreaObject(CreepingShadowsArea);
        for (ObjectGuid const& guid : _affected)
            if (Unit* unit = ObjectAccessor::GetUnit(*me, guid))
                unit->RemoveAurasDueToSpell(CreepingShadowsDamage, me->GetGUID());
        _affected.clear();
        me->RemoveAurasDueToSpell(FeedingOnDarkness);
        if (Player* player = ObjectAccessor::GetPlayer(*me, _player))
            player->RemoveAurasDueToSpell(SeedOfDarkness, me->GetGUID());

        std::set<ObjectGuid> seedAdds;
        seedAdds.swap(_seedAdds);
        for (ObjectGuid const& guid : seedAdds)
            if (Creature* add = ObjectAccessor::GetCreature(*me, guid))
                add->DespawnOrUnsummon();

        _ground.Clear();
        _player.Clear();
        _age = 0;
        _update = 0;
        _seedPublished = 0;
    }

    ObjectGuid _ground, _player;
    std::set<ObjectGuid> _affected;
    std::set<ObjectGuid> _seedAdds;
    uint32 _warmup = 0;
    uint32 _age = 0;
    uint32 _update = 0;
    uint32 _seedPublished = 0;
};

CorruptingShadowsAI* GetCorruptingShadowsAI(Unit* source)
{
    Creature* boss = source ? source->ToCreature() : nullptr;
    if (!boss || boss->GetEntry() != CorruptingShadows || boss->GetMapId() != ScenarioMap || !boss->IsAIEnabled)
        return nullptr;
    return dynamic_cast<CorruptingShadowsAI*>(boss->AI());
}

Creature* GetSeedBoss(Creature& seed)
{
    TempSummon* summon = seed.ToTempSummon();
    Unit* owner = summon ? summon->GetSummoner() : nullptr;
    CorruptingShadowsAI* boss = GetCorruptingShadowsAI(owner);
    return boss && boss->OwnsSeed(seed) ? owner->ToCreature() : nullptr;
}

bool CanMergeDarkness(Creature const& seed, Creature const& boss)
{
    return seed.IsAlive() && boss.IsAlive() && seed.IsWithinDistInMap(&boss, 3.0f) && seed.IsWithinLOSInMap(&boss);
}

struct DarknessWithinAI : ScriptedAI
{
    explicit DarknessWithinAI(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_merged)
            return;

        Creature* boss = GetSeedBoss(*me);
        if (!boss)
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (!_following)
        {
            me->GetMotionMaster()->MoveFollow(boss, 1.0f, 0.0f);
            _following = true;
        }
        if (diff < _mergeRetry)
        {
            _mergeRetry -= diff;
            return;
        }
        _mergeRetry = 250;

        if (CanMergeDarkness(*me, *boss))
            me->CastSpell(boss, MergeDarkness, false);
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (_merged || !spell || spell->Id != MergeDarkness || !target || target != GetSeedBoss(*me))
            return;
        _merged = true;
        me->StopMoving();
        me->DespawnOrUnsummon(250);
    }

    void AttackStart(Unit*) override {}
    void MoveInLineOfSight(Unit*) override {}

private:
    bool _following = false;
    bool _merged = false;
    uint32 _mergeRetry = 0;
};
}

class npc_mage_tower_corrupting_shadows : public CreatureScript
{
public:
    npc_mage_tower_corrupting_shadows() : CreatureScript("npc_mage_tower_corrupting_shadows") {}
    CreatureAI* GetAI(Creature* creature) const override { return new ClosingTheEye::CorruptingShadowsAI(creature); }
};

class npc_mage_tower_darkness_within : public CreatureScript
{
public:
    npc_mage_tower_darkness_within() : CreatureScript("npc_mage_tower_darkness_within") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        if (creature->GetMapId() == ClosingTheEye::ScenarioMap && creature->ToTempSummon())
            return new ClosingTheEye::DarknessWithinAI(creature);
        return new SmartAI(creature);
    }
};

class spell_mage_tower_seed_target : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_seed_target);

    bool Load() override { return ClosingTheEye::GetCorruptingShadowsAI(GetCaster()) != nullptr; }

    void SelectPlayer(WorldObject*& target)
    {
        auto* ai = ClosingTheEye::GetCorruptingShadowsAI(GetCaster());
        Unit* victim = target ? target->ToUnit() : nullptr;
        Player* player = ai ? ai->GetSeedPlayer() : nullptr;
        target = victim && player && victim->GetCharmerOrOwnerPlayerOrPlayerItself() == player ? player : nullptr;
    }

    void Register() override
    {
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_mage_tower_seed_target::SelectPlayer, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
    }
};

class spell_mage_tower_seed_expiry : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_seed_expiry);

    bool Load() override { return ClosingTheEye::GetCorruptingShadowsAI(GetCaster()) != nullptr; }

    void StartFeeding(AuraEffect const*, AuraEffectHandleModes)
    {
        Unit* boss = GetCaster();
        auto* ai = ClosingTheEye::GetCorruptingShadowsAI(boss);
        if (ai && GetTarget() == ai->GetSeedPlayer())
            boss->CastSpell(boss, ClosingTheEye::FeedingOnDarkness, true);
    }

    void ReleaseSeeds(AuraEffect const*, AuraEffectHandleModes)
    {
        if (_released || GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
            return;
        auto* ai = ClosingTheEye::GetCorruptingShadowsAI(GetCaster());
        Unit* target = GetTarget();
        if (ai && target && target->IsPlayer())
        {
            _released = true;
            ai->ReleaseSeeds(*target->ToPlayer());
        }
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_mage_tower_seed_expiry::StartFeeding, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_mage_tower_seed_expiry::ReleaseSeeds, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }

    bool _released = false;
};

class spell_mage_tower_merge_darkness : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_merge_darkness);

    bool Load() override
    {
        Unit* caster = GetCaster();
        return caster && caster->GetEntry() == ClosingTheEye::DarknessWithin && caster->GetMapId() == ClosingTheEye::ScenarioMap;
    }

    void SelectBoss(std::list<WorldObject*>& targets)
    {
        Creature* seed = GetCaster()->ToCreature();
        Creature* boss = seed ? ClosingTheEye::GetSeedBoss(*seed) : nullptr;
        targets.remove_if([seed, boss](WorldObject* target)
        {
            return !boss || target != boss || !ClosingTheEye::CanMergeDarkness(*seed, *boss);
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_tower_merge_darkness::SelectBoss, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

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
constexpr uint32 BileEgg = 121459;
constexpr uint32 BileSpitter = 121460;
constexpr uint32 EggMissile = 242384;
constexpr uint32 EggSummon = 242378;
constexpr uint32 HatchBileSpitter = 242379;
constexpr uint32 SummonBileSpitter = 243167;
constexpr uint32 SpitBile = 242468;
constexpr uint32 BileMissile = 242566;
constexpr uint32 Rupture = 241664;
constexpr uint32 RuptureDamage = 241666;
constexpr uint32 RuptureCastTime = 2500;
constexpr uint32 RuptureTemplateOffset = 1000000;
constexpr float RuptureHalfLength = 7.5f;
constexpr float RuptureHalfWidth = 1.25f;
constexpr float RuptureHalfHeight = 2.5f;

constexpr uint32 ChargeMovement = 1;
constexpr float RockRadius = 2.75f;
constexpr float PlayerHitRadius = 2.5f;
Position const RockSites[] = {
    { 5207.0f, 5057.0f, 626.0f },
    { 5216.0f, 5049.0f, 626.0f },
    { 5218.0f, 5061.0f, 627.0f }
};
Position const EggSites[] = {
    { 5211.0f, 5069.0f, 628.0f },
    { 5224.0f, 5054.0f, 626.0f }
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

bool IsInRuptureCross(Position const& origin, Position const& point)
{
    if (std::abs(point.GetPositionZ() - origin.GetPositionZ()) > RuptureHalfHeight)
        return false;

    float const dx = point.GetPositionX() - origin.GetPositionX();
    float const dy = point.GetPositionY() - origin.GetPositionY();
    float const cs = std::cos(origin.GetOrientation());
    float const sn = std::sin(origin.GetOrientation());
    float const along = std::abs(dx * cs + dy * sn);
    float const across = std::abs(dy * cs - dx * sn);
    return (along <= RuptureHalfLength && across <= RuptureHalfWidth) ||
        (across <= RuptureHalfLength && along <= RuptureHalfWidth);
}

struct RuptureForm
{
    uint32 warningSpell;
    uint32 impactSpell;
    std::array<uint32, 2> warnings;
    std::array<uint32, 2> impacts;
};

RuptureForm const RuptureForms[] = {
    { 241665, 241667, { 10341, 10342 }, { 10343, 7562 } },
    { 241668, 241669, { 10344, 10345 }, { 10346, 10347 } }
};

struct TugarAI : SmartAI
{
    explicit TugarAI(Creature* creature) : SmartAI(creature) {}

    void Reset() override
    {
        ++_attempt;
        _totemTimer = 0;
        StopEarthquake();
        _nextRupture = 0;
        ClearRupture();
        SmartAI::Reset();
    }

    void EnterCombat(Unit* enemy) override
    {
        ++_attempt;
        // Legion DBM observed the first Fel Surge cast near 62s. The original
        // 15-yard missile travels at 2 yards/sec before the totem begins it.
        _totemTimer = 54500;
        _earthquakeTimer = 20500;
        _nextRupture = 7500;
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
    uint64 EggAttempt() const { return _earthquakeTimer ? ActiveAttempt() : 0; }

    void FinishEggWave(uint64 attempt, bool expired)
    {
        if (!_eggWave || attempt != _eggWave || attempt != EggAttempt())
            return;
        _eggWave = 0;
        if (!expired || _eggInterrupted || me->HasUnitState(UNIT_STATE_CONTROLLED))
            return;

        for (Position position : EggSites)
        {
            float const z = me->GetHeight(position.GetPositionX(), position.GetPositionY(),
                position.GetPositionZ() + 1.0f, true, 5.0f);
            if (z <= INVALID_HEIGHT)
                continue;
            position.m_positionZ = z;
            if (me->CastSpell(position, EggMissile, true) != SPELL_CAST_OK)
                TC_LOG_WARN("scripts", "Feltotem's Fall: Bile Spitter Egg missile failed in instance %u", me->GetInstanceId());
        }
    }

    void JustDied(Unit* killer) override
    {
        StopEarthquake();
        ClearRupture();
        SmartAI::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        if (_eggWave && me->HasUnitState(UNIT_STATE_CONTROLLED))
            _eggInterrupted = true;
        SmartAI::UpdateAI(diff);
        UpdateEarthquake(diff);
        UpdateRupture(diff);
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
        _eggWave = 0;
        RemoveRocks();
    }

private:
    void UpdateEarthquake(uint32 diff);
    void UpdateRupture(uint32 diff);
    bool PublishRupture(uint32 spell, std::array<uint32, 2> const& entries);
    void ClearRupture()
    {
        for (ObjectGuid& guid : _ruptureVisuals)
        {
            if (AreaTrigger* trigger = ObjectAccessor::GetAreaTrigger(*me, guid))
                trigger->Remove();
            guid.Clear();
        }
        _rupturePlayer.Clear();
        _ruptureImpact = 0;
    }
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
    uint64 _eggWave = 0;
    uint32 _totemTimer = 0;
    uint32 _earthquakeTimer = 0;
    uint32 _rockFall = 0;
    bool _eggInterrupted = false;
    std::array<Position, 3> _rockPositions;
    std::array<ObjectGuid, 3> _rocks;
    Position _ruptureOrigin;
    ObjectGuid _rupturePlayer;
    std::array<ObjectGuid, 2> _ruptureVisuals;
    uint32 _nextRupture = 0;
    uint32 _ruptureImpact = 0;
    uint8 _ruptureForm = 0;
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

    _eggWave = attempt;
    _eggInterrupted = false;
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

bool TugarAI::PublishRupture(uint32 spell, std::array<uint32, 2> const& entries)
{
    SpellInfo const* info = sSpellMgr->GetSpellInfo(spell);
    if (!info)
        return false;

    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        Position site = _ruptureOrigin;
        site.SetOrientation(site.GetOrientation() + i * float(M_PI) / 2.0f);
        auto trigger = std::make_unique<AreaTrigger>();
        if (!trigger->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
            entries[i], me, info, site, site, nullptr, ObjectGuid::Empty, RuptureTemplateOffset + entries[i]))
        {
            ClearRupture();
            TC_LOG_WARN("scripts", "Feltotem's Fall: could not publish Fel Rupture spell %u in instance %u", spell, me->GetInstanceId());
            return false;
        }
        _ruptureVisuals[i] = trigger.release()->GetGUID();
    }
    return true;
}

void TugarAI::UpdateRupture(uint32 diff)
{
    Player* player = Participant();
    if (!player)
    {
        ClearRupture();
        return;
    }
    if (_ruptureImpact && _rupturePlayer != player->GetGUID())
        ClearRupture();

    if (_ruptureImpact > diff)
        _ruptureImpact -= diff;
    else if (_ruptureImpact)
    {
        ClearRupture();
        RuptureForm const& form = RuptureForms[_ruptureForm];
        if (PublishRupture(form.impactSpell, form.impacts))
        {
            Unit::ControlList const controlled = player->m_Controlled;
            if (IsInRuptureCross(_ruptureOrigin, player->GetPosition()))
                me->CastSpell(player, RuptureDamage, true);
            for (ObjectGuid const& guid : controlled)
                if (Unit* unit = ObjectAccessor::GetUnit(*me, guid))
                    if (unit->IsAlive() && unit->GetCharmerOrOwnerPlayerOrPlayerItself() == player &&
                        me->IsInMap(unit) && unit->InSamePhase(me) && IsInRuptureCross(_ruptureOrigin, unit->GetPosition()))
                        me->CastSpell(unit, RuptureDamage, true);
        }
    }

    if (!_nextRupture)
        return;
    if (_nextRupture > diff)
    {
        _nextRupture -= diff;
        return;
    }
    if (me->CastSpell(me, Rupture, false) != SPELL_CAST_OK)
    {
        _nextRupture = 500;
        return;
    }
    _nextRupture = urand(10900, 13400);

    Position origin = player->GetPosition();
    float const z = me->GetHeight(origin.GetPositionX(), origin.GetPositionY(), origin.GetPositionZ() + 1.0f, true, 5.0f);
    if (z <= INVALID_HEIGHT)
        return;
    origin.m_positionZ = z;
    origin.SetOrientation(frand(0.0f, float(M_PI) / 2.0f));
    ClearRupture();
    _ruptureOrigin = origin;
    _rupturePlayer = player->GetGUID();
    _ruptureForm = urand(0, 1);
    RuptureForm const& form = RuptureForms[_ruptureForm];
    if (PublishRupture(form.warningSpell, form.warnings))
        _ruptureImpact = RuptureCastTime;
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

class spell_mage_tower_earthquake_eggs : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_earthquake_eggs);

    bool Load() override
    {
        auto* tugar = Feltotem::FindTugarAI(GetCaster());
        _attempt = tugar ? tugar->EggAttempt() : 0;
        return _attempt != 0;
    }

    void ReleaseEggs(AuraEffect const*, AuraEffectHandleModes)
    {
        if (auto* tugar = Feltotem::FindTugarAI(GetCaster()))
            tugar->FinishEggWave(_attempt, GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_mage_tower_earthquake_eggs::ReleaseEggs,
            EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }

    uint64 _attempt = 0;
};

class spell_mage_tower_earthquake_egg_missile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_earthquake_egg_missile);

    bool Load() override
    {
        auto* tugar = Feltotem::FindTugarAI(GetCaster());
        _attempt = tugar ? tugar->EggAttempt() : 0;
        return true;
    }

    void Land(SpellEffIndex effect)
    {
        auto* tugar = Feltotem::FindTugarAI(GetCaster());
        if (!_attempt || !tugar || _attempt != tugar->EggAttempt())
            PreventHitDefaultEffect(effect);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_mage_tower_earthquake_egg_missile::Land, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
    }

    uint64 _attempt = 0;
};

struct npc_mage_tower_bile_egg : ScriptedAI
{
    explicit npc_mage_tower_bile_egg(Creature* creature) : ScriptedAI(creature)
    {
        SetCombatMovement(false);
    }

    void IsSummonedBy(Unit* summoner) override
    {
        auto* tugar = Feltotem::FindTugarAI(summoner);
        if (tugar && me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == Feltotem::EggSummon)
        {
            _tugar = summoner->GetGUID();
            _attempt = tugar->EggAttempt();
            me->SetReactState(REACT_PASSIVE);
        }
    }

    bool IsActive() const
    {
        auto* tugar = Feltotem::FindTugarAI(ObjectAccessor::GetCreature(*me, _tugar));
        return _attempt && tugar && _attempt == tugar->EggAttempt() && me->IsAlive();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!IsActive())
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (_published || me->HasAura(Feltotem::HatchBileSpitter) || me->HasUnitState(UNIT_STATE_CASTING))
            return;
        if (diff < _retryCast)
        {
            _retryCast -= diff;
            return;
        }
        _retryCast = 750;
        me->CastSpell(me, Feltotem::HatchBileSpitter, false);
    }

    void Hatch()
    {
        if (_hatching || !IsActive())
            return;

        _hatching = true;
        me->CastSpell(me->GetPosition(), Feltotem::SummonBileSpitter, true);
        if (_published)
            me->DespawnOrUnsummon(1);
        else if (++_failedHatches < 3)
            _hatching = false;
        else
        {
            TC_LOG_WARN("scripts", "Feltotem's Fall: Bile Spitter Egg could not hatch in instance %u", me->GetInstanceId());
            me->DespawnOrUnsummon(1);
        }
    }

    void JustSummoned(Creature* creature) override
    {
        if (_hatching && creature->GetEntry() == Feltotem::BileSpitter &&
            creature->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) == Feltotem::SummonBileSpitter)
            _published = true;
    }

    uint64 HatchAttempt() const { return _published && IsActive() ? _attempt : 0; }
    ObjectGuid GetTugar() const { return _tugar; }

private:
    ObjectGuid _tugar;
    uint64 _attempt = 0;
    uint32 _retryCast = 0;
    uint8 _failedHatches = 0;
    bool _hatching = false;
    bool _published = false;
};

class spell_mage_tower_bile_egg_hatch : public AuraScript
{
    PrepareAuraScript(spell_mage_tower_bile_egg_hatch);

    void Hatch(AuraEffect const*)
    {
        Creature* creature = GetTarget()->ToCreature();
        auto* egg = creature && creature->IsAIEnabled ? dynamic_cast<npc_mage_tower_bile_egg*>(creature->AI()) : nullptr;
        if (egg)
            egg->Hatch();
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_tower_bile_egg_hatch::Hatch, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

struct npc_mage_tower_bile_spitter : ScriptedAI
{
    explicit npc_mage_tower_bile_spitter(Creature* creature) : ScriptedAI(creature) {}

    void IsSummonedBy(Unit* summoner) override
    {
        Creature* creature = summoner->ToCreature();
        auto* egg = creature && creature->IsAIEnabled ? dynamic_cast<npc_mage_tower_bile_egg*>(creature->AI()) : nullptr;
        if (!egg || me->GetUInt32Value(UNIT_FIELD_CREATED_BY_SPELL) != Feltotem::SummonBileSpitter)
            return;
        _attempt = egg->HatchAttempt();
        _tugar = egg->GetTugar();
        if (!_attempt || !summoner->GetMaxHealth())
            return;

        auto const health = uint64(me->GetMaxHealth() * (double(summoner->GetHealth()) / summoner->GetMaxHealth()));
        me->SetHealth(std::max<uint64>(1, health));
    }

    Player* Participant() const
    {
        auto* tugar = Feltotem::FindTugarAI(ObjectAccessor::GetCreature(*me, _tugar));
        return _attempt && tugar && _attempt == tugar->EggAttempt() ? tugar->Participant() : nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        Player* player = Participant();
        if (!player)
        {
            me->DespawnOrUnsummon();
            return;
        }
        if (!UpdateVictim())
        {
            AttackStart(player);
            return;
        }
        if (diff < _spit)
            _spit -= diff;
        else if (me->CastSpell(me, Feltotem::SpitBile, false) == SPELL_CAST_OK)
            _spit = urand(8000, 11000);
        else
            _spit = 500;
        DoMeleeAttackIfReady();
    }

private:
    ObjectGuid _tugar;
    uint64 _attempt = 0;
    uint32 _spit = 5000;
};

class spell_mage_tower_spit_bile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_spit_bile);

    void Launch(SpellEffIndex)
    {
        Creature* creature = GetCaster()->ToCreature();
        auto* spitter = creature && creature->IsAIEnabled ? dynamic_cast<npc_mage_tower_bile_spitter*>(creature->AI()) : nullptr;
        Unit* target = GetHitUnit();
        if (target && spitter && target == spitter->Participant())
            creature->CastSpell(target->GetPosition(), Feltotem::BileMissile, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_mage_tower_spit_bile::Launch, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_mage_tower_bile_missile : public SpellScript
{
    PrepareSpellScript(spell_mage_tower_bile_missile);

    void Land(SpellEffIndex effect)
    {
        Creature* creature = GetCaster()->ToCreature();
        auto* spitter = creature && creature->IsAIEnabled ? dynamic_cast<npc_mage_tower_bile_spitter*>(creature->AI()) : nullptr;
        if (!spitter || !spitter->Participant())
            PreventHitDefaultEffect(effect);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_mage_tower_bile_missile::Land, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
    }
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

namespace
{
enum class AgathaPosition
{
    Center,
    Upper,
    Lower
};

uint32 GetAgathaTranslocate(AgathaPosition position, float health, uint32 remaining, bool reachedLower)
{
    if (position == AgathaPosition::Center)
        return health <= 50.0f ? 242987 : 0;
    if (position == AgathaPosition::Upper && (!remaining || (!reachedLower && health <= 35.0f)))
        return 242989;
    if (position == AgathaPosition::Lower && !remaining)
        return 242987;
    return 0;
}
}

class npc_mage_tower_agatha : public CreatureScript
{
public:
    npc_mage_tower_agatha() : CreatureScript("npc_mage_tower_agatha") {}

    struct AgathaAI : SmartAI
    {
        explicit AgathaAI(Creature* creature) : SmartAI(creature) {}

        void Reset() override
        {
            SmartAI::Reset();
            _position = AgathaPosition::Center;
            _reachedLower = false;
            _nextTranslocate = 0;
            _pendingTranslocate = 0;
            _retry = 0;
            _failures = 0;
            if (IsArenaActor())
                SetCombatMove(false);
        }

        void UpdateAI(uint32 diff) override
        {
            SmartAI::UpdateAI(diff);
            if (!IsArenaActor() || !me->IsAlive() || !me->isInCombat())
                return;

            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1221 || scenario->IsCompleted(false))
                return;

            Unit* victim = me->getVictim();
            Player* player = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player || !player->IsAlive() || !player->IsInWorld() || player->IsPreDelete() || player->HasPendingTeleport() ||
                !me->IsInMap(player) || !me->InSamePhase(player))
                return;

            UpdateTranslocate(diff);
        }

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

    private:
        bool IsArenaActor() const
        {
            return me->GetMapId() == 1616 && me->GetDBTableGUIDLow() == 14534028;
        }

        void TranslocateFailed(uint32 spell)
        {
            if (++_failures == 3)
                TC_LOG_ERROR("scripts", "An Impossible Foe: Agatha could not finish Translocate %u after three attempts", spell);
            _retry = _failures < 3 ? 1500 : 6000;
        }

        void UpdateTranslocate(uint32 diff)
        {
            if (_pendingTranslocate)
            {
                Spell const* cast = me->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (cast && cast->getState() != SPELL_STATE_FINISHED && cast->GetSpellInfo()->Id == _pendingTranslocate)
                    return;

                uint32 const spell = _pendingTranslocate;
                _pendingTranslocate = 0;
                SpellTargetPosition const* destination = sSpellMgr->GetSpellTargetPosition(spell);
                if (!destination || destination->target_mapId != me->GetMapId() ||
                    me->GetExactDist2d(destination->target_X, destination->target_Y) > 0.5f ||
                    std::abs(me->GetPositionZ() - destination->target_Z) > 1.0f)
                {
                    TranslocateFailed(spell);
                    return;
                }

                _position = spell == 242987 ? AgathaPosition::Upper : AgathaPosition::Lower;
                _reachedLower = _reachedLower || _position == AgathaPosition::Lower;
                _nextTranslocate = 68000;
                _retry = 1500;
                _failures = 0;
                return;
            }

            _nextTranslocate -= std::min(diff, _nextTranslocate);
            _retry -= std::min(diff, _retry);
            if (_retry || me->HasUnitState(UNIT_STATE_CASTING))
                return;

            uint32 const spell = GetAgathaTranslocate(_position, me->GetHealthPct(), _nextTranslocate, _reachedLower);
            if (!spell)
                return;

            SpellTargetPosition const* destination = sSpellMgr->GetSpellTargetPosition(spell);
            if (!destination || destination->target_mapId != me->GetMapId() || me->CastSpell(me, spell, false) != SPELL_CAST_OK)
            {
                TranslocateFailed(spell);
                return;
            }
            _pendingTranslocate = spell;
        }

        AgathaPosition _position = AgathaPosition::Center;
        uint32 _pendingTranslocate = 0;
        uint32 _nextTranslocate = 0;
        uint32 _retry = 0;
        uint32 _failures = 0;
        bool _reachedLower = false;
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
    new npc_mage_tower_victoria();
    new npc_mage_tower_xylem();
    new npc_mage_tower_xylem_mirror();
    new npc_mage_tower_corrupting_shadows();
    new npc_mage_tower_darkness_within();
    RegisterSpellScript(spell_mage_tower_seed_target);
    RegisterAuraScript(spell_mage_tower_seed_expiry);
    RegisterSpellScript(spell_mage_tower_merge_darkness);
    RegisterAuraScript(spell_mage_tower_xylem_darkness);
    RegisterSpellScript(spell_mage_tower_xylem_intermission_damage);
    RegisterAuraScript(spell_mage_tower_xylem_time_acceleration);
    RegisterAuraScript(spell_mage_tower_xylem_draw_power);
    new npc_mage_tower_tugar();
    new npc_mage_tower_jormog();
    RegisterCreatureAI(npc_mage_tower_bile_egg);
    RegisterCreatureAI(npc_mage_tower_bile_spitter);
    RegisterAuraScript(spell_mage_tower_earthquake_eggs);
    RegisterSpellScript(spell_mage_tower_earthquake_egg_missile);
    RegisterAuraScript(spell_mage_tower_bile_egg_hatch);
    RegisterSpellScript(spell_mage_tower_spit_bile);
    RegisterSpellScript(spell_mage_tower_bile_missile);
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
