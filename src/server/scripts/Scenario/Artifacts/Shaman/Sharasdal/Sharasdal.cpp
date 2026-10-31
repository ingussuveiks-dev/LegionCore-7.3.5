/*
    Shaman: Sharasdal
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Spell.h"
#include "SpellScript.h"
#include "Timer.h"
#include "WaypointManager.h"
#include "WaypointMovementGenerator.h"

namespace SharasdalCompanion
{
    void Recover(Creature* actor, bool& pendingHeal, bool& waitHeal)
    {
        if (!pendingHeal)
            return;
        pendingHeal = false;
        if (!waitHeal || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() || actor->GetHealthPct() < 40)
            return;
        waitHeal = false;
        actor->ClearUnitState(UNIT_STATE_STUNNED);
        actor->RemoveAura(208093);
    }
}

namespace SharasdalGrash
{
    bool Eligible(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->isGameMaster() || player->IsBeingTeleported() || player->getClass() != CLASS_SHAMAN ||
            !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() || actor->GetEntry() != 105298 ||
            actor->GetMapId() != 1600 || !player->GetMap() || player->GetMap() != actor->GetMap() ||
            player->GetInstanceId() != actor->GetInstanceId() || !player->InSamePhase(actor) ||
            !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, 100.0f) || player->GetQuestRewardStatus(40341) ||
            (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1066 && scenario->GetCurrentStep() <= 4;
    }

    bool Receipt(Scenario* scenario, uint32 tree)
    {
        if (!scenario || scenario->GetScenarioId() != 1066)
            return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(tree);
        return found != progress->end() && found->second.Counter;
    }

    uint32 Menu(uint32 step)
    {
        switch (step)
        {
            case 0: case 1: return 19509;
            case 2: return 19483;
            case 4: return 19467;
            default: return 0;
        }
    }

    bool Recruitable(Scenario* scenario)
    {
        if (!Receipt(scenario, 47121) || !Receipt(scenario, 47180)) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(47161);
        return found != progress->end() && found->second.Counter >= 2;
    }
}

namespace SharasdalWall
{
    Scenario* World(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 105298 || actor->GetMapId() != 1600 || actor->GetDBTableGUIDLow() != 362066)
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
    }

    uint32 Path(Creature* actor)
    {
        MovementGenerator* movement = actor->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
        return movement && movement->GetMovementGeneratorType() == WAYPOINT_MOTION_TYPE ?
            static_cast<WaypointMovementGenerator<Creature>*>(movement)->GetPathId() : 0;
    }

    uint64 Wall(uint32 path, uint32 node)
    {
        if (path == 10993808 && node == 3) return 134291;
        if (path == 10993810 && node == 0) return 134287;
        if (path == 10993810 && node == 3) return 134290;
        return 0;
    }
}

namespace SharasdalRescue
{
    Scenario* World(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 104853 || actor->GetMapId() != 1600 || actor->GetPositionZ() >= 795.0f)
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1066 ? scenario : nullptr;
    }

    bool Eligible(Player* player, Creature* actor)
    {
        if (!World(actor) || !player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->isGameMaster() || player->IsBeingTeleported() || player->getClass() != CLASS_SHAMAN ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, INTERACTION_DISTANCE) || player->GetQuestRewardStatus(40341) ||
            (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))
            return false;
        return true;
    }
}

Position const sumpos[3] =
{
    {-44.86f, 802.57f, 797.15f, 0.0f},
    {-44.72f, 796.29f, 797.30f, 0.0f},
    {-44.97f, 808.61f, 797.33f, 0.0f}
};


// 102826
class npc_erunak_stonespeaker : public CreatureScript
{
public:
    npc_erunak_stonespeaker() : CreatureScript("npc_erunak_stonespeaker") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_erunak_stonespeakerAI(creature);
    }

    struct npc_erunak_stonespeakerAI : ScriptedAI
    {
        npc_erunak_stonespeakerAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            wait_heal = false;
            firstbattle = false;
        }

        EventMap events;
        bool _introDone, wait_heal, firstbattle;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;

        void Reset() override
        {
            _pendingHealRecovery = false;
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 4000); // 209912
            events.RescheduleEvent(EVENT_2, 34000); // 209913
            events.RescheduleEvent(EVENT_3, 29000); // 79890
            events.RescheduleEvent(EVENT_4, 10000); // 162074
        }

        void EnterEvadeMode() override
        {
            // Retain this guide's scripted movement. Evading is not a Brute
            // victory; the instance owns the real two-death continuation.
        }

        void OnHealApplied(Unit* /*who*/, uint32 /*amount*/, int32 /*gain*/) override
        {
            if (wait_heal)
                _pendingHealRecovery = true;
        }

        void UpdateLifecycle(uint32) override
        {
            SharasdalCompanion::Recover(me, _pendingHealRecovery, wait_heal);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(208093, me);
                    Talk(7);
                    Talk(8);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                InstanceScript *script = me->GetInstanceScript();

                if (!script)
                    return;

                if (script->getScenarionStep() <= 2)
                {
                    switch (id)
                    {
                        case 2:
                            Talk(3);
                            break;
                        case 5:
                            Talk(4);
                            break;
                        case 8:
                        {
                            if (Creature* adel = me->FindNearestCreature(104853, 70.0f, true))
                            {
                                adel->AI()->Talk(0);
                                adel->AddDelayedEvent(5000, [adel]() -> void
                                {
                                    adel->AI()->Talk(1);
                                });
                            }
                        }
                        break;
                        case 9:
                            me->CreateConversation(1707);
                            break;
                    }
                }
                else if (script->getScenarionStep() == 3)
                {
                    if (id == 5)
                    {
                        DoCast(83810);
                        if (Player* pl = me->FindNearestPlayer(100.0f))
                            pl->CastSpell(pl, 208580);
                    }
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetPositionZ() >= 795.0f && !_introDone && me->IsWithinDistInMap(who, 20.0f))
            {
                _introDone = true;
                if (Creature* garsh = me->FindNearestCreature(105298, 30.0f, true))
                {
                    garsh->AI()->Talk(8);
                    garsh->GetMotionMaster()->MovePath(10993811, false);
                }
                me->GetMotionMaster()->MovePath(10993811, false, irand(-5, 5), irand(-5, 5));
                if (Creature* adel = me->FindNearestCreature(104853, 30.0f, true))
                {
                    adel->AI()->Talk(8);
                    adel->GetMotionMaster()->MovePath(10993811, false, irand(-5, 5), irand(-5, 5));
                }
                me->AddDelayedEvent(5000, [this]() -> void
                {
                    me->CreateConversation(1868);
                });

            }
            else if (!_introDone && me->IsWithinDistInMap(who, 20.0f))
            {
                _introDone = true;
                who->ToPlayer()->KilledMonsterCredit(105805);
                me->AddDelayedEvent(3000, [this]() -> void
                {
                    Talk(0);
                    if (Creature* targ = me->FindNearestCreature(102792, 70.0f, true))
                        me->AI()->AttackStart(targ);
                });
                firstbattle = true;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED)) // need for this
                return;

            _functions.Update(diff);

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(209912);
                        events.RescheduleEvent(EVENT_1, 4000); // 209912
                        break;
                    case EVENT_2:
                        DoCast(209913);
                        events.RescheduleEvent(EVENT_2, 34000); // 209913
                        break;
                    case EVENT_3:
                        DoCast(79890);
                        events.RescheduleEvent(EVENT_3, 29000); // 79890
                        break;
                    case EVENT_4:
                        DoCast(162074);
                        events.RescheduleEvent(EVENT_4, 10000); // 162074
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 105298
class npc_grash : public CreatureScript
{
public:
    npc_grash() : CreatureScript("npc_grash") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (!SharasdalGrash::Eligible(player, creature) ||
            !creature->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) ||
            creature->HasUnitState(UNIT_STATE_STUNNED) || !player->IsWithinDistInMap(creature, INTERACTION_DISTANCE))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(creature->GetInstanceId());
        uint32 step = scenario->GetCurrentStep();
        uint32 menu = SharasdalGrash::Menu(step);
        if (!menu || !creature->AI() || creature->AI()->GetData(menu) ||
            (step <= 1 && !SharasdalGrash::Recruitable(scenario)))
            return true;
        player->PlayerTalkClass->GetGossipMenu().SetMenuId(menu);
        player->ADD_GOSSIP_ITEM_DB(menu, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        player->SEND_GOSSIP_MENU(step <= 1 ? 28831 : step == 2 ? 28782 : 28747, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* /*player*/, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) override
    {
        // The AI validates the actual packet menu/index before the legacy
        // CreatureScript callback receives translated sender/action values.
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_grashAI(creature);
    }

    struct npc_grashAI : ScriptedAI
    {
        npc_grashAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            _introDone1 = false;
            wait_heal = false;
        }

        EventMap events;
        bool _introDone, _introDone1, wait_heal;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;

        ObjectGuid _introHealer;
        std::weak_ptr<uint8> _introAttempt;
        bool _introReached50 = false, _introReached90 = false, _introBusy = false, _gossipBusy = false;
        uint32 _introRetry = 0, _introTries = 0, _gossipActions = 0;
        FunctionProcessor _healFunctions;

        ObjectGuid _wallRecipient;
        std::weak_ptr<uint8> _wallAttempt;
        bool _wallPending = false, _wallBusy = false, _wallExhausted = false, _wallPresent = false;
        uint32 _wallRetry = 0, _wallTries = 0;

        bool BindWallRecipient(Player* player)
        {
            QuestStatusData* status = player->getQuestStatus(40341);
            if (!status) return false;
            player->AddQuestDelayedEvent(40341, 0, []() {});
            if (!status->ScriptLifetime) return false;
            _wallRecipient = player->GetGUID();
            _wallAttempt = status->ScriptLifetime;
            _wallPending = _wallExhausted = _wallPresent = false;
            _wallRetry = _wallTries = 0;
            return true;
        }

        void CompleteWallCredit(uint32 diff)
        {
            if (!_wallPending || _wallBusy) return;
            Scenario* scenario = SharasdalWall::World(me);
            if (!scenario) return;
            if (SharasdalGrash::Receipt(scenario, 47127))
            {
                _wallPending = false;
                return;
            }
            Player* player = ObjectAccessor::GetPlayer(*me, _wallRecipient);
            QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
            if (_wallAttempt.expired() || (player && (!status || status->ScriptLifetime != _wallAttempt.lock() ||
                player->GetQuestRewardStatus(40341) ||
                (player->GetQuestStatus(40341) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(40341) != QUEST_STATUS_COMPLETE))))
            {
                _wallPending = false;
                TC_LOG_WARN("scripts", "Sharasdal wall arrival cancelled after its original quest attempt ended");
                return;
            }
            if (scenario->GetCurrentStep() != 2)
            {
                _wallPending = false;
                TC_LOG_WARN("scripts", "Sharasdal wall arrival has no accepted criterion after its stage ended");
                return;
            }
            bool present = SharasdalGrash::Eligible(player, me);
            if (_wallExhausted && present && !_wallPresent)
            {
                _wallExhausted = false;
                _wallRetry = _wallTries = 0;
            }
            _wallPresent = present;
            if (_wallExhausted) return;
            if (_wallRetry > diff) { _wallRetry -= diff; return; }
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_wallBusy};
            _wallBusy = true;
            if (present) player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49817);
            if (SharasdalGrash::Receipt(scenario, 47127)) _wallPending = false;
            else
            {
                _wallRetry = 1000;
                if (++_wallTries >= 30)
                {
                    _wallExhausted = true;
                    TC_LOG_WARN("scripts", "Sharasdal wall arrival could not reach its original recipient/native criterion; retry when that recipient returns");
                }
            }
        }

        uint32 GetData(uint32 menu) const override
        {
            switch (menu)
            {
                case 19509: return _gossipActions & 1;
                case 19483: return _gossipActions & (1u << 2);
                case 19467: return _gossipActions & (1u << 4);
                default: return 0;
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 id = 0) override
        {
            if (id == 40341 && _wallPending && guid == _wallRecipient)
            {
                _wallExhausted = false;
                _wallRetry = _wallTries = 0;
            }
        }

        void Reset() override
        {
            CancelIntroHeal();
            _healFunctions.KillAllFunctions();
            _pendingHealRecovery = false;
            events.Reset();
        }

        void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (_introBusy || _gossipBusy || gossipListId || !SharasdalGrash::Eligible(player, me) ||
                !me->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) || me->HasUnitState(UNIT_STATE_STUNNED) ||
                !player->IsWithinDistInMap(me, INTERACTION_DISTANCE))
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            uint32 step = scenario->GetCurrentStep();
            uint32 menuIdExpected = SharasdalGrash::Menu(step);
            uint32 actionBit = 1u << (step <= 1 ? 0 : step);
            GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
            GossipMenuItem const* option = menu.GetItem(gossipListId);
            if (!menuIdExpected || menuId != menuIdExpected || menu.GetMenuId() != menuId ||
                !option || option->OptionNpc != GossipOptionNpc::None || option->IsCoded ||
                option->Sender != GOSSIP_SENDER_MAIN || option->Action != GOSSIP_ACTION_INFO_DEF + 1 ||
                (_gossipActions & actionBit) || (step <= 1 && !SharasdalGrash::Recruitable(scenario)))
                return;
            if (step <= 2)
                for (uint32 pathId : {step <= 1 ? 10993808u : 10993810u, step <= 1 ? 10993809u : 10993810u})
                {
                    WaypointPath const* path = sWaypointMgr->GetPathScript(pathId);
                    if (!path || path->empty())
                    {
                        TC_LOG_WARN("scripts", "Sharasdal Grash path %u is unavailable; retaining gossip for retry", pathId);
                        return;
                    }
                }
            if (step == 2 && (!SharasdalWall::World(me) || !BindWallRecipient(player)))
                return;
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_gossipBusy};
            _gossipBusy = true;
            if (step <= 1 && !SharasdalGrash::Receipt(scenario, 47303))
            {
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50161);
                if (!SharasdalGrash::Receipt(scenario, 47303))
                    return;
            }
            if (step == 4 && !SharasdalGrash::Receipt(scenario, 47133))
            {
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49819);
                if (!SharasdalGrash::Receipt(scenario, 47133))
                    return;
            }
            _gossipActions |= actionBit;
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (step <= 2)
                DoAction(step <= 1 ? 1 : 2);
            else
            {
                me->GetMotionMaster()->MovePoint(1, -17.96f, 802.05f, 807.66f);
            }
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 5000); // 209904
            events.RescheduleEvent(EVENT_2, 13000); // 209895
        }

        void DoAction(int32 const action) override
        {
            switch (action)
            {
                case 1:
                {
                    // need to activate it ONLY if creature isn't stunned
                    _functions.AddFunction([this]() -> void
                    {
                        me->GetMotionMaster()->MovePath(10993808, false); // 3
                        if (Creature* erunak = me->FindNearestCreature(102826, 70.0f))
                        {
                            erunak->AI()->Talk(2);
                            erunak->GetMotionMaster()->MovePath(10993809, false); // 2 5 9
                        }
                    }, _functions.CalculateTime(1000));
                    break;
                }
                case 2:
                {
                    _functions.AddFunction([this]() -> void
                    {
                        me->GetMotionMaster()->MovePath(10993810, false);  // 0 3
                        if (Creature* erunak = me->FindNearestCreature(102826, 70.0f))
                        {
                            erunak->AI()->Talk(9);
                            erunak->GetMotionMaster()->MovePath(10993810, false, irand(-5, 5), irand(-5, 5));
                        }
                        if (Creature* adel = me->FindNearestCreature(104853, 70.0f))
                            adel->GetMotionMaster()->MovePath(10993810, false, irand(-5, 5), irand(-5, 5));
                    }, _functions.CalculateTime(1000));
                    break;
                }
                case 3:
                {
                    _functions.AddFunction([this]() -> void
                    {
                        me->GetMotionMaster()->MovePoint(0, -44.47f, 802.39f, 797.16f);
                        if (Creature* erunak = me->FindNearestCreature(102826, 40.0f, true))
                            erunak->GetMotionMaster()->MovePoint(0, -18.62f, 808.39f, 807.72f);
                        if (Creature* adel = me->FindNearestCreature(104853, 40.0f, true))
                            adel->GetMotionMaster()->MovePoint(1, -19.61f, 796.32f, 807.65f);
                    }, _functions.CalculateTime(1000));
                    break;
                }
            }
        }

        void OnHealApplied(Unit* who, uint32 /*amount*/, int32 /*gain*/) override
        {
            if (wait_heal)
                _pendingHealRecovery = true;
            Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (_introBusy || (_introDone && _introDone1) || !SharasdalGrash::Eligible(player, me) ||
                me->GetPositionZ() > 795.0f || me->GetHealthPct() < 50 ||
                sScenarioMgr->GetScenario(me->GetInstanceId())->GetCurrentStep() > 1)
                return;
            player->AddQuestDelayedEvent(40341, 0, []() { });
            if (QuestStatusData* status = player->getQuestStatus(40341))
            {
                _introHealer = player->GetGUID();
                _introAttempt = status->ScriptLifetime;
                _introReached50 = true;
                _introReached90 = me->GetHealthPct() >= 90;
                _introRetry = _introTries = 0;
            }
        }

        void CancelIntroHeal()
        {
            _introHealer.Clear();
            _introAttempt.reset();
            _introReached50 = _introReached90 = false;
            _introRetry = _introTries = 0;
        }

        void CompleteIntroHeal(uint32 diff)
        {
            if (_introBusy || _introHealer.IsEmpty()) return;
            if (_introRetry > diff) { _introRetry -= diff; return; }
            Player* player = ObjectAccessor::GetPlayer(*me, _introHealer);
            QuestStatusData* status = player ? player->getQuestStatus(40341) : nullptr;
            if (_introAttempt.expired() || !status || status->ScriptLifetime != _introAttempt.lock() ||
                !SharasdalGrash::Eligible(player, me) || me->GetPositionZ() > 795.0f ||
                me->GetHealthPct() < 50 || sScenarioMgr->GetScenario(me->GetInstanceId())->GetCurrentStep() > 1)
            {
                CancelIntroHeal();
                return;
            }
            struct BusyGuard { bool& flag; ~BusyGuard() { flag = false; } } guard{_introBusy};
            _introBusy = true;
            if (!_introDone1 && _introReached50)
            {
                _introDone1 = true;
                Talk(0);
                ObjectGuid healer = _introHealer;
                std::weak_ptr<uint8> attempt = _introAttempt;
                _healFunctions.AddFunction([this, healer, attempt]()
                {
                    Player* original = ObjectAccessor::GetPlayer(*me, healer);
                    QuestStatusData* current = original ? original->getQuestStatus(40341) : nullptr;
                    if (attempt.expired() || !current || current->ScriptLifetime != attempt.lock() ||
                        !SharasdalGrash::Eligible(original, me)) return;
                    if (Creature* erunak = me->FindNearestCreature(102826, 70.0f, true))
                        if (erunak->IsInWorld() && !erunak->IsPreDelete() && erunak->IsAlive() && erunak->AI())
                            erunak->AI()->Talk(1);
                }, _healFunctions.CalculateTime(3000));
            }
            if (!_introReached90 || me->GetHealthPct() < 90) { CancelIntroHeal(); return; }
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!SharasdalGrash::Receipt(scenario, 47121))
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49815);
            // Criteria delivery can synchronously advance native state. Retain
            // the original quest attempt before emitting its second receipt.
            player = ObjectAccessor::GetPlayer(*me, _introHealer);
            status = player ? player->getQuestStatus(40341) : nullptr;
            if (_introAttempt.expired() || !status || status->ScriptLifetime != _introAttempt.lock() ||
                !SharasdalGrash::Eligible(player, me) || me->GetPositionZ() > 795.0f ||
                sScenarioMgr->GetScenario(me->GetInstanceId())->GetCurrentStep() > 1)
            {
                CancelIntroHeal();
                return;
            }
            scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!SharasdalGrash::Receipt(scenario, 47180))
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49900);
            if (SharasdalGrash::Receipt(scenario, 47121) && SharasdalGrash::Receipt(scenario, 47180))
            {
                _introDone = true;
                CancelIntroHeal();
                return;
            }
            if (++_introTries >= 30)
            {
                TC_LOG_WARN("scripts", "Sharasdal Grash heal criteria rejected after thirty attempts; retaining real-heal retry");
                CancelIntroHeal();
            }
            else _introRetry = 1000;
        }

        void UpdateLifecycle(uint32 diff) override
        {
            SharasdalCompanion::Recover(me, _pendingHealRecovery, wait_heal);
            // Advance existing replies before scheduling a fresh three-second delay.
            _healFunctions.Update(diff);
            CompleteIntroHeal(diff);
            CompleteWallCredit(diff);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(208093, me);
                    Talk(1);
                    Talk(2);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                Scenario* scenario = SharasdalWall::World(me);
                uint32 path = SharasdalWall::Path(me);
                uint64 wall = SharasdalWall::Wall(path, id);
                if (scenario && wall &&
                    ((path == 10993808 && (_gossipActions & 1) && scenario->GetCurrentStep() >= 1 && scenario->GetCurrentStep() <= 2) ||
                     (path == 10993810 && (_gossipActions & 4) && scenario->GetCurrentStep() >= 2 && scenario->GetCurrentStep() <= 3)))
                {
                    DoCast(209904);
                    if (GameObject* go = me->FindNearestGameObject(248514, 5.0f))
                        if (go->GetDBTableGUIDLow() == wall && go->IsInWorld() && me->InSamePhase(go))
                            if (InstanceScript* inst = me->GetInstanceScript())
                            {
                                inst->HandleGameObject(go->GetGUID(), true, go);
                                if (path == 10993810 && id == 0 && go->GetGoState() == GO_STATE_ACTIVE &&
                                    !_wallAttempt.expired() && !SharasdalGrash::Receipt(scenario, 47127))
                                {
                                    _wallPending = true;
                                    CompleteWallCredit(0);
                                }
                            }
                }
            }
            else if (type == POINT_MOTION_TYPE)
            {
                if (id == 1)
                {
                    DoCast(209904);
                    if (GameObject* go = me->FindNearestGameObject(248514, 5.0f))
                        if (InstanceScript* inst = me->GetInstanceScript())
                            inst->HandleGameObject(go->GetGUID(), true);
                    if (Creature* lady = me->FindNearestCreature(104856, 90.0f, true))
                    {
                        AttackStart(lady);
                        if (Creature* erunak = me->FindNearestCreature(102826, 40.0f, true))
                            erunak->AI()->AttackStart(lady);
                        if (Creature* adel = me->FindNearestCreature(104853, 40.0f, true))
                            adel->AI()->AttackStart(lady);
                    }
                }
            }
        }


        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED)) // need for this
                return;

            _functions.Update(diff);

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(209904);
                        events.RescheduleEvent(EVENT_1, 5000); // 209904
                        break;
                    case EVENT_2:
                        DoCast(209895);
                        events.RescheduleEvent(EVENT_2, 13000); // 20989
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 104853
class npc_adel_waterspeaker : public CreatureScript
{
public:
    npc_adel_waterspeaker() : CreatureScript("npc_adel_waterspeaker") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_adel_waterspeakerAI(creature);
    }

    struct npc_adel_waterspeakerAI : ScriptedAI
    {
        npc_adel_waterspeakerAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            wait_heal = false;
            if (me->GetPositionZ() >= 795.0f)
                me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        EventMap events;
        bool _introDone, wait_heal;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;

        std::shared_ptr<uint8> _rescueLife = std::make_shared<uint8>();
        FunctionProcessor _rescueFunctions;
        bool _rescueAccepted = false, _rescueSchedule = false, _rescueDone = false;
        bool _rescueNotifyDone = false, _rescueNotifyQueued = false, _rescueNotifyExhausted = false;
        uint32 _rescueDelay = 0, _rescueNotifyTries = 0;

        void Reset() override
        {
            _rescueLife = std::make_shared<uint8>();
            _rescueFunctions.KillAllFunctions();
            _rescueAccepted = _rescueSchedule = _rescueNotifyQueued = false;
            _rescueNotifyExhausted = false;
            _rescueNotifyTries = 0;
            _pendingHealRecovery = false;
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 10000); // 51876
            events.RescheduleEvent(EVENT_2, 11000); // 209914
        }


        void OnHealApplied(Unit* /*who*/, uint32 /*amount*/, int32 /*gain*/) override
        {
            if (wait_heal)
                _pendingHealRecovery = true;
        }

        void AcceptRescue(uint32 remainingDelay)
        {
            if (_rescueAccepted || _rescueDone || !SharasdalGrash::Receipt(SharasdalRescue::World(me), 47123))
                return;
            _rescueAccepted = _rescueSchedule = true;
            _rescueDelay = remainingDelay;
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
        }

        void CompleteRescue()
        {
            if (_rescueDone) return;
            if (!SharasdalGrash::Receipt(SharasdalRescue::World(me), 47123))
            {
                _rescueAccepted = false;
                return;
            }
            _rescueDone = true;
            me->SetByteValue(UNIT_FIELD_BYTES_1, 0, 0);
            me->SetAnimKitId(0);
            me->HandleEmoteCommand(53);
        }

        void NotifyGrash()
        {
            _rescueNotifyQueued = false;
            Scenario* scenario = SharasdalRescue::World(me);
            if (!_rescueDone || !scenario || scenario->GetCurrentStep() != 2 ||
                !SharasdalGrash::Receipt(scenario, 47123) || !SharasdalGrash::Receipt(scenario, 47197))
                return;
            if (Creature* grash = me->FindNearestCreature(105298, 70.0f, true))
                if (grash->IsInWorld() && !grash->IsPreDelete() && grash->IsAlive() && grash->AI() &&
                    grash->GetPositionZ() < 795.0f && me->InSamePhase(grash))
                {
                    _rescueNotifyDone = true;
                    if (!grash->AI()->GetData(19483) &&
                        (!grash->GetDisabledAI() || !grash->GetDisabledAI()->GetData(19483)) &&
                        !grash->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP))
                    {
                        grash->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        grash->AI()->Talk(5);
                    }
                    return;
                }
            if (++_rescueNotifyTries >= 30)
            {
                _rescueNotifyExhausted = true;
                TC_LOG_WARN("scripts", "Sharasdal rescue cannot resolve lower Grash after thirty attempts; retaining reset/re-entry recovery");
            }
        }

        void UpdateLifecycle(uint32 diff) override
        {
            SharasdalCompanion::Recover(me, _pendingHealRecovery, wait_heal);
            auto lifetime = _rescueLife;
            _rescueFunctions.Update(diff);
            if (lifetime != _rescueLife) return;
            Scenario* scenario = SharasdalRescue::World(me);
            if (!scenario) return;
            if (!_rescueAccepted && !_rescueDone && SharasdalGrash::Receipt(scenario, 47123))
                AcceptRescue(2000); // Recover presentation from an accepted persisted criterion; do not grant it again.
            if (_rescueSchedule)
            {
                _rescueSchedule = false;
                _rescueFunctions.AddFunction([this]() { CompleteRescue(); }, _rescueFunctions.CalculateTime(_rescueDelay));
            }
            if (_rescueDone && !_rescueNotifyDone && !_rescueNotifyQueued && !_rescueNotifyExhausted &&
                scenario->GetCurrentStep() == 2 && SharasdalGrash::Receipt(scenario, 47197))
            {
                _rescueNotifyQueued = true;
                _rescueFunctions.AddFunction([this]() { NotifyGrash(); }, _rescueFunctions.CalculateTime(_rescueNotifyTries ? 1000 : 0));
            }
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(208093, me);
                    Talk(2);
                    Talk(3);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == POINT_MOTION_TYPE)
            {
                if (id == 1)
                {
                    // Talk()
                    if (Creature* garsh = me->FindNearestCreature(105298, 50.0f, true))
                    {
                        garsh->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // gossip
                    }
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED)) // need for this
                return;

            _functions.Update(diff);

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(51876);
                        events.RescheduleEvent(EVENT_1, 10000); // 51876
                        break;
                    case EVENT_2:
                        DoCast(209914);
                        events.RescheduleEvent(EVENT_2, 11000); // 209914
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};



