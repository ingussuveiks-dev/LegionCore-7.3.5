// Shared wound policy for Taran Zhu and the original Shado-Pan companions.
// Taran's half-damage modifier remains in his original early DamageTaken.
bool _pendingWoundHeal = false;

void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*type*/) override
{
    if (damage >= me->GetHealth(attacker))
    {
        damage = 0;
        if (!wait_heal)
        {
            _pendingWoundHeal = false;
            wait_heal = true;
            me->AddAura(204237, me);
            WoundSpeech();
            me->AddUnitState(UNIT_STATE_STUNNED);
        }
    }
}

void ObserveWoundHeal()
{
    if (wait_heal) _pendingWoundHeal = true;
}

void RecoverWound()
{
    if (!_pendingWoundHeal) return;
    _pendingWoundHeal = false;
    if (!wait_heal || !me->IsInWorld() || !me->IsAlive() || me->IsPreDelete() || me->GetHealthPct() < 40)
        return;
    wait_heal = false;
    me->ClearUnitState(UNIT_STATE_STUNNED);
    me->RemoveAura(204237);
}
