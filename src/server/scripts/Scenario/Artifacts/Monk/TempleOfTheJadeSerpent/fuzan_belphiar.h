#pragma once

#include "fuzan_victory_recipient.h"

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
        _deathRecorded = false;
        _victoryOwner.Clear();
        _victoryAttempt.reset();
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

    ObjectGuid VictoryOwner() const { return _victoryOwner; }
    std::shared_ptr<QuestCancellationReceipt const> VictoryAttempt() const { return _victoryAttempt; }

    void JustDied(Unit* killer) override
    {
        // Snapshot before native52443 can invoke a quest/phase callback. This
        // also covers native instakills that do not pass through damage hooks.
        if (Scoped() && !me->IsAlive() && !_deathRecorded)
        {
            _deathRecorded = true;
            Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player) player = me->GetLootRecipient();
            if (player && player->IsInWorld() && player->GetMap() == me->GetMap() && player->getClass() == CLASS_MONK &&
                !player->isGameMaster() && !player->GetQuestRewardStatus(42765) &&
                (player->GetQuestStatus(42765) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(42765) == QUEST_STATUS_COMPLETE))
            {
                _victoryOwner = player->GetGUID();
                _victoryAttempt = player->GetQuestCancellationReceipt(42765);
                if (auto receiver = dynamic_cast<FuZanVictoryRecipient*>(me->GetInstanceScript()))
                    receiver->RecordFuZanVictory(me, _victoryOwner, _victoryAttempt);
            }
        }
        SmartAI::JustDied(killer);
    }

private:
    bool _held = false;
    bool _addedFlag = false;
    ReactStates _reaction = REACT_AGGRESSIVE;
    bool _deathRecorded = false;
    ObjectGuid _victoryOwner;
    std::shared_ptr<QuestCancellationReceipt const> _victoryAttempt;
};
