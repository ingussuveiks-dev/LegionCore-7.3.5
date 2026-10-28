#pragma once

// 201032 is a self aura. Its periodic 201031 therefore receives the ritualist
// as its explicit target, and native destination106 collapses to the caster.
// Restore the actual authored receiver for the three Warrior actors only.
class spell_stromkar_ritual_beam : public SpellScript
{
    PrepareSpellScript(spell_stromkar_ritual_beam);

    bool Load() override
    {
        Creature* caster = GetCaster()->ToCreature();
        if (!caster || caster->GetEntry() != 101875 || caster->GetMapId() != 1539)
            return false;
        uint32 guid = caster->GetDBTableGUIDLow();
        return guid == 370936 || guid == 370937 || guid == 370943;
    }

    Creature* Receiver()
    {
        Unit* caster = GetCaster();
        Scenario* scenario = sScenarioMgr->GetScenario(caster->GetInstanceId());
        Spell* channel = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (!caster->IsInWorld() || !caster->IsAlive() || caster->isInCombat() ||
            !scenario || scenario->GetScenarioId() != 1037 || scenario->GetCurrentStep() > 1 ||
            !channel || channel->GetSpellInfo()->Id != 201032 || !caster->HasAura(201032, caster->GetGUID()))
            return nullptr;
        Creature* receiver = caster->FindNearestCreature(103144, 50.0f, true);
        return receiver && receiver->GetDBTableGUIDLow() == 370922 && receiver->IsInWorld() &&
            receiver->IsAlive() && receiver->GetMap() == caster->GetMap() &&
            caster->InSamePhase(receiver) && caster->IsWithinLOSInMap(receiver) ? receiver : nullptr;
    }

    SpellCastResult Check()
    {
        return Receiver() ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void Launch(SpellEffIndex)
    {
        Creature* receiver = Receiver();
        if (!receiver || !GetHitDest())
        {
            FinishCast(SPELL_FAILED_BAD_TARGETS);
            return;
        }
        WorldLocation destination = *GetHitDest();
        destination.Relocate(*receiver);
        GetHitDest()->Relocate(destination);
        SetExplTargetDest(destination);
        // Preserve the native missile's destination attachment34 as well as
        // its position. HandleLaunchPhase precedes outbound SpellGo.
        GetSpell()->m_targets.SetUnitTarget(receiver);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_stromkar_ritual_beam::Check);
        OnEffectLaunch += SpellEffectFn(spell_stromkar_ritual_beam::Launch, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};
