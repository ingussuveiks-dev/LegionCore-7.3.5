#ifndef SILVER_HAND_FIRST_ROOM_H
#define SILVER_HAND_FIRST_ROOM_H

#include "Scenario.h"
#include "ScenarioMgr.h"

namespace SilverHandFirstRoom
{
    inline Scenario* Context(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106429 || actor->GetMapId() != 1539 || !actor->GetMap())
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 ? scenario : nullptr;
    }

    inline Player* SelectPlayer(Creature* actor)
    {
        for (auto const& reference : actor->GetMap()->GetPlayers())
        {
            Player* player = reference.getSource();
            if (player && player->IsInWorld() && !player->IsPreDelete() && player->CanContact() &&
                !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_PALADIN &&
                player->GetMap() == actor->GetMap() && player->GetInstanceId() == actor->GetInstanceId() &&
                player->InSamePhase(actor) && player->IsWithinDistInMap(actor, 100.0f) &&
                !player->GetQuestRewardStatus(42120) && player->GetQuestStatus(42120) == QUEST_STATUS_INCOMPLETE)
                return player;
        }
        return nullptr;
    }

    struct State
    {
        uint64 time = 0, retryDue = 0;
        uint32 attempts = 0;
        bool started = false, issued = false, observed = false, completed = false;
        bool dispatching = false, recipientWarning = false;

        void Update(uint32 diff) { time += diff; }

        bool Begin(Creature* actor, bool fightCompleted)
        {
            Scenario* scenario = Context(actor);
            if (!fightCompleted || started || !scenario || scenario->GetCurrentStep() != 3)
                return false;
            started = true;
            return true;
        }

        bool Issue(Creature* actor)
        {
            Scenario* scenario = Context(actor);
            if (!started || issued || !scenario || scenario->GetCurrentStep() != 3)
                return false;
            issued = true;
            return true;
        }

        void Observe(Creature* actor)
        {
            Scenario* scenario = Context(actor);
            if (issued && scenario && (scenario->GetCurrentStep() == 3 || scenario->GetCurrentStep() == 4))
                observed = true;
        }

        bool Complete(Creature* actor)
        {
            if (!observed || completed || dispatching)
                return false;
            Scenario* scenario = Context(actor);
            if (!scenario || (scenario->GetCurrentStep() != 3 && scenario->GetCurrentStep() != 4) ||
                actor->HasUnitState(UNIT_STATE_STUNNED))
                return false;
            if (scenario->GetCurrentStep() == 3)
            {
                if (time < retryDue || attempts == 30)
                    return false;
                Player* player = SelectPlayer(actor);
                if (!player)
                {
                    if (!recipientWarning)
                    {
                        recipientWarning = true;
                        TC_LOG_WARN("scripts", "Silver Hand first room: observed point3 has no eligible stage3 participant; retaining receipt");
                    }
                    return false;
                }
                ++attempts;
                retryDue = time + 1000;
                dispatching = true;
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50872);
                dispatching = false;
                scenario = Context(actor);
                if (!scenario || scenario->GetCurrentStep() != 4)
                {
                    if (attempts == 1 || attempts == 30)
                        TC_LOG_WARN("scripts", "Silver Hand first room: 50872 did not advance to stage4, attempt %u/30; retaining point3", attempts);
                    return false;
                }
            }
            completed = true;
            return true;
        }
    };
}
#endif
