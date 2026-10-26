/*
    Priest: Tuure
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "Scenario.h"
#include "ScenarioMgr.h"
#include "Item.h"
#include "Spell.h"
#include "SpellScript.h"
#include "ObjectAccessor.h"

Position const tppos[2] =
{
    {12.00f, 1189.50f, -45.98f, 0.0f},
    {58.69f, 1212.91f, -39.64f, 0.0f}
};

// 106134
class npc_vindicator_boros : public CreatureScript
{
public:
    npc_vindicator_boros() : CreatureScript("npc_vindicator_boros") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_vindicator_borosAI(creature);
    }

    struct npc_vindicator_borosAI : ScriptedAI
    {
        npc_vindicator_borosAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            _introHealDone = false;
            firstfighting = false;
            wait_heal = false;
            firstwp = false;
            secondwp = false;
        }

        EventMap events;
        bool _introDone, _introHealDone, firstfighting, wait_heal, firstwp, secondwp;
        FunctionProcessor _functions;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 4000);
            events.RescheduleEvent(EVENT_2, 9000);
        }

        void EnterEvadeMode() override
        {
            // need to activate it ONLY if creature isn't stunned
            _functions.AddFunction([this]() -> void
            {
                if (firstfighting)
                {
                    firstfighting = false;
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50562);
                    me->CreateConversation(2009);

                    me->AddDelayedEvent(16000, [this]() -> void
                    {
                        if (Creature* targ = me->FindNearestCreature(106221, 40.0f, true))
                            targ->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    });
                }
            }, _functions.CalculateTime(1000));
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
                        if (Creature* jeims = me->FindNearestCreature(106137, 20.0f, true))
                        {
                            jeims->GetMotionMaster()->MoveFollow(me, PET_FOLLOW_DIST, float(PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);
                            jeims->AI()->Talk(0);
                        }
                        me->GetMotionMaster()->MovePath(9859002, false);
                        firstwp = true;
                        me->CreateConversation(2039);
                    }, _functions.CalculateTime(1000));
                    break;
                }
                case 2:
                {
                    // need to activate it ONLY if creature isn't stunned
                    _functions.AddFunction([this]() -> void
                    {
                        me->GetMotionMaster()->MovePoint(3, 69.05f, 1220.01f, -39.70f);
                    }, _functions.CalculateTime(1000));
                    break;
                }
            }
        }

        void HealReceived(Unit* who, uint32& /*addhealth*/) override
        {
            if (!_introHealDone)
                if (me->GetHealthPct() >= 95)
                {
                    _introHealDone = true;
                    Talk(2);
                    me->SetByteValue(UNIT_FIELD_BYTES_1, 0, 0);
                    me->SetAnimKitId(0);
                    me->GetMotionMaster()->MovePoint(1, -13.92f, 1175.70f, -66.31f);
                    if (Player* pl = who->ToPlayer())
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50563);
                }

            if (wait_heal)
            {
                if (me->GetHealthPct() >= 40)
                {
                    me->ClearUnitState(UNIT_STATE_STUNNED);
                    me->RemoveAura(204237);
                    wait_heal = false;
                }
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;
                if (!wait_heal)
                {
                    wait_heal = true;
                    me->AddAura(204237, me);
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
                    if (id == 6)
                        me->CreateConversation(2007);
                    if (id == 10)
                        if (Player* pl = me->FindNearestPlayer(100.0f))
                        {
                            pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50831);
                            firstwp = false;
                        }
                }
                if (secondwp)
                {
                    if (id == 7)
                    {
                        if (Creature* jeims = me->FindNearestCreature(106137, 60.0f, true))
                            jeims->AI()->Talk(2);
                        // need to activate it ONLY if creature isn't stunned
                        _functions.AddFunction([this]() -> void
                        {
                            if (Creature* targ = me->FindNearestCreature(106318, 100.0f, true))
                                AttackStart(targ);
                        }, _functions.CalculateTime(4000));
                    }
                }
            }
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 1:
                        me->AddDelayedEvent(2000, [this]() -> void
                        {
                            DoCast(212629);
                            Talk(3);
                            if (GameObject* go = me->FindNearestGameObject(249469, 30.0f))
                                if (InstanceScript* inst = me->GetInstanceScript())
                                    inst->HandleGameObject(go->GetGUID(), true);
                            if (Creature* jeims = me->FindNearestCreature(106137, 100.0f, true))
                                if (Creature* targ = jeims->FindNearestCreature(106201, 40.0f, true))
                                    jeims->AI()->AttackStart(targ);

                        });

                        me->AddDelayedEvent(3000, [this]() -> void
                        {
                            me->GetMotionMaster()->MovePoint(2, 34.70f, 1201.05f, -65.06f);
                        });
                        break;
                    case 2:
                        Talk(4);
                        me->GetMotionMaster()->MoveJump(58.20f, 1214.60f, -74.35f, me->GetOrientation(), 10.0f, 10.0f);
                        if (Creature* targ = me->FindNearestCreature(106201, 60.0f, true))
                            AttackStart(targ);
                        firstfighting = true;
                        break;
                    case 3:
                        me->CreateConversation(2015);
                        // need to activate it ONLY if creature isn't stunned
                        _functions.AddFunction([this]() -> void
                        {
                            secondwp = true;
                            me->GetMotionMaster()->MovePath(9859003, false);
                            if (Creature* jeims = me->FindNearestCreature(106137, 70.0f, true))
                                jeims->AI()->Talk(1);
                        }, _functions.CalculateTime(8000));
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
                Talk(0, who->GetGUID());
                me->AddDelayedEvent(4000, [this]() -> void
                {
                    Talk(1);
                });
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
                        DoCast(211455);
                        events.RescheduleEvent(EVENT_1, 4000);
                        break;
                    case EVENT_2:
                        DoCast(210371);
                        events.RescheduleEvent(EVENT_2, 9000);
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


