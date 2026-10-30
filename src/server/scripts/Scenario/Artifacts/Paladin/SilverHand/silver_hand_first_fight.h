#ifndef SILVER_HAND_FIRST_FIGHT_H
#define SILVER_HAND_FIRST_FIGHT_H

#include "Scenario.h"
#include "ScenarioMgr.h"
#include "TemporarySummon.h"

namespace SilverHandFirstFight
{
    constexpr int32 DeathReceipt = 106624;

    inline bool CanRun(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106429 || actor->GetMapId() != 1539 || !actor->GetMap())
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 2;
    }

    inline bool IsOwnedBoss(Creature* actor, Creature* boss)
    {
        if (!CanRun(actor) || !boss || !boss->IsInWorld() || boss->IsPreDelete() ||
            boss->GetEntry() != 106624 || boss->GetMap() != actor->GetMap() ||
            boss->GetInstanceId() != actor->GetInstanceId())
            return false;
        TempSummon* summon = boss->ToTempSummon();
        return summon && summon->GetSummonerGUID() == actor->GetGUID();
    }

    inline Player* SelectPlayer(Creature* actor)
    {
        if (!CanRun(actor))
            return nullptr;
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
        ObjectGuid bossGuid;
        uint64 generation = 0, time = 0, retryDue = 0, deathDue = 0;
        uint32 failures = 0;
        bool requested = false, dead = false, completed = false, recipientWarning = false;

        uint64 Generation() const { return generation; }
        bool Active() const { return requested && !completed; }
        void Update(uint32 diff) { time += diff; }

        bool Begin(Creature* actor)
        {
            if (!CanRun(actor) || requested || completed)
                return false;
            requested = true;
            ++generation;
            return true;
        }

        void Cancel()
        {
            if (requested)
                ++generation;
            requested = false;
            bossGuid.Clear();
            retryDue = deathDue = 0;
            failures = 0;
            dead = recipientWarning = false;
        }

        void Register(Creature* actor, Creature* boss)
        {
            if (!Active() || !IsOwnedBoss(actor, boss) || (!bossGuid.IsEmpty() && bossGuid != boss->GetGUID()))
                return;
            if (bossGuid.IsEmpty())
            {
                bossGuid = boss->GetGUID();
                ++generation;
            }
        }

        void ObserveDeath(Creature* actor, Creature* boss)
        {
            if (!Active() || dead || !IsOwnedBoss(actor, boss) || boss->IsAlive() ||
                (!bossGuid.IsEmpty() && bossGuid != boss->GetGUID()))
                return;
            // Native InitSummon can skip an AI-disabled summoner's registration;
            // the instance's actual death receipt still proves this ownership.
            Register(actor, boss);
            dead = true;
            deathDue = time + 1000;
        }

        bool NeedsSpawn(Creature* actor)
        {
            if (!Active())
                return false;
            if (!CanRun(actor))
            {
                Cancel();
                return false;
            }
            if (dead || failures >= 30)
                return false;
            if (!bossGuid.IsEmpty())
            {
                if (Creature* boss = actor->GetMap()->GetCreature(bossGuid))
                {
                    ObserveDeath(actor, boss);
                    return false;
                }
                // A missing temporary actor is not a death receipt. Recreate
                // only that missing opponent; require its new real death.
                bossGuid.Clear();
                ++generation;
                retryDue = time + 1000;
            }
            if (time < retryDue)
                return false;
            return true;
        }

        bool Spawned(Creature* actor, Creature* boss)
        {
            if (!Active() || !CanRun(actor))
                return false;
            Register(actor, boss);
            if (IsOwnedBoss(actor, boss) && bossGuid == boss->GetGUID())
            {
                failures = 0;
                retryDue = 0;
                return true;
            }
            ++failures;
            retryDue = time + 1000;
            if (failures == 1 || failures == 30)
                TC_LOG_WARN("scripts", "Silver Hand first fight: G'norz creation failed, attempt %u/30; no death or stage credit", failures);
            return false;
        }

        Player* Complete(Creature* actor, bool ready)
        {
            if (!Active())
                return nullptr;
            if (!CanRun(actor))
            {
                Cancel();
                return nullptr;
            }
            if (!dead || time < deathDue)
                return nullptr;
            if (!ready || actor->HasUnitState(UNIT_STATE_STUNNED))
                return nullptr;
            Player* player = SelectPlayer(actor);
            if (!player)
            {
                if (!recipientWarning)
                {
                    recipientWarning = true;
                    TC_LOG_WARN("scripts", "Silver Hand first fight: observed G'norz death has no eligible stage2 participant; retaining receipt");
                }
                return nullptr;
            }
            completed = true;
            return player;
        }
    };

    inline void DeliverDeath(Creature* boss)
    {
        if (!boss || boss->IsAlive() || !boss->GetMap() || boss->GetEntry() != 106624)
            return;
        TempSummon* summon = boss->ToTempSummon();
        Creature* actor = summon ? boss->GetMap()->GetCreature(summon->GetSummonerGUID()) : nullptr;
        if (!IsOwnedBoss(actor, boss))
            return;
        CreatureAI* primary = actor->AI();
        CreatureAI* suspended = reinterpret_cast<CreatureAI*>(actor->GetDisabledAI());
        if (primary)
            primary->SetGUID(boss->GetGUID(), DeathReceipt);
        if (suspended && suspended != primary)
            suspended->SetGUID(boss->GetGUID(), DeathReceipt);
    }
}
#endif
