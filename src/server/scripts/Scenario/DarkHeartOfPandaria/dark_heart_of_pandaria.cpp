/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "dark_heart_of_pandaria.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "CreatureAI.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include "AreaTriggerAI.h"

// The Pandaria source used a project-specific combat helper. Keep its
// targeting and summon behavior local to this scenario.
enum DarkHeartTargetPriority
{
    PRIORITY_VICTIM,
    PRIORITY_NOT_VICTIM,
    PRIORITY_SELF,
};

struct DarkHeartCreatureAI : public ScriptedAI
{
    DarkHeartCreatureAI(Creature* creature) : ScriptedAI(creature), summons(me) { }

    void ExecuteTargetEvent(uint32 spellId, uint32 repeat, uint32 eventId, uint32 currentEventId,
        DarkHeartTargetPriority priority = PRIORITY_VICTIM)
    {
        if (currentEventId != eventId)
            return;

        Unit* target = nullptr;
        switch (priority)
        {
            case PRIORITY_SELF:
                target = me;
                break;
            case PRIORITY_NOT_VICTIM:
                target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me));
                if (!target)
                    target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true);
                break;
            case PRIORITY_VICTIM:
                target = me->getVictim();
                break;
        }

        if (target)
            DoCast(target, spellId);

        me->_AddCreatureSpellCooldown(spellId, time(nullptr) + 2);
        if (repeat)
            events.ScheduleEvent(eventId, repeat);
    }

protected:
    EventMap events;
    SummonList summons;
};

enum Spells
{
    SPELL_STUN_SELF              = 141480,
    SPELL_DETONATE               = 141456,
    SPELL_STONE_RAIN             = 142139,
    SPELL_PRIMAL_RAGE            = 141663,
    SPELL_SUMMON_FIERY_ANGER     = 141488,
    SPELL_SCHARGE                = 22120,
    SPELL_SPELL_SHATTER          = 141421,
    SPELL_MOLTEN_HATRED          = 141612,
    SPELL_MOLTEN_HATRED_COSMETIC = 143400,
    SPELL_FIRE_BLAST             = 13339,
    SPELL_URTHARGES_SHIELD_EFF   = 141387, // 20 at apply
    SPELL_RUPTURE_LINE_LAUNCHER  = 141418,
    SPELL_RUPTURE_LINE_MISSLE    = 141420,
    SPELL_DARK_BLOOD_VISUAL      = 142150,
    SPELL_DARK_BLOOD_EFF         = 141665,
    SPELL_HOWL_OF_TERROR         = 142152,
    SPELL_NITRO_BOOSTS           = 142568,
    SPELL_HEART_OF_YSHAARJ       = 142154,
    SPELL_VEIL_OF_DARKNESS       = 142201,
    SPELL_MALEVOLENT_FORCE       = 142840,
    SPELL_MALEVOLENT_FORCE_EFF   = 142203,

    // Spheres
    SPELL_ORB_OF_HEALING_EFF     = 141688,
    SPELL_ORB_OF_HEALING_VISUAL  = 141689,
    SPELL_ORB_OF_HEALING_AT      = 141691,
    SPELL_ORB_OF_POWER_EFF       = 141681,
    SPELL_ORB_OF_POWER_AT        = 141682,
    SPELL_ORB_OF_POWER_VISUAL    = 141683,
    SPELL_HEART_CORRUPTION       = 142155, // at under heart of yshaarj

};

enum Events
{
    EVENT_STONE_RAIN = 1,
    EVENT_SUMMON_FIERY_ANGER,
    EVENT_SPELL_SHATTER,
    EVENT_PRIMAL_RAGE,
    EVENT_FIRE_BLAST,
    EVENT_MOLTEN_HATRED,
    EVENT_RUPTURE_LINE,
    EVENT_HOWL_OF_TERROR,
    EVENT_VEIL_OF_DARKNESS,
    EVENT_MALEVOLENT_FORCE,
};

// Earthborn Hatred 70822
struct npc_darkheart_earthborn_hatred : public DarkHeartCreatureAI
{
    npc_darkheart_earthborn_hatred(Creature* creature) : DarkHeartCreatureAI(creature) { }

    bool hasTriggered;