class spell_sharasdal_breaking_bonds : public SpellScript
{
    // Native208171 keeps its real five-second SEND_EVENT; this guard owns no credit.
    PrepareSpellScript(spell_sharasdal_breaking_bonds);
    using AdelAI = npc_adel_waterspeaker::npc_adel_waterspeakerAI;
    bool _scoped = true, _effectAllowed = false, _notified = false;
    ObjectGuid _source;
    uint32 _instance = 0, _started = 0;
    std::weak_ptr<uint8> _quest, _actor;

    static AdelAI* Receiver(Creature* creature)
    {
        if (!creature) return nullptr;
        if (auto active = dynamic_cast<AdelAI*>(creature->AI())) return active;
        return dynamic_cast<AdelAI*>(creature->GetDisabledAI());
    }

    bool Validate(SpellInfo const* info) override
    {
        return info && info->Id == 208171 && info->GetEffect(EFFECT_0, DIFFICULTY_NONE)->Effect == SPELL_EFFECT_SEND_EVENT &&
            info->GetEffect(EFFECT_0, DIFFICULTY_NONE)->MiscValue == 49816;
    }

    bool Load() override
    {
        Unit* caster = GetCaster();
        Unit* original = GetOriginalTarget(); // InitExplicitTargets preserves this before removing a redundant self target.
        Creature* creature = original ? original->ToCreature() : nullptr;
        _scoped = (caster && caster->GetMapId() == 1600) || (creature && creature->GetEntry() == 104853);
        if (!_scoped) return true;
        _started = getMSTime();
        Player* player = caster ? caster->ToPlayer() : nullptr;
        AdelAI* ai = Receiver(creature);
        if (SharasdalRescue::Eligible(player, creature) && ai)
        {
            _source = creature->GetGUID();
            _instance = creature->GetInstanceId();
            _actor = ai->_rescueLife;
            player->AddQuestDelayedEvent(40341, 0, []() { });
            if (QuestStatusData* status = player->getQuestStatus(40341))
                _quest = status->ScriptLifetime;
        }
        return true; // Invalid scoped casts must retain this guard, not unload it.
    }