// 106137 106253
class npc_tuure_small_events : public CreatureScript
{
public:
    npc_tuure_small_events() : CreatureScript("npc_tuure_small_events") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tuure_small_eventsAI(creature);
    }

    struct npc_tuure_small_eventsAI : ScriptedAI
    {
        npc_tuure_small_eventsAI(Creature* creature) : ScriptedAI(creature)
        {
            wait_heal = false;
        }

        EventMap events;
        bool wait_heal;

        void Reset() override
        {
            events.Reset();
        }

        void EnterCombat(Unit* who) override
        {
            if (me->GetEntry() == 106253)
            {
                events.RescheduleEvent(EVENT_1, 4000); // 210477
                events.RescheduleEvent(EVENT_2, 11000); // 210478
                events.RescheduleEvent(EVENT_3, 15000); // 210479
            }
        }


        void HealReceived(Unit* who, uint32& /*addhealth*/) override
        {
            if (wait_heal)
            {
                if (me->GetHealthPct() >= 40)
                {
                    me->ClearUnitState(UNIT_STATE_STUNNED);
                    me->RemoveAura(204237);
                    wait_heal = false;
                }
            }
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;
                if (!wait_heal)
                {
                    wait_heal = true;
                    me->AddAura(204237, me);
                    Talk(10);
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
            if (me->HasUnitState(UNIT_STATE_STUNNED)) // need for this
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
                        DoCast(210477);
                        events.RescheduleEvent(EVENT_1, 4000); // 210477
                        break;
                    case EVENT_2:
                        DoCast(210478);
                        events.RescheduleEvent(EVENT_2, 11000); // 210478
                        break;
                    case EVENT_3:
                        DoCast(210479);
                        events.RescheduleEvent(EVENT_3, 15000); // 210479
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


enum Phases
{
    PHASE_COMBAT = 1,
    PHASE_REPHASE = 2
};

// 106107
class npc_capitan_naranot : public CreatureScript
{
public:
    npc_capitan_naranot() : CreatureScript("npc_capitan_naranot") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_capitan_naranotAI(creature);
    }

    struct npc_capitan_naranotAI : ScriptedAI
    {
        npc_capitan_naranotAI(Creature* creature) : ScriptedAI(creature)
        {
            me->setFaction(35);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
        }

        EventMap events;
        uint8 point;

        void Reset() override
        {
            events.Reset();
            me->SetReactState(REACT_DEFENSIVE);
        }

        void EnterCombat(Unit* who) override
        {
            Talk(1);
            point = 1;
            me->SetReactState(REACT_DEFENSIVE);
            events.SetPhase(PHASE_COMBAT);
            events.RescheduleEvent(EVENT_1, 8000, 0, PHASE_COMBAT); // 211480
         //   events.RescheduleEvent(EVENT_2, 25000, 0, PHASE_COMBAT); // rephase 211186 + 211190    
            if (Creature* targ = me->FindNearestCreature(106134, 100.0f, true))
                targ->AI()->AttackStart(me);
        }

        void JustDied(Unit* who) override
        {
            if (Creature* boros = me->FindNearestCreature(106134, 120.0f, true))
                boros->AI()->DoAction(2);
            if (Player* pl = me->FindNearestPlayer(100.0f))
                pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50634);
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
                        DoCast(211480);
                        events.RescheduleEvent(EVENT_1, 8000, 0, PHASE_COMBAT); // 211480
                        break;
                    case EVENT_2:
                    {
                        DoCast(211186);
                        std::list<Creature*> adds;
                        GetCreatureListWithEntryInGrid(adds, me, 106563, 10.0f);
                        if (!adds.empty())
                            for (std::list<Creature*>::iterator itr = adds.begin(); itr != adds.end(); ++itr)
                            {
                                (*itr)->CastSpell((*itr), 211194);
                            }

                        if (point >= 2)
                            point = 0;

                        me->SetReactState(REACT_PASSIVE);
                        me->CastSpell(tppos[point].GetPositionX(), tppos[point].GetPositionY(), tppos[point].GetPositionZ(), 211190);
                        me->GetMotionMaster()->MoveJump(tppos[point].GetPositionX(), tppos[point].GetPositionY(), tppos[point++].GetPositionZ(), me->GetOrientation(), 15.0f, 15.0f);
                        events.RescheduleEvent(EVENT_2, 25000, 0, PHASE_COMBAT); // rephase 211186 + 211190  
                        events.SetPhase(PHASE_REPHASE);
                        events.RescheduleEvent(EVENT_3, 2000, 0, PHASE_REPHASE);
                        break;
                    }
                    case EVENT_3:
                        if (Player* pl = me->FindNearestPlayer(100.0f, true))
                        {
                            me->CastSpell(pl, 211208);
                            if (me->GetDistance2d(pl) <= 20.0f)
                            {
                                me->SetReactState(REACT_DEFENSIVE);
                                events.SetPhase(PHASE_COMBAT);
                            }
                        }
                        events.RescheduleEvent(EVENT_4, 4000, 0, PHASE_REPHASE);
                        break;

                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

namespace TuureDefeat
{
    bool Eligible(Player* player, Creature* actor)
    {
        if (!player || !actor->IsInWorld() || !actor->IsAlive() || actor->GetEntry() != 106318 ||
            actor->GetMapId() != 1489 || !actor->GetMap() || !player->IsInWorld() || !player->CanContact() ||
            !player->IsAlive() || player->HasPendingTeleport() || player->getClass() != CLASS_PRIEST ||
            player->GetMap() != actor->GetMap() || !player->InSamePhase(actor) ||
            !player->IsWithinDistInMap(actor, 100.0f) || player->GetQuestRewardStatus(41993) ||
            (player->GetQuestStatus(41993) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41993) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(actor->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1085 && scenario->GetCurrentStep() == 5;
    }

    Player* Participant(Creature* actor)
    {
        if (!actor->GetMap()) return nullptr;
        Player* recipient = nullptr;
        for (auto const& reference : actor->GetMap()->GetPlayers())
            if (Eligible(reference.getSource(), actor))
            {
                if (recipient) return nullptr;
                recipient = reference.getSource();
            }
        return recipient;
    }

    Player* Recipient(Creature* actor, Unit* attacker)
    {
        if (!attacker) return nullptr;
        if (Player* owner = attacker->GetCharmerOrOwnerPlayerOrPlayerItself())
            return Eligible(owner, actor) ? owner : nullptr;
        // The native solo healer encounter permits its three allies to kill.
        // A nearer spectator must never replace its actual participant.
        if (attacker->GetCharmerOrOwnerGUID() || attacker->GetMap() != actor->GetMap() ||
            !attacker->InSamePhase(actor) ||
            (attacker->GetEntry() != 106134 && attacker->GetEntry() != 106137 && attacker->GetEntry() != 106253))
            return nullptr;
        return Participant(actor);
    }

    void ReleaseAllies(Creature* actor, bool heal)
    {
        std::list<Creature*> allies;
        for (uint32 entry : {106134u, 106137u, 106253u})
            GetCreatureListWithEntryInGrid(allies, actor, entry, 50.0f);
        for (Creature* ally : allies)
        {
            // Native strangulate298 owns its control state; retain other casters.
            ally->RemoveAurasDueToSpell(210703, actor->GetGUID());
            if (heal) ally->CastSpell(ally, 210807);
        }
    }
}

// 106318
class npc_lady_kalindria : public CreatureScript
{
public:
    npc_lady_kalindria() : CreatureScript("npc_lady_kalindria") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lady_kalindriaAI(creature);
    }

    struct npc_lady_kalindriaAI : ScriptedAI
    {
        npc_lady_kalindriaAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
        }

        EventMap events;
        SummonList summons;

        bool outro = false, outro_event = false, outro_event1 = false, strangleActive = false;
        std::shared_ptr<uint8> attempt = std::make_shared<uint8>(uint8(0));

        void Reset() override
        {
            attempt = std::make_shared<uint8>(uint8(0));
            me->InterruptNonMeleeSpells(true);
            TuureDefeat::ReleaseAllies(me, false);
            events.Reset();
            outro = false;
            outro_event = false;
            outro_event1 = false;
            strangleActive = false;
            summons.DespawnAll();
        }

        void EnterCombat(Unit* who) override
        {
            if (outro) return;
            Talk(0);
            events.RescheduleEvent(EVENT_1, 2000); //211185
            events.RescheduleEvent(EVENT_2, 13000); //211380 + script for summon (17)
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void DamageTakenAfterScaling(Unit* attacker, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            if (outro)
            {
                damage = 0;
                return;
            }
            Player* player = TuureDefeat::Recipient(me, attacker);
            if (!damage || !player) return;
            uint64 health = me->GetHealth(attacker);
            if (damage >= health)
            {
                damage = 0;
                outro = true; // claim before reentrant optional spell/credit callbacks
                strangleActive = false;
                attempt = std::make_shared<uint8>(uint8(0)); // cancel old combat speech
                events.Reset();
                me->StopAttack();
                me->InterruptNonMeleeSpells(true);
                Creature* actor = me;
                ObjectGuid killer = attacker->GetGUID();
                std::weak_ptr<uint8> generation = attempt;
                // The actor owns this queue. No borrowed AI or killer pointer;
                // Reset/replacement expires the attempt, victim loss does not.
                me->AddDelayedEvent(3000, [actor, killer, generation]()
                {
                    if (generation.expired() || !actor->IsInWorld() || !actor->IsAlive()) return;
                    Unit* source = ObjectAccessor::GetUnit(*actor, killer);
                    if (!source || !source->IsAlive() || source->GetMap() != actor->GetMap()) source = actor;
                    source->Kill(actor);
                });
                Talk(4);
                DoCast(210715);
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50695);
                TuureDefeat::ReleaseAllies(me, true);
                me->CreateConversation(2015);
                return; // never rearm strangle after defeat cleanup
            }
            if (!outro_event && health < me->CountPctFromMaxHealth(13, attacker))
            {
                outro_event = true;
                Talk(1);
                events.RescheduleEvent(EVENT_3, 10);
            }
            if (!outro_event1 && health < me->CountPctFromMaxHealth(9, attacker))
            {
                outro_event1 = true;
                Talk(3);
            }
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell) override
        {
            if (target && spell && spell->Id == 210703)
            {
                if (outro || !strangleActive) target->RemoveAurasDueToSpell(210703, me->GetGUID());
                else target->AddUnitState(UNIT_STATE_STUNNED);
            }
        }


        void UpdateAI(uint32 diff) override
        {
            if (outro || !UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(211185);
                        events.RescheduleEvent(EVENT_1, 2500); //211185
                        break;
                    case EVENT_2:
                        if (Player* pl = TuureDefeat::Participant(me))
                        {
                            DoCast(211380);
                            for (uint8 i = 0; i < urand(15, 18); ++i)
                            {
                                if (Creature* add = me->SummonCreature(106606, pl->GetPositionX() + irand(-7, 7), pl->GetPositionY() + irand(-7, 7), pl->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 25000))
                                {
                                    add->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                                    add->AI()->AttackStart(pl);
                                }
                            }
                        }
                        events.RescheduleEvent(EVENT_2, 34000); //211380 + script for summon (17)
                        break;
                    case EVENT_3:
                        strangleActive = true;
                        DoCast(210703);
                        if (Player* pl = TuureDefeat::Participant(me))
                            AttackStart(pl);
                        {
                            Creature* actor = me;
                            std::weak_ptr<uint8> generation = attempt;
                            me->AddDelayedEvent(3000, [actor, generation]()
                            {
                                if (!generation.expired() && actor->IsInWorld() && actor->IsAlive())
                                    sCreatureTextMgr->SendChat(actor, 2);
                            });
                        }
                        break;
                }
            }
            // DoMeleeAttackIfReady();
        }
    };
};

// 249693
class go_tuure_crate : public GameObjectScript
{
public:
    go_tuure_crate() : GameObjectScript("go_tuure_crate") { }

    struct go_tuure_crateAI : public GameObjectAI
    {
        go_tuure_crateAI(GameObject* go) : GameObjectAI(go) {}

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!isUse)
                return true;

            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50633);
            if (Creature* boja = player->FindNearestCreature(106253, 60.0f, true))
            {
                boja->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
                boja->AI()->Talk(0);
                if (Creature* boros = boja->FindNearestCreature(106134, 70.0f, true))
                    boja->GetMotionMaster()->MoveFollow(boros, PET_FOLLOW_DIST, static_cast<float>(-PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);

                if (Creature* capitan = boja->FindNearestCreature(106107, 70.0f, true))
                {
                    capitan->AI()->Talk(0);
                    capitan->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_ATTACKABLE_1);
                    capitan->SetReactState(REACT_DEFENSIVE);
                    capitan->setFaction(2102);
                }
            }
            go->Delete();
            return true;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_tuure_crateAI(go);
    }
};

namespace TuureAcquisition
{
    enum ClaimData { Finished = 0, Elapsed = 1, Current = 2, Finish = 3 };

    bool HasLeaf(Scenario* scenario, uint32 leaf)
    {
        if (!scenario) return false;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto found = progress->find(leaf);
        return found != progress->end() && found->second.Counter;
    }

    bool CanTake(Player* player, GameObject* weapon)
    {
        if (!player || !weapon || !player->IsInWorld() || !player->CanContact() || !player->IsAlive() ||
            player->HasPendingTeleport() || player->getClass() != CLASS_PRIEST || !player->GetMap() ||
            !weapon->IsInWorld() || !weapon->isSpawned() || weapon->GetEntry() != 249705 || weapon->GetMapId() != 1489 ||
            player->GetMap() != weapon->GetMap() || player->GetInstanceId() != weapon->GetInstanceId() ||
            !player->InSamePhase(weapon) || !player->canSeeOrDetect(weapon) || !player->IsWithinLOSInMap(weapon) ||
            !player->IsWithinDistInMap(weapon, weapon->GetInteractionDistance()) ||
            weapon->HasFlag(GAMEOBJECT_FIELD_FLAGS, GO_FLAG_NOT_SELECTABLE) || player->GetQuestRewardStatus(41993) ||
            (player->GetQuestStatus(41993) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41993) != QUEST_STATUS_COMPLETE))
            return false;
        Scenario* scenario = sScenarioMgr->GetScenario(weapon->GetInstanceId());
        return scenario && scenario->GetScenarioId() == 1085 && scenario->GetCurrentStep() == 6 && HasLeaf(scenario, 47656);
    }

    bool Equipped(Player* player)
    {
        Item* weapon = player->GetItemByEntry(128825);
        return weapon && player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND) == weapon;
    }
}

