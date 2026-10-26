/*
    Rogue: Devourer
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Item.h"

namespace DevourerAcquisition
{
    bool CanTake(Player* player, GameObject* weapon)
    {
        if (!player || !weapon || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->IsBeingTeleported() || player->getClass() != CLASS_ROGUE || !player->GetMap() ||
            !weapon->IsInWorld() || !weapon->isSpawned() || weapon->GetEntry() != 249347 || weapon->GetMapId() != 1607 ||
            player->GetMap() != weapon->GetMap() || player->GetInstanceId() != weapon->GetInstanceId() ||
            !player->InSamePhase(weapon) || !player->canSeeOrDetect(weapon) || !player->IsWithinLOSInMap(weapon) ||
            !player->IsWithinDistInMap(weapon, weapon->GetInteractionDistance()) ||
            weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) || player->GetQuestRewardStatus(41924) ||
            (player->GetQuestStatus(41924) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41924) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(weapon->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1078 || scenario->GetCurrentStep() != 6)
            return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto victory = progress->find(47369);
        return victory != progress->end() && victory->second.Counter; // Native final Akaari leaf50300, also for pet/guardian killers.
    }

    bool Equipped(Player* player)
    {
        Item* weapon = player->GetItemByEntry(128476);
        Item* child = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        return weapon && player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == weapon &&
            child && child->GetEntry() == 128479 && weapon->GetChildItem() == child->GetGUID();
    }
}

// 105464
class npc_valzuun : public CreatureScript
{
public:
    npc_valzuun() : CreatureScript("npc_valzuun") { }

    bool OnQuestAccept(Player* player, Creature* me, Quest const* quest) override
    {
        if (quest->GetQuestId() == 41924)
        {
            player->SetScenarioId(1078);
            if (Creature* targ = me->SummonCreature(105488, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation()))
            {
                ObjectGuid guid = player->GetGUID();
                targ->AddPlayerInPersonnalVisibilityList(guid);
                targ->AddDelayedEvent(1000, [targ]() -> void
                {
                    targ->AI()->Talk(0);
                    targ->SetWalk(true);
                    targ->GetMotionMaster()->MovePoint(0, -848.85f, 4405.71f, 716.94f);
                });

                targ->AddDelayedEvent(4000, [targ]() -> void
                {
                    targ->AI()->Talk(1);
                    targ->CastSpell(targ, 209056);
                });

                targ->AddDelayedEvent(11500, [targ, guid]() -> void
                {
                    targ->AI()->Talk(2);
                    targ->GetMotionMaster()->MovePoint(0, -851.50f, 4403.76f, 716.94f);
                    if (GameObject* go = targ->SummonGameObject(248901, -846.23f, 4409.01f, 716.97f, 1.8675f, 0.0f, 0.0f, 0.0f, 0.0f, 300))
                        go->AddPlayerInPersonnalVisibilityList(guid);
                    targ->DespawnOrUnsummon(3000);
                });
            }
        }

        return true;
    }

};

// 105537 105539 105538 105541
class npc_devourer_small_events : public CreatureScript
{
public:
    npc_devourer_small_events() : CreatureScript("npc_devourer_small_events") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_devourer_small_eventsAI(creature);
    }

    struct npc_devourer_small_eventsAI : ScriptedAI
    {
        npc_devourer_small_eventsAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            SetCanSeeEvenInPassiveMode(true);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        bool introDone;

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            {
                if (me->getVictim())
                    return;

                if (me->canStartAttack(who, false) && me->GetExactDist2d(who) <= 6.0f)
                    AttackStart(who);
            }

            if (me->GetExactDist2d(who) >= 15.0f)
                return;

            if (!me->isMoving())
            {
                if (me->GetExactDist2d(who) <= 7.0f)
                    me->SetFacingToObject(who);
                else
                    me->SetOrientation(me->GetHomePosition().GetOrientation());
            }
            if (introDone)
                return;

            if (me->GetEntry() != 105538)
                Talk(0);
            introDone = true;
        }

        void UpdateAI(uint32 diff) override
        {
            ScriptedAI::UpdateAI(diff);

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

    };
};


// 105536 first
class npc_akaari_shadowgore : public CreatureScript
{
public:
    npc_akaari_shadowgore() : CreatureScript("npc_akaari_shadowgore") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_akaari_shadowgoreAI(creature);
    }

    struct npc_akaari_shadowgoreAI : ScriptedAI
    {
        npc_akaari_shadowgoreAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            SetCanSeeEvenInPassiveMode(true);
        }

        bool introDone;
        EventMap events;
        ObjectGuid capturePlayer;
        ObjectGuid captureHelper;
        std::weak_ptr<uint8> captureLifetime;
        bool capturePending = false;
        bool captureComplete = false;

        bool CanCapture(Player* player) const
        {
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            QuestStatusData* status = player ? player->getQuestStatus(41924) : nullptr;
            return player && me->IsInWorld() && !me->IsPreDelete() && me->IsAlive() &&
                (capturePlayer.IsEmpty() || (player->GetGUID() == capturePlayer && !captureLifetime.expired() &&
                    status && status->ScriptLifetime == captureLifetime.lock())) && player->CanContact() && player->IsAlive() && !player->HasPendingTeleport() &&
                player->getClass() == CLASS_ROGUE && player->GetQuestStatus(41924) == QUEST_STATUS_INCOMPLETE &&
                me->GetMapId() == 1607 && player->GetMap() == me->GetMap() && me->InSamePhase(player) &&
                me->IsWithinDistInMap(player, 40.0f) && scenario && scenario->GetScenarioId() == 1078 &&
                scenario->GetCurrentStep() == 0;
        }

        void PresentCapture()
        {
            Talk(3);
            Creature* helper = ObjectAccessor::GetCreature(*me, captureHelper);
            Player* player = ObjectAccessor::GetPlayer(*me, capturePlayer);
            if (helper && helper->IsInWorld() && helper->IsAlive() && helper->GetGUID() == captureHelper && helper->GetEntry() == 105540 &&
                me->InSamePhase(helper) && CanCapture(player))
                helper->CastSpell(player, 209264);
            else
                TC_LOG_WARN("scripts", "Devourer capture: optional helper105540 unavailable at5s; retain10s transfer");
        }

        void FinishCapture(Player* player)
        {
            if (!CanCapture(player))
            {
                TC_LOG_WARN("scripts", "Devourer capture: participant changed during presentation; reset encounter");
                EnterEvadeMode();
                return;
            }
            // Consume this attempt before a teleport/cast/criteria callback can re-enter.
            capturePending = false;
            captureComplete = true;
            events.Reset();
            if (!player->TeleportTo(1607, 1847.77f, 1259.92f, 57.09f, 0.0f) ||
                !player->IsBeingTeleported() || player->GetTeleportDest().GetMapId() != 1607 ||
                player->GetTeleportDest().GetExactDist(1847.77f, 1259.92f, 57.09f) > 0.1f)
            {
                // TeleportTo's legacy wrapper can return true even if SafeTeleport rejects it.
                TC_LOG_WARN("scripts", "Devourer capture: transfer not accepted; reset encounter without credit");
                EnterEvadeMode();
                return;
            }
            player->CastSpell(player, 211686); // native disarm
            player->CastSpell(player, 211684); // native stun
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50261);
            me->DespawnOrUnsummon();
        }

        void Reset() override
        {
            events.Reset();
            if (Player* player = ObjectAccessor::GetPlayer(*me, capturePlayer))
                if (player->GetGUID() == capturePlayer)
                {
                    player->RemoveAurasDueToSpell(209238, me->GetGUID());
                    if (!captureHelper.IsEmpty())
                        player->RemoveAurasDueToSpell(209264, captureHelper);
                }
            if (Creature* helper = ObjectAccessor::GetCreature(*me, captureHelper))
                if (helper->GetGUID() == captureHelper && helper->GetEntry() == 105540)
                    helper->DespawnOrUnsummon();
            capturePlayer.Clear();
            captureHelper.Clear();
            captureLifetime.reset();
            capturePending = false;
            captureComplete = false;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void EnterCombat(Unit* who) override
        {
            if (capturePending || captureComplete || !capturePlayer.IsEmpty())
                return;
            Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (CanCapture(player))
            {
                // Native quest lease expires on abandon/reaccept, even between AI ticks.
                player->AddQuestDelayedEvent(41924, 0, [] { });
                if (QuestStatusData* status = player->getQuestStatus(41924))
                {
                    captureLifetime = status->ScriptLifetime;
                    if (!captureLifetime.expired())
                        capturePlayer = player->GetGUID();
                }
            }
            Talk(0);
            events.RescheduleEvent(EVENT_1, 12000); // Clons 209231 and talk 
        }


        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            Player* player = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            Scenario* scenario = sScenarioMgr->GetScenario(me->GetInstanceId());
            if (!player || player->getClass() != CLASS_ROGUE || !player->IsInWorld() ||
                player->GetMap() != me->GetMap() || !player->InSamePhase(me) ||
                player->GetQuestStatus(41924) != QUEST_STATUS_INCOMPLETE || me->GetMapId() != 1607 ||
                !scenario || scenario->GetScenarioId() != 1078 || scenario->GetCurrentStep() != 0)
                return;

            // This opening encounter ends with the existing timed capture,
            // not Akaari's death. Keep the original timed transition and effects.
            uint64 health = me->GetHealth(attacker);
            if (damage >= health)
                damage = health > 1 ? uint32(health - 1) : 0;
        }

        void MoveInLineOfSight(Unit* who) override
        {

            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetExactDist2d(who) <= 25.0f)
                if (!introDone)
                {
                    Conversation* conversation = new Conversation;
                    if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1864, who, NULL, *who))
                        delete conversation;
                    introDone = true;
                }
        }

        void UpdateAI(uint32 diff) override
        {

            if (captureComplete)
                return;
            if (capturePending)
            {
                Player* player = ObjectAccessor::GetPlayer(*me, capturePlayer);
                if (!CanCapture(player))
                {
                    TC_LOG_WARN("scripts", "Devourer capture: original participant no longer eligible; reset encounter");
                    EnterEvadeMode();
                    return;
                }
                // Capture is independent of a combat victim, casts and stuns.
                events.Update(diff);
                while (uint32 eventId = events.ExecuteEvent())
                {
                    if (eventId == EVENT_3)
                        PresentCapture();
                    else if (eventId == EVENT_4)
                    {
                        FinishCapture(player);
                        return;
                    }
                }
                return;
            }
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
                        Talk(1);
                        DoCast(209231);
                        events.RescheduleEvent(EVENT_2, 11000);
                        break;
                    case EVENT_2:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, capturePlayer);
                        if (!CanCapture(player))
                        {
                            TC_LOG_WARN("scripts", "Devourer capture: no eligible combat participant; reset encounter");
                            EnterEvadeMode();
                            return;
                        }
                        capturePending = true;
                        events.Reset();
                        events.ScheduleEvent(EVENT_3, 5000);
                        events.ScheduleEvent(EVENT_4, 10000);
                        Talk(2);
                        DoCast(209238);
                        me->SetReactState(REACT_PASSIVE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                        if (Creature* helper = me->SummonCreature(105540, me->GetPositionX() + 3.0f, me->GetPositionY() + 3.0f, me->GetPositionZ()))
                        {
                            captureHelper = helper->GetGUID();
                            helper->CastSpell(helper, 189170);
                            helper->DespawnOrUnsummon(9000);
                        }
                        else
                            TC_LOG_WARN("scripts", "Devourer capture: optional helper105540 summon failed; retain10s transfer");
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 249809 248992 249390
class go_devourer_misc : public GameObjectScript
{
public:
    go_devourer_misc() : GameObjectScript("go_devourer_misc") { }

    struct go_devourer_miscAI : public GameObjectAI
    {
        go_devourer_miscAI(GameObject* go) : GameObjectAI(go) {}

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!isUse)
                return true;

            if (player->HasItemCount(136802, 1))
            {
                uint32 criteria = 0;
                switch (go->GetEntry())
                {
                    case 249809:
                        criteria = 50908;
                        player->RemoveAura(211684);
                        break;
                    case 249390:
                        criteria = 50536;
                        break;
                }
                if (criteria != 0)
                    player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, criteria);

                go->SetPhaseMask(4, true); // hide
                if (go->GetEntry() == 249390)
                {
                    if (GameObject* doors = go->FindNearestGameObject(249391, 10.0f))
                        doors->SetPhaseMask(4, true);
                    player->DestroyItemCount(136802, 1, true); // last go. del quest item ?

                    std::list<Creature*> adds;
                    GetCreatureListWithEntryInGrid(adds, player, 105538, 90.0f);
                    GetCreatureListWithEntryInGrid(adds, player, 105537, 90.0f);
                    if (!adds.empty())
                        for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                        {
                            if ((*itr)->HasAura(209349))
                            {
                                (*itr)->GetMotionMaster()->MoveRandom(15.0f);
                                (*itr)->DespawnOrUnsummon(5000);
                                if ((*itr)->GetEntry() == 105538)
                                    (*itr)->AI()->Talk(0);

                            }
                        }
                }
                return true;
            }
            return false;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_devourer_miscAI(go);
    }
};

// 105542
class npc_devourer_zirus : public CreatureScript
{
public:
    npc_devourer_zirus() : CreatureScript("npc_devourer_zirus") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_devourer_zirusAI(creature);
    }

    struct npc_devourer_zirusAI : ScriptedAI
    {
        npc_devourer_zirusAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            introDone = false;
            SetCanSeeEvenInPassiveMode(true);
        }

        bool introDone;
        EventMap events;
        SummonList summons;

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 8000); // 194519 
            events.RescheduleEvent(EVENT_2, 11000);
        }

        void JustDied(Unit* who) override
        {
            Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            Unit* source = player ? static_cast<Unit*>(player) : me;
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1890, source, NULL, *source))
                delete conversation;
            if (player)
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50298);

            std::list<GameObject*> doors;
            GetGameObjectListWithEntryInGrid(doors, me, 248983, 100.0f);
            GetGameObjectListWithEntryInGrid(doors, me, 248984, 100.0f);
            GetGameObjectListWithEntryInGrid(doors, me, 248910, 100.0f);
            GetGameObjectListWithEntryInGrid(doors, me, 249028, 100.0f);
            if (!doors.empty())
                for (std::list<GameObject*>::iterator itr = doors.begin(); itr != doors.end(); ++itr)
                {
                    (*itr)->SetGoState(GO_STATE_ACTIVE);
                    (*itr)->SetPhaseMask(4, true);
                }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
            summon->CastSpell(summon, 210161, false);
            summon->GetMotionMaster()->MoveRotate(20000, ROTATE_DIRECTION_RIGHT);
        }

        void MoveInLineOfSight(Unit* who) override
        {

            if (who->GetTypeId() != TYPEID_PLAYER)
                return;

            if (me->GetExactDist2d(who) <= 25.0f)
                if (!introDone)
                {
                    Conversation* conversation = new Conversation;
                    if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1884, who, NULL, *who))
                        delete conversation;
                    introDone = true;
                }
        }

        void UpdateAI(uint32 diff) override
        {

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
                        DoCast(194519);
                        events.RescheduleEvent(EVENT_1, 8000);
                        break;
                    case EVENT_2:
                        float radius = 5.0f; // растояние от кастера
                        float coneAngle = 0.0f; // угол куда поставится моб с АТ
                        Position position = me->GetNearPosition(radius, coneAngle);
                        me->CastSpell(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), 210157, true);
                        coneAngle = 2.0f; // угол куда поставится моб с АТ
                        position = me->GetNearPosition(radius, coneAngle);
                        me->CastSpell(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), 210157, true);
                        coneAngle = 4.0f; // угол куда поставится моб с АТ
                        position = me->GetNearPosition(radius, coneAngle);
                        me->CastSpell(position.GetPositionX(), position.GetPositionY(), position.GetPositionZ(), 210157, true);
                        events.RescheduleEvent(EVENT_2, 11000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 105660 second
class npc_akaari_shadowgore_last : public CreatureScript
{
public:
    npc_akaari_shadowgore_last() : CreatureScript("npc_akaari_shadowgore_last") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_akaari_shadowgore_lastAI(creature);
    }

    struct npc_akaari_shadowgore_lastAI : ScriptedAI
    {
        npc_akaari_shadowgore_lastAI(Creature* creature) : ScriptedAI(creature)
        {
            introDone = false;
            SetCanSeeEvenInPassiveMode(true);
            convers = 1943;
            instance = me->GetInstanceScript();
            checktimer = 1000;
        }

        InstanceScript* instance;
        bool introDone;
        EventMap events;
        uint32 convers;
        uint32 checktimer;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 42000);
            events.RescheduleEvent(EVENT_2, 11000);
            events.RescheduleEvent(EVENT_3, 14000);
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1942, who, NULL, *who))
                delete conversation;
            convers = 1943;
        }

        void JustDied(Unit* who) override
        {
            Player* player = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
            if (!player)
                return;

            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1947, player, NULL, *player))
                delete conversation;

            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50300);
        }

        /* void MoveInLineOfSight(Unit* who)
         {

             if (who->GetTypeId() != TYPEID_PLAYER)
                 return;

             if (me->GetExactDist2d(who) <= 25.0f)
                 if(!introDone)
                 {
                     who->CastSpell(who, 209626);
                     who->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50404);
                     who->ToPlayer()->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50299);
                     introDone = true;
                 }
         }
         */
        void UpdateAI(uint32 diff) override
        {
            if (instance->getScenarionStep() == 4)
            {
                if (!introDone)
                {
                    if (checktimer <= diff)
                    {
                        if (Player* pl = me->FindNearestPlayer(25.0f, true))
                        {
                            if (pl->GetPositionZ() < 91.0f)
                                return;

                            pl->CastSpell(pl, 209626);
                            pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50404);
                            pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50299);
                            introDone = true;
                        }
                        checktimer = 1000;
                    }
                    else checktimer -= diff;
                }
            }

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
                        DoCast(206866);
                        events.RescheduleEvent(EVENT_1, 42000);
                        break;
                    case EVENT_2:
                        DoCast(224990);
                        events.RescheduleEvent(EVENT_2, 11000);
                        break;
                    case EVENT_3:
                        DoCast(209233);
                        convers++;
                        if (convers < 1947)
                        {
                            Conversation* conversation = new Conversation;
                            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), convers, me, NULL, *me))
                                delete conversation;
                        }
                        events.RescheduleEvent(EVENT_3, 14000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


//249347
class go_art_devourer : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        ObjectGuid claimant;
        uint32 elapsed = 0;
        bool finished = false;

        void Cancel()
        {
            claimant.Clear();
        }

        bool GossipHello(Player* player, bool) override
        {
            return finished || !claimant.IsEmpty() || !DevourerAcquisition::CanTake(player, go);
        }
        ObjectGuid GetGUID(int32 = 0) override { return claimant; }
        uint32 GetData(uint32) override { return finished; }
        void SetGUID(ObjectGuid const& guid, int32 = 0) override
        {
            if (finished || !claimant.IsEmpty())
                return;
            Player* player = ObjectAccessor::GetPlayer(*go, guid);
            if (!DevourerAcquisition::CanTake(player, go) || !player->GetItemByEntry(128476))
                return;
            // Native209816 starts scene1257 and209819; the latter equips the
            // real looted parent/child through209818 at its native2000ms tick.
            claimant = guid;
            elapsed = 0;
            SpellCastResult result = player->CastSpell(player, 209816, true);
            if (result != SPELL_CAST_OK)
            {
                Cancel();
                TC_LOG_WARN("scripts", "Devourer pickup: native scene209816 failed with result %u; retain looted item and object for retry", uint32(result));
            }
        }
        void UpdateAI(uint32 diff) override
        {
            if (claimant.IsEmpty() || finished)
                return;
            Player* player = ObjectAccessor::GetPlayer(*go, claimant);
            if (!DevourerAcquisition::CanTake(player, go) || !player->GetItemByEntry(128476))
            {
                Cancel();
                return;
            }
            elapsed += diff;
            if (!DevourerAcquisition::Equipped(player))
            {
                // Bound a failed native equip by the actual15s scene duration.
                // Keep native inventory errors; never grant/equip a replacement.
                if (elapsed >= 15000)
                {
                    Cancel();
                    TC_LOG_WARN("scripts", "Devourer pickup: native209818 did not equip the looted parent/child; retain item and object for retry");
                }
                return;
            }
            finished = true;
            claimant.Clear();
            // This is the native final step's real pickup event. Let Scenario
            // award completion; never issue a duplicate COMPLETE_SCENARIO.
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50301);
            if (Creature* trigger = go->FindNearestCreature(105843, 10.0f))
                trigger->DespawnOrUnsummon(1000);
            Conversation* conversation = new Conversation;
            if (!conversation->CreateConversation(sObjectMgr->GetGenerator<HighGuid::Conversation>()->Generate(), 1948, player, NULL, *player))
                delete conversation;
            go->SetLootState(GO_JUST_DEACTIVATED);
        }
    };
