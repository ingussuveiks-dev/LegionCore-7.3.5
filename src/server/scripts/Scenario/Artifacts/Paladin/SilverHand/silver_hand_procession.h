#ifndef SILVER_HAND_PROCESSION_H
#define SILVER_HAND_PROCESSION_H

#include "Scenario.h"
#include "ScenarioMgr.h"

namespace SilverHandProcessionRecovery
{
    inline Scenario* Context(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106639 || actor->GetMapId() != 1539 || !actor->GetMap())
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 ? scenario : nullptr;
    }

    struct State
    {
        uint64 time = 0, retryDue = 0;
        uint32 attempts = 0;
        bool arrived = false, completed = false, busy = false, spoken = false, recipientWarning = false;

        void Update(uint32 diff) { time += diff; }

        void Observe(Creature* actor, ObjectGuid const& guid)
        {
            Scenario* scenario = Context(actor);
            if (!guid.IsEmpty() && scenario && (scenario->GetCurrentStep() == 4 || scenario->GetCurrentStep() == 5))
                arrived = true;
        }

        bool Finish(Creature* actor, ObjectGuid& guid)
        {
            Scenario* scenario = Context(actor);
            if (!arrived || completed || guid.IsEmpty() || !scenario || scenario->GetCurrentStep() != 5)
                return false;
            completed = true;
            guid.Clear();
            return true;
        }

        bool Ready(Creature* actor, ObjectGuid const& guid) const
        {
            Scenario* scenario = Context(actor);
            return arrived && !completed && !busy && !guid.IsEmpty() && scenario && scenario->GetCurrentStep() == 4 &&
                time >= retryDue && attempts < 30;
        }
    };
}
#endif
