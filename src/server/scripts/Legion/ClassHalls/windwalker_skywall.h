#pragma once

#include "Map.h"
#include "Player.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SmartAI.h"
#include "TemporarySummon.h"
#include "Timer.h"

namespace WindwalkerSkywall
{
constexpr uint32 MapId = 1528, ScenarioId = 983, QuestId = 40570;
constexpr uint32 Minions = 3, Dragon = 4, Tyrant = 5;
constexpr uint32 MinionsTree = 46395, DragonTree = 46406;

inline Scenario* Progress(Creature const& actor)
{
    if (actor.GetMapId() != MapId)
        return nullptr;
    Scenario* scenario = sScenarioMgr->GetScenario(actor.GetInstanceId());
    return scenario && scenario->GetScenarioId() == ScenarioId && scenario->GetMap() == actor.GetMap() ? scenario : nullptr;
}

inline uint64 Count(Scenario& scenario, uint32 tree)
{
    auto const* progress = scenario.GetAchievementMgr().GetCriteriaProgressMap();
    auto const found = progress->find(tree);
    return found == progress->end() ? 0 : found->second.Counter;
}

inline bool Participant(Player* player, Creature const& actor, float distance)
{
    return player && player->IsInWorld() && player->IsAlive() && player->CanContact() && !player->isGameMaster() &&
        player->GetMap() == actor.GetMap() && player->getClass() == CLASS_MONK && !player->HasPendingTeleport() &&
        player->GetQuestStatus(QuestId) == QUEST_STATUS_INCOMPLETE && !player->GetQuestRewardStatus(QuestId) &&
        player->InSamePhase(&actor) && player->IsWithinDistInMap(&actor, distance);
}
}

struct npc_skywall_zaurac : SmartAI
{
    explicit npc_skywall_zaurac(Creature* creature) : SmartAI(creature) { }

    void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
    {
        if (!IsWaveDragon())
            return;

        // The surrender action needs a living actor even after level scaling.
        uint64 health = me->GetHealth(attacker);
        if (damage >= health)
            damage = health ? uint32(health - 1) : 0;
    }

    void UpdateAI(uint32 diff) override
    {
        SmartAI::UpdateAI(diff);
        if (!IsWaveDragon())
            return;

        // The native health event changes faction. The source image can already
        // have despawned, so the credit belongs to Zaurac's actual surrender.
        if (me->getFaction() == 35 && GetScript()->HasRunEvent(7, SMART_EVENT_HEALTH_PCT))
            surrendered = true;
        if (!surrendered)
            return;

        Scenario* scenario = WindwalkerSkywall::Progress(*me);
        if (!scenario)
            return;
        if (!credited && scenario->GetCurrentStep() == WindwalkerSkywall::Minions &&
            (!retrying || GetMSTimeDiffToNow(retryAt) >= 2000))
            RecordSurrender(*scenario);

        scenario = WindwalkerSkywall::Progress(*me);
        if (credited && scenario && scenario->GetCurrentStep() == WindwalkerSkywall::Dragon)
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        else
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }

    void OnSpellClick(Unit* clicker) override
    {
        if (!IsWaveDragon())
        {
            SmartAI::OnSpellClick(clicker);
            return;
        }

        Scenario* scenario = WindwalkerSkywall::Progress(*me);
        Player* player = clicker ? clicker->ToPlayer() : nullptr;
        if (credited && scenario && scenario->GetCurrentStep() == WindwalkerSkywall::Dragon &&
            WindwalkerSkywall::Participant(player, *me, INTERACTION_DISTANCE + me->GetCombatReach()) &&
            me->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK) && player->IsWithinLOSInMap(me))
            SmartAI::OnSpellClick(clicker);
    }

private:
    bool surrendered = false, credited = false, retrying = false, warned = false;
    uint32 retryAt = 0;

    bool IsWaveDragon() const
    {
        TempSummon* summon = me->ToTempSummon();
        return me->GetMapId() == WindwalkerSkywall::MapId && summon && summon->GetSummonerGUID().GetEntry() == 100762;
    }

    void RecordSurrender(Scenario& scenario)
    {
        retrying = true;
        retryAt = getMSTime();
        // Native difficulty 12 admits one player, including a monk whose
        // guardian landed the surrendering blow.
        for (auto const& reference : me->GetMap()->GetPlayers())
            if (Player* player = reference.getSource(); WindwalkerSkywall::Participant(player, *me, 50.0f))
            {
                uint64 before = WindwalkerSkywall::Count(scenario, WindwalkerSkywall::MinionsTree);
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 47952);
                Scenario* current = WindwalkerSkywall::Progress(*me);
                credited = current == &scenario && WindwalkerSkywall::Count(*current, WindwalkerSkywall::MinionsTree) > before;
                if (!credited && !warned)
                {
                    warned = true;
                    TC_LOG_WARN("scripts", "Skywall: native Zaurac surrender47952 was not acknowledged; retaining the encounter for retry");
                }
                return;
            }
    }
};

struct npc_skywall_typhinius : SmartAI
{
    explicit npc_skywall_typhinius(Creature* creature) : SmartAI(creature) { }

    void InitializeAI() override
    {
        SmartAI::InitializeAI();
        UpdateAdmission();
    }

    void UpdateAI(uint32 diff) override
    {
        UpdateAdmission();
        SmartAI::UpdateAI(diff);
    }

    void DamageTakenAfterScaling(Unit*, uint32& damage, DamageEffectType) override
    {
        if (IsSkywallBoss() && !Admitted())
            damage = 0;
    }

private:
    bool IsSkywallBoss() const { return me->GetMapId() == WindwalkerSkywall::MapId && me->GetDBTableGUIDLow() == 372120; }

    bool Admitted() const
    {
        Scenario* scenario = WindwalkerSkywall::Progress(*me);
        return scenario && scenario->GetCurrentStep() == WindwalkerSkywall::Tyrant &&
            WindwalkerSkywall::Count(*scenario, WindwalkerSkywall::DragonTree) != 0;
    }

    void UpdateAdmission() const
    {
        if (!IsSkywallBoss())
            return;

        uint32 const immunity = UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC;
        if (Admitted())
            me->RemoveFlag(UNIT_FIELD_FLAGS, immunity);
        else
            me->SetFlag(UNIT_FIELD_FLAGS, immunity);
    }
};