namespace TuurePresentation
{
    bool Complete(Player* player, SpellScene const* scene, std::string const& trigger)
    {
        if (!scene || scene->MiscValue != 1265 || scene->SceneScriptPackageID != 1565)
            return false;
        if (trigger != "complete" || !player || !player->HasSceneStatus(1265, SCENE_COMPLETE) ||
            !player->IsInWorld() || !player->CanContact() || !player->IsAlive() || player->HasPendingTeleport() ||
            player->getClass() != CLASS_PRIEST || player->GetMapId() != 1489 ||
            player->GetQuestRewardStatus(41993) ||
            (player->GetQuestStatus(41993) != QUEST_STATUS_INCOMPLETE && player->GetQuestStatus(41993) != QUEST_STATUS_COMPLETE) ||
            !player->GetItemByEntry(128825))
            return true;
        Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
        if (!scenario || scenario->GetScenarioId() != 1085 || scenario->GetCurrentStep() != 6)
            return true;
        auto const* progress = scenario->GetAchievementMgr().GetCriteriaProgressMap();
        auto victory = progress->find(47656); // Native Lady Calindris50695, not a forged early scene trigger.
        if (victory == progress->end() || !victory->second.Counter)
            return true;
        // The scene equips a cinematic clone only. Native210734 validates and
        // equips the real existing staff, retaining phase/events/conversation.
        // Accepted casts may run later: do not diagnose inventory before execution.
        SpellCastResult result = player->CastSpell(player, 210734, true);
        if (result != SPELL_CAST_OK)
            TC_LOG_WARN("scripts", "Tuure pickup: native equip210734 failed with result %u; retain inventory and native errors", uint32(result));
        return true;
    }
}