    AdelAI* Ready(Player* player, bool after)
    {
        if (!player || _source.IsEmpty() || player->GetInstanceId() != _instance || _quest.expired() || _actor.expired())
            return nullptr;
        Creature* creature = ObjectAccessor::GetCreature(*player, _source);
        AdelAI* ai = Receiver(creature);
        QuestStatusData* status = player->getQuestStatus(40341);
        if (!SharasdalRescue::Eligible(player, creature) || !ai || ai->_rescueLife != _actor.lock() ||
            !status || status->ScriptLifetime != _quest.lock() ||
            !creature->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK))
            return nullptr;
        Scenario* scenario = SharasdalRescue::World(creature);
        if (!SharasdalGrash::Receipt(scenario, 47197)) return nullptr;
        if (after)
            return (scenario->GetCurrentStep() == 1 || scenario->GetCurrentStep() == 2) &&
                SharasdalGrash::Receipt(scenario, 47123) ? ai : nullptr;
        return scenario->GetCurrentStep() == 1 && !SharasdalGrash::Receipt(scenario, 47123) ? ai : nullptr;
    }

    SpellCastResult Check()
    {
        return !_scoped || Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr, false) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }

    void BeforeEvent(SpellEffIndex effect)
    {
        if (!_scoped) return;
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        _effectAllowed = player && GetHitUnit() == player && Ready(player, false);
        if (!_effectAllowed)
        {
            PreventHitDefaultEffect(effect);
            TC_LOG_WARN("scripts", "Sharasdal Breaking Bonds rejected a stale actor/quest before native49816; no rescue transition");
        }
    }

    void AfterEvent()
    {
        if (!_scoped || !_effectAllowed || _notified) return;
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        AdelAI* ai = GetHitUnit() == player ? Ready(player, true) : nullptr;
        if (!ai) return;
        _notified = true;
        uint32 elapsed = getMSTimeDiff(_started, getMSTime());
        ai->AcceptRescue(elapsed < 7000 ? 7000 - elapsed : 0);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_sharasdal_breaking_bonds::Check);
        OnEffectHitTarget += SpellEffectFn(spell_sharasdal_breaking_bonds::BeforeEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
        AfterHit += SpellHitFn(spell_sharasdal_breaking_bonds::AfterEvent);
    }
};

