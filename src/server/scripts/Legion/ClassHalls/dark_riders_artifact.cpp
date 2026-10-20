#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "ScenarioMgr.h"
#include "GameObject.h"
#include "Player.h"
#include <map>
#include <set>

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
namespace DarkRidersArrival
{
bool CheckArrival(Player* player, uint32 scenario, uint32 step)
{
    if (!player || scenario != 988 || step != 0 || !player->IsInWorld() || !player->IsAlive() ||
        player->GetMapId() != 1533 || player->IsBeingTeleported() || player->GetQuestRewardStatus(40623) ||
        (player->GetQuestStatus(40623) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40623) != QUEST_STATUS_COMPLETE))
        return false;

    Creature* marker = player->FindNearestCreature(100850, 48.0f);
    if (!marker || marker->GetDBTableGUIDLow() != 369334 || !marker->IsAIEnabled || !player->InSamePhase(marker))
        return false;

    // Gateway arrival can leave Revil behind and need not produce another
    // movement callback. Replay the exact barrier actor's Player-only event;
    // native LOS, range, conditions and one-shot admission remain authoritative.
    marker->AI()->MoveInLineOfSight_Safe(player);
    return true;
}
}
namespace DarkRidersSpirits
{
void UpdateContact(Player* player, uint32 scenario, uint32 step)
{
    if (!player || scenario != 988)
        return;

    bool contact = false;
    if (step <= 1 && player->IsInWorld() && player->IsAlive() && player->GetMapId() == 1533 &&
        !player->IsBeingTeleported() && !player->GetQuestRewardStatus(40623) &&
        (player->GetQuestStatus(40623) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(40623) == QUEST_STATUS_COMPLETE))
        if (Creature* spirit = player->FindNearestCreature(100845, 3.0f))
            contact = spirit->GetDBTableGUIDLow() && player->InSamePhase(spirit) &&
                player->GetExactDist(spirit) <= 3.0f && spirit->IsWithinLOSInMap(player);

    // 198721 on the NPC is only a visual. Native 198654 supplies the75%
    // slow, animation and1-second199568 damage ticks on the Player (CASTER1).
    // Do not refresh a present aura every250ms: that can starve its tick timer.
    if (contact)
    {
        if (!player->HasAura(198654))
            player->CastSpell(player, 198654, true);
    }
    else
        player->RemoveAurasDueToSpell(198654);
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
        std::set<ObjectGuid> spiritPlayers;
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
        void OnPlayerLeave(Player* player) override
        {
            Scenario* scenario = Progress();
            if (spiritPlayers.erase(player->GetGUID()) || (scenario && scenario->GetScenarioId() == 988))
                player->RemoveAurasDueToSpell(198654);
        }
        void onScenarionNextStep(uint32) override { Sync(); }
        void Update(uint32 diff) override
        {
            // Scenario construction can follow initial actor registration.
            if (refresh>diff) { refresh-=diff; return; }
            refresh=250;
            Sync();
            Scenario* scenario = Progress();
            if (scenario && scenario->GetScenarioId() == 988)
                instance->ApplyOnEveryPlayer([this, scenario](Player* player)
                {
                    if (DarkRidersArrival::CheckArrival(player, scenario->GetScenarioId(), scenario->GetCurrentStep()) &&
                        scenario->GetCurrentStep() == 1)
                        TC_LOG_INFO("scripts.ai", "Dark Riders: barrier arrival credited player %u; stage 2 active", player->GetGUIDLow());
                    DarkRidersSpirits::UpdateContact(player, scenario->GetScenarioId(), scenario->GetCurrentStep());
                    if (player->HasAura(198654))
                        spiritPlayers.insert(player->GetGUID());
                    else
                        spiritPlayers.erase(player->GetGUID());
                });
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