    void Reset() override
    {
        events.Reset();
        summons.DespawnAll();
        hasTriggered = false;
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* urtharges = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_URTHARGES_THE_DESTROYER) : ObjectGuid::Empty))
            urtharges->AI()->DoAction(4);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/) override
    {
        if (HealthBelowPct(30) && !hasTriggered)
        {
            hasTriggered = true;
            events.ScheduleEvent(EVENT_PRIMAL_RAGE, 500);
        }
    }

    void EnterCombat(Unit* who) override
    {
        DoCast(who, SPELL_SCHARGE);
        events.ScheduleEvent(EVENT_STONE_RAIN, urand(3 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
        events.ScheduleEvent(EVENT_SUMMON_FIERY_ANGER, 13 * IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_SPELL_SHATTER, urand(6500, 15000));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(SPELL_STONE_RAIN, urand(8 * IN_MILLISECONDS, 18 * IN_MILLISECONDS), EVENT_STONE_RAIN, eventId, PRIORITY_NOT_VICTIM);
            ExecuteTargetEvent(SPELL_SUMMON_FIERY_ANGER, 25 * IN_MILLISECONDS, EVENT_SUMMON_FIERY_ANGER, eventId, PRIORITY_SELF);
            ExecuteTargetEvent(SPELL_SPELL_SHATTER, 10 * IN_MILLISECONDS, EVENT_SPELL_SHATTER, eventId, PRIORITY_NOT_VICTIM);
            ExecuteTargetEvent(SPELL_PRIMAL_RAGE, 0, EVENT_PRIMAL_RAGE, eventId, PRIORITY_SELF);
            break;
        }

        DoMeleeAttackIfReady();
    }
};

// Fiery Anger 70824
struct npc_darkheart_fiery_anger : public DarkHeartCreatureAI
{
    npc_darkheart_fiery_anger(Creature* creature) : DarkHeartCreatureAI(creature) { }

    bool hasTriggered;

    void Reset() override
    {
        events.Reset();
        hasTriggered = false;
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (!me->GetDBTableGUIDLow())
            return;

        if (Creature* urtharges = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_URTHARGES_THE_DESTROYER) : ObjectGuid::Empty))
            urtharges->AI()->DoAction(2);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/) override
    {
        if (HealthBelowPct(35) && !hasTriggered)
        {
            hasTriggered = true;
            me->InterruptNonMeleeSpells(true);

            Talk(TALK_INTRO);
            DoCast(me, SPELL_DETONATE, true);
        }
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.ScheduleEvent(EVENT_MOLTEN_HATRED, urand(3 * IN_MILLISECONDS, 10 * IN_MILLISECONDS));
        events.ScheduleEvent(EVENT_FIRE_BLAST, urand(5 * IN_MILLISECONDS, 13 * IN_MILLISECONDS));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(SPELL_MOLTEN_HATRED, urand(8 * IN_MILLISECONDS, 18 * IN_MILLISECONDS), EVENT_MOLTEN_HATRED, eventId, PRIORITY_NOT_VICTIM);
            ExecuteTargetEvent(SPELL_FIRE_BLAST, urand(3 * IN_MILLISECONDS, 10 * IN_MILLISECONDS), EVENT_FIRE_BLAST, eventId);
            break;
        }

        DoMeleeAttackIfReady();
    }
};

// Urtharges the Destroyer 70959
struct npc_darkheart_urtharges_the_destroyer : public DarkHeartCreatureAI
{
    npc_darkheart_urtharges_the_destroyer(Creature* creature) : DarkHeartCreatureAI(creature) { }

    bool hasTriggered;

