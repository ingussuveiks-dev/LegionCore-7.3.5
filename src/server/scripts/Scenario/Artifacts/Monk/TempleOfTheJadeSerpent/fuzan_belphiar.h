struct npc_fuzan_belphiar : SmartAI
{
    explicit npc_fuzan_belphiar(Creature* creature) : SmartAI(creature) { }
    ~npc_fuzan_belphiar() override { Release(); }

    void Release()
    {
        if (!_held) return;
        if (_addedFlag) me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetReactState(_reaction);
        _held = false;
    }

    bool Scoped() const
    {
        return me->GetEntry() == 109397 && me->GetDBTableGUIDLow() == 373405 && me->GetMapId() == 1625;
    }

    bool Blocked() const
    {
        if (!Scoped()) return false;
        Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
        return !scenario || (scenario->GetScenarioId() == 1137 && scenario->GetCurrentStep() != 2);
    }

    bool UpdateGate()
    {
        bool blocked = Blocked();
        if (blocked)
        {
            if (!_held)
            {
                _held = true;
                _addedFlag = !me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                _reaction = me->GetReactState();
            }
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
            if (me->isInCombat()) me->CombatStop(true);
        }
        else Release();
        return blocked;
    }

    void InitializeAI() override
    {
        SmartAI::InitializeAI();
        UpdateGate();
    }

    void Reset() override
    {
        SmartAI::Reset();
        UpdateGate();
    }

    void UpdateAI(uint32 diff) override
    {
        UpdateGate();
        SmartAI::UpdateAI(diff);
    }

    bool CanAIAttack(Unit const* target) const override
    {
        return !Blocked() && SmartAI::CanAIAttack(target);
    }

    void AttackStart(Unit* target) override
    {
        if (!UpdateGate()) SmartAI::AttackStart(target);
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        if (UpdateGate()) damage = 0;
        else SmartAI::DamageTaken(attacker, damage, type);
    }

    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType type) override
    {
        // The native dispatcher also calls the suspended original AI during
        // charm. Do not let a scaled one-hit kill consume the only52443 and
        // maiden summon before the actual rescue/door prerequisites finish.
        if (UpdateGate()) damage = 0;
        else SmartAI::DamageTakenAfterScaling(attacker, damage, type);
    }

private:
    bool _held = false;
    bool _addedFlag = false;
    ReactStates _reaction = REACT_AGGRESSIVE;
};
