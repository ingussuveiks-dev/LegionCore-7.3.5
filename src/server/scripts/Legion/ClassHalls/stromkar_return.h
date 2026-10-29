#pragma once

#include "AreaTrigger.h"

namespace StromkarReturn
{
    struct State { bool ready = false, warned = false; ObjectGuid valkyr; uint32 retry = 0; };

    bool Source(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || !actor->IsAlive() || actor->GetEntry() != 104307 || actor->GetMapId() != 1539)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1037 && scenario->GetCurrentStep() == 10;
    }

    bool Eligible(Player* player)
    {
        return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
            player->getClass() == CLASS_WARRIOR && player->GetMapId() == 1539 &&
            !player->GetQuestRewardStatus(41105) &&
            (player->GetQuestStatus(41105) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(41105) == QUEST_STATUS_COMPLETE) &&
            player->GetQuestObjectiveData(41105, 103114) > 0 && player->HasItemCount(128910, 1, true);
    }

    Creature* Owner(Creature* actor)
    {
        TempSummon* summon = actor && actor->GetEntry() == 93819 ? actor->ToTempSummon() : nullptr;
        Unit* owner = summon ? summon->GetSummoner() : nullptr;
        Creature* source = owner ? owner->ToCreature() : nullptr;
        return Source(source) && actor->GetMap() == source->GetMap() && actor->InSamePhase(source) ? source : nullptr;
    }

    // Use the existing custom11869 visual and native215997 enter/leave actions.
    // The ordinary93819 SmartAI RESET uses hardcoded Dalaran coordinates; it
    // must not run for this explicitly owned tomb summon.
    struct ValkyrAI : ScriptedAI
    {
        explicit ValkyrAI(Creature* actor) : ScriptedAI(actor) { }
        ObjectGuid area;
        uint32 retry = 0;
        bool warned = false;

        void ClearArea()
        {
            if (AreaTrigger* trigger = me->GetMap()->GetAreaTrigger(area))
                if (trigger->GetCasterGUID() == me->GetGUID())
                    trigger->Remove();
            area.Clear();
        }

        void Reset() override
        {
            ClearArea();
            retry = 0;
            me->SetReactState(REACT_PASSIVE);
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!Owner(me))
            {
                ClearArea();
                me->DespawnOrUnsummon();
                return;
            }
            if (AreaTrigger* trigger = me->GetMap()->GetAreaTrigger(area))
                if (trigger->IsInWorld() && trigger->GetCasterGUID() == me->GetGUID())
                    return;
            if (retry > diff) { retry -= diff; return; }
            retry = 1000;
            Position pos = me->GetPosition();
            AreaTrigger* trigger = new AreaTrigger;
            if (!trigger->CreateAreaTrigger(sObjectMgr->GetGenerator<HighGuid::AreaTrigger>()->Generate(),
                0, me, nullptr, pos, pos, nullptr, ObjectGuid::Empty, 11869))
            {
                delete trigger;
                if (!warned)
                {
                    TC_LOG_WARN("scripts.ai", "Stromkar return: native custom11869 creation failed; retrying without transfer or credit");
                    warned = true;
                }
                return;
            }
            trigger->SetDuration(600000);
            area = trigger->GetGUID();
        }
    };

    void Signal(Creature* actor, uint32 id, uint32 value, State& state)
    {
        if (id != 3 || value != 1 || !actor || !actor->IsInWorld() || !actor->IsAlive() ||
            actor->GetEntry() != 104307 || actor->GetMapId() != 1539)
            return;
        Scenario* progress = sScenarioMgr->GetScenario(actor->GetInstanceId());
        if (progress && progress->GetScenarioId() == 1037 &&
            (progress->GetCurrentStep() == 9 || progress->GetCurrentStep() == 10))
            state.ready = true; // speech finished; Update still requires real completed stage10
    }

    void Update(Creature* actor, uint32 diff, State& state)
    {
        if (!state.ready || !Source(actor))
            return;
        Creature* current = ObjectAccessor::GetCreature(*actor, state.valkyr);
        if (current && current->IsInWorld() && current->IsAlive() && Owner(current) == actor)
            return;
        if (state.retry > diff) { state.retry -= diff; return; }
        state.retry = 1000;
        bool witness = false;
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (Player* player = reference.getSource())
                if (Eligible(player) && player->InSamePhase(actor) && player->IsWithinDistInMap(actor, 60.0f))
                    witness = true;
        if (!witness)
            return;
        // A local, collision-supported pose beside the real final Thoradin;
        // preserve the native93819 appearance rather than inventing a portal NPC.
        Position pos = actor->GetPosition();
        actor->MovePositionToFirstCollision(pos, 2.0f, 0.0f);
        Creature* summon = actor->SummonCreature(93819, pos, TEMPSUMMON_TIMED_DESPAWN, 600000);
        if (summon && Owner(summon) == actor)
            state.valkyr = summon->GetGUID();
        else if (!state.warned)
        {
            TC_LOG_WARN("scripts.ai", "Stromkar return: native Val'kyr summon unavailable; retrying after final speech without transfer or credit");
            state.warned = true;
        }
    }
}

class npc_stromkar_valkyr : public CreatureScript
{
public:
    npc_stromkar_valkyr() : CreatureScript("npc_stromkar_valkyr") { }
    CreatureAI* GetAI(Creature* actor) const override
    {
        if (StromkarReturn::Owner(actor))
            return new StromkarReturn::ValkyrAI(actor);
        return new SmartAI(actor); // retain all ordinary93819 data/event behavior
    }
};

class condition_stromkar_return : public ConditionScript
{
public:
    condition_stromkar_return() : ConditionScript("condition_stromkar_return") { }
    bool OnConditionCheck(Condition* condition, ConditionSourceInfo& source) override
    {
        if (condition && condition->SourceType == CONDITION_SOURCE_TYPE_AREATRIGGER_ACTION &&
            condition->SourceGroup == 11869 && condition->SourceEntry == 0 &&
            condition->ConditionType == CONDITION_QUEST_OBJECTIVE_DONE && condition->ConditionTarget == 1)
            return StromkarReturn::Eligible(source.mConditionTargets[1] ? source.mConditionTargets[1]->ToPlayer() : nullptr);
        return true;
    }
};