    void Reset() override
    {
        events.Reset();
        summons.DespawnAll();
        DoCast(me, SPELL_URTHARGES_SHIELD_EFF);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetInstanceScript())
            me->GetInstanceScript()->SetData(STEP_URTHARGES_DEFEATED, DONE);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        Talk(TALK_INTRO);
        events.ScheduleEvent(EVENT_RUPTURE_LINE, 5 * IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_STONE_RAIN, urand(6 * IN_MILLISECONDS, 15 * IN_MILLISECONDS));
        events.ScheduleEvent(EVENT_SPELL_SHATTER, urand(10500, 19500));
    }

    void DoAction(int32 actionId) override
    {
        if (Aura* shield = me->GetAura(SPELL_URTHARGES_SHIELD_EFF))
        {
            if (shield->GetStackAmount() <= actionId)
            {
                Talk(TALK_SPECIAL_1);
                me->RemoveAurasDueToSpell(SPELL_URTHARGES_SHIELD_EFF);

                if (me->GetInstanceScript())
                    me->GetInstanceScript()->DoRemoveAurasDueToSpellOnPlayers(SPELL_URTHARGES_SHIELD);
            }
            else
            {
                shield->SetStackAmount(shield->GetStackAmount() - actionId);

                if (!me->GetInstanceScript())
                    return;

                // Update Alt Power
                for (auto&& itr : me->GetInstanceScript()->instance->GetPlayers())
                    if (Player* target = itr.getSource())
                        target->SetPower(POWER_ALTERNATE, 5 * shield->GetStackAmount());
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(SPELL_RUPTURE_LINE_LAUNCHER, 12 * IN_MILLISECONDS, EVENT_RUPTURE_LINE, eventId);
            ExecuteTargetEvent(SPELL_STONE_RAIN, urand(18 * IN_MILLISECONDS, 28 * IN_MILLISECONDS), EVENT_STONE_RAIN, eventId, PRIORITY_NOT_VICTIM);
            ExecuteTargetEvent(SPELL_SPELL_SHATTER, 10 * IN_MILLISECONDS, EVENT_SPELL_SHATTER, eventId, PRIORITY_NOT_VICTIM);
            break;
        }

        DoMeleeAttackIfReady();
    }
};