public:
    go_art_devourer() : GameObjectScript("go_art_devourer") {}

    GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }
    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!DevourerAcquisition::CanTake(player, go) || go->AI()->GetData(0) || !go->AI()->GetGUID(0).IsEmpty())
            return true;
        if (player->HasItemCount(128476, 1, true))
        {
            if (player->GetItemByEntry(128476))
                go->SetLootState(GO_JUST_DEACTIVATED, player);
            return true; // Bank-only ownership must not create another weapon.
        }
        return false; // Normal loot/storage, including full-bag errors and retry.
    }

    void OnLootStateChanged(GameObject* go, uint32 state, Unit* target) override
    {
        Player* player = target ? target->ToPlayer() : nullptr;
        if (!go || state != GO_JUST_DEACTIVATED || go->getLootState() != GO_JUST_DEACTIVATED ||
            !DevourerAcquisition::CanTake(player, go) || !player->GetItemByEntry(128476))
            return;
        // A summoned GO would otherwise be deleted on its next update, before
        // the native equip tick or a failed cast/storage attempt can be retried.
        go->SetLootState(GO_READY);
        go->AI()->SetGUID(player->GetGUID(), 0);
    }
};


void AddSC_Devourer()
{
    new npc_valzuun();
    new npc_devourer_small_events();
    new npc_akaari_shadowgore();
    new go_devourer_misc();
    new npc_devourer_zirus();
    new npc_akaari_shadowgore_last();

    new go_art_devourer();
};
