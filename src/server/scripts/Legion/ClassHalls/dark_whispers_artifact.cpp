#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScenarioMgr.h"

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

class instance_dark_whispers_artifact : public InstanceMapScript
{
public:
    instance_dark_whispers_artifact() : InstanceMapScript("instance_dark_whispers_artifact", 1498) {}
    struct Instance : InstanceScript
    {
        explicit Instance(InstanceMap* map) : InstanceScript(map) {}
        void OnCreatureDamageTaken(Creature* creature, Unit* attacker, uint32& damage) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            DarkWhispersArtifact::ProtectFinale(creature, attacker, damage, scenario ? scenario->GetScenarioId() : 0);
        }
    };
    InstanceScript* GetInstanceScript(InstanceMap* map) const override { return new Instance(map); }
};

void AddSC_dark_whispers_artifact() { new instance_dark_whispers_artifact(); }
