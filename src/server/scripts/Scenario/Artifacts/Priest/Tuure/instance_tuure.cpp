/*
    Priest: Tuure
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Player.h"
#include "Tuure.h"
#include "WaypointMovementGenerator.h"
#include "CreatureTextMgr.h"


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
        ObjectGuid borosGuid, nextOpponentGuid, defeatedGuid, recipientGuid, bojaGuid, captainGuid;
        std::weak_ptr<uint8> questAttempt;
        uint32 victoryDelay = 1000, unlockDelay = 16000, retryDelay = 0;
        bool delivering = false, deliveryWarning = false;
        enum class FirstRoute { Waiting, Pending, Moving, Arrived, Done };
        FirstRoute firstRoute = FirstRoute::Waiting;
        ObjectGuid routeRecipient, routeDefeated, routeActor, jaceGuid;
        std::weak_ptr<uint8> routeAttempt;
        uint32 routeDelay = 1000, routeRetry = 0;
        bool routeDelivering = false, routeKillWarning = false, routeArrivalWarning = false, routePathWarning = false, routeOpening = false, routeMiddle = false, routeAlliedKill = false;

        bool HasRouteProgress(uint32 tree)
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085) return false;
            auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
            auto found = progress->find(tree);
            return found != progress->end() && found->second.Counter;
        }

        void CancelRoute()
        {
            firstRoute = FirstRoute::Waiting;
            routeRecipient.Clear();
            routeDefeated.Clear();
            routeActor.Clear();
            routeAttempt.reset();
            routeDelay = 1000;
            routeRetry = 0;
            routeKillWarning = routeArrivalWarning = routePathWarning = routeOpening = routeMiddle = routeAlliedKill = false;
        }

        void RecordRouteDeath(Creature* creature, Unit* killer)
        {
            if (creature->GetEntry() != 106221 || creature->GetDBTableGUIDLow() != 361429) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 2 ||
                !TuureFirstFight::HasVictory(scenario)) return;
            if (firstRoute != FirstRoute::Waiting)
            {
                Player* original = TuureFirstFight::Find(instance, routeRecipient);
                QuestStatusData* status = original ? original->getQuestStatus(41993) : nullptr;
                if (firstRoute == FirstRoute::Done || (!routeAttempt.expired() && status &&
                    status->ScriptLifetime == routeAttempt.lock())) return;
                CancelRoute();
            }
            Player* player = TuureFirstFight::Recipient(creature, killer);
            if (!player) return;
            player->AddQuestDelayedEvent(41993, 0, []() { });
            QuestStatusData* status = player->getQuestStatus(41993);
            if (!status || !status->ScriptLifetime) return;
            routeRecipient = player->GetGUID();
            routeDefeated = creature->GetGUID();
            routeAttempt = status->ScriptLifetime;
            routeAlliedKill = !killer->GetCharmerOrOwnerPlayerOrPlayerItself(); // Recipient admits only the three unowned allies
            firstRoute = FirstRoute::Pending;
        }

        void UpdateRoute(uint32 diff)
        {
            if (firstRoute == FirstRoute::Waiting || firstRoute == FirstRoute::Done || routeDelivering) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085) return;
            if (scenario->GetCurrentStep() > 2) { firstRoute = FirstRoute::Done; return; }
            Player* player = TuureFirstFight::Find(instance, routeRecipient);
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            if (routeAttempt.expired() || (player && (!status || status->ScriptLifetime != routeAttempt.lock())))
            {
                CancelRoute();
                return;
            }
            if (scenario->GetCurrentStep() != 2 || !TuureFirstFight::HasVictory(scenario)) return;
            Creature* defeated = instance->GetCreature(routeDefeated);
            if (defeated && (defeated->IsAlive() || defeated->GetEntry() != 106221 || defeated->GetDBTableGUIDLow() != 361429))
            {
                CancelRoute();
                return;
            }
            if (defeated && (!defeated->IsInWorld() || defeated->IsPreDelete())) defeated = nullptr;
            Creature* boros = GetBoros();
            if (!boros || boros->HasUnitState(UNIT_STATE_STUNNED) || !boros->GetCharmerGUID().IsEmpty()) return;
            if (firstRoute == FirstRoute::Moving)
            {
                MovementGenerator* movement = boros->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
                if (boros->GetGUID() != routeActor || !movement || movement->GetMovementGeneratorType() != WAYPOINT_MOTION_TYPE ||
                    static_cast<WaypointMovementGenerator<Creature>*>(movement)->GetPathId() != 9859002)
                {
                    firstRoute = FirstRoute::Pending;
                    routeDelay = 1000;
                    return; // reissue the native route after replacement/interruption, never teleport
                }
                uint32 node = boros->GetCurrentWaypointID(); // native OnArrived writes this even without an AI
                if (node != uint32(-1) && node >= 6 && node < 13 && !routeMiddle)
                {
                    routeMiddle = true;
                    boros->CreateConversation(2007);
                }
                if (node == uint32(-1) || node < 10 || node >= 13) return;
                firstRoute = FirstRoute::Arrived;
            }
            boros = GetBoros(); // optional conversations can run script callbacks
            player = TuureFirstFight::Find(instance, routeRecipient);
            status = player ? player->getQuestStatus(41993) : nullptr;
            if (routeAttempt.expired() || (player && (!status || status->ScriptLifetime != routeAttempt.lock())))
            {
                CancelRoute();
                return;
            }
            if (!boros) return;
            if (!TuureFirstFight::Eligible(player, boros, 2, 2)) return;
            if (routeRetry > diff) { routeRetry -= diff; return; }
            routeRetry = 0;
            if (!HasRouteProgress(47883))
            {
                if (!routeAlliedKill)
                {
                    if (!routeKillWarning) TC_LOG_WARN("scripts", "Tuure route: real player-owned106221 kill lacks native47883; await native reward, no substitute grant");
                    routeKillWarning = true;
                    routeRetry = 5000;
                    return;
                }
                // KillRewarder delivers native scenario credit before CreatureDies.
                // An untagged ally kill has no KillRewarder: forward only this real death
                // to the scenario, not to individual quests/loot/XP/guild achievements.
                routeDelivering = true;
                scenario->UpdateAchievementCriteria(CRITERIA_TYPE_KILL_CREATURE, 106221, 1, 0, defeated, player);
                routeDelivering = false;
                if (!HasRouteProgress(47883))
                {
                    if (!routeKillWarning) TC_LOG_WARN("scripts", "Tuure route: native106221 criterion47883 rejected real death; retain receipt");
                    routeKillWarning = true;
                    routeRetry = 5000;
                    return;
                }
                player = TuureFirstFight::Find(instance, routeRecipient);
                status = player ? player->getQuestStatus(41993) : nullptr;
                boros = GetBoros();
                scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
                if (routeAttempt.expired() || !status || status->ScriptLifetime != routeAttempt.lock() || !boros ||
                    !scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 2 ||
                    !TuureFirstFight::Eligible(player, boros, 2, 2)) return;
            }
            if (firstRoute == FirstRoute::Arrived)
            {
                if (!HasRouteProgress(47882))
                {
                    routeDelivering = true;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50831);
                    routeDelivering = false;
                }
                if (HasRouteProgress(47882)) firstRoute = FirstRoute::Done;
                else
                {
                    if (!routeArrivalWarning) TC_LOG_WARN("scripts", "Tuure route: native50831 rejected actual waypoint10; retain arrival");
                    routeArrivalWarning = true;
                    routeRetry = 5000;
                }
                return;
            }
            if (firstRoute != FirstRoute::Pending) return;
            if (routeDelay > diff) { routeDelay -= diff; return; }
            routeDelay = 0;
            auto const* path = sWaypointMgr->GetPathScript(9859002);
            if (!path || path->size() != 13)
            {
                if (!routePathWarning) TC_LOG_WARN("scripts", "Tuure route: native9859002 thirteen-point path missing; retain earned transition");
                routePathWarning = true;
                routeRetry = 5000;
                return;
            }
            firstRoute = FirstRoute::Moving;
            routeActor = boros->GetGUID();
            boros->UpdateWaypointID(uint32(-1)); // discard an earlier path's arrival, never manufacture a waypoint
            boros->GetMotionMaster()->MovePath(9859002, false);
            if (!routeOpening)
            {
                routeOpening = true;
                if (Creature* jace = instance->GetCreature(jaceGuid))
                    if (jace->IsInWorld() && !jace->IsPreDelete() && jace->IsAlive() &&
                        jace->GetEntry() == 106137 && jace->GetDBTableGUIDLow() == 361432 &&
                        boros->InSamePhase(jace) && boros->IsWithinDistInMap(jace, 20.0f))
                    {
                        jace->GetMotionMaster()->MoveFollow(boros, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
                        sCreatureTextMgr->SendChat(jace, 0);
                    }
                if (Creature* speaker = GetBoros()) speaker->CreateConversation(2039);
            }
        }

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

        ObjectGuid GetGuidData(uint32 entry) const override
        {
            switch (entry)
            {
                case 106134: return borosGuid;
                case 106137: return jaceGuid;
                case 106253: return bojaGuid;
                case 106107: return captainGuid;
                default: return InstanceScript::GetGuidData(entry);
            }
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
            {
                nextOpponentGuid = creature->GetGUID();
                if (creature->IsAlive() && firstRoute != FirstRoute::Waiting && firstRoute != FirstRoute::Done &&
                    !HasRouteProgress(47883)) CancelRoute();
            }
            else if (creature->GetEntry() == 106137 && creature->GetDBTableGUIDLow() == 361432)
                jaceGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106253 && creature->GetDBTableGUIDLow() == 361433)
                bojaGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106107 && creature->GetDBTableGUIDLow() == 361436)
                captainGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106201 && creature->GetDBTableGUIDLow() == 361428 &&
                creature->IsAlive() && firstFight == FirstFight::Pending &&
                !TuureFirstFight::HasVictory(sScenarioMgr->GetScenario(instance->GetInstanceId())))
                CancelFirstFight();
        }

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (!creature || creature->GetMap() != instance || creature->IsAlive()) return;
            RecordRouteDeath(creature, killer);
            if (creature->GetEntry() != 106201 || creature->GetDBTableGUIDLow() != 361428) return;
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

        void OnUnitDeath(Unit* /*creature*/) override
        {
            // A state-only death has no killer. Real Kill owns both transitions.
        }
         
        void Update(uint32 diff) override
        {
            UpdateRoute(diff);
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
