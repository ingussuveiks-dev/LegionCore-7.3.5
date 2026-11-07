/*
    Warrior: EarthWarder
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Conversation.h"
#include "ConversationData.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellMgr.h"
#include "FunctionProcessor.h"
#include "MoveSpline.h"
#include "WaypointMovementGenerator.h"
#include "PathGenerator.h"
#include "QuestObjectScope.h"
#include "magnar_start.h"
#include "magnar_combat.h"
#include "hruthnir_pillik.h"
#include <memory>
#include <set>
#include <array>

Position const trashPos[12] =
{
    {4723.45f, 387.95f, -37.10f, 4.66f},
    {4708.64f, 308.05f, -37.10f, 1.42f},
    //
    {4696.31f, 303.57f, -36.15f, 1.57f},
    {4715.72f, 397.23f, -36.15f, 4.53f},
    //
    {4731.75f, 311.36f, -36.15f, 2.18f},
    //
    {4721.45f, 306.58f, -36.15f, 2.09f},
    {4741.29f, 319.75f, -36.15f, 2.37f},
    //
    {4750.02f, 365.74f, -36.13f, 3.91f},
    {4736.81f, 385.62f, -36.14f, 3.99f},
    {4747.12f, 377.10f, -36.15f, 3.90f},

    {4703.47f, 397.29f, -36.13f, 4.56f},
    {4685.47f, 308.99f, -36.12f, 1.24f}
};

namespace HruthnirGossip
{
    bool CanStart(Player* player, Creature* creature)
    {
        if (!player || !creature || !player->IsInWorld() || !player->CanContact() ||
            !creature->IsInWorld() || !creature->IsAlive() || creature->IsDespawn() ||
            creature->GetEntry() != 96468 || creature->GetMapId() != 1495 ||
            player->getClass() != CLASS_WARRIOR || !player->GetMap() ||
            player->GetMap() != creature->GetMap() || player->GetInstanceId() != creature->GetInstanceId() ||
            !player->InSamePhase(creature) || !player->canSeeOrDetect(creature) ||
            !player->IsWithinLOSInMap(creature) ||
            player->GetNPCIfCanInteractWith(creature->GetGUID(), UNIT_NPC_FLAG_GOSSIP) != creature ||
            player->GetQuestRewardStatus(39191) ||
            (player->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE))
            return false;

        Scenario* scenario = sScenarioMgr->GetScenario(creature->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 3;
    }
}

// 96468
class npc_hrutnir : public CreatureScript
{
public:
    npc_hrutnir() : CreatureScript("npc_hrutnir") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (HruthnirGossip::CanStart(player, creature) && creature->IsAIEnabled && creature->AI() && creature->AI()->GetData(1061))
            return false;
        player->PlayerTalkClass->ClearMenus();
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    bool OnGossipSelect(Player*, Creature*, uint32, uint32) override
    {
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hrutnirAI(creature);
    }

    struct npc_hrutnirAI : ScriptedAI
    {
        npc_hrutnirAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            firstfighting = false;
            secondfighting = false;
            thirdfighting = false;
            fourfighting = false;
            // me->SetReactState(REACT_DEFENSIVE);
        }

        ~npc_hrutnirAI() override
        {
            CancelMagnarIntro();
            CancelMagnarStart();
            RemoveMagnarConversation();
        }

        EventMap events;
        bool _introDone, firstfighting, secondfighting, thirdfighting, fourfighting;
        bool magnarArrivalPending = false;
        std::shared_ptr<bool> magnarStartToken;
        uint32 magnarStartInstance = 0;
        std::shared_ptr<bool> magnarOfferToken;
        bool magnarIntroPending = false;
        bool magnarIntroFinished = false;
        bool magnarIntroFailureReported = false;
        ObjectGuid magnarIntroPlayer;
        uint32 magnarIntroTimer = 0;
        uint32 magnarIntroGeneration = 0;
        bool magnarIntroCasting = false;
        bool magnarChamberStarted = false;
        uint32 magnarIntroInstanceGeneration = 0;
        bool magnarOfferReady = false;
        ObjectGuid magnarConversation = ObjectGuid::Empty;
        FunctionProcessor _presentationFunctions;
        ObjectGuid magnarStartOwner;
        std::shared_ptr<QuestCancellationReceipt const> magnarStartAttempt;
        bool magnarStartReached = false;
        bool magnarStartPresented = false;
        bool magnarStartCueDue = false;
        bool magnarStartRetryQueued = false;
        bool magnarStartBusy = false;
        struct PillikProgress
        {
            uint32 instance = 0, epoch = 0, retry = 0;
            bool queued = false, approached = false, voiceSent = false, moveRequested = false;
            bool complete = false, busy = false, failureReported = false;
            uint32 entryRetry = 0;
            bool entryComplete = false, entryBusy = false, entryFailureReported = false;
        };
        std::shared_ptr<PillikProgress> pillikProgress = std::make_shared<PillikProgress>();
        uint32 magnarReofferTimer = 0;
        ObjectGuid magnarStartedBoss;
        uint32 magnarRoutePoint = 0;
        uint32 magnarRouteTimer = 0;
        bool magnarRouteFailureReported = false;

        void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId != 18743 || gossipListId != 0 || !HruthnirGossip::CanStart(player, me) || !IsMagnarOfferReady())
                return;
            GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
            GossipMenuItem const* item = menu.GetItem(0);
            if (menu.GetMenuId() != menuId || !item || item->OptionNpc != GossipOptionNpc::None ||
                item->Sender != 0 || item->Action != 0 || item->IsCoded || item->BoxMoney != 0)
                return;
            auto attempt = player->GetQuestCancellationReceipt(39191);
            if (!attempt || attempt->load(std::memory_order_acquire))
                return;
            magnarOfferReady = false;
            magnarOfferToken.reset();
            magnarStartInstance = me->GetInstanceId();
            magnarStartToken = std::make_shared<bool>(false);
            magnarStartOwner = player->GetGUID();
            magnarStartAttempt = std::move(attempt);
            magnarStartReached = magnarStartPresented = magnarStartCueDue = magnarStartRetryQueued = magnarStartBusy = false;
            magnarArrivalPending = true;
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            ScheduleMagnarApproach();
            me->GetMotionMaster()->MovePoint(7, 4720.80f, 347.91f, -37.71f);
        }

        void Reset() override
        {
            CancelMagnarIntro();
            events.Reset();
            CancelMagnarStart();
            magnarOfferToken.reset();
            magnarOfferReady = false;
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            RemoveMagnarConversation();
        }

        bool IsPillikContext(bool active = true) const
        {
            if (!me->IsInWorld() || !me->IsAlive() || me->IsDespawn() || me->GetEntry() != 96468 ||
                me->GetMapId() != 1495 || !me->GetMap() ||
                (active ? (!me->IsAIEnabled || me->AI() != this) : (me->AI() != this && me->GetDisabledAI() != this)))
                return false;
            InstanceScript* instance = me->GetInstanceScript();
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return instance && instance->GetGuidData(PillikHandoff::Companion) == me->GetGUID() &&
                scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() <= 2;
        }

        bool IsPillikPlayer(Player* player) const
        {
            return player && player->IsInWorld() && player->CanContact() && !player->IsBeingTeleported() &&
                player->getClass() == CLASS_WARRIOR && player->GetMap() == me->GetMap() &&
                player->GetInstanceId() == me->GetInstanceId() && player->InSamePhase(me) &&
                player->IsWithinDistInMap(me, 100.0f) && !player->GetQuestRewardStatus(39191) &&
                (player->GetQuestStatus(39191) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(39191) == QUEST_STATUS_COMPLETE);
        }

        void QueuePillikHandoff()
        {
            if (!IsPillikContext(false) || sScenarioMgr->GetScenario(me->GetInstanceId())->GetCurrentStep() > 1)
                return;
            uint32 epoch = me->GetInstanceScript()->GetData(1060);
            if (pillikProgress->queued && pillikProgress->epoch == epoch)
                return;
            pillikProgress = std::make_shared<PillikProgress>();
            pillikProgress->queued = true;
            pillikProgress->instance = me->GetInstanceId();
            pillikProgress->epoch = epoch;
        }

        void TryPillikEntry(uint32 diff = 0)
        {
            auto state = pillikProgress;
            if (!state->queued || state->entryComplete || state->entryBusy)
                return;
            if (state->entryRetry > diff)
            {
                state->entryRetry -= diff;
                return;
            }
            state->entryRetry = 500;
            if (!IsPillikContext() || state->instance != me->GetInstanceId() ||
                state->epoch != me->GetInstanceScript()->GetData(1060))
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(state->instance);
            if (scenario->GetCurrentStep() != 0)
            {
                state->entryComplete = true;
                return;
            }
            Player* player = nullptr;
            for (auto const& reference : me->GetMap()->GetPlayers())
                if (IsPillikPlayer(reference.getSource()) && reference.getSource()->IsAlive() &&
                    (!player || me->GetDistance(reference.getSource()) < me->GetDistance(player)))
                    player = reference.getSource();
            if (!player)
                return;
            Creature* companion = me;
            state->entryBusy = true;
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46288);
            // The criterion can re-enter or destroy this AI. Release only the
            // captured state before checking the current controller identity.
            state->entryBusy = false;
            if (companion->AI() != this || pillikProgress != state || !IsPillikContext() ||
                state->instance != me->GetInstanceId() || state->epoch != me->GetInstanceScript()->GetData(1060))
                return;
            state->entryComplete = sScenarioMgr->GetScenario(state->instance)->GetCurrentStep() != 0;
            if (!state->entryComplete && !state->entryFailureReported)
            {
                TC_LOG_WARN("scripts", "Pillik approach: event46288 did not advance scenario909 in instance %u; actual waypoint retained for retry", state->instance);
                state->entryFailureReported = true;
            }
        }

        bool ArmPillikHandoff()
        {
            auto state = pillikProgress;
            if (!state->queued || !IsPillikContext(false) || state->instance != me->GetInstanceId() ||
                state->epoch != me->GetInstanceScript()->GetData(1060))
                return false;
            state->approached = true;
            return true;
        }

        void UpdatePillikHandoff(uint32 diff)
        {
            auto state = pillikProgress;
            if (!state->approached || state->complete || state->busy)
                return;
            if (state->retry > diff)
            {
                state->retry -= diff;
                return;
            }
            state->retry = 500;
            TryPillikHandoff();
        }

        void TryPillikHandoff()
        {
            auto state = pillikProgress;
            if (!state->approached || state->complete || state->busy)
                return;
            state->busy = true;
            ContinuePillikHandoff(state);
            // The callback may destroy this AI. This local shared state remains
            // valid, and no post-callback member access is needed to release it.
            state->busy = false;
        }

        void ContinuePillikHandoff(std::shared_ptr<PillikProgress> const& state)
        {
            Creature* companion = me;
            InstanceScript* instance = me->GetInstanceScript();
            auto current = [this, companion, instance, state]()
            {
                return companion->AI() == this && pillikProgress == state && IsPillikContext() &&
                    me->GetInstanceScript() == instance && me->GetInstanceId() == state->instance &&
                    instance->GetData(1060) == state->epoch && instance->GetData(PillikHandoff::Death);
            };
            if (!current())
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(state->instance);
            if (scenario->GetCurrentStep() == 0)
                return;
            if (Creature* boss = ObjectAccessor::GetCreature(*me, instance->GetGuidData(PillikHandoff::Actor)))
                if (boss->IsAlive())
                    return;
            Player* player = ObjectAccessor::GetPlayer(*me, instance->GetGuidData(PillikHandoff::Recipient));
            if (!IsPillikPlayer(player))
            {
                player = nullptr;
                for (auto const& reference : me->GetMap()->GetPlayers())
                    if (IsPillikPlayer(reference.getSource()) &&
                        (!player || me->GetDistance(reference.getSource()) < me->GetDistance(player)))
                        player = reference.getSource();
            }
            if (!player)
                return;
            ObjectGuid recipientGuid = player->GetGUID();
            if (!state->voiceSent)
            {
                state->voiceSent = true;
                secondfighting = false;
                Talk(5);
                if (!current())
                    return;
            }
            if (!state->moveRequested)
            {
                state->moveRequested = true;
                me->GetMotionMaster()->MovePoint(2, 4809.14f, 164.68f, -11.20f);
                if (!current())
                    return;
            }
            scenario = sScenarioMgr->GetScenario(state->instance);
            if (scenario->GetCurrentStep() == 1)
            {
                player = ObjectAccessor::GetPlayer(*me, recipientGuid);
                if (!IsPillikPlayer(player))
                    return;
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46289);
                if (!current())
                    return;
                scenario = sScenarioMgr->GetScenario(state->instance);
            }
            state->complete = scenario->GetCurrentStep() == 2;
            if (!state->complete && !state->failureReported)
            {
                TC_LOG_WARN("scripts", "Pillik handoff: event46289 did not advance scenario909 in instance %u; native handoff retained for retry", state->instance);
                state->failureReported = true;
            }
        }

        void CancelMagnarStart()
        {
            magnarStartToken.reset();
            magnarStartedBoss.Clear();
            magnarStartOwner.Clear();
            magnarStartAttempt.reset();
            magnarStartInstance = 0;
            magnarArrivalPending = false;
            magnarStartReached = magnarStartPresented = magnarStartCueDue = magnarStartRetryQueued = magnarStartBusy = false;
        }

        void RejoinMagnarStart(Player* player)
        {
            if (magnarStartToken && !*magnarStartToken && magnarStartAttempt && !magnarStartAttempt->load() &&
                player && player->GetGUID() == magnarStartOwner && player->getClass() == CLASS_WARRIOR &&
                player->GetMap() == me->GetMap() && player->GetInstanceId() == magnarStartInstance &&
                !player->GetQuestRewardStatus(39191) &&
                (player->GetQuestStatus(39191) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(39191) == QUEST_STATUS_COMPLETE))
                player->AdoptQuestCancellationReceipt(39191, magnarStartAttempt);
        }

        void ScheduleMagnarApproach()
        {
            if (!magnarStartToken || *magnarStartToken || !magnarArrivalPending || magnarStartReached || magnarStartPresented)
                return;
            std::weak_ptr<bool> weak = magnarStartToken;
            Creature* companion = me;
            auto* expectedAI = this;
            me->AddDelayedEvent(1000, [weak, companion, expectedAI]()
            {
                auto token = weak.lock();
                if (!token || *token || (companion->AI() != expectedAI && companion->GetDisabledAI() != expectedAI))
                    return;
                if (expectedAI->magnarStartToken == token)
                    expectedAI->RetryMagnarApproach(token);
            });
        }

        void RetryMagnarApproach(std::shared_ptr<bool> const& token)
        {
            if (magnarStartToken != token || *token || !magnarArrivalPending || magnarStartReached || magnarStartPresented)
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(magnarStartInstance);
            if (!magnarStartAttempt || magnarStartAttempt->load() || !me->IsInWorld() || !me->IsAlive() ||
                me->IsDespawn() || me->GetEntry() != 96468 || me->GetMapId() != 1495 ||
                me->GetInstanceId() != magnarStartInstance || !scenario || scenario->GetScenarioId() != 909 ||
                scenario->GetCurrentStep() != 3 || !HasMagnarChamberRequest())
            {
                CancelMagnarStart();
                RemoveMagnarConversation();
                return;
            }
            Player* owner = ObjectAccessor::GetPlayer(*me, magnarStartOwner);
            if (!me->IsAIEnabled || me->AI() != this || !owner || !owner->IsInWorld() || !owner->CanContact() ||
                owner->IsBeingTeleported() || owner->getClass() != CLASS_WARRIOR || owner->GetMap() != me->GetMap() ||
                owner->GetInstanceId() != magnarStartInstance || !owner->InSamePhase(me))
            {
                ScheduleMagnarApproach();
                return;
            }
            if (owner->GetQuestRewardStatus(39191) ||
                (owner->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && owner->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE) ||
                owner->GetQuestCancellationReceipt(39191) != magnarStartAttempt)
            {
                CancelMagnarStart();
                RemoveMagnarConversation();
                return;
            }
            // Queue before MovePoint: replacing a generator can invoke native
            // callbacks that revoke the request or destroy this controller.
            ScheduleMagnarApproach();
            if (!owner->IsAlive() || !owner->IsWithinDistInMap(me, 100.0f) || me->isInCombat() ||
                me->HasUnitState(UNIT_STATE_NOT_MOVE) || me->IsMovementPreventedByCasting() ||
                !me->movespline->Finalized())
                return;
            MotionMaster* motion = me->GetMotionMaster();
            if (motion->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                // MovePath installs the authored escort in the idle slot. A
                // finished non-repeating route can remain under the point move.
                auto* route = dynamic_cast<WaypointMovementGenerator<Creature>*>(motion->top());
                if (!route || route->GetPathId() != 9859004 || !route->IsPathComplete())
                    return;
            }
            me->GetMotionMaster()->MovePoint(7, 4720.80f, 347.91f, -37.71f);
            // Only the actual arrival callback may acknowledge this movement.
        }

        void ScheduleMagnarStart(uint32 delay, bool cue = false)
        {
            if (!magnarStartToken || *magnarStartToken || magnarStartRetryQueued)
                return;
            magnarStartRetryQueued = true;
            std::weak_ptr<bool> weak = magnarStartToken;
            Creature* companion = me;
            auto* expectedAI = this;
            me->AddDelayedEvent(delay, [weak, companion, expectedAI, cue]()
            {
                auto token = weak.lock();
                if (!token || *token || (companion->AI() != expectedAI && companion->GetDisabledAI() != expectedAI))
                    return;
                if (expectedAI->magnarStartToken != token)
                    return;
                expectedAI->magnarStartRetryQueued = false;
                if (cue)
                    expectedAI->magnarStartCueDue = true;
                expectedAI->TryMagnarStart();
            });
        }

        void TryMagnarStart()
        {
            auto token = magnarStartToken;
            if (!token || *token || !magnarStartReached || magnarStartBusy)
                return;
            Creature* companion = me;
            magnarStartBusy = true;
            ContinueMagnarStart(token);
            if ((companion->AI() == this || companion->GetDisabledAI() == this) && magnarStartToken == token)
                magnarStartBusy = false;
        }

        void ContinueMagnarStart(std::shared_ptr<bool> const& token)
        {
            Creature* companion = me;
            Scenario* scenario = sScenarioMgr->GetScenario(magnarStartInstance);
            if (!magnarStartAttempt || magnarStartAttempt->load() || !me->IsInWorld() || !me->IsAlive() ||
                me->IsDespawn() || me->GetEntry() != 96468 || me->GetMapId() != 1495 ||
                me->GetInstanceId() != magnarStartInstance || !scenario || scenario->GetScenarioId() != 909 ||
                scenario->GetCurrentStep() != 3 || !HasMagnarChamberRequest())
            {
                CancelMagnarStart();
                RemoveMagnarConversation();
                return;
            }
            Player* owner = ObjectAccessor::GetPlayer(*me, magnarStartOwner);
            if (!me->IsAIEnabled || me->AI() != this || !owner || !owner->IsInWorld() || !owner->CanContact() ||
                owner->IsBeingTeleported() || owner->getClass() != CLASS_WARRIOR || owner->GetMap() != me->GetMap() ||
                owner->GetInstanceId() != magnarStartInstance || !owner->InSamePhase(me))
            {
                ScheduleMagnarStart(1000);
                return;
            }
            if (owner->GetQuestRewardStatus(39191) ||
                (owner->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && owner->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE) ||
                owner->GetQuestCancellationReceipt(39191) != magnarStartAttempt)
            {
                CancelMagnarStart();
                RemoveMagnarConversation();
                return;
            }
            if (!magnarStartPresented)
            {
                if (!(me->GetExactDistSq(4720.80f, 347.91f, -37.71f) <= 4.0f) || !StartMagnarConversation())
                {
                    if ((companion->AI() == this || companion->GetDisabledAI() == this) && magnarStartToken == token)
                        ScheduleMagnarStart(1000);
                    return;
                }
                magnarStartPresented = true;
                magnarArrivalPending = false;
                ScheduleMagnarStart(6000, true);
                return;
            }
            if (!magnarStartCueDue)
                return;
            Creature* boss = me->FindNearestCreature(96034, 60.0f, true);
            if (!boss || boss->IsDespawn() || !boss->IsAIEnabled || !boss->AI() ||
                !owner->InSamePhase(boss) || !owner->IsWithinDistInMap(boss, 100.0f))
            {
                ScheduleMagnarStart(1000);
                return;
            }
            ObjectGuid bossGuid = boss->GetGUID();
            CreatureAI* expectedBossAI = boss->AI();
            if (!expectedBossAI->GetData(MagnarStart::Accepted))
                expectedBossAI->DoAction(1);
            // DoAction may decline admission or reset its own encounter. Only
            // the actual same actor/AI's read-only receipt consumes the cue.
            if ((companion->AI() != this && companion->GetDisabledAI() != this) || magnarStartToken != token)
                return;
            if (!magnarStartAttempt || magnarStartAttempt->load())
            {
                CancelMagnarStart();
                RemoveMagnarConversation();
                return;
            }
            boss = ObjectAccessor::GetCreature(*me, bossGuid);
            if (boss && boss->IsInWorld() && boss->IsAlive() && !boss->IsDespawn() &&
                boss->AI() == expectedBossAI && boss->IsAIEnabled && expectedBossAI->GetData(MagnarStart::Accepted))
            {
                magnarStartedBoss = bossGuid;
                *token = true;
            }
            else
                ScheduleMagnarStart(1000);
        }

        void ResetMagnarOffer(Creature* boss)
        {
            // A false acceptance receipt alone is not a reset: missing,
            // replaced or disabled actors must not revoke an accepted start.
            if (!magnarStartToken || !*magnarStartToken || !boss || magnarStartedBoss != boss->GetGUID() ||
                !me->IsInWorld() || !me->IsAlive() || me->IsDespawn() || me->GetEntry() != 96468 ||
                me->GetMapId() != 1495 || (me->AI() != this && me->GetDisabledAI() != this) ||
                !boss->IsInWorld() || !boss->IsAlive() || boss->IsDespawn() || boss->GetEntry() != 96034 ||
                boss->GetMap() != me->GetMap() || boss->GetInstanceId() != me->GetInstanceId() ||
                !boss->IsAIEnabled || !boss->AI() || boss->AI()->GetData(MagnarStart::Accepted) ||
                !HasMagnarChamberRequest() || !me->GetInstanceScript() ||
                me->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) != me->GetGUID())
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetScenarioId() != 909 || scenario->GetCurrentStep() != 3)
                return;
            CancelMagnarStart();
            magnarReofferTimer = 0;
            // Remove may reenter or destroy this AI. Do not touch it afterwards.
            RemoveMagnarConversation();
        }

        void RemoveMagnarConversation()
        {
            ObjectGuid guid = magnarConversation;
            magnarConversation.Clear();
            if (guid)
                if (Conversation* conversation = ObjectAccessor::GetConversation(*me, guid))
                    conversation->Remove();
        }

        void JustDied(Unit*) override
        {
            Reset();
        }

        bool IsMagnarOfferReady() const
        {
            return magnarOfferReady && magnarOfferToken && *magnarOfferToken && !magnarArrivalPending && !magnarStartToken &&
                IsMagnarOfferContext(me->GetInstanceId()) && HasMagnarChamberRequest() && IsMagnarOfferPosition();
        }

        bool StartMagnarConversation()
        {
            Creature* companion = me;
            uint32 instanceId = magnarStartInstance;
            std::shared_ptr<bool> token = magnarStartToken;
            if (!token || *token)
                return false;
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 497, companion, nullptr, *companion))
            {
                delete conversation;
                return false;
            }
            if (companion->AI() != this || magnarStartToken != token || *token || !IsMagnarOfferContext(instanceId) || !conversation->IsInWorld())
            {
                conversation->Remove();
                return false;
            }
            magnarConversation = conversation->GetGUID();
            return true;
        }

        bool IsMagnarOfferContext(uint32 instanceId) const
        {
            if (!me->IsInWorld() || !me->IsAlive() || me->IsDespawn() || !me->GetMap() ||
                me->GetEntry() != 96468 || me->GetMapId() != 1495 || me->GetInstanceId() != instanceId ||
                !me->IsAIEnabled || me->AI() != this)
                return false;
            Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
            return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 3;
        }

        bool IsMagnarOfferPosition() const
        {
            return me->GetExactDistSq(4763.73f, 340.87f, -37.19f) <= 4.0f;
        }

        bool HasMagnarChamberRequest() const
        {
            if (!magnarChamberStarted)
                return false;
            InstanceScript* instance = me->GetInstanceScript();
            return instance && instance->GetData(1059) && instance->GetData(1060) == magnarIntroInstanceGeneration;
        }

        bool IsMagnarIntroPlayer(Player* player) const
        {
            return player && player->IsInWorld() && player->CanContact() && player->IsAlive() &&
                !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_WARRIOR &&
                player->GetMap() == me->GetMap() && player->GetInstanceId() == me->GetInstanceId() &&
                player->InSamePhase(me) && player->IsWithinDistInMap(me, 150.0f) &&
                !player->GetQuestRewardStatus(39191) &&
                (player->GetQuestStatus(39191) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(39191) == QUEST_STATUS_COMPLETE);
        }

        bool IsMagnarIntroContext() const
        {
            if (!me->IsInWorld() || !me->IsAlive() || me->IsDespawn() || !me->GetMap() ||
                me->GetEntry() != 96468 || me->GetMapId() != 1495 || !me->IsAIEnabled || me->AI() != this)
                return false;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 2;
        }

        uint32 GetData(uint32 type) const override
        {
            if (type == MagnarStart::RouteCursor)
            {
                return magnarRoutePoint >= 1 && magnarRoutePoint <= 13 &&
                    IsMagnarOfferContext(me->GetInstanceId()) && HasMagnarChamberRequest() &&
                    me->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) == me->GetGUID() &&
                    me->GetPhases().count(6999) ? magnarRoutePoint : 0;
            }
            if (type == 1061)
            {
                return IsMagnarOfferReady();
            }
            if (type != 1059 || !magnarIntroFinished || magnarChamberStarted || !IsMagnarIntroContext())
                return 0;
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance || instance->GetData(1060) != magnarIntroInstanceGeneration)
                return 0;
            Player* player = ObjectAccessor::GetPlayer(*me, magnarIntroPlayer);
            return IsMagnarIntroPlayer(player) && player->HasSceneStatus(1059, SCENE_COMPLETE);
        }

        void CancelMagnarIntro()
        {
            if (magnarIntroFinished && !magnarChamberStarted)
                magnarIntroPending = true;
            magnarIntroFinished = false;
            ObjectGuid playerGuid = magnarIntroPlayer;
            magnarIntroPlayer.Clear();
            magnarIntroTimer = 0;
            magnarIntroCasting = false;
            ++magnarIntroGeneration;
            if (me->IsInWorld())
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    player->RemoveAurasDueToSpell(189721);
        }

        void RestoreMagnarChamber()
        {
            // A new AI/actor can lose its local chamber state while the real
            // instance retains the completed scene/guard handoff. Restore only
            // that receipt; the ordinary reoffer still owns all motion/timers.
            if (magnarChamberStarted || magnarIntroPending || magnarIntroFinished ||
                magnarArrivalPending || magnarStartToken || magnarOfferToken || magnarIntroInstanceGeneration ||
                !IsMagnarOfferContext(me->GetInstanceId()))
                return;
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance || !instance->GetData(MagnarStart::ChamberDelivered) || !instance->GetData(1059) ||
                instance->GetGuidData(PillikHandoff::Companion) != me->GetGUID())
                return;
            uint32 cursor = instance->GetData(MagnarStart::RouteCursor);
            magnarIntroInstanceGeneration = instance->GetData(1060);
            magnarChamberStarted = true;
            _introDone = true;
            // Only this full GUID's real native route intent survives an AI
            // replacement. A DB respawn at the entrance starts its own route.
            if (cursor >= 1 && cursor <= 13 && me->GetExactDistSq(4884.71f, -77.9149f, 60.9946f) > 25.0f)
                magnarRoutePoint = cursor;
        }

        void StartMagnarChamberApproach()
        {
            Creature* companion = me;
            InstanceScript* instance = companion->GetInstanceScript();
            if (instance && instance->GetData(MagnarStart::ChamberDelivered) &&
                instance->GetGuidData(PillikHandoff::Companion) == companion->GetGUID() &&
                companion->GetDBTableGUIDLow() == 361306 && companion->GetPhases().count(6999) &&
                companion->GetExactDistSq(4884.71f, -77.9149f, 60.9946f) > 25.0f)
            {
                // The original point6 is the same destination as route leg12.
                // Remember its intent even if native movement must defer.
                magnarRoutePoint = 12;
                magnarRouteTimer = 0;
                magnarRouteFailureReported = false;
                uint32 epoch = magnarIntroInstanceGeneration;
                instance->SetData(MagnarStart::RouteCursor, epoch);
                if (companion->AI() != this || !companion->IsInWorld())
                    return;
                if (magnarRoutePoint != 12 || magnarIntroInstanceGeneration != epoch ||
                    !IsMagnarOfferContext(companion->GetInstanceId()) || !HasMagnarChamberRequest() ||
                    !instance->GetData(MagnarStart::ChamberDelivered) ||
                    instance->GetGuidData(PillikHandoff::Companion) != companion->GetGUID() ||
                    !companion->GetPhases().count(6999))
                    return;
            }
            // Preserve the original point6 motion, arrival and offer clocks.
            companion->GetMotionMaster()->MovePoint(6, 4763.73f, 340.87f, -37.19f);
        }

        bool GetMagnarRoutePoint(float& x, float& y, float& z) const
        {
            switch (magnarRoutePoint)
            {
                case 1: x = 4864.44f; y = -33.53f; z = 48.68f; return true;
                case 9: x = 4809.14f; y = 164.68f; z = -11.20f; return true;
                case 10: x = 4824.81f; y = 246.52f; z = -24.29f; return true;
                case 11: x = 4822.31f; y = 328.56f; z = -30.14f; return true;
                case 12: x = 4763.73f; y = 340.87f; z = -37.19f; return true;
                default: break;
            }
            if (magnarRoutePoint < 2 || magnarRoutePoint > 8)
                return false;
            auto route = sWaypointMgr->GetPathScript(9859004);
            if (!route || route->size() != 7)
                return false;
            auto const& node = (*route)[magnarRoutePoint - 2];
            x = node.x; y = node.y; z = node.z;
            return true;
        }

        void UpdateMagnarChamberRoute(uint32 diff)
        {
            if (!magnarChamberStarted || magnarArrivalPending || magnarStartToken || magnarOfferToken ||
                magnarRoutePoint > 12 || !IsMagnarOfferContext(me->GetInstanceId()) || !HasMagnarChamberRequest() ||
                me->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) != me->GetGUID() ||
                !me->GetPhases().count(6999))
                return;
            // A real DB recreation starts back at the entrance. Reuse the
            // authored corridor, without replaying its old combat/credit hooks.
            if (!magnarRoutePoint)
            {
                if (me->GetExactDistSq(4884.71f, -77.9149f, 60.9946f) > 25.0f)
                    return;
                magnarRoutePoint = 1;
            }
            if (magnarRouteTimer > diff)
            {
                magnarRouteTimer -= diff;
                return;
            }
            magnarRouteTimer = 1000;
            if (me->isInCombat() || me->HasUnitState(UNIT_STATE_NOT_MOVE) || me->IsMovementPreventedByCasting() ||
                !me->movespline->Finalized() || me->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                return;
            bool participant = false;
            for (auto const& reference : me->GetMap()->GetPlayers())
                if (IsMagnarIntroPlayer(reference.getSource()))
                    participant = true;
            if (!participant)
                return;
            float x = 0, y = 0, z = 0;
            bool known = GetMagnarRoutePoint(x, y, z);
            PathGenerator path(me);
            bool complete = known && path.CalculatePath(x, y, z, false) && (path.GetPathType() & PATHFIND_NORMAL) &&
                !(path.GetPathType() & (PATHFIND_NOPATH | PATHFIND_INCOMPLETE | PATHFIND_NOT_USING_PATH | PATHFIND_SHORTCUT | PATHFIND_SHORT));
            auto const& end = path.GetActualEndPosition();
            if (!complete || !((end.x-x)*(end.x-x) + (end.y-y)*(end.y-y) + (end.z-z)*(end.z-z) <= 1.0f))
            {
                if (!magnarRouteFailureReported)
                    TC_LOG_WARN("scripts", "Earthwarder chamber recovery: native route point %u is unavailable; retaining arrival for retry", magnarRoutePoint);
                magnarRouteFailureReported = true;
                return;
            }
            magnarRouteFailureReported = false;
            Creature* companion = me;
            uint32 epoch = magnarIntroInstanceGeneration;
            uint32 cursor = magnarRoutePoint;
            companion->GetInstanceScript()->SetData(MagnarStart::RouteCursor, epoch);
            // The virtual receipt boundary may replace this AI. Revalidate
            // before using any local state or issuing the native movement.
            if (companion->AI() != this || !companion->IsInWorld())
                return;
            if (epoch != magnarIntroInstanceGeneration || cursor != magnarRoutePoint ||
                !IsMagnarOfferContext(companion->GetInstanceId()) || !HasMagnarChamberRequest() ||
                companion->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) != companion->GetGUID() ||
                !companion->GetPhases().count(6999) || companion->isInCombat() ||
                companion->HasUnitState(UNIT_STATE_NOT_MOVE) || companion->IsMovementPreventedByCasting() ||
                !companion->movespline->Finalized() ||
                companion->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                return;
            // Motion can replace this AI. Real arrival owns advancement.
            me->GetMotionMaster()->MovePoint(8, x, y, z);
        }

        void OnMagnarRouteArrival()
        {
            float x, y, z;
            if (!magnarRoutePoint || !IsMagnarOfferContext(me->GetInstanceId()) || !HasMagnarChamberRequest() ||
                me->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) != me->GetGUID() ||
                !me->GetPhases().count(6999) || !GetMagnarRoutePoint(x, y, z) || !(me->GetExactDistSq(x, y, z) <= 4.0f))
                return;
            ++magnarRoutePoint;
            magnarRouteTimer = 0;
            // At13 the ordinary nearby reoffer still requires a ready boss,
            // then owns real point6 arrival and its full original13s offer.
            me->GetInstanceScript()->SetData(MagnarStart::RouteCursor, magnarIntroInstanceGeneration);
        }

        void UpdateMagnarReoffer(uint32 diff)
        {
            if (!magnarChamberStarted || magnarArrivalPending || magnarStartToken ||
                (magnarRoutePoint && magnarRoutePoint <= 12))
                return;
            if (magnarOfferToken && !(magnarOfferReady && *magnarOfferToken && !IsMagnarOfferPosition()))
                return;
            if (magnarReofferTimer > diff)
            {
                magnarReofferTimer -= diff;
                return;
            }
            magnarReofferTimer = 1000;
            if (!IsMagnarOfferContext(me->GetInstanceId()) || !HasMagnarChamberRequest() ||
                me->GetInstanceScript()->GetGuidData(PillikHandoff::Companion) != me->GetGUID() ||
                me->isInCombat() || me->HasUnitState(UNIT_STATE_NOT_MOVE) ||
                me->IsMovementPreventedByCasting() || !me->movespline->Finalized())
                return;
            Creature* boss = me->FindNearestCreature(96034, 60.0f, true);
            if (!boss || boss->IsDespawn() || !boss->IsAIEnabled || !boss->AI() || boss->AI()->GetData(MagnarStart::Accepted))
                return;
            bool participant = false;
            for (auto const& reference : me->GetMap()->GetPlayers())
                if (IsMagnarIntroPlayer(reference.getSource()))
                    participant = true;
            if (!participant)
                return;
            MotionMaster* motion = me->GetMotionMaster();
            if (motion->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
            {
                auto* route = dynamic_cast<WaypointMovementGenerator<Creature>*>(motion->top());
                if (!route || route->GetPathId() != 9859004 || !route->IsPathComplete())
                    return;
            }
            if (magnarOfferToken)
            {
                magnarOfferReady = false;
                magnarOfferToken.reset();
                me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }
            // Reuse real point6 arrival and its complete original13s offer.
            // MovePoint may revoke/destroy this AI; do not access it afterwards.
            motion->MovePoint(6, 4763.73f, 340.87f, -37.19f);
        }

        void UpdateMagnarIntro(uint32 diff)
        {
            if ((!magnarIntroPending && !magnarIntroFinished) || magnarChamberStarted || magnarIntroCasting)
                return;
            if (!IsMagnarIntroContext())
            {
                CancelMagnarIntro();
                magnarIntroPending = false;
                return;
            }
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance || instance->GetData(1060) != magnarIntroInstanceGeneration)
            {
                CancelMagnarIntro();
                magnarIntroPending = false;
                return;
            }
            if (magnarIntroTimer > diff)
            {
                magnarIntroTimer -= diff;
                return;
            }
            magnarIntroTimer = 500;
            Player* player = ObjectAccessor::GetPlayer(*me, magnarIntroPlayer);
            if (IsMagnarIntroPlayer(player))
            {
                if (player->HasSceneStatus(1059, SCENE_COMPLETE))
                {
                    magnarIntroPending = false;
                    magnarIntroFinished = true;
                    return;
                }
                if (player->HasAura(189721))
                    return;
            }
            if (!magnarIntroPlayer.IsEmpty())
                CancelMagnarIntro();
            magnarIntroTimer = 500;
            player = nullptr;
            for (auto const& reference : me->GetMap()->GetPlayers())
                if (IsMagnarIntroPlayer(reference.getSource()) &&
                    (!player || me->GetDistance(reference.getSource()) < me->GetDistance(player)))
                    player = reference.getSource();
            if (!player)
                return;
            if (player->HasAura(189721))
                return;
            SpellScene const* scene = sSpellMgr->GetSpellScene(1059);
            if (!scene || scene->SceneScriptPackageID != 1458 || scene->PlaybackFlags != 16 ||
                scene->CustomDuration != 0 || scene->scriptID != 0 || sSpellMgr->GetSceneTriggerEvent(1059))
            {
                if (!magnarIntroFailureReported)
                    TC_LOG_WARN("scripts", "Earthwarder intro: missing or incompatible1059/package1458 binding; chamber handoff retained for retry");
                magnarIntroFailureReported = true;
                return;
            }
            uint32 generation = magnarIntroGeneration;
            magnarIntroPlayer = player->GetGUID();
            magnarIntroCasting = true;
            SpellCastResult result = player->CastSpell(player, 189721, true);
            if (generation != magnarIntroGeneration)
            {
                if (magnarIntroPlayer.IsEmpty() && player->GetMap() == me->GetMap())
                    player->RemoveAurasDueToSpell(189721);
                return;
            }
            magnarIntroCasting = false;
            if (!magnarIntroPending || !IsMagnarIntroContext() || !IsMagnarIntroPlayer(player))
            {
                CancelMagnarIntro();
                return;
            }
            if (result != SPELL_CAST_OK || !player->HasAura(189721) || !player->HasSceneStatus(1059, SCENE_LAUNCH))
            {
                CancelMagnarIntro();
                magnarIntroTimer = 500;
                if (!magnarIntroFailureReported)
                    TC_LOG_WARN("scripts", "Earthwarder intro:189721 did not launch (result %u); chamber handoff retained for retry", uint32(result));
                magnarIntroFailureReported = true;
                return;
            }
            magnarIntroFailureReported = false;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 6000); // 57846
            events.RescheduleEvent(EVENT_2, 18000); // 32736
            events.RescheduleEvent(EVENT_3, 20000); // 13730
            events.RescheduleEvent(EVENT_4, 24000); // 9080
            if (who && who->GetEntry() == 96034)
            {
                events.RescheduleEvent(EVENT_5, 25000); // 79871
                events.RescheduleEvent(EVENT_6, 29000); // 191882
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            damage = 0;
        }

        void EnterEvadeMode() override
        {
            if (firstfighting) // start
            {
                firstfighting = false;
                Talk(3);
                _presentationFunctions.AddDelayedEvent(5000, [this]() -> void
                {
                    Talk(4);
                    me->GetMotionMaster()->MovePath(9859004, false);
                });
            }
            if (secondfighting) // demons
            {
                TryPillikHandoff();
            }
            if (thirdfighting) // electrify balls
            {
                thirdfighting = false;
                me->GetMotionMaster()->MovePoint(4, 4824.81f, 246.52f, -24.29f);
            }
            if (fourfighting)
            {
                fourfighting = false;
                Talk(11);
                me->GetMotionMaster()->MovePoint(5, 4822.31f, 328.56f, -30.14f);
            }
        }

        void KilledUnit(Unit* who) override
        {
            if (who->GetEntry() == 96443)
            {
                thirdfighting = false;
                me->GetMotionMaster()->MovePoint(4, 4824.81f, 246.52f, -24.29f);
            }
        }

        void DoAction(int32 const action) override
        {
            switch (action)
            {
                case 1:
                    fourfighting = true;
                    if (Creature* targ = me->FindNearestCreature(94813, 40.0f, true))
                        AttackStart(targ);
                    Talk(10);
                    break;
                case 2:
                {
                    InstanceScript* instance = me->GetInstanceScript();
                    if (!instance)
                        break;
                    uint32 generation = instance->GetData(1060);
                    if (magnarIntroInstanceGeneration != generation)
                    {
                        CancelMagnarIntro();
                        magnarOfferToken.reset();
                        CancelMagnarStart();
                        magnarOfferReady = false;
                        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        magnarIntroPending = false;
                        magnarChamberStarted = false;
                        magnarIntroInstanceGeneration = generation;
                        magnarRoutePoint = magnarRouteTimer = 0;
                        magnarRouteFailureReported = false;
                        RemoveMagnarConversation();
                    }
                    if (!magnarChamberStarted && IsMagnarOfferContext(me->GetInstanceId()))
                    {
                        if (!instance->GetData(1059))
                            break;
                        magnarChamberStarted = true;
                        magnarIntroPending = false;
                        magnarIntroFinished = false;
                        magnarIntroPlayer.Clear();
                        Talk(14);
                        StartMagnarChamberApproach();
                        break;
                    }
                    if (magnarIntroPending || magnarIntroFinished || magnarChamberStarted || !IsMagnarIntroContext())
                        break;
                    magnarIntroPending = true;
                    magnarIntroTimer = 0;
                    me->StopAttack();
                    me->SetReactState(REACT_PASSIVE);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    break;
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == 8)
            {
                OnMagnarRouteArrival();
                return;
            }
            if (type == POINT_MOTION_TYPE && id == 6 && magnarRoutePoint == 12)
            {
                Creature* companion = me;
                OnMagnarRouteArrival();
                if (companion->AI() != this || !companion->IsInWorld())
                    return;
            }
            if (type != POINT_MOTION_TYPE || (id != 6 && id != 7))
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                if (id == 6)
                {
                    QueuePillikHandoff();
                    Talk(18);
                    _presentationFunctions.AddDelayedEvent(5000, [this, weak = std::weak_ptr<PillikProgress>(pillikProgress)]() -> void
                    {
                        auto state = weak.lock();
                        if (!state || pillikProgress != state || !ArmPillikHandoff())
                            return;
                        if (Creature* targ = me->FindNearestCreature(94714, 85.0f, true))
                        {
                            sCreatureTextMgr->SendChat(targ, 5);
                            AttackStart(targ);
                            secondfighting = true;
                            std::list<Creature*> adds;
                            GetCreatureListWithEntryInGrid(adds, targ, 96571, 50.0f);
                            if (!adds.empty())
                                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                                {
                                    if (!(*itr)->InSamePhase(targ))
                                        continue;
                                    (*itr)->Kill((*itr));
                                    (*itr)->DespawnOrUnsummon(1000);
                                }
                        }
                    });
                    TryPillikEntry();
                }
            }
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 1:
                        DoCast(193571);
                        Talk(2);
                        if (Creature* targ = me->FindNearestCreature(96571, 30.0f, true))
                        {
                            AttackStart(targ);
                            firstfighting = true;
                        }
                        break;
                    case 2:
                        if (GameObject* go = me->FindNearestGameObject(243522, 30.0f))
                            if (InstanceScript* inst = me->GetInstanceScript())
                                inst->HandleGameObject(go->GetGUID(), true);
                        secondfighting = false;
                        _presentationFunctions.AddDelayedEvent(4000, [this]() -> void
                        {
                            Talk(19);
                            Talk(6);
                            if (Creature* targ = me->FindNearestCreature(96443, 70.0f, true))
                                AttackStart(targ);
                            thirdfighting = true;
                        });
                        break;
                    case 3:
                        Talk(19);
                        Talk(6);
                        if (Creature* targ = me->FindNearestCreature(96443, 70.0f, true))
                            AttackStart(targ);
                        thirdfighting = true;
                        break;
                    case 4:
                        Talk(7);
                        Talk(8);
                        _presentationFunctions.AddDelayedEvent(15000, [this]() -> void
                        {
                            Talk(9);
                        });
                        break;
                    case 5:
                        Talk(12);
                        _presentationFunctions.AddDelayedEvent(2000, [this]() -> void
                        {
                            Talk(13);
                        });
                        break;
                    case 6:
                    {
                        if (magnarOfferToken || magnarArrivalPending || magnarStartToken ||
                            !IsMagnarOfferContext(me->GetInstanceId()) || !HasMagnarChamberRequest() || !IsMagnarOfferPosition())
                            break;
                        me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                        magnarOfferReady = false;
                        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        magnarOfferToken = std::make_shared<bool>(false);
                        std::weak_ptr<bool> offerToken = magnarOfferToken;
                        Creature* companion = me;
                        auto* expectedAI = this;
                        uint32 instanceId = me->GetInstanceId();
                        Talk(15);
                        me->AddDelayedEvent(13000, [offerToken, companion, expectedAI, instanceId]() -> void
                        {
                            std::shared_ptr<bool> token = offerToken.lock();
                            if (!token || *token || companion->AI() != expectedAI ||
                                expectedAI->magnarOfferToken != token || !expectedAI->IsMagnarOfferContext(instanceId) || !expectedAI->HasMagnarChamberRequest())
                                return;
                            if (!expectedAI->IsMagnarOfferPosition())
                            {
                                expectedAI->magnarOfferToken.reset();
                                return;
                            }
                            *token = true;
                            expectedAI->Talk(16);
                            if (companion->AI() != expectedAI || expectedAI->magnarOfferToken != token ||
                                !expectedAI->IsMagnarOfferContext(instanceId) || !expectedAI->HasMagnarChamberRequest())
                                return;
                            if (!expectedAI->IsMagnarOfferPosition())
                            {
                                expectedAI->magnarOfferToken.reset();
                                return;
                            }
                            expectedAI->magnarOfferReady = true;
                            companion->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            companion->SetReactState(REACT_DEFENSIVE);
                            companion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        });
                        break;
                    }
                    case 7:
                    {
                        if (!magnarArrivalPending || magnarStartReached || !IsMagnarOfferContext(magnarStartInstance) ||
                            !(me->GetExactDistSq(4720.80f, 347.91f, -37.71f) <= 4.0f))
                            break;
                        me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                        magnarStartReached = true;
                        TryMagnarStart();
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            RestoreMagnarChamber();
            if (!_introDone && me->IsWithinDistInMap(who, 20.0f))
            {
                _introDone = true;
                Talk(0);
                _presentationFunctions.AddDelayedEvent(5000, [this]() -> void
                {
                    Talk(1);
                    me->GetMotionMaster()->MovePoint(1, 4864.44f, -33.53f, 48.68f);
                });
            }
        }

        void UpdateLifecycle(uint32 diff) override
        {
            // Match the native Unit queue's ALIVE/CORPSE clock even when
            // combat, stun or a suspended AI prevents ordinary UpdateAI.
            if (me->getDeathState() == ALIVE || me->getDeathState() == CORPSE)
                _presentationFunctions.Update(diff);
        }

        void UpdateAI(uint32 diff) override
        {
            Creature* companion = me;
            RestoreMagnarChamber();
            UpdateMagnarChamberRoute(diff);
            if (companion->AI() != this || !companion->IsInWorld())
                return;
            UpdateMagnarReoffer(diff);
            if (companion->AI() != this || !companion->IsInWorld())
                return;
            TryPillikEntry(diff);
            if (companion->AI() != this || !companion->IsInWorld())
                return;
            UpdatePillikHandoff(diff);
            if (companion->AI() != this || !companion->IsInWorld())
                return;
            UpdateMagnarIntro(diff);
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(57846);
                        events.RescheduleEvent(EVENT_1, 6000); // 57846
                        break;
                    case EVENT_2:
                        DoCast(32736);
                        events.RescheduleEvent(EVENT_2, 18000); // 32736
                        break;
                    case EVENT_3:
                        DoCast(13730);
                        events.RescheduleEvent(EVENT_3, 20000); // 13730
                        break;
                    case EVENT_4:
                        DoCast(9080);
                        events.RescheduleEvent(EVENT_4, 24000); // 9080
                        break;
                    case EVENT_5:
                        DoCast(79871);
                        break;
                    case EVENT_6:
                        DoCast(191882);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


void MagnarStart::Rejoin(Creature* companion, Player* player)
{
    if (!companion || companion->GetEntry() != 96468 || companion->GetMapId() != 1495)
        return;
    if (auto current = dynamic_cast<npc_hrutnir::npc_hrutnirAI*>(companion->AI()))
        current->RejoinMagnarStart(player);
    if (auto suspended = dynamic_cast<npc_hrutnir::npc_hrutnirAI*>(companion->GetDisabledAI()))
        if (suspended != companion->AI())
            suspended->RejoinMagnarStart(player);
}

void MagnarStart::ResetOffer(Creature* companion, Creature* boss)
{
    if (!companion)
        return;
    if (auto current = dynamic_cast<npc_hrutnir::npc_hrutnirAI*>(companion->AI()))
        current->ResetMagnarOffer(boss);
    else if (auto suspended = dynamic_cast<npc_hrutnir::npc_hrutnirAI*>(companion->GetDisabledAI()))
        suspended->ResetMagnarOffer(boss);
}

// 96034
class npc_magnar_icebreaker : public CreatureScript
{
public:
    npc_magnar_icebreaker() : CreatureScript("npc_magnar_icebreaker") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_magnar_icebreakerAI(creature);
    }

    struct npc_magnar_icebreakerAI : ScriptedAI
    {
        npc_magnar_icebreakerAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            wave = 1;
            count_dies = 0;
            first_phase = true;
        }

        ~npc_magnar_icebreakerAI() override
        {
            // A replaced first-phase AI must not orphan its owned reserve and
            // let a retried start create a second roster. Despawn is not death.
            if (first_phase && me->FindMap())
            {
                encounterStarted = releasingWave = true;
                pendingActivation.clear();
                activeWave.clear();
                count_dies = 0;
                summons.DespawnAll();
            }
            // An interrupted defeat must not leave its old504 alive while a
            // replacement restores the earned duel and waits for a new hit.
            ObjectGuid conversationGuid = finaleConversation;
            finaleConversation.Clear();
            if (Conversation* conversation = ObjectAccessor::GetConversation(*me, conversationGuid))
                conversation->Remove();
        }

        SummonList summons;
        std::list<ObjectGuid> trashList;

        EventMap events;
        bool stop_combat;
        bool first_phase;
        bool finaleStarted = false;
        uint8 wave;
        uint8 count_dies;
        bool releasingWave = false;
        std::set<ObjectGuid> activeWave;
        bool encounterStarted = false;
        uint32 finaleGeneration = 0;
        ObjectGuid finaleConversation;
        ObjectGuid finalePlayer;
        uint32 finaleTimer = 0;
        uint32 finaleInstanceId = 0;
        bool finalePending = false;
        bool finaleDeparting = false;
        std::array<ObjectGuid, 12> reserveGUIDs{};
        std::array<bool, 12> reserveDeaths{};
        uint32 pendingWave = 0;
        uint32 reserveRetry = 0;
        bool restoringReserve = false;
        std::set<ObjectGuid> pendingActivation;
        uint32 reserveGeneration = 0;

        void Reset() override
        {
            Creature* actor = me;
            bool restoredCombat = first_phase && MagnarCombat::Restore(actor, this, summons, reserveGUIDs, reserveDeaths);
            if (restoredCombat)
            {
                first_phase = false;
                wave = 7;
            }
            // InitializeAI also resets a replacement that has no local start
            // claim. The companion owns the acknowledged same-boss receipt.
            bool retryFirstPhase = first_phase && !releasingWave;
            ++reserveGeneration;
            uint32 generation = reserveGeneration;
            pendingActivation.clear();
            restoringReserve = false;
            ++finaleGeneration;
            ObjectGuid conversationGuid = finaleConversation;
            finaleConversation.Clear();
            finalePlayer.Clear();
            finaleTimer = 0;
            finaleInstanceId = 0;
            finalePending = false;
            finaleDeparting = false;
            if (Conversation* conversation = ObjectAccessor::GetConversation(*me, conversationGuid))
                conversation->Remove();
            if ((actor->AI() != this && actor->GetDisabledAI() != this) || reserveGeneration != generation)
                return;
            if ((finaleStarted || restoredCombat) && !first_phase)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_ATTACKABLE_1);
                me->setFaction(14);
                me->SetReactState(REACT_DEFENSIVE);
            }
            events.Reset();
            stop_combat = false;
            finaleStarted = false;
            encounterStarted = true;
            releasingWave = true;
            activeWave.clear();
            count_dies = 0;
            if (first_phase)
            {
                wave = 1;
                reserveGUIDs = {};
                reserveDeaths = {};
                pendingWave = reserveRetry = 0;
                trashList.clear();
                summons.DespawnAll();
            }
            if ((actor->AI() != this && actor->GetDisabledAI() != this) || reserveGeneration != generation)
                return;
            releasingWave = false;
            encounterStarted = false;
            if (restoredCombat && me->isInCombat() && me->getVictim())
                EnterCombat(me->getVictim());
            if (retryFirstPhase && actor->IsAIEnabled && actor->AI() == this && actor->IsInWorld() &&
                actor->IsAlive() && !actor->IsDespawn() && actor->GetEntry() == 96034 && actor->GetMapId() == 1495)
                if (InstanceScript* instance = actor->GetInstanceScript())
                    if (Creature* companion = actor->GetMap()->GetCreature(instance->GetGuidData(PillikHandoff::Companion)))
                        MagnarStart::ResetOffer(companion, actor);
        }

        uint32 GetData(uint32 type) const override
        {
            if (type != MagnarStart::Accepted || !encounterStarted || !me->IsInWorld() || !me->IsAlive() ||
                me->IsDespawn() || me->GetEntry() != 96034 || me->GetMapId() != 1495 || me->AI() != this)
                return 0;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 3;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 7000); // 191883
            events.RescheduleEvent(EVENT_2, 14000); // 191885
            events.RescheduleEvent(EVENT_3, 15000); // 128749
            if (first_phase)
                events.RescheduleEvent(EVENT_4, 33000); // event
        }

        bool IsEncounterPlayer(Player* player, uint32 step) const
        {
            if (!player || !player->IsInWorld() || !player->CanContact() || player->getClass() != CLASS_WARRIOR ||
                !me->GetMap() || player->GetMap() != me->GetMap() || player->GetInstanceId() != me->GetInstanceId() ||
                !player->InSamePhase(me) || !player->IsWithinDistInMap(me, 100.0f) || player->GetQuestRewardStatus(39191) ||
                (player->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE))
                return false;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == step;
        }

        Player* GetEncounterPlayer(Unit* attacker, uint32 step) const
        {
            Player* owner = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (IsEncounterPlayer(owner, step))
                return owner;
            Player* candidate = nullptr;
            if (!me->GetMap())
                return nullptr;
            for (auto const& reference : me->GetMap()->GetPlayers())
            {
                Player* player = reference.getSource();
                if (IsEncounterPlayer(player, step) && (!candidate || me->GetDistance(player) < me->GetDistance(candidate)))
                    candidate = player;
            }
            return candidate;
        }

        bool StartFinaleConversation(Player* player, uint32 generation)
        {
            Map* map = me->GetMap();
            uint32 instanceId = me->GetInstanceId();
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 504, me, nullptr, *me))
            {
                delete conversation;
                if (generation == finaleGeneration)
                    finaleStarted = false;
                TC_LOG_WARN("scripts", "Magnar finale: conversation 504 creation failed in instance %u; defeat remains retryable", instanceId);
                return false;
            }
            if (generation != finaleGeneration || !finaleStarted || first_phase || !me->IsInWorld() || !me->IsAlive() ||
                me->IsDespawn() || me->GetEntry() != 96034 || me->GetMapId() != 1495 ||
                me->GetMap() != map || me->GetInstanceId() != instanceId || !IsEncounterPlayer(player, 4))
            {
                conversation->Remove();
                if (generation == finaleGeneration)
                    finaleStarted = false;
                return false;
            }
            finaleConversation = conversation->GetGUID();
            return true;
        }

        uint32 GetFinaleHandoffDelay() const
        {
            auto const* lines = sConversationDataStore->GetConversationData(uint32(504));
            if (!lines || lines->size() != 4)
                return 0;
            uint32 previousStart = 0;
            for (uint32 index = 0; index < 4; ++index)
            {
                auto const& line = (*lines)[index];
                if (line.id != 1341 + index || line.idx != index || line.unk1 || line.unk2 != index % 2 ||
                    (index == 0 ? line.textId != 0 : line.textId <= previousStart))
                    return 0;
                previousStart = line.textId;
            }
            return previousStart;
        }

        void UpdateFinale(uint32 diff)
        {
            if (!finalePending && !finaleDeparting)
                return;
            if (first_phase || !finaleStarted || !me->IsInWorld() || !me->IsAlive() || me->IsDespawn() ||
                me->GetEntry() != 96034 || me->GetMapId() != 1495 || !me->GetMap())
                return;
            if (me->GetInstanceId() != finaleInstanceId)
                return;
            if (finaleTimer > diff)
            {
                finaleTimer -= diff;
                return;
            }
            finaleTimer = 0;
            if (finaleDeparting)
            {
                finaleDeparting = false;
                finaleConversation.Clear();
                finalePlayer.Clear();
                me->DespawnOrUnsummon();
                return;
            }
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 5)
            {
                finalePending = false;
                finaleDeparting = true;
                finaleTimer = 6000;
                return;
            }
            Player* player = ObjectAccessor::GetPlayer(*me, finalePlayer);
            if (!IsEncounterPlayer(player, 4))
                player = GetEncounterPlayer(nullptr, 4);
            if (!player)
                return;
            uint32 generation = finaleGeneration;
            Map* map = me->GetMap();
            finalePending = false;
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 45784);
            if (generation != finaleGeneration || !finaleStarted)
                return;
            if (first_phase || !me->IsInWorld() ||
                !me->IsAlive() || me->IsDespawn() || me->GetEntry() != 96034 || me->GetMapId() != 1495 ||
                me->GetMap() != map || me->GetInstanceId() != finaleInstanceId)
            {
                finalePending = true;
                return;
            }
            scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 5)
            {
                finaleDeparting = true;
                finaleTimer = 6000;
            }
            else
                finalePending = true;
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType) override
        {
            if (!damage || !me->IsAlive() || me->GetEntry() != 96034 || me->GetMapId() != 1495)
                return;
            if (finaleStarted)
            {
                damage = 0;
                return;
            }
            if (damage < me->GetHealth(attacker))
                return;
            damage = 0;
            if (first_phase)
                return;
            Player* player = GetEncounterPlayer(attacker, 4);
            if (!player)
                return;
            uint32 delay = GetFinaleHandoffDelay();
            if (!delay)
            {
                TC_LOG_WARN("scripts", "Magnar finale: invalid conversation 504 handoff data in instance %u; defeat remains retryable", me->GetInstanceId());
                return;
            }
            finaleStarted = true;
            if (!StartFinaleConversation(player, finaleGeneration))
                return;
            stop_combat = true;
            me->RemoveAura(128749);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->setFaction(35);
            finalePlayer = player->GetGUID();
            finaleTimer = delay;
            finaleInstanceId = me->GetInstanceId();
            finalePending = true;
            uint32 generation = finaleGeneration;
            me->StopAttack(true, true);
            if (generation == finaleGeneration && finaleStarted)
            {
                me->GetMotionMaster()->MoveIdle();
                me->RemoveAura(191883);
            }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
            trashList.push_back(summon->GetGUID());
            summon->SetReactState(REACT_PASSIVE);
            summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
        }

        void SummonedCreatureDies(Creature* summon, Unit* killer) override
        {
            if (!summon || summon->IsAlive())
                return;
            pendingActivation.erase(summon->GetGUID());
            for (uint32 slot = 0; slot < reserveGUIDs.size(); ++slot)
                if (reserveGUIDs[slot] == summon->GetGUID())
                    reserveDeaths[slot] = true;
            summons.Despawn(summon);
            trashList.remove(summon->GetGUID());
            if (!activeWave.erase(summon->GetGUID()) || !count_dies)
                return;
            --count_dies;
            AdvanceWaveIfReady(killer);
        }

        void AdvanceWaveIfReady(Unit* killer = nullptr)
        {
            if (!releasingWave && !restoringReserve && !count_dies && first_phase && stop_combat && wave > 1 && wave <= 6)
            {
                if (wave < 6)
                    LaunchEventWave(wave++);
                else
                {
                    if (me->GetEntry() != 96034 || me->GetMapId() != 1495)
                        return;
                    Player* player = GetEncounterPlayer(killer, 3);
                    if (!player)
                        return;
                    ++wave;
                    first_phase = false;
                    stop_combat = false;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    me->SetReactState(REACT_DEFENSIVE);
                    MagnarCombat::Remember(me, this, reserveGUIDs, reserveDeaths);
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46427);
                }
            }
        }

        void DoAction(int32 const action) override
        {
            if (action != 1 || encounterStarted || !me->IsInWorld() || !me->IsAlive() || me->IsDespawn() ||
                me->GetEntry() != 96034 || me->GetMapId() != 1495 || !GetEncounterPlayer(nullptr, 3))
                return;
            Creature* companion = me->FindNearestCreature(96468, 70.0f, true);
            if (!companion || !companion->IsAlive() || !companion->IsAIEnabled || !companion->AI())
                return;
            encounterStarted = true;
            trashList.clear();
            uint32 generation = reserveGeneration;
            SummonTrash();
            if (generation != reserveGeneration || !encounterStarted)
                return;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_DEFENSIVE);
            me->setFaction(14);
            me->GetMotionMaster()->MoveJump(4714.82f, 349.11f, -37.77f, me->GetOrientation(), 5.0f, 5.0f);
            AttackStart(companion);
            EnterCombat(companion);
        }

        void UpdateAI(uint32 diff) override
        {
            uint32 generation = reserveGeneration;
            UpdateFinale(diff);
            if (generation != reserveGeneration || finaleStarted)
                return;
            UpdateReserve(diff);
            if (generation != reserveGeneration)
                return;
            if (wave == 6)
                AdvanceWaveIfReady();

            if (!UpdateVictim())
                return;

            if (stop_combat)
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING) || me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(191883);
                        events.RescheduleEvent(EVENT_1, 15000); // 191883
                        break;
                    case EVENT_2:
                        DoCast(191885);
                        events.RescheduleEvent(EVENT_2, 15000); // 191885
                        break;
                    case EVENT_3:
                        DoCast(128749);
                        events.RescheduleEvent(EVENT_3, 15000); // 128749
                        break;
                    case EVENT_4:
                        me->RemoveAura(128749);
                        me->StopAttack();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        stop_combat = true;
                        if (Creature* targ = me->FindNearestCreature(96468, 100.0f, true))
                            targ->AttackStop();
                        me->GetMotionMaster()->MoveJump(4686.67f, 354.12f, -27.10f, me->GetOrientation(), 5.0f, 5.0f);
                        LaunchEventWave(wave++);
                        if (generation != reserveGeneration)
                            return;
                        events.RescheduleEvent(EVENT_5, 1000);
                        break;
                    case EVENT_5:
                        me->CreateConversation(498);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        Creature* GetWaveCompanion() const
        {
            Creature* companion = me->FindNearestCreature(96468, 100.0f, true);
            return companion && companion->IsInWorld() && companion->IsAlive() && !companion->IsDespawn() &&
                companion->GetMap() == me->GetMap() && companion->GetInstanceId() == me->GetInstanceId() &&
                companion->IsAIEnabled && companion->AI() ? companion : nullptr;
        }

        bool IsWaveActorReady(Creature* actor) const
        {
            return actor && actor->IsInWorld() && actor->IsAlive() && !actor->IsDespawn() &&
                actor->GetMap() == me->GetMap() && actor->GetInstanceId() == me->GetInstanceId() &&
                actor->IsAIEnabled && actor->AI();
        }

        void ActivateWaveActor(ObjectGuid guid)
        {
            if (!pendingActivation.count(guid) || !activeWave.count(guid))
                return;
            Map* map = me->GetMap();
            Creature* actor = map->GetCreature(guid);
            Creature* companion = GetWaveCompanion();
            if (!IsWaveActorReady(actor) || !companion)
                return;
            uint32 generation = reserveGeneration;
            // Claim before callbacks. A missing AI retains this exact actor,
            // rather than consuming a release or creating another opponent.
            pendingActivation.erase(guid);
            actor->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            if (generation != reserveGeneration || me->GetMap() != map || !activeWave.count(guid))
                return;
            actor = map->GetCreature(guid);
            if (!IsWaveActorReady(actor) || GetWaveCompanion() != companion)
            {
                pendingActivation.insert(guid);
                return;
            }
            actor->SetReactState(REACT_DEFENSIVE);
            actor->RemoveAura(191821);
            if (generation != reserveGeneration || me->GetMap() != map || !activeWave.count(guid))
                return;
            actor = map->GetCreature(guid);
            if (!IsWaveActorReady(actor) || GetWaveCompanion() != companion)
            {
                pendingActivation.insert(guid);
                return;
            }
            actor->AI()->AttackStart(companion);
            if (generation != reserveGeneration || me->GetMap() != map || !activeWave.count(guid))
                return;
            actor = map->GetCreature(guid);
            if (!IsWaveActorReady(actor) || GetWaveCompanion() != companion)
            {
                pendingActivation.insert(guid);
                return;
            }
            companion->AI()->AttackStart(actor);
        }

        void LaunchEventWave(uint32 wave)
        {
            uint8 count = 0;
            bool end = false;
            switch (wave)
            {
                case 1:
                    count = 2;
                    break;
                case 2:
                    count = 2;
                    break;
                case 3:
                    count = 1;
                    break;
                case 4:
                    count = 2;
                    break;
                case 5:
                    count = 3;
                    end = true;
                    break;
                default:
                    return;
            }

            // Do not consume a partial batch: retain the authored wave request
            // until its actual living reserve can supply all opponents.
            pendingWave = wave;
            count_dies = count;
            if (!GetWaveCompanion())
                return;
            std::list<ObjectGuid> selected;
            for (auto guid : trashList)
                if (Creature* actor = me->GetMap()->GetCreature(guid))
                    if (actor->IsAlive() && actor->IsInWorld())
                    {
                        // Preserve the original roster order: do not skip a
                        // temporarily disabled selected AI for a later entry.
                        if (!IsWaveActorReady(actor))
                            return;
                        selected.push_back(guid);
                        if (selected.size() == count)
                            break;
                    }
            if (selected.size() < count)
                return;
            pendingWave = 0;
            count_dies = count;
            activeWave.clear();
            pendingActivation.clear();
            releasingWave = true;
            auto pendingTrash = trashList;
            uint32 generation = reserveGeneration;
            // Register the complete selected roster before any actor callback.
            // An actor disappearing during another activation still belongs to
            // this wave and can be replaced without inventing a death.
            for (auto guid : selected)
            {
                activeWave.insert(guid);
                pendingActivation.insert(guid);
            }
            for (auto guid : selected)
            {
                ActivateWaveActor(guid);
                if (generation != reserveGeneration)
                    return;
            }

            if (end)
            {
                for (auto trashGuid : pendingTrash)
                {
                    if (Creature* summon = me->GetMap()->GetCreature(trashGuid))
                    {
                        if (!summon->IsAlive())
                            continue;
                        summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        if (generation != reserveGeneration)
                            return;
                        summon = me->GetMap()->GetCreature(trashGuid);
                        if (!summon || !summon->IsInWorld() || !summon->IsAlive())
                            continue;
                        if (Creature* targ = summon->FindNearestCreature(64367, 30.0f, true))
                            summon->GetMotionMaster()->MoveJump(targ->GetPositionX(), targ->GetPositionY(), targ->GetPositionZ(), summon->GetOrientation(), 5.0f, 5.0f);
                        summon->CastSpell(summon, 191032);
                        if (generation != reserveGeneration)
                            return;
                    }
                }
                trashList.clear();
                me->SetHealth(me->CountPctFromMaxHealth(80));
            }
            releasingWave = false;
            AdvanceWaveIfReady();
        }

        void SummonTrash()
        {
            // Retain each original entry/pose and full GUID. Missing is not
            // dead: replace only a missing, uncredited owned reserve slot.
            if (restoringReserve)
                return;
            uint32 generation = reserveGeneration;
            restoringReserve = true;
            for (uint32 slot = 0; slot < reserveGUIDs.size(); ++slot)
            {
                if (reserveDeaths[slot])
                    continue;
                ObjectGuid old = reserveGUIDs[slot];
                // The fifth release intentionally sends the remaining reserve
                // away. Only a still-counted opponent can need recovery then.
                if (wave == 6 && !pendingWave && !activeWave.count(old))
                    continue;
                if (Creature* actor = me->GetMap()->GetCreature(old))
                    if (actor->IsInWorld())
                        continue;
                uint32 entry = slot >= 10 ? 94813 : slot == 4 || slot == 9 ? 94707 : 97123;
                Creature* actor = me->SummonCreature(entry, trashPos[slot]);
                if (generation != reserveGeneration)
                    return;
                if (actor)
                {
                    reserveGUIDs[slot] = actor->GetGUID();
                    trashList.remove(old);
                    if (activeWave.erase(old))
                    {
                        activeWave.insert(actor->GetGUID());
                        pendingActivation.erase(old);
                        pendingActivation.insert(actor->GetGUID());
                        ActivateWaveActor(actor->GetGUID());
                        if (generation != reserveGeneration)
                            return;
                        if (wave == 6)
                        {
                            actor = me->GetMap()->GetCreature(reserveGUIDs[slot]);
                            if (!actor || !actor->IsInWorld() || !actor->IsAlive())
                                continue;
                            if (Creature* marker = actor->FindNearestCreature(64367, 30.0f, true))
                                actor->GetMotionMaster()->MoveJump(marker->GetPositionX(), marker->GetPositionY(), marker->GetPositionZ(), actor->GetOrientation(), 5.0f, 5.0f);
                            actor->CastSpell(actor, 191032);
                        }
                    }
                }
            }
            restoringReserve = false;
            AdvanceWaveIfReady();
        }

        void UpdateReserve(uint32 diff)
        {
            if (!encounterStarted || !first_phase || releasingWave || restoringReserve ||
                !me->IsInWorld() || !me->IsAlive() || me->IsDespawn() ||
                me->GetEntry() != 96034 || me->GetMapId() != 1495 || !GetEncounterPlayer(nullptr, 3))
                return;
            if (reserveRetry > diff) { reserveRetry -= diff; return; }
            reserveRetry = 1000;
            uint32 generation = reserveGeneration;
            // A disabled parent AI can miss the native summon death callback.
            // Recover only from that same real dead object, never from absence.
            for (uint32 slot = 0; slot < reserveGUIDs.size(); ++slot)
                if (!reserveDeaths[slot])
                    if (Creature* actor = me->GetMap()->GetCreature(reserveGUIDs[slot]))
                        if (!actor->IsAlive())
                        {
                            SummonedCreatureDies(actor, nullptr);
                            if (generation != reserveGeneration)
                                return;
                        }
            if (!first_phase || !GetEncounterPlayer(nullptr, 3))
                return;
            SummonTrash();
            if (generation != reserveGeneration)
                return;
            if (pendingWave)
                LaunchEventWave(pendingWave);
            if (generation != reserveGeneration)
                return;
            auto pending = pendingActivation;
            for (auto guid : pending)
            {
                ActivateWaveActor(guid);
                if (generation != reserveGeneration)
                    return;
            }
        }

    };
};

namespace EarthwarderAcquisition
{
    bool CanTake(Player* player, GameObject* weapon)
    {
        if (!player || !weapon || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_WARRIOR ||
            !weapon->IsInWorld() || !weapon->isSpawned() || weapon->GetEntry() != 248831 || weapon->GetMapId() != 1495 ||
            !player->GetMap() || player->GetMap() != weapon->GetMap() || player->GetInstanceId() != weapon->GetInstanceId() ||
            !player->InSamePhase(weapon) || !player->canSeeOrDetect(weapon) || !player->IsWithinLOSInMap(weapon) ||
            !player->IsWithinDistInMap(weapon, weapon->GetInteractionDistance()) ||
            weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) ||
            player->GetQuestRewardStatus(39191) ||
            (player->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(weapon->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 909 && scenario->GetCurrentStep() == 5;
    }
}

// 248831
class go_art_earthwarder : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        bool GossipHello(Player* player, bool) override
        {
            return !EarthwarderAcquisition::CanTake(player, go) || go->getLootState() == GO_JUST_DEACTIVATED;
        }
    };

public:
    go_art_earthwarder() : GameObjectScript("go_art_earthwarder") {}

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!EarthwarderAcquisition::CanTake(player, go) || go->getLootState() == GO_JUST_DEACTIVATED)
            return true;
        if (player->HasItemCount(128289, 1, true))
        {
            go->SetLootState(GO_JUST_DEACTIVATED, player);
            return true;
        }
        return false;
    }
    GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }

    void OnLootStateChanged(GameObject* go, uint32 state, Unit* target) override
    {
        if (target && go && state == GO_JUST_DEACTIVATED && go->getLootState() == GO_JUST_DEACTIVATED &&
            EarthwarderAcquisition::CanTake(target->ToPlayer(), go) && target->ToPlayer()->HasItemCount(128289, 1, true))
        {
            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                target->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 45785);
                target->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_COMPLETE_SCENARIO, 909, 1);
                if (Creature* trigger = go->FindNearestCreature(94854, 10.0f))
                    trigger->DespawnOrUnsummon(1000);

                target->CastSpell(target, 205340, true);

                target->ToPlayer()->KilledMonsterCredit(94851);


            }
        }
    }
};

#include "../../../../Legion/ClassHalls/warrior_pickup_return_api.h"

namespace EarthwarderPresentation
{
    bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
    {
        if (!scene || scene->MiscValue != 1215 || scene->SceneScriptPackageID != 1575)
            return false;
        if (trigger != "complete" || !player || !player->HasSceneStatus(1215, SCENE_COMPLETE) ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
            player->getClass() != CLASS_WARRIOR || player->GetMapId() != 1495 || player->GetQuestRewardStatus(39191) ||
            (player->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE) ||
            !player->GetItemByEntry(128289))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 909 || scenario->GetCurrentStep() < 5 || scenario->GetCurrentStep() > 6)
            return true;
        if (WarriorPickupReturn::Complete(player, 1215))
            return true;
        SpellCastResult result = player->CastSpell(player, 208562, true);
        if (result != SPELL_CAST_OK)
        {
            TC_LOG_WARN("scripts", "Earthwarder pickup: native equip 208562 failed with result %u for player %u; existing items retained", uint32(result), player->GetGUIDLow());
            return true;
        }
        Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        if (!weapon || weapon->GetEntry() != 128289 || !child || child->GetEntry() != 128288 || weapon->GetChildItem() != child->GetGUID())
            TC_LOG_WARN("scripts", "Earthwarder pickup: native equip 208562 accepted but parent/child is not equipped for player %u; retain native inventory errors", player->GetGUIDLow());
        return true;
    }
}

class scene_earthwarder_pickup : public SceneTriggerScript
{
public:
    scene_earthwarder_pickup() : SceneTriggerScript("scene_earthwarder_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return EarthwarderPresentation::Complete(player, scene, trigger);
    }
};

void AddSC_EarthWarder()
{
    new npc_hrutnir();
    new npc_magnar_icebreaker();
    new go_art_earthwarder();
    new scene_earthwarder_pickup();
};