class scene_tuure_pickup : public SceneTriggerScript
{
public:
    scene_tuure_pickup() : SceneTriggerScript("scene_tuure_pickup") { }
    bool OnTrigger(Player* player, SpellScene const* scene, std::string trigger) override
    {
        return TuurePresentation::Complete(player, scene, trigger);
    }
};

// 249705
class go_art_tuure : public GameObjectScript
{
    struct AI : GameObjectAI
    {
        explicit AI(GameObject* object) : GameObjectAI(object) { }
        ObjectGuid claimant;
        std::weak_ptr<uint8> lifetime;
        uint32 elapsed = 0, retry = 0;
        bool finished = false;

        void Cancel() { claimant.Clear(); lifetime.reset(); }
        bool Current(Player* player) const
        {
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            return player && player->GetGUID() == claimant && !lifetime.expired() &&
                status && status->ScriptLifetime == lifetime.lock();
        }
        ObjectGuid GetGUID(int32 = 0) override { return claimant; }
        uint32 GetData(uint32 id) override
        {
            if (id == TuureAcquisition::Elapsed) return elapsed;
            if (id == TuureAcquisition::Current) return Current(ObjectAccessor::GetPlayer(*go, claimant));
            return finished;
        }
        bool GossipHello(Player* player, bool) override
        {
            return finished || !claimant.IsEmpty() || !TuureAcquisition::CanTake(player, go);
        }
        void SetData(uint32 id, uint32 value) override
        {
            if (id != TuureAcquisition::Finish || !value || finished) return;
            finished = true;
            Cancel();
            if (Creature* visual = go->FindNearestCreature(106326, 10.0f))
                visual->DespawnOrUnsummon(1000);
            go->SetLootState(GO_JUST_DEACTIVATED);
        }
        void SetGUID(ObjectGuid const& guid, int32 = 0) override
        {
            if (finished || !claimant.IsEmpty()) return;
            Player* player = ObjectAccessor::GetPlayer(*go, guid);
            if (!player || player->GetGUID() != guid || !TuureAcquisition::CanTake(player, go) || !player->GetItemByEntry(128825)) return;
            player->AddQuestDelayedEvent(41993, 0, [] { });
            QuestStatusData* status = player->getQuestStatus(41993);
            if (!status || !status->ScriptLifetime) return;
            claimant = guid;
            lifetime = status->ScriptLifetime;
            elapsed = retry = 0;
            SpellCastResult result = player->CastSpell(player, 210733, true);
            if (result != SPELL_CAST_OK)
            {
                Cancel();
                TC_LOG_WARN("scripts", "Tuure pickup: scene210733 request failed (%u); retain staff/object for retry", uint32(result));
            }
        }
        void UpdateAI(uint32 diff) override
        {
            if (claimant.IsEmpty() || finished) return;
            Player* player = ObjectAccessor::GetPlayer(*go, claimant);
            if (!Current(player) || !TuureAcquisition::CanTake(player, go) || !player->GetItemByEntry(128825)) { Cancel(); return; }
            elapsed += std::min(diff, 30000u - elapsed);
            if (elapsed >= 30000)
            {
                Cancel();
                TC_LOG_WARN("scripts", "Tuure pickup: scene/equip/departure did not finish within30s; retain staff/object and native errors for retry");
                return;
            }
            if (elapsed < 14000 || !player->HasSceneStatus(1265, SCENE_COMPLETE) || !TuureAcquisition::Equipped(player)) return;
            Scenario* scenario = sScenarioMgr->GetScenario(go->GetInstanceId());
            if (!TuureAcquisition::HasLeaf(scenario, 47679)) return;
            if (retry > diff) { retry -= diff; return; }
            retry = 5000;
            SpellCastResult result = player->CastSpell(player, 210818, false);
            if (result != SPELL_CAST_OK)
                TC_LOG_WARN("scripts", "Tuure pickup: native departure210818 request failed (%u); retain claim for bounded retry", uint32(result));
        }
    };
public:
    go_art_tuure() : GameObjectScript("go_art_tuure") { }
    GameObjectAI* GetAI(GameObject* object) const override { return new AI(object); }
    bool OnGossipHello(Player* player, GameObject* object) override
    {
        if (!TuureAcquisition::CanTake(player, object) || object->AI()->GetData(TuureAcquisition::Finished)) return true;
        if (player->GetItemByEntry(128825)) { object->AI()->SetGUID(player->GetGUID(), 0); return true; }
        return player->HasItemCount(128825, 1, true); // Bank-only copy must be retrieved, never duplicated.
    }
    void OnLootStateChanged(GameObject* object, uint32 state, Unit* unit) override
    {
        Player* player = unit ? unit->ToPlayer() : nullptr;
        if (state != GO_JUST_DEACTIVATED || object->AI()->GetData(TuureAcquisition::Finished) ||
            !TuureAcquisition::CanTake(player, object) || !player->GetItemByEntry(128825)) return;
        object->SetLootState(GO_READY); // Real completed storage; keep retry access until accepted departure.
        object->AI()->SetGUID(player->GetGUID(), 0);
    }
};