// Grizzle Gearslip 70956
class npc_darkheart_grizzle_gearslip : public CreatureScript
{
    public:
        npc_darkheart_grizzle_gearslip() : CreatureScript("npc_darkheart_grizzle_gearslip") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            creature->AI()->DoAction(ACTION_START_INTRO);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        struct npc_darkheart_grizzle_gearslipAI : public ScriptedAI
        {
            npc_darkheart_grizzle_gearslipAI(Creature* creature) : ScriptedAI(creature) { }

            TaskScheduler scheduler;
            uint32 delay;
            float x, y;

            void InitializeAI() override
            {
                delay = 0;
                x = 0.0f; y = 0.0f;

                if (me->GetDBTableGUIDLow() && (!me->GetInstanceScript() || me->GetInstanceScript()->GetData(STEP_TALK_WITH_LEADER) != DONE))
                    me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            void IsSummonedBy(Unit* summoner) override
            {
                me->GetMotionMaster()->MovePoint(0, gearslipPath[1]);

                scheduler.Schedule(Milliseconds(me->movespline->Duration()), [this](TaskContext context)
                {
                    Talk(TALK_SPECIAL_3);

                    me->SummonCreature(NPC_MALKOROK, gearslipPath[0], TEMPSUMMON_MANUAL_DESPAWN);
                });
            }

            void DoAction(int32 actionId) override
            {
                switch (actionId)
                {
                    case ACTION_START_INTRO:
                        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        Talk(TALK_INTRO);

                        if (me->GetInstanceScript())
                            me->GetInstanceScript()->SetData(STEP_TALK_WITH_LEADER, DONE);

                        delay = 0;
                        scheduler.Schedule(Milliseconds(delay += 5500), [this](TaskContext context)
                        {
                            Talk(TALK_SPECIAL_1);
                        });

                        scheduler.Schedule(Milliseconds(delay += 8000), [this](TaskContext context)
                        {
                            Talk(TALK_SPECIAL_2);
                        });
                        break;
                    case ACTION_GRIZZLE_ANSWER:
                        delay = 0;
                        Talk(TALK_SPECIAL_4);

                        scheduler.
                            Schedule(Milliseconds(delay+=6800), [this](TaskContext context)
                        {
                            Talk(TALK_SPECIAL_5);

                            GetPositionWithDistInOrientation(me, 2.0f, me->GetOrientation(), x, y);
                            me->GetMotionMaster()->MovePoint(0, x, y, me->GetPositionZ());
                        });

                        scheduler.
                            Schedule(Milliseconds(delay += 5000), [this](TaskContext context)
                        {
                            if (Creature* malkorok = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_MALKOROK) : ObjectGuid::Empty))
                                malkorok->AI()->DoAction(ACTION_START_INTRO);
                        });
                        break;
                    case ACTION_LEAVE_MALKOROK:
                        Talk(TALK_SPECIAL_8);
                        me->GetMotionMaster()->MovePoint(0, gearslipPath[0]);

                        scheduler.
                            Schedule(Milliseconds(me->movespline->Duration()), [this](TaskContext context)
                        {
                            me->DespawnOrUnsummon();
                        });
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                scheduler.Update(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkheart_grizzle_gearslipAI(creature);
        }
};

// Mouth of Terror 70691
struct npc_darkheart_mouth_of_terror : public DarkHeartCreatureAI
{
    npc_darkheart_mouth_of_terror(Creature* creature) : DarkHeartCreatureAI(creature) { }

    bool hasTriggered;

    void Reset() override
    {
        events.Reset();
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.ScheduleEvent(EVENT_HOWL_OF_TERROR, urand(3 * IN_MILLISECONDS, 10 * IN_MILLISECONDS));
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(SPELL_HOWL_OF_TERROR, urand(8 * IN_MILLISECONDS, 18 * IN_MILLISECONDS), EVENT_HOWL_OF_TERROR, eventId, PRIORITY_SELF);
            break;
        }

        DoMeleeAttackIfReady();
    }
};

// Crafty the Ambitious 71358
class npc_darkheart_crafty_the_ambitious : public CreatureScript
{
    public:
        npc_darkheart_crafty_the_ambitious() : CreatureScript("npc_darkheart_crafty_the_ambitious") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            creature->AI()->DoAction(ACTION_START_INTRO);
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        struct npc_darkheart_crafty_the_ambitiousAI : public ScriptedAI
        {
            npc_darkheart_crafty_the_ambitiousAI(Creature* creature) : ScriptedAI(creature) { }

            TaskScheduler scheduler;
            uint32 delay;

            void InitializeAI() override
            {
                delay = 0;
                me->SetFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            void DoAction(int32 actionId) override
            {
                if (actionId == ACTION_START_INTRO)
                {
                    me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                    Talk(TALK_INTRO);

                    delay = 0;
                    scheduler.Schedule(Milliseconds(delay += 5500), [this](TaskContext context)
                    {
                        Talk(TALK_SPECIAL_1);
                    });

                    scheduler.Schedule(Milliseconds(delay += 9000), [this](TaskContext context)
                    {
                        Talk(TALK_SPECIAL_2);

                        if (me->GetInstanceScript())
                            me->GetInstanceScript()->SetData(STEP_END_OF_MINE, DONE);

                        if (GameObject* strangeWall = ObjectAccessor::GetGameObject(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(GO_STRANGE_WALL) : ObjectGuid::Empty))
                            strangeWall->SetGoState(GO_STATE_ACTIVE);

                        Movement::MoveSplineInit init(*me);

                        for (auto&& itr : craftyPath)
                            init.Path().push_back(G3D::Vector3(itr.GetPositionX(), itr.GetPositionY(), itr.GetPositionZ()));

                        init.SetWalk(true);
                        init.Launch();

                        delay = me->movespline->Duration();
                        scheduler.Schedule(Milliseconds(delay), [this](TaskContext context)
                        {
                            Talk(TALK_SPECIAL_3);
                        });

                        scheduler.Schedule(Milliseconds(delay += 3000), [this](TaskContext context)
                        {
                            DoCast(me, SPELL_NITRO_BOOSTS, true);
                            me->GetMotionMaster()->MoveCharge(darkheartDest.GetPositionX(), darkheartDest.GetPositionY(), darkheartDest.GetPositionZ(), 20.0f, EVENT_CHARGE);
                        });
                    });
                }
            }

            void MovementInform(uint32 type, uint32 pointId) override
            {
                if (pointId == EVENT_CHARGE)
                {
                    me->SetAnimTier(AnimTier::Fly);
                    me->SetCanFly(true);
                    me->SetDisableGravity(true);
                    me->UpdateMovementFlags();

                    if (Creature* norushen = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_NORUSHEN) : ObjectGuid::Empty))
                        norushen->AI()->Talk(TALK_INTRO);

                    me->SetUInt32Value(UNIT_FIELD_EMOTE_STATE, EMOTE_STATE_FLYFALL);

                    me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 4.0f);

                    if (GameObject* heartOfYshaarj = ObjectAccessor::GetGameObject(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(GO_HEART_OF_YSHAARJ) : ObjectGuid::Empty))
                        heartOfYshaarj->SetGoState(GO_STATE_ACTIVE);

                    scheduler.
                        Schedule(Milliseconds(7000), [this](TaskContext context)
                    {
                        if (Creature* echo = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_ECHO_OF_YSHAARJ) : ObjectGuid::Empty))
                        {
                            echo->SetVisible(true);
                            echo->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
                        }

                        me->DespawnOrUnsummon();
                    });
                }
            }

            void UpdateAI(uint32 diff) override
            {
                scheduler.Update(diff);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_darkheart_crafty_the_ambitiousAI(creature);
        }
};

