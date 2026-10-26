/*
    Priest: Tuure
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Player.h"
#include "Tuure.h"


class instance_tuure : public InstanceMapScript
{
public:
    instance_tuure() : InstanceMapScript("instance_tuure", 1489) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_tuure_InstanceMapScript(map);
    }

    struct instance_tuure_InstanceMapScript : public InstanceScript
    {
        instance_tuure_InstanceMapScript(InstanceMap* map) : InstanceScript(map) {}
        
        WorldLocation loc_res_pla;  // for respawn
        enum class FirstFight { Waiting, Pending, Unlocking, Done };
        FirstFight firstFight = FirstFight::Waiting;
        ObjectGuid borosGuid, nextOpponentGuid, defeatedGuid, recipientGuid;
        std::weak_ptr<uint8> questAttempt;
        uint32 victoryDelay = 1000, unlockDelay = 16000, retryDelay = 0;
        bool delivering = false, deliveryWarning = false;

        void CancelFirstFight()
        {
            firstFight = FirstFight::Waiting;
            defeatedGuid.Clear();
            recipientGuid.Clear();
            questAttempt.reset();
            victoryDelay = 1000;
            retryDelay = 0;
            deliveryWarning = false;
        }

        Creature* GetBoros()
        {
            Creature* actor = instance->GetCreature(borosGuid);
            return actor && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() &&
                actor->GetEntry() == 106134 && actor->GetDBTableGUIDLow() == 361419 ? actor : nullptr;
        }

        void BeginUnlock()
        {
            firstFight = FirstFight::Unlocking;
            questAttempt.reset();
            unlockDelay = 16000;
            // Resolve after criteria callbacks; never retain a borrowed actor.
            if (Creature* speaker = GetBoros())
                speaker->CreateConversation(2009);
            else
                TC_LOG_WARN("scripts", "Tuure first fight: conversation actor106134 disappeared after native50562; retain earned unlock");
        }

        void OnCreatureCreate(Creature* creature) override
        {
            if (!creature || creature->GetMap() != instance) return;
            InstanceScript::OnCreatureCreate(creature);
            if (creature->GetEntry() == 106134 && creature->GetDBTableGUIDLow() == 361419)
                borosGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106221 && creature->GetDBTableGUIDLow() == 361429)
                nextOpponentGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106201 && creature->GetDBTableGUIDLow() == 361428 &&
                creature->IsAlive() && firstFight == FirstFight::Pending &&
                !TuureFirstFight::HasVictory(sScenarioMgr->GetScenario(instance->GetInstanceId())))
                CancelFirstFight();
        }

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (!creature || creature->GetMap() != instance || creature->IsAlive() ||
                creature->GetEntry() != 106201 || creature->GetDBTableGUIDLow() != 361428) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() > 2 ||
                (scenario->GetCurrentStep() == 2 && !TuureFirstFight::HasVictory(scenario)))
                return;
            if (firstFight == FirstFight::Pending)
            {
                Player* original = TuureFirstFight::Find(instance, recipientGuid);
                QuestStatusData* status = original ? original->getQuestStatus(41993) : nullptr;
                if (!questAttempt.expired() && status && status->ScriptLifetime == questAttempt.lock()) return;
                CancelFirstFight();
            }
            if (firstFight != FirstFight::Waiting) return;
            Player* player = TuureFirstFight::Recipient(creature, killer);
            if (!player) return;
            player->AddQuestDelayedEvent(41993, 0, []() { });
            QuestStatusData* status = player->getQuestStatus(41993);
            if (!status || !status->ScriptLifetime) return;
            defeatedGuid = creature->GetGUID();
            recipientGuid = player->GetGUID();
            questAttempt = status->ScriptLifetime;
            firstFight = FirstFight::Pending;
        }

        WorldLocation* GetClosestGraveYard(float x, float y, float z) override
        // OnPlayerDeath
        {            
            // Init data
            loc_res_pla.WorldRelocate(1489, x, y, z);

            return &loc_res_pla;
        }

        void OnUnitDeath(Unit* creature) override
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!creature || creature->GetMap() != instance || !scenario || scenario->GetScenarioId() != 1085) return;
            switch (creature->GetEntry())
            {
                case 106221:
                    if (Creature* targ = creature->FindNearestCreature(106134, 100.0f, true))
                        targ->AI()->DoAction(1);
                    break;
            }
        }
         
        void Update(uint32 diff) override
        {
            if (firstFight == FirstFight::Waiting || firstFight == FirstFight::Done) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || delivering) return;
            if (firstFight == FirstFight::Unlocking)
            {
                if (scenario->GetCurrentStep() > 2) { firstFight = FirstFight::Done; return; }
                if (scenario->GetCurrentStep() != 2 || !TuureFirstFight::HasVictory(scenario)) return;
                if (unlockDelay > diff) { unlockDelay -= diff; return; }
                unlockDelay = 0;
                Creature* boros = GetBoros();
                Creature* target = instance->GetCreature(nextOpponentGuid);
                if (!boros || !target || !target->IsInWorld() || target->IsPreDelete() || !target->IsAlive() ||
                    target->GetEntry() != 106221 || target->GetDBTableGUIDLow() != 361429 ||
                    !boros->InSamePhase(target) || !boros->IsWithinDistInMap(target, 40.0f)) return;
                firstFight = FirstFight::Done;
                target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                return;
            }
            if (firstFight != FirstFight::Pending) return;
            if (scenario->GetCurrentStep() >= 1 && TuureFirstFight::HasVictory(scenario))
            {
                if (scenario->GetCurrentStep() > 2) { firstFight = FirstFight::Done; return; }
                Creature* boros = GetBoros();
                if (!boros || boros->HasUnitState(UNIT_STATE_STUNNED)) return;
                if (victoryDelay > diff) { victoryDelay -= diff; return; }
                victoryDelay = 0;
                BeginUnlock(); // already accepted: never send a second50562
                return;
            }
            Player* player = TuureFirstFight::Find(instance, recipientGuid);
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            if (questAttempt.expired() || (player && (!status || status->ScriptLifetime != questAttempt.lock())))
            {
                TC_LOG_WARN("scripts", "Tuure first fight: original quest41993 attempt ended before50562 delivery; cancel stale receipt");
                CancelFirstFight();
                return;
            }
            if (Creature* defeated = instance->GetCreature(defeatedGuid))
                if (defeated->IsAlive()) { CancelFirstFight(); return; }
            Creature* boros = GetBoros();
            if (!boros || boros->HasUnitState(UNIT_STATE_STUNNED) ||
                !TuureFirstFight::Eligible(player, boros, 1, 1)) return;
            if (victoryDelay > diff) { victoryDelay -= diff; return; }
            victoryDelay = 0;
            if (retryDelay > diff) { retryDelay -= diff; return; }
            retryDelay = 0;
            delivering = true;
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50562);
            delivering = false;
            if (!TuureFirstFight::HasVictory(sScenarioMgr->GetScenario(instance->GetInstanceId())))
            {
                if (!deliveryWarning)
                    TC_LOG_WARN("scripts", "Tuure first fight: native50562 did not update real criterion47519; retain death receipt and retry native delivery");
                deliveryWarning = true;
                retryDelay = 5000;
                return;
            }
            BeginUnlock();
        }
    };
};

void AddSC_instance_tuure()
{
    new instance_tuure();
}