namespace TuureCast
{
    struct State
    {
        bool scoped = false, issued = false;
        ObjectGuid source;
        std::weak_ptr<uint8> lifetime;

        void Bind(Player* player)
        {
            if (!player || player->GetMapId() != 1489) return;
            Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
            scoped = scenario && scenario->GetScenarioId() == 1085;
            if (!scoped) return;
            std::list<GameObject*> weapons;
            GetGameObjectListWithEntryInGrid(weapons, player, 249705, 5.0f);
            for (GameObject* weapon : weapons)
                if (weapon && weapon->AI() && weapon->AI()->GetGUID(0) == player->GetGUID() &&
                    weapon->AI()->GetData(TuureAcquisition::Current) && TuureAcquisition::CanTake(player, weapon))
                {
                    source = weapon->GetGUID();
                    break;
                }
            if (QuestStatusData* status = player->getQuestStatus(41993)) lifetime = status->ScriptLifetime;
        }
        bool SameAttempt(Player* player) const
        {
            QuestStatusData* status = player ? player->getQuestStatus(41993) : nullptr;
            return status && !lifetime.expired() && status->ScriptLifetime == lifetime.lock();
        }
        GameObject* Current(Player* player) const
        {
            if (!SameAttempt(player)) return nullptr;
            GameObject* weapon = ObjectAccessor::GetGameObject(*player, source);
            return weapon && weapon->GetGUID() == source && weapon->AI() &&
                weapon->AI()->GetGUID(0) == player->GetGUID() && weapon->AI()->GetData(TuureAcquisition::Current) &&
                !weapon->AI()->GetData(TuureAcquisition::Finished) && TuureAcquisition::CanTake(player, weapon) &&
                player->GetItemByEntry(128825) ? weapon : nullptr;
        }
        bool Ready(Player* player, bool departure) const
        {
            GameObject* weapon = Current(player);
            if (!weapon) return false;
            if (!departure) return true;
            Scenario* scenario = sScenarioMgr->GetScenario(player->GetInstanceId());
            return weapon->AI()->GetData(TuureAcquisition::Elapsed) >= 14000 &&
                player->HasSceneStatus(1265, SCENE_COMPLETE) && TuureAcquisition::Equipped(player) &&
                TuureAcquisition::HasLeaf(scenario, 47679);
        }
        void Depart(Player* player, Spell* spell, SpellEffIndex effect)
        {
            if (issued) return;
            if (!Ready(player, true))
            {
                TC_LOG_WARN("scripts", "Tuure pickup: stale/ineligible queued departure rejected; no teleport or50702");
                return;
            }
            issued = true; // Before native callbacks can re-enter this same effect.
            Player::ScopedTeleportDelay guard(*player);
            spell->EffectTeleportUnits(effect); // Actual native effect, including ordinary admission/failures.
            if (!guard.IsPending() || player->GetTeleportDest().GetMapId() != 1220 ||
                player->GetTeleportDest().GetExactDist(-828.85f, 4372.0f, 738.64f) > 0.1f || !SameAttempt(player))
            {
                TC_LOG_WARN("scripts", "Tuure pickup: native departure was not accepted for original attempt/destination; no50702 credit");
                return;
            }
            if (GameObject* weapon = ObjectAccessor::GetGameObject(*player, source))
                if (weapon->GetGUID() == source && weapon->AI() && weapon->AI()->GetGUID(0) == player->GetGUID())
                    weapon->AI()->SetData(TuureAcquisition::Finish, 1);
            // Still in the source map under native deferral. Native final-tree completion rewards1085.
            player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 50702);
        }
    };
}

