#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScenarioMgr.h"
#include "GameObject.h"
#include <map>

namespace DarkRidersArtifact
{
bool IsStageActor(uint32 entry)
{
    return entry==100850 || entry==101257 || entry==100344;
}
bool ActorAvailable(uint32 entry,bool summoned,uint32 step)
{
    if (entry==100850) return step==(summoned?3u:1u);
    if (entry==101257) return step==2;
    if (entry==100344) return step==4 || step==5;
    return true;
}
void ProtectFinale(Creature* creature,Unit* attacker,uint32& damage,uint32 scenario)
{
    if (!creature || !attacker || scenario!=988 ||
        (creature->GetEntry()!=100344 && creature->GetEntry()!=100850) || !creature->IsAlive()) return;
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
        struct ActorGate
        {
            uint32 flags=0;
            ReactStates reaction=REACT_PASSIVE;
            bool reactionHeld=false;
        };
        std::map<ObjectGuid,ActorGate> actors;
        std::map<ObjectGuid,uint32> objects;
        uint32 refresh=0;
        Scenario* Progress() { return sScenarioMgr->GetScenario(instance->GetInstanceId()); }
        void Sync()
        {
            Scenario* progress=Progress();
            if (!progress || progress->GetScenarioId()!=988) return;
            uint32 step=progress->GetCurrentStep();
            for (auto& actor:actors)
                if (Creature* creature=instance->GetCreature(actor.first))
                {
                    bool lock=!DarkRidersArtifact::ActorAvailable(creature->GetEntry(),creature->isSummon(),step);
                    if (lock)
                    {
                        actor.second.flags|=UNIT_FLAG_NON_ATTACKABLE & ~creature->GetUInt32Value(UNIT_FIELD_FLAGS);
                        creature->SetFlag(UNIT_FIELD_FLAGS,UNIT_FLAG_NON_ATTACKABLE);
                        if (!actor.second.reactionHeld)
                        {
                            actor.second.reaction=creature->GetReactState();
                            actor.second.reactionHeld=true;
                        }
                        creature->SetReactState(REACT_PASSIVE);
                    }
                    else
                    {
                        if (actor.second.flags)
                        {
                            creature->RemoveFlag(UNIT_FIELD_FLAGS,actor.second.flags);
                            actor.second.flags=0;
                        }
                        if (actor.second.reactionHeld)
                        {
                            creature->SetReactState(actor.second.reaction);
                            actor.second.reactionHeld=false;
                        }
                    }
                }
            for (auto& object:objects)
                if (GameObject* go=instance->GetGameObject(object.first))
                {
                    if (step!=3)
                    {
                        object.second|=GO_FLAG_NOT_SELECTABLE & ~go->GetUInt32Value(GAMEOBJECT_FIELD_FLAGS);
                        go->SetFlag(GAMEOBJECT_FIELD_FLAGS,GO_FLAG_NOT_SELECTABLE);
                    }
                    else if (object.second)
                    {
                        go->RemoveFlag(GAMEOBJECT_FIELD_FLAGS,object.second);
                        object.second=0;
                    }
                }
        }
        void OnCreatureCreate(Creature* creature) override
        {
            InstanceScript::OnCreatureCreate(creature);
            if (DarkRidersArtifact::IsStageActor(creature->GetEntry()))
            {
                actors.emplace(creature->GetGUID(),ActorGate{});
                Sync();
            }
        }
        void OnCreatureRemove(Creature* creature) override
        {
            actors.erase(creature->GetGUID());
            InstanceScript::OnCreatureRemove(creature);
        }
        void OnGameObjectCreate(GameObject* go) override
        {
            InstanceScript::OnGameObjectCreate(go);
            if (go->GetEntry()==245994)
            {
                objects.emplace(go->GetGUID(),0);
                Sync();
            }
        }
        void OnGameObjectRemove(GameObject* go) override
        {
            objects.erase(go->GetGUID());
            InstanceScript::OnGameObjectRemove(go);
        }
        void OnPlayerEnter(Player*) override { Sync(); }
        void onScenarionNextStep(uint32) override { Sync(); }
        void Update(uint32 diff) override
        {
            // Scenario construction can follow initial actor registration.
            if (refresh>diff) { refresh-=diff; return; }
            refresh=250;
            Sync();
        }
        void OnCreatureDamageTaken(Creature* creature,Unit* attacker,uint32& damage) override
        {
            Scenario* scenario=Progress();
            if (scenario && scenario->GetScenarioId()==988 && creature &&
                DarkRidersArtifact::IsStageActor(creature->GetEntry()) &&
                !DarkRidersArtifact::ActorAvailable(creature->GetEntry(),creature->isSummon(),scenario->GetCurrentStep()))
            {
                damage=0;
                return;
            }
            DarkRidersArtifact::ProtectFinale(creature,attacker,damage,scenario?scenario->GetScenarioId():0);
        }
    };
    InstanceScript* GetInstanceScript(InstanceMap* map) const override {return new Instance(map);}
};
void AddSC_dark_riders_artifact(){new instance_dark_riders_artifact();}
