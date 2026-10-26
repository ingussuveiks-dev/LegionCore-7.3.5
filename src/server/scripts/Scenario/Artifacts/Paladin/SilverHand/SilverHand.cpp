/*
    Paladin: SilverHand
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "WaypointManager.h"


namespace SilverHandCompanion
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
        actor->RemoveAura(210985);
    }
}

namespace SilverHandEntrance
{
    bool CanArrive(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || !actor->IsAlive() || actor->GetEntry() != 106429 || actor->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, 20.0f) || player->GetQuestRewardStatus(42120) ||
            player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() <= 1;
    }
}

namespace SilverHandProcession
{
    bool CanArrive(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || !actor->IsAlive() || actor->GetEntry() != 106639 || actor->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, 100.0f) || player->GetQuestRewardStatus(42120) ||
            player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 4;
    }
}

namespace SilverHandRitualEntry
{
    bool CanAdvance(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106676 || actor->GetMapId() != 1539 || !actor->GetMap())
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 8;
    }

    bool CanEnter(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || !actor->IsAlive() || actor->GetEntry() != 106676 || actor->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, 20.0f) || player->GetQuestRewardStatus(42120) ||
            player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1092)
            return false;
        uint32 step = scenario->GetCurrentStep();
        return step >= 5 && step <= 8;
    }
}

namespace SilverHandRitualArrival
{
    bool CanCredit(Player* player, Creature* actor)
    {
        return player && SilverHandRitualEntry::CanAdvance(actor) && player->IsInWorld() && !player->IsPreDelete() &&
            player->CanContact() && !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_PALADIN &&
            player->GetMap() == actor->GetMap() && player->GetInstanceId() == actor->GetInstanceId() &&
            player->InSamePhase(actor) && player->IsWithinDistInMap(actor, 100.0f) &&
            !player->GetQuestRewardStatus(42120) && player->GetQuestStatus(42120) == QUEST_STATUS_INCOMPLETE;
    }

    Player* SelectPlayer(Creature* actor)
    {
        if (!SilverHandRitualEntry::CanAdvance(actor))
            return nullptr;
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (CanCredit(reference.getSource(), actor))
                return reference.getSource();
        return nullptr;
    }
}

namespace SilverHandRitualWaypoint
{
    Player* SelectPlayer(Creature* actor, uint32 step)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106676 || actor->GetMapId() != 1539 || !actor->GetMap() || (step != 5 && step != 6))
            return nullptr;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1092 || scenario->GetCurrentStep() != step)
            return nullptr;
        for (auto const& reference : actor->GetMap()->GetPlayers())
        {
            Player* player = reference.getSource();
            if (player && player->IsInWorld() && !player->IsPreDelete() && player->CanContact() &&
                !player->isGameMaster() && !player->IsBeingTeleported() && player->getClass() == CLASS_PALADIN &&
                player->GetMap() == actor->GetMap() && player->GetInstanceId() == actor->GetInstanceId() &&
                player->InSamePhase(actor) && player->IsWithinDistInMap(actor, 100.0f) &&
                !player->GetQuestRewardStatus(42120) && player->GetQuestStatus(42120) == QUEST_STATUS_INCOMPLETE)
                return player;
        }
        return nullptr;
    }
}

// 106429
class npc_travar_first : public CreatureScript
{
public:
    npc_travar_first() : CreatureScript("npc_travar_first") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_travar_firstAI(creature);
    }

    struct npc_travar_firstAI : ScriptedAI
    {
        npc_travar_firstAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            wait_heal = false;
            firstwp = false;
            secondwp = false;
            firstfight = false;
        }

        EventMap events;
        bool _introDone, wait_heal, firstwp, secondwp, firstfight;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;
        ObjectGuid _arrivalPlayerGuid;
        FunctionProcessor _entryFunctions;
        uint64 _entryGeneration = 0;
        bool _entryPending = false;

        void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId != 19580 || gossipListId != 0 || !firstwp || !SilverHandEntrance::CanArrive(player, me) ||
                !me->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) ||
                !player->IsWithinDistInMap(me, INTERACTION_DISTANCE))
                return;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!scenario || scenario->GetCurrentStep() != 1)
                return;
            GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
            GossipMenuItem const* option = menu.GetItem(gossipListId);
            if (menu.GetMenuId() != menuId || !option || option->OptionNpc != GossipOptionNpc::None ||
                option->Sender || option->Action || option->IsCoded)
                return;
            WaypointPath const* path = sWaypointMgr->GetPathScript(10993804);
            if (!path || path->empty())
            {
                TC_LOG_WARN("scripts", "Silver Hand entrance path 10993804 is unavailable; retaining Travard gossip for retry");
                return;
            }
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            me->GetMotionMaster()->MovePath(10993804, false);
        }

        void CompleteEntranceArrival()
        {
            if (_arrivalPlayerGuid.IsEmpty())
                return;
            Player* player = ObjectAccessor::GetPlayer(*me, _arrivalPlayerGuid);
            _arrivalPlayerGuid.Clear();
            if (!SilverHandEntrance::CanArrive(player, me))
            {
                _introDone = false;
                return;
            }
            Talk(2);
            me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50765);
            firstwp = true;
            if (Creature* efrin = me->FindNearestCreature(106371, 50.0f, true))
                efrin->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(- PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
            if (Creature* duval = me->FindNearestCreature(106370, 50.0f, true))
                duval->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
        }

        void Reset() override
        {
            events.Reset();
            _pendingHealRecovery = false;
            if (_entryPending)
            {
                ++_entryGeneration;
                _entryPending = false;
                _introDone = false;
            }
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 5000); // 215689
            events.RescheduleEvent(EVENT_2, 8000); // 210371
        }

        void EnterEvadeMode() override
        {
            // need to activate it ONLY if creature isn't stunned
            _functions.AddFunction([this]() -> void
            {
                if (firstfight)
                {
                    firstfight = false;
                    Talk(18);
                    me->GetMotionMaster()->MovePoint(2, 1962.02f, 2336.25f, 80.52f);
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50850);
                }
            }, _functions.CalculateTime(1000));
        }

        void HealReceived(Unit*, uint32& addhealth) override
        {
            if (wait_heal && addhealth)
                _pendingHealRecovery = true;
        }

        void UpdateLifecycle(uint32 diff) override
        {
            SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal);
            _entryFunctions.Update(diff);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(210985, me);
                    Talk(9);
                    Talk(10);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                if (firstwp)
                {
                    if (id == 7)
                        Talk(4);
                    if (id == 10)
                    {
                        if (Player* pl = me->FindNearestPlayer(100.0f))
                            pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50766);
                        Talk(6);
                        firstwp = false;
                        VisualOpenDoor();
                        secondwp = true;
                        _functions.AddFunction([this]() -> void
                        {
                            Talk(7);
                        }, _functions.CalculateTime(5000));

                        _functions.AddFunction([this]() -> void
                        {
                            Talk(8);
                            me->GetMotionMaster()->MovePath(10993805, false);
                        }, _functions.CalculateTime(9000));
                    }
                }
                if (secondwp)
                {
                    if (id == 18)
                    {
                        Talk(17);
                        if (Creature* gnorc = me->SummonCreature(106624, 1919.57f, 2334.55f, 75.41f, 0.0f))
                        {
                            gnorc->CastSpell(gnorc, 208257);
                            AttackStart(gnorc);
                            gnorc->AI()->AttackStart(me);
                        }
                        _entryFunctions.AddDelayedEvent(2000, [this]() -> void
                        {
                            firstfight = true;
                        });
                    }
                }
            }
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 1:
                        CompleteEntranceArrival();
                        break;
                    case 2:
                    {
                        DoCast(185414);
                        std::list<Creature*> adds;
                        GetCreatureListWithEntryInGrid(adds, me, 106094, 30.0f);
                        uint8 count = 0;
                        if (!adds.empty())
                            for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                            {
                                if (count >= 3)
                                    break;

                                if ((*itr)->HasAura(211502))
                                {
                                    (*itr)->SetVisible(true);
                                    count++;
                                }
                            }

                        _functions.AddFunction([this]() -> void
                        {
                            me->RemoveAura(185414);
                            std::list<Creature*> adds;
                            GetCreatureListWithEntryInGrid(adds, me, 106094, 30.0f);
                            if (!adds.empty())
                                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                                {
                                    if ((*itr)->HasAura(211502))
                                        (*itr)->SetVisible(false);
                                }
                            Talk(19);
                        }, _functions.CalculateTime(4000));

                        _functions.AddFunction([this]() -> void
                        {
                            Talk(20);
                            me->GetMotionMaster()->MovePoint(3, 1919.26f, 2335.29f, 75.49f);
                        }, _functions.CalculateTime(7000));
                    }
                    break;
                    case 3:
                    {
                        if (Player* pl = me->FindNearestPlayer(100.0f))
                            pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50872);
                        if (Creature* cr = me->FindNearestCreature(101461, 20.0f, true))
                            cr->DespawnOrUnsummon();

                        std::list<GameObject*> gob;
                        GetGameObjectListWithEntryInGrid(gob, me, 251349, 12.0f);
                        if (!gob.empty())
                            for (std::list<GameObject*>::iterator itr = gob.begin(); itr != gob.end(); ++itr)
                                (*itr)->SetPhaseMask(2, true); // hide   
                    }
                    break;
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who ? who->ToPlayer() : nullptr;
            if (_introDone || !SilverHandEntrance::CanArrive(player, me))
                return;

            _introDone = true;
            _entryPending = true;
            ObjectGuid playerGuid = player->GetGUID();
            uint64 generation = _entryGeneration;
            Talk(0);
            _entryFunctions.AddDelayedEvent(5000, [this, playerGuid, generation]() -> void
            {
                if (!_entryPending || generation != _entryGeneration)
                    return;
                _entryPending = false;
                Player* player = ObjectAccessor::GetPlayer(*me, playerGuid);
                if (!SilverHandEntrance::CanArrive(player, me))
                {
                    _introDone = false;
                    return;
                }
                Talk(1);
                _arrivalPlayerGuid = playerGuid;
                me->GetMotionMaster()->MovePoint(1, 2137.70f, 2397.60f, 118.54f);
                player->KilledMonsterCredit(106416);
            });
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
                        DoCast(215689);
                        events.RescheduleEvent(EVENT_1, 5000); // 215689
                        break;
                    case EVENT_2:
                        DoCast(210371);
                        events.RescheduleEvent(EVENT_2, 8000); // 210371
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        void VisualOpenDoor()
        {
            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, me, 106094, 10.0f);
            if (!adds.empty())
                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                {
                    (*itr)->RemoveAura(200181);
                    (*itr)->RemoveAura(204891);
                }

            // 251349
            if (GameObject* go = me->FindNearestGameObject(251349, 30.0f))
                go->SetPhaseMask(2, true); // hide
        }
    };
};

// 106639
class npc_travar_second : public CreatureScript
{
public:
    npc_travar_second() : CreatureScript("npc_travar_second") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_travar_secondAI(creature);
    }

    struct npc_travar_secondAI : ScriptedAI
    {
        npc_travar_secondAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // gossip
            wait_heal = false;
        }

        EventMap events;
        bool wait_heal;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;
        ObjectGuid _processionPlayerGuid;

        void sGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId != 19599 || gossipListId != 0 || !SilverHandProcession::CanArrive(player, me) ||
                !me->HasFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP) || !player->IsWithinDistInMap(me, INTERACTION_DISTANCE))
                return;
            GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
            GossipMenuItem const* option = menu.GetItem(gossipListId);
            if (menu.GetMenuId() != menuId || !option || option->OptionNpc != GossipOptionNpc::None ||
                option->Sender || option->Action || option->IsCoded)
                return;
            WaypointPath const* path = sWaypointMgr->GetPathScript(10993806);
            if (!path || path->empty())
            {
                TC_LOG_WARN("scripts", "Silver Hand procession path 10993806 is unavailable; retaining Travard gossip for retry");
                return;
            }
            me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            if (Creature* efrin = me->FindNearestCreature(106371, 30.0f, true))
                efrin->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(- PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
            if (Creature* duval = me->FindNearestCreature(106370, 30.0f, true))
                duval->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
            _processionPlayerGuid = player->GetGUID();
            me->GetMotionMaster()->MovePath(10993806, false);
            Talk(0);
        }

        void CompleteProcession()
        {
            if (_processionPlayerGuid.IsEmpty())
                return;
            Player* player = ObjectAccessor::GetPlayer(*me, _processionPlayerGuid);
            _processionPlayerGuid.Clear();
            if (!SilverHandProcession::CanArrive(player, me))
                return;
            Talk(5);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50897);
        }

        void Reset() override
        {
            events.Reset();
            _pendingHealRecovery = false;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 5000); // 215689
            events.RescheduleEvent(EVENT_2, 8000); // 210371
        }

        void HealReceived(Unit*, uint32& addhealth) override
        {
            if (wait_heal && addhealth)
                _pendingHealRecovery = true;
        }

        void UpdateLifecycle(uint32) override
        {
            SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(210985, me);
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
                if (id == 7)
                    Talk(3);
                if (id == 11)
                {
                    VisualOpenDoor();
                    Talk(4);
                }
                if (id == 14)
                    CompleteProcession();
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
                        DoCast(215689);
                        events.RescheduleEvent(EVENT_1, 5000); // 215689
                        break;
                    case EVENT_2:
                        DoCast(210371);
                        events.RescheduleEvent(EVENT_2, 8000); // 210371
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        void VisualOpenDoor()
        {
            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, me, 106094, 5.0f);
            if (!adds.empty())
                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                {
                    (*itr)->RemoveAura(200181);
                    (*itr)->RemoveAura(204891);
                }

            std::list<GameObject*> gob;
            GetGameObjectListWithEntryInGrid(gob, me, 251349, 5.0f);
            if (!gob.empty())
                for (std::list<GameObject*>::iterator itr = gob.begin(); itr != gob.end(); ++itr)
                    (*itr)->SetPhaseMask(2, true); // hide   
        }
    };
};

// 106676
class npc_travar_third : public CreatureScript
{
public:
    npc_travar_third() : CreatureScript("npc_travar_third") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_travar_thirdAI(creature);
    }

    struct npc_travar_thirdAI : ScriptedAI
    {
        npc_travar_thirdAI(Creature* creature) : ScriptedAI(creature)
        {
            wait_heal = false;
            _introDone = false;
        }

        EventMap events;
        bool wait_heal, _introDone;
        bool _pendingHealRecovery = false;
        FunctionProcessor _functions;
        FunctionProcessor _entryFunctions;
        uint64 _entryGeneration = 0;
        bool _entryPending = false;
        bool _ritualStarted = false;
        bool _ritualMotionIssued = false;
        bool _ritualArrived = false;
        bool _ritualCompleted = false;
        bool _mawWaypointCredited = false;
        bool _prisonWaypointCredited = false;
        bool _mawWaypointArrived = false;
        bool _prisonWaypointArrived = false;

        void Reset() override
        {
            events.Reset();
            _pendingHealRecovery = false;
            if (_entryPending)
            {
                ++_entryGeneration;
                _entryPending = false;
                _introDone = false;
            }
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 5000); // 215689
            events.RescheduleEvent(EVENT_2, 8000); // 210371
        }

        void DoAction(int32 const action) override
        {
            if (action != 1 || _ritualStarted || !SilverHandRitualEntry::CanAdvance(me))
                return;
            _ritualStarted = true;
            _functions.AddFunction([this]() -> void
            {
                if (!SilverHandRitualEntry::CanAdvance(me))
                {
                    _ritualStarted = false;
                    return;
                }
                Talk(8);
                _ritualMotionIssued = true;
                me->GetMotionMaster()->MovePoint(1, 1914.39f, 2335.43f, 75.56f);
            }, _functions.CalculateTime(1000));
        }
        void HealReceived(Unit*, uint32& addhealth) override
        {
            if (wait_heal && addhealth)
                _pendingHealRecovery = true;
        }

        void UpdateLifecycle(uint32 diff) override
        {
            SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal);
            CompletePrisonWaypoints();
            CompleteRitualArrival();
            _entryFunctions.Update(diff);
        }

        void CompletePrisonWaypoints()
        {
            if (_mawWaypointArrived && !_mawWaypointCredited)
                if (Player* player = SilverHandRitualWaypoint::SelectPlayer(me, 5))
                {
                    _mawWaypointCredited = true;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50877);
                }
            if (_prisonWaypointArrived && !_prisonWaypointCredited)
                if (Player* player = SilverHandRitualWaypoint::SelectPlayer(me, 6))
                {
                    _prisonWaypointCredited = true;
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50912);
                }
        }

        void CompleteRitualArrival()
        {
            if (!_ritualMotionIssued || !_ritualArrived || _ritualCompleted)
                return;
            Player* player = SilverHandRitualArrival::SelectPlayer(me);
            if (!player)
                return;
            _ritualCompleted = true;
            me->DespawnOrUnsummon(1000);
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50915);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(210985, me);
                    Talk(2);
                    Talk(3);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == WAYPOINT_MOTION_TYPE)
            {
                if (id == 8)
                    Talk(4);
                if (id == 9)
                {
                    bool firstArrival = !_mawWaypointArrived;
                    _mawWaypointArrived = true;
                    if (firstArrival)
                        Talk(5);
                    CompletePrisonWaypoints();
                    if (firstArrival && !_mawWaypointCredited)
                        TC_LOG_WARN("scripts", "Silver Hand waypoint9 has no eligible stage5 participant; retaining observed arrival for credit retry");
                }
                if (id == 19)
                {
                    bool firstArrival = !_prisonWaypointArrived;
                    _prisonWaypointArrived = true;
                    if (firstArrival)
                        Talk(6);
                    CompletePrisonWaypoints();
                    if (firstArrival && !_prisonWaypointCredited)
                        TC_LOG_WARN("scripts", "Silver Hand waypoint19 has no eligible stage6 participant; retaining observed arrival for credit retry");
                }
            }
            else if (type == POINT_MOTION_TYPE)
            {
                if (id == 1 && _ritualMotionIssued && !_ritualArrived)
                {
                    _ritualArrived = true;
                    CompleteRitualArrival();
                    if (!_ritualCompleted)
                        TC_LOG_WARN("scripts", "Silver Hand ritual arrival has no eligible stage8 participant; retaining Travard for credit retry");
                }
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who ? who->ToPlayer() : nullptr;
            if (_introDone || !SilverHandRitualEntry::CanEnter(player, me))
                return;
            _introDone = true;
            _entryPending = true;
            ObjectGuid guid = player->GetGUID();
            uint64 generation = _entryGeneration;
            _entryFunctions.AddDelayedEvent(1000, [guid, generation, this]() -> void
            {
                if (!_entryPending || generation != _entryGeneration)
                    return;
                _entryPending = false;
                Player* player = ObjectAccessor::GetPlayer(*me, guid);
                if (!SilverHandRitualEntry::CanEnter(player, me))
                {
                    _introDone = false;
                    return;
                }
                WaypointPath const* path = sWaypointMgr->GetPathScript(10993807);
                if (!path || path->empty())
                {
                    _introDone = false;
                    TC_LOG_WARN("scripts", "Silver Hand ritual entry path 10993807 is unavailable; retaining entry for retry");
                    return;
                }
                Talk(0);
                if (Creature* efrin = me->FindNearestCreature(106371, 30.0f, true))
                    efrin->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(- PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
                if (Creature* duval = me->FindNearestCreature(106370, 30.0f, true))
                    duval->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
                me->GetMotionMaster()->MovePath(10993807, false);
            });
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
                        DoCast(215689);
                        events.RescheduleEvent(EVENT_1, 5000); // 215689
                        break;
                    case EVENT_2:
                        DoCast(210371);
                        events.RescheduleEvent(EVENT_2, 8000); // 210371
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        void VisualOpenDoor()
        {
            std::list<Creature*> adds;
            GetCreatureListWithEntryInGrid(adds, me, 106094, 5.0f);
            if (!adds.empty())
                for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                {
                    (*itr)->RemoveAura(200181);
                    (*itr)->RemoveAura(204891);
                }

            std::list<GameObject*> gob;
            GetGameObjectListWithEntryInGrid(gob, me, 251349, 5.0f);
            if (!gob.empty())
                for (std::list<GameObject*>::iterator itr = gob.begin(); itr != gob.end(); ++itr)
                    (*itr)->SetPhaseMask(2, true); // hide   
        }
    };
};

namespace SilverHandAberration
{
    bool CanCredit(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || player->IsPreDelete() || !player->CanContact() || player->isGameMaster() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || actor->IsPreDelete() || actor->IsAlive() || actor->GetEntry() != 106669 || actor->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->IsWithinDistInMap(actor, 100.0f) ||
            player->GetQuestRewardStatus(42120) || player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 5;
    }
}

// 106669
class npc_horrific_aberration : public CreatureScript
{
public:
    npc_horrific_aberration() : CreatureScript("npc_horrific_aberration") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_horrific_aberrationAI(creature);
    }

    struct npc_horrific_aberrationAI : ScriptedAI
    {
        npc_horrific_aberrationAI(Creature* creature) : ScriptedAI(creature) {}

        EventMap events;
        bool outroDone;
        bool _deathCreditDone = false;

        void Reset() override
        {
            events.Reset();
            outroDone = false;
            _deathCreditDone = false;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 30000); // 215797
            events.RescheduleEvent(EVENT_2, 2500); //  215690
        }

        void JustDied(Unit* who) override
        {
            if (_deathCreditDone || !who || !me->GetMap() || !who->IsInWorld() || who->GetMap() != me->GetMap())
                return;
            Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (!SilverHandAberration::CanCredit(player, me))
            {
                player = nullptr;
                for (auto const& reference : me->GetMap()->GetPlayers())
                    if (SilverHandAberration::CanCredit(reference.getSource(), me))
                    {
                        player = reference.getSource();
                        break;
                    }
            }
            if (!player)
                return;
            _deathCreditDone = true;
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50896);
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (me->HealthBelowPct(50) && !outroDone)
            {
                outroDone = true;
                events.RescheduleEvent(EVENT_3, 1000);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED))
                return;

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
                        DoCast(215797);
                        events.RescheduleEvent(EVENT_1, 30000); // 215797
                        break;
                    case EVENT_2:
                        DoCast(215690);
                        events.RescheduleEvent(EVENT_2, 2500); //  215690
                        break;
                    case EVENT_3:
                        DoCast(169871);
                        if (Creature* efrin = me->FindNearestCreature(106371, 30.0f, true))
                        {
                            me->Kill(efrin);
                            efrin->DespawnOrUnsummon(3000);
                        }
                        if (Creature* duval = me->FindNearestCreature(106370, 30.0f, true))
                        {
                            me->Kill(duval);
                            duval->DespawnOrUnsummon(3000);
                        }
                        if (Creature* travar = me->FindNearestCreature(106676, 30.0f, true))
                            travar->AI()->Talk(1);
                        break;

                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

// 106371, 106370, 106774
class npc_efrin_and_duval : public CreatureScript
{
public:
    npc_efrin_and_duval() : CreatureScript("npc_efrin_and_duval") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_efrin_and_duvalAI(creature);
    }

    struct npc_efrin_and_duvalAI : ScriptedAI
    {
        npc_efrin_and_duvalAI(Creature* creature) : ScriptedAI(creature)
        {
            wait_heal = false;
        }

        EventMap events;
        bool wait_heal;
        bool _pendingHealRecovery = false;

        void Reset() override
        {
            events.Reset();
            _pendingHealRecovery = false;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 4500); // 212290
            events.RescheduleEvent(EVENT_2, 9300); // 210369
            events.RescheduleEvent(EVENT_3, 7000); // 210371
        }

        void HealReceived(Unit*, uint32& addhealth) override
        {
            if (wait_heal && addhealth)
                _pendingHealRecovery = true;
        }

        void UpdateLifecycle(uint32) override
        {
            SilverHandCompanion::Recover(me, _pendingHealRecovery, wait_heal);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth(attacker))
            {
                damage = 0;
                if (!wait_heal)
                {
                    _pendingHealRecovery = false;
                    wait_heal = true;
                    me->AddAura(210985, me);
                    Talk(0);
                    Talk(1);
                    me->AddUnitState(UNIT_STATE_STUNNED);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED))
                return;

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
                        DoCast(212290);
                        events.RescheduleEvent(EVENT_1, 4500); // 212290
                        break;
                    case EVENT_2:
                        DoCast(210369);
                        events.RescheduleEvent(EVENT_2, 9300); // 210369
                        break;
                    case EVENT_3:
                        DoCast(210371);
                        events.RescheduleEvent(EVENT_3, 7000); // 210371
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

namespace SilverHandRescue
{
    bool CanCredit(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || player->IsPreDelete() || !player->CanContact() || player->isGameMaster() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() || actor->GetMapId() != 1539 ||
            (actor->GetEntry() != 106933 && actor->GetEntry() != 106721 && actor->GetEntry() != 106722) ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->IsWithinDistInMap(actor, 100.0f) ||
            player->GetQuestRewardStatus(42120) || player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 7;
    }

    Player* SelectPlayer(Player* preferred, Creature* actor)
    {
        if (CanCredit(preferred, actor))
            return preferred;
        if (!actor || !actor->GetMap())
            return nullptr;
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (CanCredit(reference.getSource(), actor))
                return reference.getSource();
        return nullptr;
    }
}

// 106933, 106721, 106722
class npc_silverhand_events : public CreatureScript
{
public:
    npc_silverhand_events() : CreatureScript("npc_silverhand_events") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_silverhand_eventsAI(creature);
    }

    struct npc_silverhand_eventsAI : ScriptedAI
    {
        npc_silverhand_eventsAI(Creature* creature) : ScriptedAI(creature)
        {
            wait_heal = false;
        }

        bool wait_heal;
        uint64 _healGeneration = 0;
        FunctionProcessor _healFunctions;

        void Reset() override
        {
            ++_healGeneration;
        }

        void UpdateLifecycle(uint32 diff) override
        {
            _healFunctions.Update(diff);
        }

        void HealReceived(Unit* who, uint32& addhealth) override
        {
            if (wait_heal || !addhealth || !who || !who->IsInWorld() || who->GetMap() != me->GetMap())
                return;
            Player* player = SilverHandRescue::SelectPlayer(who->GetCharmerOrOwnerPlayerOrPlayerItself(), me);
            if (!player)
                return;
            ObjectGuid guid = player->GetGUID();
            uint64 generation = _healGeneration;
            _healFunctions.AddDelayedEvent(0, [guid, generation, this]() -> void
            {
                if (wait_heal || generation != _healGeneration || me->GetHealthPct() < 90)
                    return;
                Player* player = SilverHandRescue::SelectPlayer(ObjectAccessor::GetPlayer(*me, guid), me);
                if (!player)
                    return;
                wait_heal = true;
                me->ClearUnitState(UNIT_STATE_STUNNED);
                me->RemoveAura(211699);
                me->RemoveAura(211698);
                me->DespawnOrUnsummon(3000);
                me->GetMotionMaster()->MovePoint(0, 1875.49f, 2329.93f, 49.64f);
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, (me->GetEntry() == 106933 ? 50913 : 50914));
            });
        }
    };
};

namespace SilverHandFinalEntry
{
    bool CanStart(Player* player, Creature* actor)
    {
        if (!player || !actor || !player->IsInWorld() || player->IsPreDelete() || !player->CanContact() ||
            !player->IsAlive() || player->isGameMaster() || player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() || actor->GetEntry() != 106777 || actor->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != actor->GetMap() || player->GetInstanceId() != actor->GetInstanceId() ||
            !player->InSamePhase(actor) || !player->canSeeOrDetect(actor) || !player->IsWithinLOSInMap(actor) ||
            !player->IsWithinDistInMap(actor, 100.0f) || player->GetQuestRewardStatus(42120) ||
            player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 9;
    }

    Player* SelectPlayer(Creature* actor)
    {
        if (!actor || !actor->GetMap())
            return nullptr;
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (CanStart(reference.getSource(), actor))
                return reference.getSource();
        return nullptr;
    }

    bool CanStartWave(Creature* actor)
    {
        if (!actor || !actor->IsInWorld() || actor->IsPreDelete() || !actor->IsAlive() ||
            actor->GetEntry() != 106777 || actor->GetMapId() != 1539 || !actor->GetMap())
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1092 && scenario->GetCurrentStep() == 9;
    }
}

// 106777
class npc_travar_last : public CreatureScript
{
public:
    npc_travar_last() : CreatureScript("npc_travar_last") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_travar_lastAI(creature);
    }

    struct npc_travar_lastAI : ScriptedAI
    {
        npc_travar_lastAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            startCriteria = false;
            timer = 1000;
            count_died = 0;
        }

        bool _introDone, startCriteria;
        uint32 timer;
        uint8 count_died;
        FunctionProcessor _waveFunctions;
        GuidSet _waveSummons;

        void UpdateLifecycle(uint32 diff) override
        {
            _waveFunctions.Update(diff);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (_introDone)
                return;

            Player* player = who ? who->ToPlayer() : nullptr;
            if (SilverHandFinalEntry::CanStart(player, me))
            {
                _introDone = true;
                startCriteria = true;
                DoCast(215258);
                Talk(0);
                std::list<Creature*> adds;
                GetCreatureListWithEntryInGrid(adds, me, 106094, 30.0f);
                uint8 count = 0;
                if (!adds.empty())
                    for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                    {
                        if ((*itr)->HasAura(211502))
                        {
                            (*itr)->SetVisible(true);
                            count++;
                        }
                    }
                _waveFunctions.AddDelayedEvent(5000, [this]() -> void
                {
                    if (!SilverHandFinalEntry::CanStartWave(me))
                    {
                        _introDone = false;
                        startCriteria = false;
                        TC_LOG_DEBUG("scripts", "Silver Hand ritual: pending first wave context changed; rearming entry without spawning");
                        return;
                    }
                    SummonAdds(1);
                });
            }
        }

        void JustSummoned(Creature* summon) override
        {
            if (!_introDone || !summon || !summon->IsAlive() ||
                (summon->GetEntry() != 106580 && summon->GetEntry() != 106578))
                return;
            TempSummon* temporary = summon->ToTempSummon();
            if (!temporary || temporary->GetSummonerGUID() != me->GetGUID() || !me->GetMap() ||
                summon->GetMap() != me->GetMap() || summon->GetInstanceId() != me->GetInstanceId())
                return;
            _waveSummons.insert(summon->GetGUID());
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            if (!summon || summon->IsAlive() || !me->IsInWorld() || me->IsPreDelete() || !me->IsAlive() ||
                count_died >= 6 || !_waveSummons.erase(summon->GetGUID()))
                return;
            count_died++;
            if ((count_died == 3 || count_died == 6) && !SilverHandFinalEntry::CanStartWave(me))
            {
                TC_LOG_DEBUG("scripts", "Silver Hand ritual: dropping stale follow-up wave at death count %u", uint32(count_died));
                return;
            }
            switch (count_died)
            {
                case 3:
                    SummonAdds(2);
                    break;
                case 6:
                    SummonAdds(3);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (me->HasUnitState(UNIT_STATE_STUNNED))
                return;

            if (UpdateVictim())
                return;

            if (startCriteria)
                if (timer <= diff)
                {
                    if (Player* pl = SilverHandFinalEntry::SelectPlayer(me))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 51838);
                    timer = 1000;
                }
                else
                    timer -= diff;
        }

        void RetryWave(uint8 wave, uint32 unavailableEntry)
        {
            TC_LOG_DEBUG("scripts", "Silver Hand ritual: actor %u unavailable for wave %u; retry in1000ms", unavailableEntry, uint32(wave));
            _waveFunctions.AddDelayedEvent(1000, [this, wave]() -> void
            {
                if (SilverHandFinalEntry::CanStartWave(me))
                    SummonAdds(wave);
                else
                    TC_LOG_DEBUG("scripts", "Silver Hand ritual: dropping retry for stale wave %u", uint32(wave));
            });
        }

        void SummonAdds(uint8 wave)
        {
            switch (wave)
            {
                case 1:
                case 2:
                {
                    if (Creature* defenders = me->FindNearestCreature(106774, 70.0f, true))
                    {
                        if (Creature* targ = me->SummonCreature(106580, 1911.84f, 2334.61f, 73.39f, 0.0f))
                        {
                            JustSummoned(targ);
                            targ->AI()->AttackStart(defenders);
                            for (uint8 i = 0; i < 2; ++i)
                            {
                                if (Creature* add = me->SummonCreature(106578, targ->GetPositionX() + irand(-3, 3), targ->GetPositionY() + irand(-2, 2), targ->GetPositionZ(), targ->GetOrientation()))
                                {
                                    JustSummoned(add);
                                    add->AI()->AttackStart(defenders);
                                }
                            }
                        }
                        else
                            RetryWave(wave, 106580);
                    }
                    else
                        RetryWave(wave, 106774);
                }
                break;
                case 3:
                {
                    if (Creature* defenders = me->FindNearestCreature(106774, 70.0f, true))
                    {
                        if (Creature* mordot = me->SummonCreature(106719, 1901.32f, 2334.41f, 62.75f, 0.0f))
                        {
                            mordot->AI()->AttackStart(defenders);
                            mordot->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                            mordot->AI()->Talk(0);
                        }
                        else
                            RetryWave(wave, 106719);
                    }
                    else
                        RetryWave(wave, 106774);
                }
                break;
            }
        }

    };
};

namespace SilverHandAcquisition
{
    bool CanTake(Player* player, GameObject* weapon)
    {
        if (!player || !weapon || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN ||
            !weapon->IsInWorld() || !weapon->isSpawned() || weapon->GetEntry() != 249824 || weapon->GetMapId() != 1539 ||
            !player->GetMap() || player->GetMap() != weapon->GetMap() || player->GetInstanceId() != weapon->GetInstanceId() ||
            !player->InSamePhase(weapon) || !player->canSeeOrDetect(weapon) || !player->IsWithinLOSInMap(weapon) ||
            !player->IsWithinDistInMap(weapon, weapon->GetInteractionDistance()) ||
            weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) || player->GetQuestRewardStatus(42120) ||
            (player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42120) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(weapon->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1092)
            return false;
        return scenario->GetCurrentStep() == 10 ||
            (scenario->GetCurrentStep() == 11 && player->GetItemByEntry(128823));
    }

    bool CanReturn(Player* player, uint32 instanceId)
    {
        if (!player || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_PALADIN || !player->GetMap() || player->GetMapId() != 1539 ||
            player->GetInstanceId() != instanceId || player->GetQuestRewardStatus(42120) ||
            (player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42120) != QUEST_STATUS_COMPLETE) ||
            !player->HasSceneStatus(1267, SCENE_COMPLETE))
            return false;
        Item* weapon = player->GetItemByEntry(128823);
        if (!weapon || player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != weapon)
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(instanceId);
        return scenario && scenario->GetScenarioId() == 1092 && scenario->IsCompleted(false);
    }
}

// 249824
class go_art_silverhand : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        bool GossipHello(Player* player, bool) override
        {
            return !SilverHandAcquisition::CanTake(player, go) || go->getLootState() == GO_JUST_DEACTIVATED;
        }
    };

public:
    go_art_silverhand() : GameObjectScript("go_art_silverhand") {}

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!SilverHandAcquisition::CanTake(player, go) || go->getLootState() == GO_JUST_DEACTIVATED)
            return true;
        if (player->GetItemByEntry(128823))
        {
            go->SetLootState(GO_JUST_DEACTIVATED, player);
            return true;
        }
        return false;
    }
    GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }

    void OnLootStateChanged(GameObject* go, uint32 state, Unit* target) override
    {
        Player* player = target ? target->ToPlayer() : nullptr;
        if (!go || state != GO_JUST_DEACTIVATED || go->getLootState() != GO_JUST_DEACTIVATED ||
            !SilverHandAcquisition::CanTake(player, go))
            return;
        Item* weapon = player->GetItemByEntry(128823);
        if (!weapon || (player->HasSceneStatus(1267, SCENE_LAUNCH) && !player->HasSceneStatus(1267, SCENE_COMPLETE) &&
            player->HasAura(211838)))
            return;
        Scenario* scenario = sScenarioMgr->GetScenario(go->GetInstanceId());
        if (!scenario)
            return;
        if (scenario->IsCompleted(false) && player->HasSceneStatus(1267, SCENE_COMPLETE) &&
            player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == weapon)
            return;
        SpellCastResult result = player->CastSpell(player, 211838, true);
        if (result != SPELL_CAST_OK)
        {
            TC_LOG_WARN("scripts", "Silver Hand pickup: scene 211838 failed with result %u for player %u; retain existing item and reopen object", uint32(result), player->GetGUIDLow());
            go->SetLootState(GO_READY);
            return;
        }
        if (Creature* trigger = go->FindNearestCreature(106781, 10.0f))
            trigger->DespawnOrUnsummon(1000);
        uint32 instanceId = go->GetInstanceId();
        player->AddDelayedEvent(12000, [player, instanceId]() -> void
        {
            if (SilverHandAcquisition::CanReturn(player, instanceId))
            {
                SpellCastResult result = player->CastSpell(player, 211862);
                if (result != SPELL_CAST_OK)
                    TC_LOG_WARN("scripts", "Silver Hand pickup: return scene 211862 failed with result %u for player %u; no arrival credit granted", uint32(result), player->GetGUIDLow());
            }
        });
    }
};

namespace SilverHandCollapse
{
    bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
    {
        if (!scene || scene->MiscValue != 1268 || scene->SceneScriptPackageID != 1619)
            return false;
        if (trigger != "complete" || !player || !player->HasSceneStatus(1268, SCENE_COMPLETE) ||
            !player->HasSceneStatus(1267, SCENE_COMPLETE) || !player->HasAura(211862) ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
            player->getClass() != CLASS_PALADIN || !player->GetMap() || player->GetMapId() != 1539 ||
            player->GetQuestRewardStatus(42120) ||
            (player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42120) != QUEST_STATUS_COMPLETE))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1092 || !scenario->IsCompleted(false))
            return true;
        Item* weapon = player->GetItemByEntry(128823);
        if (!weapon || player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != weapon)
            return true;
        SpellTargetPosition const* destination = sSpellMgr->GetSpellTargetPosition(211867);
        if (!destination || destination->target_mapId != 1539)
        {
            TC_LOG_WARN("scripts", "Silver Hand collapse: missing or foreign-map 211867 destination for player %u; exit not cast", player->GetGUIDLow());
            return true;
        }
        SpellCastResult result = player->CastSpell(player, 211867, true);
        if (result != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Silver Hand collapse: native exit 211867 failed with result %u for player %u; no Dalaran arrival credit granted", uint32(result), player->GetGUIDLow());
        return true;
    }
}

class scene_silver_hand_collapse : public SceneTriggerScript
{
public:
    scene_silver_hand_collapse() : SceneTriggerScript("scene_silver_hand_collapse") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return SilverHandCollapse::Complete(player, scene, trigger);
    }
};

namespace SilverHandPresentation
{
    bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
    {
        if (!scene || scene->MiscValue != 1267 || scene->SceneScriptPackageID != 1618)
            return false;
        if (trigger != "complete" || !player || !player->HasSceneStatus(1267, SCENE_COMPLETE) ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->IsBeingTeleported() ||
            player->getClass() != CLASS_PALADIN || player->GetMapId() != 1539 || player->GetQuestRewardStatus(42120) ||
            (player->GetQuestStatus(42120) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(42120) != QUEST_STATUS_COMPLETE))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1092 || scenario->GetCurrentStep() < 10 || scenario->GetCurrentStep() > 11)
            return true;
        Item* weapon = player->GetItemByEntry(128823);
        if (!weapon)
            return true;
        if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != weapon)
        {
            uint16 destination;
            InventoryResult result = player->CanEquipItem(NULL_SLOT, destination, weapon, true);
            if (result != EQUIP_ERR_OK)
            {
                player->SendEquipError(result, weapon);
                return true;
            }
            if (destination != weapon->GetPos())
                player->SwapItem(weapon->GetPos(), destination);
            if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) != weapon)
            {
                TC_LOG_WARN("scripts", "Silver Hand pickup: native swap did not equip existing 128823 for player %u; presentation credit retained", player->GetGUIDLow());
                return true;
            }
        }
        SpellCastResult result = player->CastSpell(player, 211842, true);
        if (result != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Silver Hand pickup: native 211842 failed with result %u for player %u; existing equipped item retained", uint32(result), player->GetGUIDLow());
        return true;
    }
}

class scene_silver_hand_pickup : public SceneTriggerScript
{
public:
    scene_silver_hand_pickup() : SceneTriggerScript("scene_silver_hand_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return SilverHandPresentation::Complete(player, scene, trigger);
    }
};

void AddSC_SilverHand()
{
    new npc_travar_first();
    new npc_travar_second();
    new npc_travar_third();
    new npc_horrific_aberration();
    new npc_efrin_and_duval();
    new npc_silverhand_events();
    new npc_travar_last();
    new go_art_silverhand();
    new scene_silver_hand_pickup();
    new scene_silver_hand_collapse();
};