class spell_tuure_pickup_cast : public SpellScript
{
    PrepareSpellScript(spell_tuure_pickup_cast);
    TuureCast::State state;
    bool Load() override { state.Bind(GetCaster() ? GetCaster()->ToPlayer() : nullptr); return true; }
    SpellCastResult Check()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        return !state.scoped || (state.Ready(player, false) && (GetSpellInfo()->Id != 210734 || player->HasSceneStatus(1265, SCENE_COMPLETE))) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS;
    }
    void Hit(SpellEffIndex effect)
    {
        if (!state.scoped) return;
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!state.Ready(player, false) || (GetSpellInfo()->Id == 210734 && !player->HasSceneStatus(1265, SCENE_COMPLETE)))
        {
            PreventHitDefaultEffect(effect);
            TC_LOG_WARN("scripts", "Tuure pickup: stale/ineligible scene or equip effect rejected; retain original attempt guards");
        }
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_tuure_pickup_cast::Check);
        OnEffectHit += SpellEffectFn(spell_tuure_pickup_cast::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_tuure_pickup_cast::Hit, EFFECT_ALL, SPELL_EFFECT_ANY);
    }
};

class spell_tuure_departure : public SpellScript
{
    PrepareSpellScript(spell_tuure_departure);
    TuureCast::State state;
    bool Load() override { state.Bind(GetCaster() ? GetCaster()->ToPlayer() : nullptr); return true; }
    SpellCastResult Check() { return !state.scoped || state.Ready(GetCaster() ? GetCaster()->ToPlayer() : nullptr, true) ? SPELL_CAST_OK : SPELL_FAILED_BAD_TARGETS; }
    void Hit(SpellEffIndex effect)
    {
        if (!state.scoped) return;
        PreventHitDefaultEffect(effect);
        state.Depart(GetHitPlayer(), GetSpell(), effect);
    }
    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_tuure_departure::Check);
        OnEffectHitTarget += SpellEffectFn(spell_tuure_departure::Hit, EFFECT_0, SPELL_EFFECT_TELEPORT_L);
    }
};

void AddSC_Tuure()
{
    new npc_vindicator_boros();
    new npc_tuure_small_events();
    new npc_capitan_naranot();
    new npc_lady_kalindria();
    new go_tuure_crate();
    new go_art_tuure();
    new scene_tuure_pickup();
    RegisterSpellScript(spell_tuure_pickup_cast);
    RegisterSpellScript(spell_tuure_departure);
};