// 102839
class npc_kraliss : public CreatureScript
{
public:
    npc_kraliss() : CreatureScript("npc_kraliss") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_kralissAI(creature);
    }

    struct npc_kralissAI : ScriptedAI
    {
        npc_kralissAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 18000); // 209748
            events.RescheduleEvent(EVENT_2, 20000); // 209764 на рандома. Через спелл хит 209755, который делает рывок и 209757
        }

        void JustDied(Unit* /*who*/) override
        {
            // The native instance Kill dispatch retains this real victory and
            // its eligible player/owner; proximity alone is not ownership.
        }

        void SpellHitTarget(Unit* who, SpellInfo const* spell) override
        {
            if (spell->Id == 20508)
                me->CastSpell(who, 209757);
            else if (spell->Id == 209755)
                me->CastSpell(who, 209757);
        }
        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        Talk(0);
                        events.RescheduleEvent(EVENT_3, 1000);
                        events.RescheduleEvent(EVENT_1, 18000); // 209748
                        break;
                    case EVENT_2:
                        if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        {
                            me->CastSpell(pTarget, 209764);
                            me->CastSpell(pTarget, 209755);
                        }
                        events.RescheduleEvent(EVENT_2, 20000); // 209764 на рандома. Через спелл хит 209755, который делает рывок и 209757
                        break;
                    case EVENT_3:
                        DoCast(209748);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