// Echo of Yshaarj 71123
struct npc_darkheart_echo_of_yshaarj : public DarkHeartCreatureAI
{
    npc_darkheart_echo_of_yshaarj(Creature* creature) : DarkHeartCreatureAI(creature) { }

    bool hasTriggered;

    void Reset() override
    {
        events.Reset();
        hasTriggered = false;
    }

    void EnterCombat(Unit* /*who*/) override
    {
        if (Creature* norushen = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_NORUSHEN) : ObjectGuid::Empty))
            norushen->AI()->Talk(TALK_SPECIAL_1);

        if (Creature* controller = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_HEART_CONTROLLER) : ObjectGuid::Empty))
            controller->CastSpell(controller, SPELL_HEART_CORRUPTION, true);

        events.ScheduleEvent(EVENT_VEIL_OF_DARKNESS, urand(5 * IN_MILLISECONDS, 10 * IN_MILLISECONDS));
        events.ScheduleEvent(EVENT_MALEVOLENT_FORCE, 12 * IN_MILLISECONDS);
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(TALK_SPECIAL_2);

        if (me->GetInstanceScript())
            me->GetInstanceScript()->SetData(STEP_ECHO_DEFEATED, DONE);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
        summon->CastSpell(summon, SPELL_MALEVOLENT_FORCE_EFF, true);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/) override
    {
        if (HealthBelowPct(50) && !hasTriggered)
        {
            hasTriggered = true;
            Talk(TALK_INTRO);

            if (me->GetInstanceScript())
                me->GetInstanceScript()->SetData(STEP_ECHO_DEFEATED, SPECIAL);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            ExecuteTargetEvent(SPELL_VEIL_OF_DARKNESS, urand(8 * IN_MILLISECONDS, 21 * IN_MILLISECONDS), EVENT_VEIL_OF_DARKNESS, eventId, PRIORITY_SELF);
            ExecuteTargetEvent(SPELL_MALEVOLENT_FORCE, 13 * IN_MILLISECONDS, EVENT_MALEVOLENT_FORCE, eventId, PRIORITY_NOT_VICTIM);
            break;
        }

        DoMeleeAttackIfReady();
    }
};

// Malkorok 71177
struct npc_darkheart_malkorok : public ScriptedAI
{
    npc_darkheart_malkorok(Creature* creature) : ScriptedAI(creature) { }

    TaskScheduler scheduler;
    uint32 delay;

