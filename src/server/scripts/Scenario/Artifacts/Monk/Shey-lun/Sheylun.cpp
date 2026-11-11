/*
    Monk: Sheylun
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "ScenarioMgr.h"
#include "Scenario.h"
#include "Map.h"
#include "sheilun_plan.h"
#include "sheilun_acquisition.h"
#include "sheilun_return.h"
#include "sheilun_acquisition_scripts.h"


// 101881
class npc_tazhan_chu : public CreatureScript
{
public:
    npc_tazhan_chu() : CreatureScript("npc_tazhan_chu") { }

    bool OnGossipSelect(Player*, Creature*, uint32, uint32) override
    {
        // Native sGossipSelect receives the actual menu and option index.
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tazhan_chuAI(creature);
    }

    struct npc_tazhan_chuAI : ScriptedAI
    {
        npc_tazhan_chuAI(Creature* creature) : ScriptedAI(creature)
        {
            _introDone = false;
            _introHealDone = false;
            wait_heal = false;
            countgossip = 0;
            me->SetReactState(REACT_AGGRESSIVE);
        }

        EventMap events;
        bool _introDone, _introHealDone, wait_heal;
        FunctionProcessor _functions;
        uint8 countgossip;

        #include "sheilun_taran_intro.h"
        #include "sheilun_companion_wounds.h"

        // These callbacks capture this AI, so they must not outlive it.
        // Keep the older combat/rescue queue on its original UpdateAI clock.
        FunctionProcessor _presentationFunctions;

        void WoundSpeech() { Talk(11); Talk(12); }

        void Reset() override
        {
            events.Reset();
            me->SetWalk(false);
            CancelIntroHeal();
            _pendingWoundHeal = false;
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 7000); // 203859
            events.RescheduleEvent(EVENT_2, 17000); // 203854
            events.RescheduleEvent(EVENT_3, 19000); // 203858
            events.RescheduleEvent(EVENT_4, 23000); // 202517
        }

        void DoAction(int32 const action) override
        {
            if (action == 1) SheilunPlan::Refresh(me);
        }

        void sGossipSelect(Player* player, uint32 menu, uint32 option) override
        {
            SheilunPlan::Start(player, me, menu, option);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            damage = damage * 2;
        }

        void SetData(uint32 type, uint32 id) override
        {
            if (type == 1 && id == 1)
            {
                _functions.AddFunction([this]() -> void
                {
                    if (Creature* mob = me->FindNearestCreature(101882, 107.0f, true))
                        mob->AI()->DoAction(true);
                    if (Creature* mob = me->FindNearestCreature(102063, 107.0f, true))
                        mob->AI()->DoAction(true);
                    if (Creature* mob = me->FindNearestCreature(101885, 107.0f, true))
                        mob->AI()->DoAction(true);
                    if (Creature* mob = me->FindNearestCreature(101883, 107.0f, true))
                        mob->AI()->DoAction(true);
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48383);

                    Talk(8);
                    me->SetWalk(false);
                    me->GetMotionMaster()->MovePoint(3, -1018.74f, -2994.68f, 12.30f);
                }, _functions.CalculateTime(1000));

            }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*dmgType*/) override
        {
            damage /= 2;
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 1:
                    {
                        Talk(3);
                        DoCast(202053);

                        _presentationFunctions.AddDelayedEvent(4000, [this]() -> void
                        {
                            std::list<GameObject*> gob;
                            GetGameObjectListWithEntryInGrid(gob, me, 246321, 35.0f);
                            if (!gob.empty())
                                for (std::list<GameObject*>::iterator itr = gob.begin(); itr != gob.end(); ++itr)
                                    (*itr)->SetPhaseMask(2, true); // hide   
                            Talk(4);
                            me->GetMotionMaster()->MovePoint(2, -1017.80f, -3048.89f, 12.82f);
                        });
                        break;
                    }
                    case 2:
                    {
                        Talk(5);
                        break;
                    }
                    case 3:
                    {
                        Talk(9);
                        if (GameObject* go = me->FindNearestGameObject(246395, 50.0f))
                            go->SetPhaseMask(2, true); // hide
                        _functions.AddFunction([this]() -> void
                        {
                            me->GetMotionMaster()->MovePoint(4, -1017.72f, -2933.42f, 19.48f);
                        }, _functions.CalculateTime(1000));
                        break;
                    }
                    case 4:
                    {
                        Talk(10);
                        if (Creature* mob = me->FindNearestCreature(101882, 107.0f, true))
                            mob->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // госсип
                        if (Creature* mob = me->FindNearestCreature(101885, 107.0f, true))
                            mob->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // госсип
                        if (Creature* mob = me->FindNearestCreature(101883, 107.0f, true))
                            mob->SetUInt32Value(UNIT_FIELD_NPC_FLAGS, 1); // госсип
                        break;
                    }
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
                _presentationFunctions.AddDelayedEvent(5000, [this]() -> void
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

            if (me->HasAura(204779))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(203859);
                        events.RescheduleEvent(EVENT_1, 7000); // 203859
                        break;
                    case EVENT_2:
                        DoCast(203854);
                        events.RescheduleEvent(EVENT_2, 17000); // 203854
                        break;
                    case EVENT_3:
                        DoCast(203858);
                        events.RescheduleEvent(EVENT_3, 19000); // 203858
                        break;
                    case EVENT_4:
                        DoCast(202517);
                        events.RescheduleEvent(EVENT_4, 23000); // 202517
                        break;

                }
            }
            DoMeleeAttackIfReady();
        }

    };
};


