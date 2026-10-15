#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScenarioMgr.h"

namespace DarkRidersArtifact
{
void ProtectFinale(Creature* creature,Unit* attacker,uint32& damage,uint32 scenario)
{
    if (!creature || !attacker || scenario!=988 || creature->GetEntry()!=100344 || !creature->IsAlive()) return;
    // SmartAI's old invincibility hook runs before target-relative damage
    // scaling. This hook receives the actual damage used by Unit's death check.
    if (damage>=creature->GetHealth(attacker))
    {
        damage=0;
        if (creature->GetHealth()>1)
        {
            creature->SetHealth(1);
            TC_LOG_INFO("scripts.ai","Ulthalesh: protected Ariden's finale from a scaled killing blow");
        }
    }
}
}
class instance_dark_riders_artifact : public InstanceMapScript
{
public:
    instance_dark_riders_artifact():InstanceMapScript("instance_dark_riders_artifact",1533){}
    struct Instance : InstanceScript
    {
        explicit Instance(InstanceMap* map):InstanceScript(map){}
        void OnCreatureDamageTaken(Creature* creature,Unit* attacker,uint32& damage) override
        {
            Scenario* scenario=sScenarioMgr->GetScenario(instance->GetInstanceId());
            DarkRidersArtifact::ProtectFinale(creature,attacker,damage,scenario?scenario->GetScenarioId():0);
        }
    };
    InstanceScript* GetInstanceScript(InstanceMap* map) const override {return new Instance(map);}
};
void AddSC_dark_riders_artifact(){new instance_dark_riders_artifact();}