    void IsSummonedBy(Unit* summoner) override
    {
        delay = 0;
        me->GetMotionMaster()->MovePoint(0, malkorokSpawnPos);

        scheduler.
            Schedule(Milliseconds(delay += me->movespline->Duration()), [this](TaskContext context)
        {
            Talk(TALK_INTRO);

            if (Creature* grizzle = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_GRIZZLE_GEARSLIP + 1) : ObjectGuid::Empty))
                grizzle->SetFacingToObject(me);
        });

        scheduler.
            Schedule(Milliseconds(delay += 5500), [this](TaskContext context)
        {
            if (Creature* grizzle = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_GRIZZLE_GEARSLIP + 1) : ObjectGuid::Empty))
                grizzle->AI()->DoAction(ACTION_GRIZZLE_ANSWER);
        });
    }

    void DoAction(int32 actionId) override
    {
        if (actionId == ACTION_START_INTRO)
        {
            delay = 0;

            Talk(TALK_SPECIAL_1);

            scheduler.
                Schedule(Milliseconds(delay += 6000), [this](TaskContext context)
            {
                if (Creature* grizzle = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_GRIZZLE_GEARSLIP + 1) : ObjectGuid::Empty))
                    grizzle->AI()->Talk(TALK_SPECIAL_6);
            });

            scheduler.
                Schedule(Milliseconds(delay += 5000), [this](TaskContext context)
            {
                Talk(TALK_SPECIAL_2);
            });

            scheduler.
                Schedule(Milliseconds(delay += 6500), [this](TaskContext context)
            {
                Talk(TALK_SPECIAL_3);
            });

            scheduler.
                Schedule(Milliseconds(delay += 7000), [this](TaskContext context)
            {
                Talk(TALK_SPECIAL_4);
            });

            scheduler.
                Schedule(Milliseconds(delay += 5950), [this](TaskContext context)
            {
                if (Creature* grizzle = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_GRIZZLE_GEARSLIP + 1) : ObjectGuid::Empty))
                    grizzle->AI()->Talk(TALK_SPECIAL_7);
            });

            scheduler.
                Schedule(Milliseconds(delay += 4000), [this](TaskContext context)
            {
                Talk(TALK_SPECIAL_5);
            });

            scheduler.
                Schedule(Milliseconds(delay += 6000), [this](TaskContext context)
            {
                if (Creature* grizzle = ObjectAccessor::GetCreature(*me, me->GetInstanceScript() ? me->GetInstanceScript()->GetGuidData(NPC_GRIZZLE_GEARSLIP + 1) : ObjectGuid::Empty))
                    grizzle->AI()->DoAction(ACTION_LEAVE_MALKOROK);
            });
        }
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
    }
};

// Detonate 141456
class spell_darkheart_detonate : public AuraScript
{
    PrepareAuraScript(spell_darkheart_detonate);

    void HandleAuraEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes mode)
    {
        if (Unit* owner = GetOwner()->ToUnit())
            owner->CastSpell(owner, SPELL_STUN_SELF, true);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_darkheart_detonate::HandleAuraEffectApply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
    }
};

// Urtharges Shield Eff 141387
class spell_darkheart_urtharges_shield_eff : public AuraScript
{
    PrepareAuraScript(spell_darkheart_urtharges_shield_eff);

    void HandleAuraEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes mode)
    {
        SetStackAmount(20);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_darkheart_urtharges_shield_eff::HandleAuraEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
    }
};

// Rupture Line Launcher 141418
class spell_darkheart_rupture_line_launcher : public SpellScript
{
    PrepareSpellScript(spell_darkheart_rupture_line_launcher);

