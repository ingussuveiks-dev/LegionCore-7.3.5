#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScenarioMgr.h"
#include "Player.h"

namespace DarkWhispersArtifact
{
void ProtectFinale(Creature* creature, Unit* attacker, uint32& damage, uint32 scenario)
{
    if (!creature || !attacker || scenario != 1097 || creature->GetEntry() != 106652 || !creature->IsAlive())
        return;

    // Unit has already normalized damage for this attacker. Keep the actor
    // alive for SmartAI's native low-health retreat, credit and skull summon.
    if (damage >= creature->GetHealth(attacker))
    {
        damage = 0;
        if (creature->GetHealth() > 1)
        {
            creature->SetHealth(1);
            TC_LOG_INFO("scripts.ai", "Dark Whispers: protected Mephistroth's finale from a scaled killing blow");
        }
    }
}
}

namespace DarkWhispersArrival
{
bool CheckArrival(Player* player, uint32 scenario, uint32 step)
{
    if (!player || scenario != 1097 || step != 2 || !player->IsInWorld() || !player->IsAlive() ||
        player->GetMapId() != 1498 || player->IsBeingTeleported() || player->GetQuestRewardStatus(42125) ||
        (player->GetQuestStatus(42125) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42125) != QUEST_STATUS_COMPLETE))
        return false;

    Creature* marker = player->FindNearestCreature(99739, 30.0f);
    // Entry 99739 is also at the entrance and is used by other scenarios.
    // Retry only the native marker on the far side of this barrier.
    if (!marker || marker->GetDBTableGUIDLow() != 376351 || !marker->IsAIEnabled || !player->InSamePhase(marker))
        return false;

    // A teleport can finish without another movement/LOS notification.
    // Replay the native SmartAI LOS event with the actual Player invoker;
    // its range, LOS, stage and nonrepeatable checks remain authoritative.
    marker->AI()->MoveInLineOfSight_Safe(player);
    return true;
}
}

class instance_dark_whispers_artifact : public InstanceMapScript
{
public:
    instance_dark_whispers_artifact() : InstanceMapScript("instance_dark_whispers_artifact", 1498) {}
    struct Instance : InstanceScript
    {
        explicit Instance(InstanceMap* map) : InstanceScript(map) {}
        uint32 arrivalTimer = 0;
        void Update(uint32 diff) override
        {
            if (arrivalTimer > diff)
            {
                arrivalTimer -= diff;
                return;
            }
            arrivalTimer = 500;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1097 || scenario->GetCurrentStep() != 2)
                return;
            instance->ApplyOnEveryPlayer([scenario](Player* player)
            {
                if (DarkWhispersArrival::CheckArrival(player, scenario->GetScenarioId(), scenario->GetCurrentStep()) &&
                    scenario->GetCurrentStep() == 3)
                    TC_LOG_INFO("scripts.ai", "Dark Whispers: barrier arrival credited player %u; stage 4 active", player->GetGUIDLow());
            });
        }
        void OnCreatureDamageTaken(Creature* creature, Unit* attacker, uint32& damage) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            DarkWhispersArtifact::ProtectFinale(creature, attacker, damage, scenario ? scenario->GetScenarioId() : 0);
        }
    };
    InstanceScript* GetInstanceScript(InstanceMap* map) const override { return new Instance(map); }
};

void AddSC_dark_whispers_artifact() { new instance_dark_whispers_artifact(); }
