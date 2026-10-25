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
#include <memory>
#include <set>

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
        if (HruthnirGossip::CanStart(player, creature))
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

        void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId != 18743 || gossipListId != 0 || !HruthnirGossip::CanStart(player, me))
                return;
            GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
            GossipMenuItem const* item = menu.GetItem(0);
            if (menu.GetMenuId() != menuId || !item || item->OptionNpc != GossipOptionNpc::None ||
                item->Sender != 0 || item->Action != 0 || item->IsCoded || item->BoxMoney != 0)
                return;
            magnarOfferToken.reset();
            magnarStartInstance = me->GetInstanceId();
            magnarStartToken = std::make_shared<bool>(false);
            magnarArrivalPending = true;
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            me->GetMotionMaster()->MovePoint(7, 4720.80f, 347.91f, -37.71f);
        }

        void Reset() override
        {
            CancelMagnarIntro();
            events.Reset();
            magnarArrivalPending = false;
            magnarStartToken.reset();
            magnarStartInstance = 0;
            magnarOfferToken.reset();
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
                me->AddDelayedEvent(5000, [this]() -> void
                {
                    Talk(4);
                    me->GetMotionMaster()->MovePath(9859004, false);
                });
            }
            if (secondfighting) // demons
            {
                // secondfighting = false;
                Talk(5);
                me->GetMotionMaster()->MovePoint(2, 4809.14f, 164.68f, -11.20f);
                if (Player* pl = me->FindNearestPlayer(100.0f))
                    pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46289);
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
                        magnarStartToken.reset();
                        magnarArrivalPending = false;
                        magnarStartInstance = 0;
                        magnarIntroPending = false;
                        magnarChamberStarted = false;
                        magnarIntroInstanceGeneration = generation;
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
                        me->GetMotionMaster()->MovePoint(6, 4763.73f, 340.87f, -37.19f);
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
            if (type != POINT_MOTION_TYPE || (id != 6 && id != 7))
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                if (id == 6)
                {
                    Talk(18);
                    me->AddDelayedEvent(5000, [this]() -> void
                    {
                        if (Creature* targ = me->FindNearestCreature(94714, 85.0f, true))
                        {
                            targ->AI()->Talk(5);
                            AttackStart(targ);
                            secondfighting = true;
                            std::list<Creature*> adds;
                            GetCreatureListWithEntryInGrid(adds, targ, 96571, 50.0f);
                            if (!adds.empty())
                                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                                {
                                    (*itr)->Kill((*itr));
                                    (*itr)->DespawnOrUnsummon(1000);
                                }
                        }
                    });
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 46288);
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
                        me->AddDelayedEvent(4000, [this]() -> void
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
                        me->AddDelayedEvent(15000, [this]() -> void
                        {
                            Talk(9);
                        });
                        break;
                    case 5:
                        Talk(12);
                        me->AddDelayedEvent(2000, [this]() -> void
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
                            companion->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                            companion->SetReactState(REACT_DEFENSIVE);
                            companion->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        });
                        break;
                    }
                    case 7:
                    {
                        if (!magnarArrivalPending || !IsMagnarOfferContext(magnarStartInstance) ||
                            !(me->GetExactDistSq(4720.80f, 347.91f, -37.71f) <= 4.0f))
                            break;
                        me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                        magnarArrivalPending = false;
                        std::weak_ptr<bool> startToken = magnarStartToken;
                        Creature* companion = me;
                        uint32 instanceId = magnarStartInstance;
                        auto* expectedAI = this;
                        me->CreateConversation(497);
                        me->AddDelayedEvent(6000, [startToken, companion, instanceId, expectedAI]() -> void
                        {
                            std::shared_ptr<bool> token = startToken.lock();
                            if (!token || *token)
                                return;
                            *token = true;
                            if (!companion->IsInWorld() || !companion->IsAlive() || companion->IsDespawn() ||
                                companion->GetEntry() != 96468 || companion->GetMapId() != 1495 || companion->GetInstanceId() != instanceId)
                                return;
                            if (!companion->IsAIEnabled || companion->AI() != expectedAI)
                                return;
                            if (!expectedAI->IsMagnarOfferContext(instanceId))
                                return;
                            if (Creature* boss = companion->FindNearestCreature(96034, 60.0f, true))
                                if (boss->IsAIEnabled && boss->AI() && !boss->IsDespawn())
                                    boss->AI()->DoAction(1);
                        });
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

            if (!_introDone && me->IsWithinDistInMap(who, 20.0f))
            {
                _introDone = true;
                Talk(0);
                me->AddDelayedEvent(5000, [this]() -> void
                {
                    Talk(1);
                    me->GetMotionMaster()->MovePoint(1, 4864.44f, -33.53f, 48.68f);
                });
            }
        }

        void UpdateAI(uint32 diff) override
        {
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

        void Reset() override
        {
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
            if (finaleStarted && !first_phase)
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
                trashList.clear();
                summons.DespawnAll();
            }
            releasingWave = false;
            encounterStarted = false;
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
            summons.Despawn(summon);
            trashList.remove(summon->GetGUID());
            if (!activeWave.erase(summon->GetGUID()) || !count_dies)
                return;
            --count_dies;
            AdvanceWaveIfReady(killer);
        }

        void AdvanceWaveIfReady(Unit* killer = nullptr)
        {
            if (!releasingWave && !count_dies && first_phase && stop_combat && wave > 1 && wave <= 6)
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
            SummonTrash();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_DEFENSIVE);
            me->setFaction(14);
            me->GetMotionMaster()->MoveJump(4714.82f, 349.11f, -37.77f, me->GetOrientation(), 5.0f, 5.0f);
            AttackStart(companion);
            EnterCombat(companion);
        }

        void UpdateAI(uint32 diff) override
        {
            UpdateFinale(diff);
            if (finaleStarted)
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
                        events.RescheduleEvent(EVENT_5, 1000);
                        break;
                    case EVENT_5:
                        me->CreateConversation(498);
                        break;
                }
            }
            DoMeleeAttackIfReady();
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

            count_dies = count;
            activeWave.clear();
            releasingWave = true;
            auto pendingTrash = trashList;

            for (auto trashGuid : pendingTrash)
            {
                if (!count)
                    break;
                if (Creature* summon = me->GetMap()->GetCreature(trashGuid))
                {
                    if (!summon->IsAlive())
                        continue;
                    activeWave.insert(trashGuid);
                    summon->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    summon->SetReactState(REACT_DEFENSIVE);
                    summon->RemoveAura(191821);
                    if (Creature* targ = summon->FindNearestCreature(96468, 100.0f, true))
                    {
                        summon->AI()->AttackStart(targ);
                        targ->AI()->AttackStart(summon);
                    }
                    count--;
                }
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
                        if (Creature* targ = summon->FindNearestCreature(64367, 30.0f, true))
                            summon->GetMotionMaster()->MoveJump(targ->GetPositionX(), targ->GetPositionY(), targ->GetPositionZ(), summon->GetOrientation(), 5.0f, 5.0f);
                        summon->CastSpell(summon, 191032);
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
            for (uint8 i = 0; i < 4; ++i)
                me->SummonCreature(97123, trashPos[i]);
            me->SummonCreature(94707, trashPos[4]);
            for (uint8 i = 5; i < 9; ++i)
                me->SummonCreature(97123, trashPos[i]);
            me->SummonCreature(94707, trashPos[9]);

            me->SummonCreature(94813, trashPos[10]);
            me->SummonCreature(94813, trashPos[11]);
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

namespace EarthwarderReturn
{
    bool CanReturn(Player* player, uint32 instanceId)
    {
        if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_WARRIOR || !player->GetMap() ||
            player->GetMapId() != 1495 || player->GetInstanceId() != instanceId || player->GetQuestRewardStatus(39191) ||
            (player->GetQuestStatus(39191) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(39191) != QUEST_STATUS_COMPLETE) ||
            !player->HasItemCount(128289, 1, true))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
        return scenario && scenario->GetScenarioId() == 909 && scenario->IsCompleted(false);
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

                Player* player = target->ToPlayer();
                uint32 instanceId = go->GetInstanceId();
                target->AddDelayedEvent(12000, [player, instanceId]() -> void
                {
                    if (EarthwarderReturn::CanReturn(player, instanceId))
                        player->CastSpell(player, 192085);
                });
            }
        }
    }
};

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