// 101882, 102063, 101885, 101883
class npc_sheylun_small_events : public CreatureScript
{
public:
    npc_sheylun_small_events() : CreatureScript("npc_sheylun_small_events") { }

    bool OnGossipSelect(Player*, Creature*, uint32, uint32) override
    {
        // Sender/action are both zero for these native white-bubble options.
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sheylun_small_eventsAI(creature);
    }

    struct npc_sheylun_small_eventsAI : ScriptedAI
    {
        npc_sheylun_small_eventsAI(Creature* creature) : ScriptedAI(creature)
        {
            wait_heal = false;
            // me->SetReactState(REACT_AGGRESSIVE);
        }

        EventMap events;
        bool wait_heal;
        FunctionProcessor _functions;

        #include "sheilun_companion_wounds.h"

        void WoundSpeech() { Talk(0); Talk(1); }
        void sGossipSelect(Player* player, uint32 menu, uint32 option) override
        {
            SheilunPlan::Select(player, me, menu, option);
        }
        void OnHealApplied(Unit* /*who*/, uint32 /*amount*/, int64 /*gain*/) override { ObserveWoundHeal(); }
        void UpdateLifecycle(uint32 /*diff*/) override { RecoverWound(); }

        void Reset() override
        {
            _pendingWoundHeal = false;
            events.Reset();
            if (me->GetEntry() == 101882)
                DoCast(203836);
            if (me->GetEntry() == 101885)
                DoCast(204624);
            if (me->GetEntry() == 101883)
                DoCast(204593);
        }

        void EnterCombat(Unit* who) override
        {
            if (me->GetEntry() == 101882)
            {
                events.RescheduleEvent(EVENT_1, 5000); // 203832
                events.RescheduleEvent(EVENT_2, 11000); // 203831
            }
            if (me->GetEntry() == 101885)
            {
                events.RescheduleEvent(EVENT_3, 10000); // 203869
                events.RescheduleEvent(EVENT_4, 22000); // 203867
                events.RescheduleEvent(EVENT_5, 2600); // 203865
            }
            if (me->GetEntry() == 101883)
            {
                events.RescheduleEvent(EVENT_6, 2500); // 131995
                events.RescheduleEvent(EVENT_7, 3500); // 80016 31
            }
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            damage = damage * 2;
        }

        void DoAction(int32 const action) override
        {

            if (Creature* tazhan = me->FindNearestCreature(101881, 100.0f))
                me->GetMotionMaster()->MoveFollow(tazhan, urand(1, 4), (urand(1, 2) == 2 ? PET_FOLLOW_ANGLE : -PET_FOLLOW_ANGLE), MOTION_SLOT_IDLE);

            switch (me->GetEntry())
            {
                case 101882: //fay
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48380);
                    break;
                case 101883: //birdman
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48381);
                    break;
                case 101885: //Taoshi
                    if (Player* pl = me->FindNearestPlayer(100.0f))
                        pl->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 48382);
                    break;
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

