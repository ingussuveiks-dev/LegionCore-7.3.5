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
#include "MoveSpline.h"


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

        enum class CaptainState { Waiting, Victory, PointWait, PointMove, Exposition, Path, AttackWait, Done };
        CaptainState captainState = CaptainState::Waiting;
        ObjectGuid captainOwner, captainDefeated, captainRouteActor, ladyGuid;
        std::weak_ptr<uint8> captainAttempt;
        float captainX = 0, captainY = 0, captainZ = 0, captainReach = 100.0f;
        uint32 captainDelay = 1000, captainRetry = 0;
        bool captainAssist = false, captainDelivering = false, captainSpoke = false, jaceStartSpoke = false, jaceEndSpoke = false;
        bool captainCreditWarning = false, captainActorWarning = false, captainMoveWarning = false, captainPathWarning = false, captainLadyWarning = false;

        void CancelCaptain()
        {
            captainState = CaptainState::Waiting;
            captainAssist = false;
            captainOwner.Clear();
            captainDefeated.Clear();
            captainRouteActor.Clear();
            captainAttempt.reset();
            captainDelay = 1000;
            captainRetry = 0;
            captainSpoke = jaceStartSpoke = jaceEndSpoke = false;
            captainCreditWarning = captainActorWarning = captainMoveWarning = captainPathWarning = captainLadyWarning = false;
        }

        Creature* CaptainActor(uint32 entry, uint64 spawn)
        {
            Creature* actor = instance->GetCreature(entry == 106318 ? ladyGuid : GetGuidData(entry));
            return actor && actor->IsInWorld() && !actor->IsPreDelete() && actor->IsAlive() && actor->GetMap() == instance &&
                actor->GetEntry() == entry && actor->GetDBTableGUIDLow() == spawn ? actor : nullptr;
        }

        void UpdateEarnedCageActors()
        {
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() < 4 ||
                scenario->GetCurrentStep() > 5 || !HasRouteProgress(47561)) return;
            // A new actor's constructor restores its template locks even when
            // the already-consumed cage no longer has an AI to release it.
            if (scenario->GetCurrentStep() == 4)
                if (Creature* captain = CaptainActor(106107, 361436))
                    if (!captain->isInCombat() && captain->GetCharmerGUID().IsEmpty() && captain->getFaction() == 35)
                    {
                        captain->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        captain->SetReactState(REACT_DEFENSIVE);
                        captain->setFaction(2102);
                    }
            Creature* boja = CaptainActor(106253, 361433);
            Creature* boros = GetBoros();
            if (boja && boros && !boja->isInCombat() && boja->GetCharmerGUID().IsEmpty() &&
                boja->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC) && boja->InSamePhase(boros) && boja->IsWithinDistInMap(boros, 70.0f))
            {
                boja->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                boja->GetMotionMaster()->MoveFollow(boros, PET_FOLLOW_DIST, static_cast<float>(-PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
            }
        }

        Player* CaptainRecipient(Creature* actor, Unit* source)
        {
            if (!actor || !source) return nullptr;
            if (Player* owner = source->GetCharmerOrOwnerPlayerOrPlayerItself())
                return TuureFirstFight::Eligible(owner, actor, 4, 4) ? owner : nullptr;
            if (source->GetCharmerOrOwnerGUID() || source->GetMap() != instance || !source->InSamePhase(actor) ||
                (source->GetEntry() != 106134 && source->GetEntry() != 106137 && source->GetEntry() != 106253)) return nullptr;
            Player* result = nullptr;
            for (auto const& reference : instance->GetPlayers())
                if (TuureFirstFight::Eligible(reference.getSource(), actor, 4, 4))
                {
                    if (result) return nullptr;
                    result = reference.getSource();
                }
            return result;
        }

        bool BindCaptain(Player* player)
        {
            if (!player) return false;
            player->AddQuestDelayedEvent(41993, 0, []() { });
            QuestStatusData* status = player->getQuestStatus(41993);
            if (!status || !status->ScriptLifetime) return false;
            captainOwner = player->GetGUID();
            captainAttempt = status->ScriptLifetime;
            return true;
        }

        bool CaptainOwnerReady(Player* player, Unit* anchor)
        {
            return player && anchor && anchor->IsInWorld() && !anchor->IsPreDelete() && anchor->GetMap() == instance &&
                player->IsInWorld() && player->CanContact() && !player->isGameMaster() && !player->HasPendingTeleport() &&
                player->GetMap() == instance && player->getClass() == CLASS_PRIEST && player->InSamePhase(anchor) &&
                !player->GetQuestRewardStatus(41993) &&
                (player->GetQuestStatus(41993) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(41993) == QUEST_STATUS_COMPLETE);
        }

        void EnterCombatForScript(Creature* creature, Unit* enemy) override
        {
            if (!creature || creature->GetMap() != instance) return;
            InstanceScript::EnterCombatForScript(creature, enemy);
            if (captainState != CaptainState::Waiting || creature->GetEntry() != 106107 ||
                creature->GetDBTableGUIDLow() != 361436 || !creature->IsAlive() || !HasRouteProgress(47561)) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 4) return;
            if (BindCaptain(CaptainRecipient(creature, enemy))) captainAssist = true;
        }

        void DeliverCaptainVictory()
        {
            if (captainState != CaptainState::Victory || captainDelivering) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085) return;
            if (HasRouteProgress(47909))
            {
                if (scenario->GetCurrentStep() == 5) { captainState = CaptainState::PointWait; captainDelay = 1000; }
                else if (scenario->GetCurrentStep() > 5) captainState = CaptainState::Done;
                return;
            }
            if (scenario->GetCurrentStep() != 4 || !HasRouteProgress(47561)) return;
            Player* player = TuureFirstFight::Find(instance, captainOwner);
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            if (captainAttempt.expired() || (player && (!status || status->ScriptLifetime != captainAttempt.lock()))) { CancelCaptain(); return; }
            Creature* anchor = instance->GetCreature(captainDefeated);
            if (anchor && (anchor->IsAlive() || anchor->GetEntry() != 106107 || anchor->GetDBTableGUIDLow() != 361436)) { CancelCaptain(); return; }
            if (!anchor || !anchor->IsInWorld() || anchor->IsPreDelete()) anchor = GetBoros();
            if (!CaptainOwnerReady(player, anchor) || !player->IsWithinDist3d(captainX, captainY, captainZ, captainReach)) return;
            captainDelivering = true;
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50634);
            captainDelivering = false;
            if (captainState != CaptainState::Victory) return;
            if (!HasRouteProgress(47909))
            {
                if (!captainCreditWarning) TC_LOG_WARN("scripts", "Tuure captain: real death50634 not accepted by native47909; retain original receipt");
                captainCreditWarning = true;
                captainRetry = 5000;
                return;
            }
            scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (scenario && scenario->GetScenarioId() == 1085 && scenario->GetCurrentStep() == 5)
            {
                captainState = CaptainState::PointWait;
                captainDelay = 1000;
            }
            else if (scenario && scenario->GetScenarioId() == 1085 && scenario->GetCurrentStep() > 5)
                captainState = CaptainState::Done;
        }

        void RecordCaptainDeath(Creature* creature, Unit* killer)
        {
            if (creature->GetEntry() != 106107 || creature->GetDBTableGUIDLow() != 361436 || !HasRouteProgress(47561)) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 4) return;
            if (captainState != CaptainState::Waiting)
            {
                Player* original = TuureFirstFight::Find(instance, captainOwner);
                QuestStatusData* status = original ? original->getQuestStatus(41993) : nullptr;
                if (captainState != CaptainState::Victory || (!captainAttempt.expired() && status && status->ScriptLifetime == captainAttempt.lock())) return;
                CancelCaptain();
            }
            if (!BindCaptain(CaptainRecipient(creature, killer))) return;
            captainDefeated = creature->GetGUID();
            captainX = creature->GetPositionX(); captainY = creature->GetPositionY(); captainZ = creature->GetPositionZ();
            captainReach = 100.0f + creature->GetObjectSize();
            captainState = CaptainState::Victory;
            captainAssist = false;
            DeliverCaptainVictory(); // original immediate native credit; movement still waits its real1s
        }

        void CaptainJace(Creature* boros, uint8 text, float range, bool& spoken)
        {
            if (spoken) return;
            if (Creature* jace = CaptainActor(106137, 361432))
                if (jace->InSamePhase(boros) && jace->IsWithinDistInMap(boros, range))
                {
                    spoken = true;
                    sCreatureTextMgr->SendChat(jace, text);
                }
        }

        void UpdateCaptain(uint32 diff)
        {
            if (captainDelivering || (captainState == CaptainState::Waiting && !captainAssist) || captainState == CaptainState::Done) return;
            Scenario* scenario = sScenarioMgr->GetScenario(instance->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 1085) return;
            if (scenario->GetCurrentStep() > 5) { captainState = CaptainState::Done; captainAssist = false; return; }
            Player* player = TuureFirstFight::Find(instance, captainOwner);
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            if (captainAttempt.expired() || (player && (!status || status->ScriptLifetime != captainAttempt.lock()))) { CancelCaptain(); return; }
            if (captainRetry > diff) { captainRetry -= diff; return; }
            captainRetry = 0;
            if (captainState == CaptainState::Victory) { DeliverCaptainVictory(); return; }
            Creature* boros = GetBoros();
            if (!boros && captainState != CaptainState::Waiting)
            {
                if (!captainActorWarning) TC_LOG_WARN("scripts", "Tuure captain: actual Boros361419 missing after accepted47909; retain route");
                captainActorWarning = true;
                return;
            }
            if (!CaptainOwnerReady(player, boros) || boros->HasUnitState(UNIT_STATE_STUNNED) || !boros->GetCharmerGUID().IsEmpty()) return;
            if (captainState == CaptainState::Waiting)
            {
                Creature* captain = CaptainActor(106107, 361436);
                if (scenario->GetCurrentStep() == 4 && captain && captain->isInCombat() && boros->InSamePhase(captain) &&
                    boros->IsWithinDistInMap(captain, 100.0f) && boros->IsAIEnabled && boros->AI())
                {
                    captainAssist = false;
                    if (!boros->getVictim() || boros->getVictim()->GetGUID() != captain->GetGUID()) boros->AI()->AttackStart(captain);
                }
                return;
            }
            if (scenario->GetCurrentStep() != 5 || !HasRouteProgress(47909)) return;
            if (captainState == CaptainState::PointWait)
            {
                if (!boros->IsWithinDist3d(captainX, captainY, captainZ, captainReach + 20.0f)) return;
                if (captainDelay > diff) { captainDelay -= diff; return; }
                captainState = CaptainState::PointMove;
                captainRouteActor = boros->GetGUID();
                boros->GetMotionMaster()->MovePoint(3, 69.05f, 1220.01f, -39.70f);
                return;
            }
            if (boros->GetGUID() != captainRouteActor)
            {
                captainState = CaptainState::PointWait; captainDelay = 1000; return;
            }
            if (captainState == CaptainState::PointMove)
            {
                if (boros->movespline && boros->movespline->Finalized() && boros->IsWithinDist3d(69.05f, 1220.01f, -39.70f, 0.5f, false))
                {
                    captainState = CaptainState::Exposition;
                    captainDelay = 8000;
                    if (!captainSpoke) { captainSpoke = true; boros->CreateConversation(2015); }
                    return;
                }
                MovementGenerator* point = boros->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_ACTIVE);
                if (!point || point->GetMovementGeneratorType() != POINT_MOTION_TYPE)
                {
                    if (!captainMoveWarning) TC_LOG_WARN("scripts", "Tuure captain: native point3 ended short; await real arrival, retry native movement");
                    captainMoveWarning = true;
                    captainState = CaptainState::PointWait; captainDelay = 1000;
                }
                return;
            }
            if (captainState == CaptainState::Exposition)
            {
                if (captainDelay > diff) { captainDelay -= diff; return; }
                captainDelay = 0;
                auto const* path = sWaypointMgr->GetPathScript(9859003);
                if (!path || path->size() != 8)
                {
                    if (!captainPathWarning) TC_LOG_WARN("scripts", "Tuure captain: native eight-point9859003 path missing; retain earned transition");
                    captainPathWarning = true; captainRetry = 5000; return;
                }
                captainState = CaptainState::Path;
                boros->UpdateWaypointID(uint32(-1));
                boros->GetMotionMaster()->MovePath(9859003, false);
                if (Creature* current = GetBoros()) CaptainJace(current, 1, 70.0f, jaceStartSpoke);
                return;
            }
            if (captainState == CaptainState::Path)
            {
                MovementGenerator* movement = boros->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
                bool nativePath = movement && movement->GetMovementGeneratorType() == WAYPOINT_MOTION_TYPE &&
                    static_cast<WaypointMovementGenerator<Creature>*>(movement)->GetPathId() == 9859003;
                // OnArrived writes node7 before the nonrepeating generator is removed.
                // The last-node case therefore also requires its real final pose/spline,
                // not an extant generator or a timer/AI callback that may no longer exist.
                bool finished = (!movement || movement->GetMovementGeneratorType() == IDLE_MOTION_TYPE || nativePath) &&
                    boros->GetCurrentWaypointID() == 7 && boros->movespline && boros->movespline->Finalized() &&
                    boros->IsWithinDist3d(192.805f, 1277.29f, -58.7333f, 0.5f, false);
                if (!finished && !nativePath)
                {
                    captainState = CaptainState::PointWait; captainDelay = 1000; return;
                }
                CaptainJace(boros, 1, 70.0f, jaceStartSpoke);
                if (!finished) return;
                captainState = CaptainState::AttackWait;
                captainDelay = 4000;
                if (Creature* current = GetBoros()) CaptainJace(current, 2, 60.0f, jaceEndSpoke);
                return;
            }
            if (captainState != CaptainState::AttackWait) return;
            if (captainDelay > diff) { captainDelay -= diff; return; }
            captainDelay = 0;
            Creature* lady = CaptainActor(106318, 361888);
            if (!lady)
            {
                if (!captainLadyWarning) TC_LOG_WARN("scripts", "Tuure captain: actual Lady361888 missing after native route; retain combat handoff");
                captainLadyWarning = true; return;
            }
            if (!lady->InSamePhase(boros) || !player->InSamePhase(lady) || !player->IsWithinDistInMap(lady, 100.0f) ||
                !boros->IsWithinDistInMap(lady, 100.0f) || !boros->IsAIEnabled || !boros->AI()) return;
            captainState = CaptainState::Done;
            if (!boros->getVictim() || boros->getVictim()->GetGUID() != lady->GetGUID()) boros->AI()->AttackStart(lady);
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
            {
                captainGuid = creature->GetGUID();
                if (creature->IsAlive() && captainState == CaptainState::Victory && !HasRouteProgress(47909)) CancelCaptain();
            }
            else if (creature->GetEntry() == 106318 && creature->GetDBTableGUIDLow() == 361888)
                ladyGuid = creature->GetGUID();
            else if (creature->GetEntry() == 106201 && creature->GetDBTableGUIDLow() == 361428 &&
                creature->IsAlive() && firstFight == FirstFight::Pending &&
                !TuureFirstFight::HasVictory(sScenarioMgr->GetScenario(instance->GetInstanceId())))
                CancelFirstFight();
        }

        void CreatureDies(Creature* creature, Unit* killer) override
        {
            if (!creature || creature->GetMap() != instance || creature->IsAlive()) return;
            if (creature->GetEntry() == 106107)
            {
                RecordCaptainDeath(creature, killer);
                return; // native credit callbacks may remove the defeated actor
            }
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
            UpdateEarnedCageActors();
            UpdateCaptain(diff);
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