    void HandleEffectHitTarget(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            if (Unit* target = GetHitUnit())
                caster->CastSpell(target, SPELL_RUPTURE_LINE_MISSLE, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_darkheart_rupture_line_launcher::HandleEffectHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Detonate Eff 141458
class spell_darkheart_detonate_eff : public SpellScript
{
    PrepareSpellScript(spell_darkheart_detonate_eff);

    void HandleAfterCaster()
    {
        if (Unit* caster = GetCaster())
            caster->Kill(caster);
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_darkheart_detonate_eff::HandleAfterCaster);
    }
};

// Dark Blood 142150
class spell_darkheart_dark_blood : public AuraScript
{
    PrepareAuraScript(spell_darkheart_dark_blood);

    void HandleAuraEffectRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
    {
        if (Unit* owner = GetOwner()->ToUnit())
            owner->CastSpell(owner, SPELL_DARK_BLOOD_EFF, true);
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_darkheart_dark_blood::HandleAuraEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// Dark Blood Eff 141665
class spell_darkheart_dark_blood_eff : public SpellScript
{
    PrepareSpellScript(spell_darkheart_dark_blood_eff);

    void FilterTargets(std::list<WorldObject*> &targets)
    {
        targets.remove_if([=](WorldObject* target) { return target->GetEntry() != NPC_QUIVERING_BLOOD && target->GetEntry() != NPC_BLOOD_OF_YSHAARJ && target->GetEntry() != NPC_MOUTH_OF_TERROR; });
    }

    void HandleDamage(SpellEffIndex effIndex)
    {
        if (Unit* target = GetHitUnit())
            SetHitDamage(target->GetMaxHealth() - target->GetMaxHealth() * (GetSpellInfo()->Effects[EFFECT_0]->BasePoints / 100));
    }

    void Register()
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_darkheart_dark_blood_eff::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        OnEffectHitTarget += SpellEffectFn(spell_darkheart_dark_blood_eff::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

// 942, 943, 944, 950. Summoned by 141161, 141165, 141171, 141195 - Artifact AreaTrigger
class sat_darkheart_artifact : public AreaTriggerScript
{
public:
    sat_darkheart_artifact() : AreaTriggerScript("sat_darkheart_artifact") { }

    struct sat_darkheart_artifactAI : AreaTriggerAI
    {
        sat_darkheart_artifactAI(AreaTrigger* areaTrigger) : AreaTriggerAI(areaTrigger), triggered(false) { }

        bool triggered;

        void OnUnitEnter(Unit* unit) override
        {
            if (triggered || !unit->ToPlayer())
                return;

            Creature* caster = at->GetCaster() ? at->GetCaster()->ToCreature() : nullptr;
            if (!caster)
                return;

            auto artifact = invArtifactsType.find(caster->GetEntry());
            if (artifact == invArtifactsType.end())
                return;

            triggered = true;
            caster->RemoveAurasDueToSpell(artifact->second[0]);

            if (InstanceScript* instance = caster->GetInstanceScript())
                instance->SetData(DATA_ARTIFACT, artifact->second[3]);

            if (artifact->second[2])
                caster->AI()->Talk(TALK_INTRO);
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areaTrigger) const override
    {
        return new sat_darkheart_artifactAI(areaTrigger);
    }
};

// 1023. Summoned by 142155 - Heart Corruption Visual
class sat_darkheart_heart_corruption_visual : public AreaTriggerScript
{
public:
    sat_darkheart_heart_corruption_visual() : AreaTriggerScript("sat_darkheart_heart_corruption_visual") { }

    struct sat_darkheart_heart_corruption_visualAI : AreaTriggerAI
    {
        sat_darkheart_heart_corruption_visualAI(AreaTrigger* areaTrigger) : AreaTriggerAI(areaTrigger) { }

        void OnUnitEnter(Unit* unit) override
        {
            if (Player* player = unit->ToPlayer())
                player->CastSpell(player, SPELL_HEART_OF_YSHAARJ, true);
        }

        void OnUnitExit(Unit* unit) override
        {
            if (Player* player = unit->ToPlayer())
                player->RemoveAurasDueToSpell(SPELL_HEART_OF_YSHAARJ);
        }
    };

    AreaTriggerAI* GetAI(AreaTrigger* areaTrigger) const override
    {
        return new sat_darkheart_heart_corruption_visualAI(areaTrigger);
    }
};

void AddSC_dark_heart_of_pandaria()
{
    RegisterCreatureAI(npc_darkheart_earthborn_hatred);
    RegisterCreatureAI(npc_darkheart_fiery_anger);
    RegisterCreatureAI(npc_darkheart_urtharges_the_destroyer);
    new npc_darkheart_grizzle_gearslip();
    RegisterCreatureAI(npc_darkheart_mouth_of_terror);
    new npc_darkheart_crafty_the_ambitious();
    RegisterCreatureAI(npc_darkheart_echo_of_yshaarj);
    RegisterCreatureAI(npc_darkheart_malkorok);

    RegisterAuraScript(spell_darkheart_detonate);
    RegisterAuraScript(spell_darkheart_urtharges_shield_eff);
    RegisterSpellScript(spell_darkheart_rupture_line_launcher);
    RegisterSpellScript(spell_darkheart_detonate_eff);
    RegisterAuraScript(spell_darkheart_dark_blood);
    RegisterSpellScript(spell_darkheart_dark_blood_eff);
    new sat_darkheart_artifact();
    new sat_darkheart_heart_corruption_visual();
}