// 104856
class npc_lady_zitrin : public CreatureScript
{
public:
    npc_lady_zitrin() : CreatureScript("npc_lady_zitrin") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lady_zitrinAI(creature);
    }

    struct npc_lady_zitrinAI : ScriptedAI
    {
        npc_lady_zitrinAI(Creature* creature) : ScriptedAI(creature)
        {
            if (me->GetPositionX() <= -15.0f)
                intro = true;
            else
                intro = false;
            _introDone = false;
            introtimer = 2000;
        }

        uint32 introtimer;
        EventMap events;
        bool _introDone, intro;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 2500); // 209919
            events.RescheduleEvent(EVENT_2, 16000); // 209707
            events.RescheduleEvent(EVENT_3, 28000); // 209934 // add
            events.RescheduleEvent(EVENT_4, 25000); // 209983 + script
        }

        void JustDied(Unit* /*who*/) override
        {
            // Native instance Kill dispatch retains only the real boss victory.
            // Conversation1923 belongs to the later artifact acquisition.
        }


        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetPositionX() <= -15.0f && !_introDone && me->IsWithinDistInMap(who, 40.0f))
            {
                _introDone = true;
                intro = false;
                who->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50314);
                me->GetMotionMaster()->MovePoint(0, 10.37f, 803.32f, 806.75f);
                if (Creature* garsh = me->FindNearestCreature(105298, 200.0f, true))
                    garsh->AI()->DoAction(3);
                me->DespawnOrUnsummon(1000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (intro)
            {
                if (introtimer <= diff)
                {
                    uint8 rand = urand(0, 2);
                    if (Creature* add = me->SummonCreature(105251, sumpos[rand].GetPositionX(), sumpos[rand].GetPositionY(), sumpos[rand].GetPositionZ(), 0.0f))
                    {
                        add->GetMotionMaster()->MovePoint(1, -148.25f, 802.57f, 796.49f);
                        add->CastSpell(add, 209132);
                    }
                    introtimer = 6000;
                }
                else
                    introtimer -= diff;
            }
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(209919);
                        events.RescheduleEvent(EVENT_1, 2500); // 209919
                        break;
                    case EVENT_2:
                        DoCast(209707);
                        events.RescheduleEvent(EVENT_2, 16000); // 209707
                        break;
                    case EVENT_3:
                        DoCast(209934);
                        events.RescheduleEvent(EVENT_3, 28000); // 209934 // add
                        break;
                    case EVENT_4:
                        if (Creature* garsh = me->FindNearestCreature(105298, 40.0f, true))
                        {
                            Talk(2);
                            me->CastSpell(garsh, 209983);
                            garsh->AI()->Talk(9);
                        }
                        events.RescheduleEvent(EVENT_4, 25000); // 209983 + script
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


// 105251
class npc_sharasdal_waterwall : public CreatureScript
{
public:
    npc_sharasdal_waterwall() : CreatureScript("npc_sharasdal_waterwall") {}

    struct npc_sharasdal_waterwallAI : ScriptedAI
    {
        npc_sharasdal_waterwallAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
        }

        void Reset() override {}

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (me->GetVehicleKit())
            {
                me->GetVehicleKit()->RemoveAllPassengers();
                me->DespawnOrUnsummon(100);
            }
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sharasdal_waterwallAI(creature);
    }
};

// 249342
#include "sharasdal_acquisition.h"

void AddSC_Sharasdal()
{
    new npc_erunak_stonespeaker();
    new npc_grash();
    new npc_adel_waterspeaker();
    RegisterSpellScript(spell_sharasdal_breaking_bonds);
    new npc_kraliss();
    new npc_lady_zitrin();
    new npc_sharasdal_waterwall();
    new go_art_sharasdal();
    RegisterSpellScript(spell_sharasdal_acquisition);
    RegisterAuraScript(aura_sharasdal_acquisition);
};
