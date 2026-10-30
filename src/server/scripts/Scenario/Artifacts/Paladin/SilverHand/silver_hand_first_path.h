#ifndef SILVER_HAND_FIRST_PATH_H
#define SILVER_HAND_FIRST_PATH_H

#include "Scenario.h"
#include "ScenarioMgr.h"
#include "WaypointManager.h"

namespace SilverHandFirstPath
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
        bool requested = false, observed = false, completed = false, dispatching = false;
        bool departureReady = false, departed = false, recipientWarning = false, pathWarning = false;

        void Update(uint32 diff) { time += diff; }

        bool Request(Creature* actor)
        {
            Scenario* scenario = Context(actor);
            if (requested || !scenario || scenario->GetCurrentStep() != 1)
                return false;
            requested = true;
            return true;
        }

        void Observe(Creature* actor)
        {
            Scenario* scenario = Context(actor);
            if (requested && scenario && (scenario->GetCurrentStep() == 1 || scenario->GetCurrentStep() == 2))
                observed = true;
        }

        bool PathAvailable()
        {
            WaypointPath const* path = sWaypointMgr->GetPathScript(10993805);
            if (path && !path->empty())
                return true;
            if (!pathWarning)
            {
                pathWarning = true;
                TC_LOG_WARN("scripts", "Silver Hand first path: 10993805 unavailable; retaining observed arrival/departure for retry");
            }
            return false;
        }

        bool Complete(Creature* actor)
        {
            if (!observed || completed || dispatching)
                return false;
            Scenario* scenario = Context(actor);
            if (!scenario || (scenario->GetCurrentStep() != 1 && scenario->GetCurrentStep() != 2) ||
                actor->HasUnitState(UNIT_STATE_STUNNED) || !PathAvailable())
                return false;
            if (scenario->GetCurrentStep() == 1)
            {
                if (time < retryDue || attempts == 30)
                    return false;
                Player* player = SelectPlayer(actor);
                if (!player)
                {
                    if (!recipientWarning)
                    {
                        recipientWarning = true;
                        TC_LOG_WARN("scripts", "Silver Hand first path: observed arrival has no eligible stage1 participant; retaining receipt");
                    }
                    return false;
                }
                ++attempts;
                retryDue = time + 1000;
                dispatching = true;
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50766);
                dispatching = false;
                // Native criteria dispatch advances the shared Scenario synchronously.
                // A rejected event must not publish the next room or force a step.
                scenario = Context(actor);
                if (!scenario || scenario->GetCurrentStep() != 2)
                {
                    if (attempts == 1 || attempts == 30)
                        TC_LOG_WARN("scripts", "Silver Hand first path: 50766 did not advance to stage2, attempt %u/30; retaining arrival", attempts);
                    return false;
                }
            }
            completed = true;
            return true;
        }

        bool CanContinue(Creature* actor) const
        {
            Scenario* scenario = Context(actor);
            return completed && !departed && scenario && scenario->GetCurrentStep() == 2;
        }

        bool Depart(Creature* actor)
        {
            if (!departureReady || !CanContinue(actor) || actor->HasUnitState(UNIT_STATE_STUNNED) || !PathAvailable())
                return false;
            departed = true;
            return true;
        }
    };
}
#endif
