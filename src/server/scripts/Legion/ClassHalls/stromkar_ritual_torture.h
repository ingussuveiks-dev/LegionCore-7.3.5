#pragma once

namespace StromkarRitualTorture
{
bool Source(Creature* actor)
{
    if (!actor || actor->GetEntry() != 101875 || actor->GetMapId() != 1539)
        return false;
    uint32 spawn = actor->GetDBTableGUIDLow();
    return spawn == 370936 || spawn == 370937 || spawn == 370943;
}

Creature* Receiver(Creature* actor)
{
    if (!Source(actor) || !actor->IsInWorld() || !actor->IsAlive() || actor->isInCombat())
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
    Spell* channel = actor->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    if (!scenario || scenario->GetScenarioId() != 1037 || scenario->GetCurrentStep() > 1 ||
        !channel || channel->GetSpellInfo()->Id != 201032 || !actor->HasAura(201032, actor->GetGUID()))
        return nullptr;
    Creature* receiver = actor->FindNearestCreature(103144, 50.0f, true);
    return receiver && receiver->GetDBTableGUIDLow() == 370922 && receiver->IsInWorld() &&
        receiver->IsAlive() && receiver->GetMap() == actor->GetMap() &&
        actor->InSamePhase(receiver) && actor->IsWithinLOSInMap(receiver) ? receiver : nullptr;
}
}

// Keep the original channel/hand animation. Its periodic201031 is a travelling
// Zig_Missile, while native204125 owns the continuous Thoradin torture chain.
class spell_stromkar_ritual_channel : public AuraScript
{
    PrepareAuraScript(spell_stromkar_ritual_channel);
    ObjectGuid receiver;
    uint32 retry = 0;
    bool warned = false;

    bool Load() override
    {
        Unit* owner = GetUnitOwner();
        return owner && StromkarRitualTorture::Source(owner->ToCreature()) && GetCasterGUID() == owner->GetGUID();
    }

    void Clear()
    {
        if (Unit* owner = GetUnitOwner())
            if (Creature* target = ObjectAccessor::GetCreature(*owner, receiver))
                target->RemoveAurasDueToSpell(204125, owner->GetGUID());
        receiver.Clear();
    }

    void Periodic(AuraEffect const*)
    {
        PreventDefaultAction();
    }

    void Update(uint32 diff, AuraEffect*)
    {
        if (retry > diff) { retry -= diff; return; }
        retry = 500;
        Unit* owner = GetUnitOwner();
        Creature* target = StromkarRitualTorture::Receiver(owner ? owner->ToCreature() : nullptr);
        if (!target)
        {
            Clear();
            return;
        }
        if (receiver != target->GetGUID())
        {
            Clear();
            receiver = target->GetGUID();
        }
        if (!target->HasAura(204125, owner->GetGUID()))
        {
            SpellCastResult result = owner->CastSpell(target, 204125, true);
            if (result != SPELL_CAST_OK && !warned)
            {
                warned = true;
                TC_LOG_WARN("scripts", "Stromkar ritual: native Shadow Torture204125 failed with result %u for ritualist spawn %u; retrying", uint32(result), owner->ToCreature()->GetDBTableGUIDLow());
            }
        }
    }

    void Remove(AuraEffect const*, AuraEffectHandleModes) { Clear(); }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_stromkar_ritual_channel::Periodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        OnEffectUpdate += AuraEffectUpdateFn(spell_stromkar_ritual_channel::Update, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_stromkar_ritual_channel::Remove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_stromkar_ritual_torture : public AuraScript
{
    PrepareAuraScript(spell_stromkar_ritual_torture);
    uint32 retry = 0;

    bool Load() override
    {
        Creature* owner = GetUnitOwner() ? GetUnitOwner()->ToCreature() : nullptr;
        return owner && owner->GetEntry() == 103144 && owner->GetDBTableGUIDLow() == 370922 &&
            owner->GetMapId() == 1539 && StromkarRitualTorture::Source(GetCaster() ? GetCaster()->ToCreature() : nullptr);
    }

    void Update(uint32 diff, AuraEffect*)
    {
        if (retry > diff) { retry -= diff; return; }
        retry = 500;
        if (StromkarRitualTorture::Receiver(GetCaster() ? GetCaster()->ToCreature() : nullptr) != GetUnitOwner())
            GetAura()->Remove();
    }

    void Register() override
    {
        OnEffectUpdate += AuraEffectUpdateFn(spell_stromkar_ritual_torture::Update, EFFECT_0, SPELL_AURA_DUMMY);
    }
};