            _functions.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->HasAura(204779))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_1:
                        DoCast(203832);
                        events.RescheduleEvent(EVENT_1, 5000); // 203832
                        break;
                    case EVENT_2:
                        DoCast(203831);
                        events.RescheduleEvent(EVENT_2, 11000); // 203831
                        break;
                    case EVENT_3:
                        DoCast(203869);
                        events.RescheduleEvent(EVENT_3, 10000); // 203869
                        break;
                    case EVENT_4:
                        DoCast(203867);
                        events.RescheduleEvent(EVENT_4, 22000); // 203867
                        break;
                    case EVENT_5:
                        DoCast(203865);
                        events.RescheduleEvent(EVENT_5, 2600); // 203865
                        break;
                    case EVENT_6:
                        DoCast(131995);
                        events.RescheduleEvent(EVENT_6, 2500); // 131995
                        break;
                    case EVENT_7:
                        DoCast(80016);
                        events.RescheduleEvent(EVENT_7, 31000); // 80016 31
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 246351
class go_sheylun_crate : public GameObjectScript
{
public:
    go_sheylun_crate() : GameObjectScript("go_sheylun_crate") { }

    struct go_sheylun_crateAI : public GameObjectAI
    {
        go_sheylun_crateAI(GameObject* go) : GameObjectAI(go) {}

        bool GossipHello(Player* player, bool isUse) override
        {
            if (!isUse)
                return true;

            if (Creature* mob = go->FindNearestCreature(101882, 7.0f, true))
                mob->AI()->DoAction(true);
            if (Creature* mob = go->FindNearestCreature(102063, 7.0f, true))
                mob->AI()->DoAction(true);
            if (Creature* mob = go->FindNearestCreature(101885, 7.0f, true))
                mob->AI()->DoAction(true);
            if (Creature* mob = go->FindNearestCreature(101883, 7.0f, true))
                mob->AI()->DoAction(true);
            go->Delete();
            return true;
        }

    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_sheylun_crateAI(go);
    }
};


// 101887
class npc_aspersiy : public CreatureScript
{
public:
    npc_aspersiy() : CreatureScript("npc_aspersiy") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_aspersiyAI(creature);
    }

    struct npc_aspersiyAI : public Scripted_NoMovementAI
    {
        npc_aspersiyAI(Creature* creature) : Scripted_NoMovementAI(creature), summons(me) {}

        EventMap events;
        SummonList summons;

        void Reset() override
        {
            events.Reset();
            summons.DespawnAll();
        }

        void EnterCombat(Unit* who) override
        {
            events.RescheduleEvent(EVENT_1, 21000); // 204779
            events.RescheduleEvent(EVENT_2, 35000); // 204773  21
            events.RescheduleEvent(EVENT_3, 36000); // 43 summons
            for (uint32 entry : {101881, 101882, 101885, 101883})
                if (Creature* ally = me->FindNearestCreature(entry, 100.0f, true))
                    if (CreatureAI* ai = ally->AI())
                    {
                        SheilunAcquisition::BattleAlly(me, ally);
                        ai->AttackStart(me);
                    }
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
        }

        void JustDied(Unit* who) override
        {
            SheilunAcquisition::Defeated(me, who);
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType dmgType) override
        {
            damage *= 2;
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
                        if (me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 60.0f, false))
                                me->CastSpell(target, 204779);
                            else
                                DoCast(204779);
                        events.RescheduleEvent(EVENT_1, 21000); // 204779
                        break;
                    case EVENT_2:
                        DoCast(204773);
                        events.RescheduleEvent(EVENT_2, 21000); // 204773  21
                        break;
                    case EVENT_3:
                        for (uint8 i = 0; i < urand(7, 8); ++i)
                            me->SummonCreature(103466, me->GetPositionX() + irand(-5, 5), me->GetPositionY() + irand(-5, 5), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000);
                        events.RescheduleEvent(EVENT_3, 43000); // 43 summons
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

    };
};

// 256913
class go_art_sheylun : public GameObjectScript
{
public:
    go_art_sheylun() : GameObjectScript("go_art_sheylun") {}

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (SheilunAcquisition::Instance* controller = SheilunAcquisition::Controller(player)) controller->Open(player, go);
        return true;
    }
};

void AddSC_Sheylun()
{
    new npc_tazhan_chu();
    new npc_sheylun_small_events();
    new go_sheylun_crate();
    new npc_aspersiy();
    new go_art_sheylun();
    SheilunAcquisition::Register();
    RegisterInstanceScript(instance_sheylun, 1541);
};
